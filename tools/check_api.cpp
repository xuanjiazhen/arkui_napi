/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <cctype>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "api_meta.h"
#include "check_helper.h"
#include "check_match.h"
#include "error_codes.h"
#include "load_abc.h"
#include "load_so.h"
#include "parse_json.h"

using namespace ohos::check;

namespace {

// The unknown-module json lives next to the result file and its name is derived from the
// result file's ("result.json" -> "unknown.json", "result_xxx.json" -> "unknown_xxx.json";
// other stems are kept, e.g. "check.json" -> "unknown_check.json"), so the two artifacts of
// one run are always found together and parallel runs with different result files never
// overwrite each other. Both '/' and '\\' count as directory separators (Windows runs).
std::string DeriveSiblingJsonPath(const std::string& resultFile, const std::string& stemName)
{
    const std::string resultStem = "result";
    size_t slash = resultFile.find_last_of("/\\");
    size_t dirEnd = (slash == std::string::npos) ? 0 : slash + 1;
    size_t dot = resultFile.find_last_of('.');
    // The extension is stripped only when the last dot belongs to the file name: a dot inside
    // the dir part or a leading hidden-file dot (dot == dirEnd) stays in the stem.
    size_t stemEnd = (dot == std::string::npos || dot <= dirEnd) ? resultFile.size() : dot;
    std::string stem = resultFile.substr(dirEnd, stemEnd - dirEnd);
    if (StartsWith(stem, resultStem)) {
        stem.erase(0, resultStem.size()); // "result_xxx" -> "_xxx", "result" -> ""
    } else if (!stem.empty()) {
        stem = "_" + stem; // separator so kept stems stay readable ("check" -> "unknown_check")
    }
    return resultFile.substr(0, dirEnd) + stemName + stem + ".json";
}

// Check task configuration assembled from CLI parsing (kit->files map, whitelist location, ignore sources).
struct CheckConfig {
    std::map<std::string, std::vector<std::string>> kitFileMap;
    std::string legalDir;
    std::string resultFile;
    std::string unknownFile; // unknown-module json, derived from resultFile
    bool whitelistIsFile = false; // legalDir is a whitelist json file (single-file mode)
    std::vector<ApiMeta> ignoreList;
    std::map<std::string, std::string> ignoreInheritMap; // inheritance map from the ignore list
    std::vector<std::string> unknownModules; // scan-dir: .so files with no whitelist registration
    std::set<std::string> skipModules; // normalized module names given via --skip-module
    std::string moduleSoDir; // module .so dir (scan-dir arg / platform default; ABC fallback)
    std::string moduleMapFile; // --module-map path, empty when the option is absent
    ParsedModuleMap moduleMap; // parsed module map (xlsx_to_module_map.py output)
    std::string unsupportedFile; // unsupported-module json, derived from resultFile
    // module -> reason, filled while resolving scan-dir inputs (modules whose mapped
    // binaries are not requireNapi-loadable or absent from the module dir); check-time
    // judgements (multi/single-file mode) are collected separately in main.
    std::vector<std::pair<std::string, std::string>> unsupportedModules;
};

// Deprecated-module bucket: API<=10 frozen snapshot checked against the current shared
// implementation, so mismatches are expected noise. Results are routed to DeprecatedModules
// and excluded from IllegalApis and the exit code.
constexpr const char* DEPRECATED_KIT_NAME = "API10LessDeprecatedModules";

// Positional-argument layout (see PrintUsage):
//   multi-file / scan-dir: <checklist json | module so dir> <legal dir> <result file>
//   single-file:           <kit> <so module> <legal dir | file> <result file>
// The ignore list is supplied only via the --ignore-file option.
constexpr size_t ARG_INPUT = 0;
constexpr size_t ARG_LEGAL_DIR = 1;
constexpr size_t ARG_RESULT_FILE = 2;
constexpr size_t ARG_COUNT = 3;
constexpr size_t SF_ARG_SO_MODULE = 1;
constexpr size_t SF_ARG_LEGAL = 2;
constexpr size_t SF_ARG_RESULT_FILE = 3;
constexpr size_t SF_ARG_COUNT = 4;
constexpr int MIN_ARGC = 2; // program name + at least one argument

struct KitContext {
    const LegalIndex* legalIndex = nullptr;
    const std::map<std::string, std::string>* inheritMap = nullptr;
    const std::set<std::string>* expandableNames = nullptr;
    std::string soDir; // module .so dir for the mixed-so ABC fallback
    const ParsedModuleMap* moduleMap = nullptr; // --module-map data (null when absent)
};

// Per-run counters; the current-kit and deprecated-bucket channels are tallied separately.
struct CheckSummary {
    // current-kit channel
    size_t filePassed = 0;
    size_t fileFlagged = 0;
    size_t fileFailed = 0;
    size_t apiExposed = 0;
    size_t apiIllegal = 0;
    // deprecated-bucket channel (excluded from the counters above and from the exit code)
    size_t depModulePassed = 0;
    size_t depModuleFlagged = 0;
    size_t depModuleFailed = 0;
    size_t depApiExposed = 0;
    size_t depApiFlagged = 0;
    size_t moduleSkipped = 0; // modules skipped via --skip-module
    size_t moduleUnsupported = 0; // judged not requireNapi-loadable via the module map
    CheckErrorCode firstError = CHECK_OK;
};

void PrintUsage(const char* name)
{
    std::cerr << "Usage(multi-file):  " << name <<
        " <check list json> <legal dir of kit json> <result file> [options]" << std::endl;
    std::cerr << "Usage(scan-dir):    " << name <<
        " <module so dir> <legal dir of kit json> <result file> [options]" << std::endl;
    std::cerr << "Usage(single-file): " << name <<
        " <kit name> <so module name> <legal dir of kit json> <result file>" <<
        " [options]" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  --log-level <DEBUG|INFO|WARN|ERROR>   log level, default INFO" << std::endl;
    std::cerr << "  --log-file <file>                     write log to file (WARN/ERROR" <<
        " still printed to stderr)" << std::endl;
    std::cerr << "  --ignore-file <path>...               ignore list file(s)/dir(s), greedy until" <<
        " next option" << std::endl;
    std::cerr << "  --skip-module <name[,name...]>        skip checking given module(s), e.g." <<
        " @ohos.util (crash isolation); summary shows skip count" << std::endl;
    std::cerr << "  --module-map <file>                   module map json (xlsx_to_module_map.py" <<
        " output): real binary files per d.ts module; resolves renamed/shared so and" <<
        " abc-backed modules the file-name convention cannot, and judges modules whose" <<
        " binaries are not requireNapi-loadable (reported as unsupported, never as" <<
        " load failures)" << std::endl;
    std::cerr << "Note: options should be placed after positional arguments." << std::endl;
    std::cerr << "Note: unknown modules (scan-dir) are archived next to the result file as" <<
        " unknown*.json derived from its name (result.json -> unknown.json); modules whose" <<
        " mapped binaries cannot be requireNapi-loaded are archived as unsupported*.json." << std::endl;
}

bool FileExists(const std::string& path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

bool IsDirectory(const std::string& path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
}

bool IsRegularFile(const std::string& path)
{
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}

std::string ToLower(const std::string& s)
{
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

// Comma-joined path list for one-line reasons ("a, b (+2 more)" stays readable).
std::string JoinPathList(const std::vector<std::string>& paths)
{
    const size_t maxShown = 2;
    std::string joined;
    size_t shown = 0;
    for (const std::string& p : paths) {
        if (shown >= maxShown) {
            break;
        }
        if (!joined.empty()) {
            joined += ", ";
        }
        joined += p;
        shown++;
    }
    if (paths.size() > maxShown) {
        joined += " (+" + std::to_string(paths.size() - maxShown) + " more)";
    }
    return joined;
}

// Resolves {legalDir}/{kit}.json; falls back to a case-insensitive scan because checklist
// keys and whitelist file names often differ in case (e.g. DrmKit vs DRMKit.json).
std::string FindWhitelistFile(const std::string& legalDir, const std::string& kit)
{
    std::string direct = legalDir + "/" + kit + ".json";
    if (IsRegularFile(direct)) {
        return direct;
    }
    DIR* dir = opendir(legalDir.c_str());
    if (dir == nullptr) {
        return direct;
    }
    std::string lowered = ToLower(kit) + ".json";
    std::string found;
    struct dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        if (ToLower(entry->d_name) == lowered) {
            found = legalDir + "/" + entry->d_name;
            break;
        }
    }
    closedir(dir);
    if (!found.empty()) {
        LogWarn() << "kit whitelist resolved case-insensitively: " << kit << " -> " << found;
        return found;
    }
    return direct;
}

// Loads an ignore source: a json file, or a directory (every .json inside is parsed and merged).
void LoadIgnoreSource(const std::string& path,
                      std::vector<ApiMeta>& ignoreList,
                      std::map<std::string, std::string>& ignoreInheritMap)
{
    if (IsDirectory(path)) {
        DIR* dir = opendir(path.c_str());
        if (dir == nullptr) {
            LogWarn() << "opendir failed: " << path;
            return;
        }
        struct dirent* entry = nullptr;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (EndsWith(name, ".json")) {
                std::string child = path + "/" + name;
                LogInfo() << "load ignore file: " << child;
                ParseLegalApiList(child, ignoreList, ignoreInheritMap);
            }
        }
        closedir(dir);
        return;
    }
    LogInfo() << "load ignore file: " << path;
    if (ParseLegalApiList(path, ignoreList, ignoreInheritMap) != CHECK_OK) {
        LogWarn() << "parse ignore file failed: " << path;
    }
}

// Maps a .so/.abc file back to its requireName-form module name (all lowercase), the
// inverse of GetNativeModulePath: strip the lib/so/_napi parts and turn each relDir
// separator into a dot ("hms/core/ar" + "libarengine.z.so" -> "hms.core.ar.arengine";
// "file" + "photoaccesshelper.abc" -> "file.photoaccesshelper" - the abc fallback root
// mirrors the so search root one-to-one). Returns false for non-module file names.
bool ModuleFileToRequireName(const std::string& relDir, const std::string& fileName,
                             std::string& name)
{
    std::string stem;
    if (EndsWith(fileName, ".abc")) {
        stem = fileName.substr(0, fileName.size() - 4); // 4: strlen(".abc")
    } else if (StartsWith(fileName, "lib")) {
        stem = fileName.substr(3); // 3: strlen("lib")
        if (EndsWith(stem, ".z.so")) {
            stem = stem.substr(0, stem.size() - 5); // 5: strlen(".z.so")
        } else if (EndsWith(stem, ".so")) {
            stem = stem.substr(0, stem.size() - 3); // 3: strlen(".so")
        } else {
            return false;
        }
        const std::string napiSuffix = "_napi";
        if (EndsWith(stem, napiSuffix)) {
            stem = stem.substr(0, stem.size() - napiSuffix.size());
        }
    } else {
        return false;
    }
    if (stem.empty()) {
        return false;
    }
    std::string dottedRelDir = relDir;
    std::replace(dottedRelDir.begin(), dottedRelDir.end(), '/', '.');
    name = ToLower(dottedRelDir.empty() ? stem : dottedRelDir + "." + stem);
    return true;
}

// Normalized module-dir rel path of a scanned file (lowercase; ".z.so" reduced to ".so"),
// the key shared with the module map (NormalizeModuleFileRel).
std::string ScanFileRel(const std::string& relDir, const std::string& fileName)
{
    std::string rel = ToLower(relDir.empty() ? fileName : relDir + "/" + fileName);
    const std::string zSoSuffix = ".z.so";
    if (EndsWith(rel, zSoSuffix)) {
        rel = rel.substr(0, rel.size() - zSoSuffix.size()) + ".so";
    }
    return rel;
}

// Files found under the module dir, indexed for the two scan-dir lookups: rel -> derived
// requireName (module-map claiming + unknown detection) and name -> rel (conventional
// whitelist claiming).
struct ScanFileSet {
    std::map<std::string, std::string> relToName;
    std::map<std::string, std::string> nameToRel;
    size_t Count() const { return relToName.size(); }
};

// Recursively collects all module files (.so and the engine's abc fallback ".abc") under
// moduleDir.
void EnumerateModuleFiles(const std::string& moduleDir, const std::string& relDir,
                          ScanFileSet& files)
{
    std::string full = relDir.empty() ? moduleDir : moduleDir + "/" + relDir;
    DIR* dir = opendir(full.c_str());
    if (dir == nullptr) {
        LogWarn() << "opendir failed: " << full;
        return;
    }
    struct dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (name == "." || name == "..") {
            continue;
        }
        std::string childRel = relDir.empty() ? name : relDir + "/" + name;
        struct stat st;
        if (stat((moduleDir + "/" + childRel).c_str(), &st) != 0) {
            continue;
        }
        if (S_ISDIR(st.st_mode)) {
            EnumerateModuleFiles(moduleDir, childRel, files);
        } else if (S_ISREG(st.st_mode)) {
            std::string derived;
            if (!ModuleFileToRequireName(relDir, name, derived)) {
                continue;
            }
            std::string rel = ScanFileRel(relDir, name);
            files.relToName[rel] = derived;
            // First file wins for the conventional name->rel index: the .so and the _napi
            // variant derive the same requireName and either satisfies the claim.
            files.nameToRel.emplace(derived, rel);
        }
    }
    closedir(dir);
}

// Claims one whitelist file's modules for its kit. Two channels per canonical module name:
// 1. module map (when --module-map was given and covers the module): every mapped module
//    file present under the scan dir claims the module for the kit - this resolves the
//    conventions the file-name rules cannot express (renamed so like
//    file/libphotoaccesshelpernative.z.so backing @ohos.file.photoAccessHelper, one shared
//    so backing several d.ts modules, ani so under module/ani/...).
// 2. name convention (fallback, also for map-covered modules whose mapped files are all
//    absent): the requireName form of the module matches a scanned file, exactly the
//    pre-module-map behavior, first kit wins.
// Modules covered by the map but without any loadable binary (framework abc, platformsdk,
// ndk, NA) or without their binaries in the scan dir are judged unsupported and reported
// instead of being checked or failing.
// Each module is claimed by at most one kit (first claim wins; the caller maps the
// deprecated bucket last so a module registered both in a normal kit and in the
// deprecated snapshot always belongs to the normal kit).
void MapWhitelistFile(const std::string& legalDir, const std::string& name,
                      const ScanFileSet& scanFiles, CheckConfig& config,
                      std::set<std::string>& claimedModules,
                      std::set<std::string>& claimedRequireNames,
                      std::set<std::string>& claimedFiles)
{
    std::string kit = name.substr(0, name.size() - 5); // 5: strlen(".json")
    std::set<std::string> canonicalNames;
    if (CollectModuleNames(legalDir + "/" + name, canonicalNames) != CHECK_OK) {
        LogWarn() << "scan-dir: skip invalid whitelist: " << name;
        return;
    }
    for (const std::string& canonical : canonicalNames) {
        const std::string mKey = ToLower(canonical);
        if (!claimedModules.insert(mKey).second) {
            continue; // already claimed (or judged) by an earlier kit
        }
        const ModuleMapEntry* entry =
            config.moduleMap.Empty() ? nullptr : config.moduleMap.Find(mKey);
        if (entry != nullptr) {
            std::vector<std::string> presentRels;
            for (const std::string& rel : entry->moduleFiles) {
                if (scanFiles.relToName.count(rel) > 0) {
                    presentRels.push_back(rel);
                }
            }
            if (!presentRels.empty()) {
                for (const std::string& rel : presentRels) {
                    claimedFiles.insert(rel);
                }
                config.kitFileMap[kit].push_back(canonical);
                continue;
            }
            // No mapped binary in the scan dir: keep the conventional fallback alive
            // before judging the module unsupported (a stale map must not hide a real
            // conventionally-named file). It respects the conventional first-claim-wins
            // rule: this is the map-data-gap corner, not the many-to-one case (that one
            // goes through the present mapped files above).
            const std::string convName = ToLower(ModuleNameForRequireNapi(canonical));
            if (scanFiles.nameToRel.count(convName) > 0 && claimedRequireNames.insert(convName).second) {
                claimedFiles.insert(scanFiles.nameToRel.at(convName));
                config.kitFileMap[kit].push_back(canonical);
                continue;
            }
            std::string reason = entry->nonModuleReason;
            if (reason.empty()) {
                reason = "mapped module binary not present under scan dir: " +
                    JoinPathList(entry->files);
            }
            if (!entry->kit.empty()) {
                reason += " (kit: " + entry->kit + ")";
            }
            config.unsupportedModules.push_back({canonical, reason});
            continue;
        }
        // Conventional claim (pre-module-map behavior): requireName of the module matches
        // a scanned file; first kit wins, so the shared-so case without a map still keeps
        // its old single-owner semantics.
        std::string key = ToLower(ModuleNameForRequireNapi(canonical));
        if (scanFiles.nameToRel.count(key) > 0 && claimedRequireNames.insert(key).second) {
            claimedFiles.insert(scanFiles.nameToRel.at(key));
            config.kitFileMap[kit].push_back(canonical);
        }
    }
}

// scan-dir input assembly: enumerate module files (.so + abc fallback), then map them
// against the canonical module names collected from each {kit}.json (module map first,
// name convention as fallback). Hits go to kitFileMap, misses to unknownModules.
CheckErrorCode ResolveScanDirInputs(const std::string& moduleDir, const std::string& legalDir,
                                    CheckConfig& config)
{
    ScanFileSet scanFiles;
    EnumerateModuleFiles(moduleDir, "", scanFiles);
    LogInfo() << "scan-dir: " << scanFiles.Count() << " module file(s) under " << moduleDir;
    if (scanFiles.Count() == 0) {
        std::cerr << "no module so found under: " << moduleDir << std::endl;
        return ERROR_PARAM_INVALID;
    }

    DIR* dir = opendir(legalDir.c_str());
    if (dir == nullptr) {
        std::cerr << "open whitelist dir failed: " << legalDir << std::endl;
        return ERROR_FILE_OPEN_FAILED;
    }
    std::vector<std::string> whitelistFiles;
    struct dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        if (EndsWith(entry->d_name, ".json")) {
            whitelistFiles.push_back(entry->d_name);
        }
    }
    closedir(dir);
    // readdir order is filesystem dependent: sort for determinism, and map the deprecated
    // bucket last so a module registered both in a normal kit and in the deprecated
    // snapshot always belongs to the normal kit.
    std::sort(whitelistFiles.begin(), whitelistFiles.end());
    const std::string deprecatedJson = std::string(DEPRECATED_KIT_NAME) + ".json";
    std::set<std::string> claimedModules;     // module names resolved (claimed or judged)
    std::set<std::string> claimedRequireNames; // conventionally claimed requireNames
    std::set<std::string> claimedFiles;       // scan-dir rels backing a claimed module
    for (const std::string& name : whitelistFiles) {
        if (name != deprecatedJson) {
            MapWhitelistFile(legalDir, name, scanFiles, config, claimedModules,
                             claimedRequireNames, claimedFiles);
        }
    }
    for (const std::string& name : whitelistFiles) {
        if (name == deprecatedJson) {
            MapWhitelistFile(legalDir, name, scanFiles, config, claimedModules,
                             claimedRequireNames, claimedFiles);
        }
    }

    for (const auto& [rel, derivedName] : scanFiles.relToName) {
        if (claimedFiles.count(rel) == 0 && claimedRequireNames.count(derivedName) == 0) {
            config.unknownModules.push_back(derivedName);
        }
    }
    std::sort(config.unknownModules.begin(), config.unknownModules.end());
    LogInfo() << "scan-dir: " << config.kitFileMap.size() << " kit(s), " <<
        config.unknownModules.size() << " unknown module(s), " <<
        config.unsupportedModules.size() << " unsupported module(s)";
    return CHECK_OK;
}

struct ModuleCheckResult {
    std::vector<ApiMeta> exposedList;
    std::vector<ApiMeta> illegalList;
    size_t exposedCount = 0;
    std::set<std::string> fileAliases;  // sibling modules sharing the loaded binary
    bool unsupported = false;            // judged not requireNapi-loadable (module map)
    std::string unsupportedReason;
};

struct ModuleLoadInfo {
    std::string kit;
    std::string file;       // original checklist/CLI value (used as the abc displayName)
    std::string moduleName; // normalized module name (or the .abc path)
};

CheckErrorCode LoadAbcExposed(napi_env env, const KitContext& kitCtx,
                              const ModuleLoadInfo& info, ModuleCheckResult& result)
{
    AbcLoadParam abcParam;
    abcParam.kit = info.kit;
    abcParam.displayName = info.file;
    abcParam.abcFilePath = info.moduleName;
    abcParam.expandableNames = kitCtx.expandableNames;
    return LoadAbcFileAndGetExposedMeta(env, abcParam, result.exposedList);
}

// requireNapi candidates for one module: the requireName of the module itself (device
// search order: so path[0], _napi path[1], abc path[2]) followed by the module-map files
// (renamed/shared so, ani so) - each mapped rel derives its own requireName the same way
// the scan does, so requireNapi reaches it through the same path rules.
std::vector<std::string> RequireNameCandidates(const KitContext& kitCtx,
                                              const ModuleLoadInfo& info,
                                              const ModuleMapEntry* entry)
{
    std::vector<std::string> candidates;
    candidates.push_back(ModuleNameForRequireNapi(info.moduleName));
    if (entry == nullptr) {
        return candidates;
    }
    for (const std::string& rel : entry->moduleFiles) {
        std::string soDir;
        std::string fileName;
        size_t slash = rel.find_last_of('/');
        if (slash == std::string::npos) {
            fileName = rel;
        } else {
            soDir = rel.substr(0, slash);
            fileName = rel.substr(slash + 1);
        }
        std::string derived;
        if (!ModuleFileToRequireName(soDir, fileName, derived)) {
            continue;
        }
        std::string path = kitCtx.soDir.empty() ? rel : kitCtx.soDir + "/" + rel;
        if (IsRegularFile(path)) {
            candidates.push_back(derived);
        }
    }
    return candidates;
}

// The rel forms GetNativeModulePath would derive for one requireName: so path[0],
// _napi path[1] and the abc fallback path[2], normalized like the module-map keys.
std::vector<std::string> CandidateModuleRels(const std::string& requireName)
{
    std::string lower = ToLower(requireName);
    size_t lastDot = lower.find_last_of('.');
    std::string stem = (lastDot == std::string::npos) ? lower : lower.substr(lastDot + 1);
    std::string dirPart = (lastDot == std::string::npos) ? "" : lower.substr(0, lastDot);
    std::replace(dirPart.begin(), dirPart.end(), '.', '/');
    std::vector<std::string> rels;
    rels.push_back((dirPart.empty() ? "" : dirPart + "/") + "lib" + stem + ".so");
    rels.push_back((dirPart.empty() ? "" : dirPart + "/") + "lib" + stem + "_napi.so");
    rels.push_back(dirPart.empty() ? stem + ".abc" : dirPart + "/" + stem + ".abc");
    return rels;
}

// Alias set for the loaded module: every module the map registers for the actually
// existing candidate files of the loaded requireName (the shared-so case: exports of
// libstruct.z.so legitimately belong to all @ohos.util.* modules backing it).
std::set<std::string> CollectFileAliases(const ParsedModuleMap& moduleMap,
                                         const std::string& soDir,
                                         const std::string& loadedRequireName)
{
    std::set<std::string> aliases;
    for (const std::string& rel : CandidateModuleRels(loadedRequireName)) {
        auto it = moduleMap.fileOwners.find(rel);
        if (it == moduleMap.fileOwners.end()) {
            continue;
        }
        std::string path = soDir.empty() ? rel : soDir + "/" + rel;
        if (!IsRegularFile(path)) {
            continue; // owners of absent candidates must not widen the whitelist scope
        }
        aliases.insert(it->second.begin(), it->second.end());
    }
    return aliases;
}

// requireNapi loading with the module-map fallbacks. Candidates are tried in order: for
// each, requireNapi first, then the mixed-so ABC fallback (embedded GetABCCode). The
// displayName stays the checked module name so whitelist file scoping keeps working.
CheckErrorCode LoadSoExposed(napi_env env, const KitContext& kitCtx,
                             const ModuleLoadInfo& info, const ModuleMapEntry* entry,
                             ModuleCheckResult& result)
{
    CheckErrorCode lastErr = ERROR_PARAM_INVALID;
    for (const std::string& candidate : RequireNameCandidates(kitCtx, info, entry)) {
        SoLoadParam param;
        param.kit = info.kit;
        param.displayName = info.moduleName;
        param.requireName = candidate;
        CheckErrorCode err = LoadSoAndGetExposedMeta(env, param, result.exposedList,
                                                     kitCtx.expandableNames);
        if (err == CHECK_OK) {
            if (kitCtx.moduleMap != nullptr && !kitCtx.moduleMap->Empty()) {
                result.fileAliases = CollectFileAliases(*kitCtx.moduleMap, kitCtx.soDir, candidate);
            }
            if (candidate != ModuleNameForRequireNapi(info.moduleName)) {
                LogInfo() << "module loaded via mapped binary candidate: " << info.moduleName <<
                    " -> requireNapi(" << candidate << ")";
            }
            return CHECK_OK;
        }
        lastErr = err;
        // Mixed .so fallback: requireNapi needs runtime context a bare process lacks, so
        // dlopen the .so directly and execute its embedded ABC instead.
        AbcLoadParam abcParam;
        abcParam.kit = info.kit;
        abcParam.displayName = info.moduleName;
        abcParam.requireName = candidate;
        abcParam.expandableNames = kitCtx.expandableNames;
        for (const std::string& soPath : CandidateSoPaths(kitCtx.soDir, candidate)) {
            if (!IsRegularFile(soPath) || !SoHasEmbeddedAbc(soPath)) {
                continue;
            }
            abcParam.soPath = soPath;
            err = LoadMixedSoAbcAndGetExposedMeta(env, abcParam, result.exposedList);
            if (err == CHECK_OK) {
                if (kitCtx.moduleMap != nullptr && !kitCtx.moduleMap->Empty()) {
                    result.fileAliases = CollectFileAliases(*kitCtx.moduleMap, kitCtx.soDir,
                                                            candidate);
                }
                LogInfo() << "module loaded via mapped mixed-so candidate: " << info.moduleName <<
                    " (so: " << soPath << ")";
                return CHECK_OK;
            }
            lastErr = err;
        }
    }
    return lastErr;
}

// The map-based judgement: can any binary of this module be requireNapi-loaded in this
// run at all? Covers both "wrong kind of binary" (framework abc, platformsdk, ndk, NA)
// and "right kind, but absent from the module dir".
bool HasLoadableBinary(const std::string& soDir, const ModuleMapEntry& entry)
{
    for (const std::string& rel : entry.moduleFiles) {
        std::string path = soDir.empty() ? rel : soDir + "/" + rel;
        if (IsRegularFile(path)) {
            return true;
        }
    }
    return false;
}

std::string UnsupportedReasonFor(const ModuleMapEntry& entry)
{
    if (!entry.nonModuleReason.empty()) {
        return entry.nonModuleReason;
    }
    return "mapped module binary not found: " + JoinPathList(entry.files);
}

// The conventionally-named binaries of the module itself: the rel forms GetNativeModulePath
// derives for its own requireName (so path[0], _napi path[1], abc path[2]). A real
// conventionally-named file must not be hidden by a map classification (stale maps
// happen): its presence defers the unsupported judgement to an actual load attempt.
bool HasConventionalBinary(const std::string& soDir, const std::string& moduleName)
{
    const std::string requireName = ModuleNameForRequireNapi(moduleName);
    if (requireName.empty()) {
        return false;
    }
    for (const std::string& rel : CandidateModuleRels(requireName)) {
        std::string path = soDir.empty() ? rel : soDir + "/" + rel;
        if (IsRegularFile(path)) {
            return true;
        }
    }
    return false;
}

// Checks a single artifact file. Whitelist structures are prebuilt per kit (KitContext) so
// they are not reparsed for every file of the same kit.
CheckErrorCode CheckSingleFile(napi_env env,
                               const KitContext& kitCtx,
                               const std::string& kit,
                               const std::string& file,
                               ModuleCheckResult& result)
{
    // Per-module isolation: clear stale pending exceptions and open a dedicated handle scope,
    // otherwise one module's getter exception short-circuits every later requireNapi lookup.
    ModuleCheckGuard guard(env);

    result.exposedCount = 0;
    CheckErrorCode err = CHECK_OK;
    ModuleLoadInfo info;
    info.kit = kit;
    info.file = file;
    info.moduleName = NormalizeModuleName(file);
    const ModuleMapEntry* entry = nullptr;
    if (kitCtx.moduleMap != nullptr && !kitCtx.moduleMap->Empty()) {
        entry = kitCtx.moduleMap->Find(ToLower(info.moduleName));
        // Judge first: modules whose binaries are not requireNapi-loadable at all are
        // reported as unsupported without a load attempt (they would only fail noisily).
        // A real conventionally-named binary defers to the load instead: the claim path
        // keeps the conventional fallback alive for exactly this case (a stale map must
        // not hide a loadable file), so the map classification must not hide it either.
        if (entry != nullptr && !entry->nonModuleReason.empty()) {
            if (HasConventionalBinary(kitCtx.soDir, info.moduleName)) {
                LogInfo() << "map classifies as non-requireNapi, conventional binary "
                             "present, loading it: " << info.moduleName <<
                    " (map reason: " << entry->nonModuleReason << ")";
            } else {
                result.unsupported = true;
                result.unsupportedReason = entry->nonModuleReason;
                LogInfo() << "module judged unsupported (not requireNapi-loadable): " <<
                    info.moduleName << ", reason: " << result.unsupportedReason;
                return CHECK_OK;
            }
        }
    }
    if (EndsWith(info.moduleName, ".abc")) {
        err = LoadAbcExposed(env, kitCtx, info, result);
#if defined(OHOS_PLATFORM) || defined(LINUX_PLATFORM)
    } else if (StartsWith(info.moduleName, "@")) {
        err = LoadSoExposed(env, kitCtx, info, entry, result);
#else
    } else if (StartsWith(info.moduleName, "@")) {
        LogError() << "非ohos/linux平台不支持检查 so 模块: " << info.moduleName;
        return ERROR_PARAM_INVALID;
#endif
    } else {
        LogError() << "当前仅支持模块名(@ohos./@arkts./@hms./@system.开头): " << info.moduleName;
        return ERROR_PARAM_INVALID;
    }
    if (err != CHECK_OK) {
        // Second judgement: neither the mapped module-dir binaries nor a conventionally
        // named one is present, and the load failed - the map explains the failure, so
        // report it as unsupported instead of a plain load failure. A conventionally
        // named file that exists but fails to load is a real failure and stays one.
        if (entry != nullptr && !HasLoadableBinary(kitCtx.soDir, *entry) &&
            !HasConventionalBinary(kitCtx.soDir, info.moduleName)) {
            result.unsupported = true;
            result.unsupportedReason = UnsupportedReasonFor(*entry);
            LogInfo() << "module judged unsupported (binary unavailable): " << info.moduleName <<
                ", reason: " << result.unsupportedReason;
            return CHECK_OK;
        }
        return err;
    }

    result.exposedCount = result.exposedList.size();

    err = CheckIllegalExposed(*kitCtx.legalIndex, result.exposedList, *kitCtx.inheritMap,
                              result.illegalList, &result.fileAliases);
    const size_t nExposed = result.exposedList.size();
    const size_t nIllegal = result.illegalList.size();
    if (err != CHECK_OK) {
        LogWarn() << "检查不通过: 存在非法API暴露: " << info.moduleName << " (导出 " << nExposed <<
            " 条, 合法 " << (nExposed - nIllegal) << ", 非法 " << nIllegal << ")";
        return err;
    }
    LogInfo() << "检查通过: 无非法API暴露: " << info.moduleName << " (导出 " << nExposed << " 条)";
    return CHECK_OK;
}

// Option handlers: each consumes its value arguments and returns the next argv index,
// or -1 after printing the error (usage where relevant).
int HandleLogLevelOption(int argc, char* argv[], int index)
{
    LogLevel level = LogLevel::INFO;
    int valueIndex = index + 1;
    if (valueIndex >= argc || !ParseLogLevel(argv[valueIndex], level)) {
        std::cerr << "invalid --log-level value" << std::endl;
        PrintUsage(argv[0]);
        return -1;
    }
    SetLogLevel(level);
    return valueIndex + 1;
}

int HandleLogFileOption(int argc, char* argv[], int index)
{
    int valueIndex = index + 1;
    if (valueIndex >= argc || !SetLogFile(argv[valueIndex])) {
        std::cerr << "invalid --log-file path" << std::endl;
        return -1;
    }
    return valueIndex + 1;
}

int HandleIgnoreFileOption(int argc, char* argv[], int index,
                           std::vector<std::string>& ignoreSources)
{
    // Greedy: consume paths until the next option; only *.json or existing paths are
    // accepted so interleaved positionals are not swallowed.
    int next = index + 1;
    bool consumed = false;
    while (next < argc && argv[next][0] != '-' &&
           (EndsWith(argv[next], ".json") || FileExists(argv[next]))) {
        ignoreSources.push_back(argv[next]);
        ++next;
        consumed = true;
    }
    if (!consumed) {
        std::cerr << "--ignore-file requires at least one path" << std::endl;
        PrintUsage(argv[0]);
        return -1;
    }
    return next;
}

int HandleSkipModuleOption(int argc, char* argv[], int index,
                           std::vector<std::string>& skipModules)
{
    // Crash isolation: skip the listed modules (comma-separated, repeatable).
    int valueIndex = index + 1;
    if (valueIndex >= argc || argv[valueIndex][0] == '-') {
        std::cerr << "--skip-module requires a module name list" << std::endl;
        PrintUsage(argv[0]);
        return -1;
    }
    skipModules.push_back(argv[valueIndex]);
    return valueIndex + 1;
}

int HandleModuleMapOption(int argc, char* argv[], int index, std::string& moduleMapFile)
{
    int valueIndex = index + 1;
    if (valueIndex >= argc || argv[valueIndex][0] == '-') {
        std::cerr << "--module-map requires a json file path" << std::endl;
        PrintUsage(argv[0]);
        return -1;
    }
    moduleMapFile = argv[valueIndex];
    return valueIndex + 1;
}

// Parses command-line options and positionals; prints errors (usage in some cases) on failure.
bool ParseCommandLine(int argc, char* argv[], std::vector<std::string>& positionals,
                      std::vector<std::string>& ignoreSources,
                      std::vector<std::string>& skipModules,
                      std::string& moduleMapFile)
{
    int index = 1;
    while (index < argc) {
        std::string arg = argv[index];
        if (arg == "--log-level") {
            index = HandleLogLevelOption(argc, argv, index);
        } else if (arg == "--log-file") {
            index = HandleLogFileOption(argc, argv, index);
        } else if (arg == "--ignore-file") {
            index = HandleIgnoreFileOption(argc, argv, index, ignoreSources);
        } else if (arg == "--skip-module") {
            index = HandleSkipModuleOption(argc, argv, index, skipModules);
        } else if (arg == "--module-map") {
            index = HandleModuleMapOption(argc, argv, index, moduleMapFile);
        } else if (arg.size() > 1 && arg[0] == '-' && arg[1] == '-') {
            std::cerr << "unknown option: " << arg << std::endl;
            PrintUsage(argv[0]);
            return false;
        } else {
            positionals.push_back(arg);
            ++index;
        }
        if (index < 0) {
            return false;
        }
    }
    return true;
}

CheckErrorCode ResolveMultiFileInputs(const char* programName,
                                      const std::vector<std::string>& positionals,
                                      CheckConfig& config)
{
    if (positionals.size() != ARG_COUNT) {
        PrintUsage(programName);
        return ERROR_PARAM_INVALID;
    }
    if (ParseKitToCheckFiles(positionals[ARG_INPUT], config.kitFileMap) != CHECK_OK) {
        std::cerr << "Parse input file fail: " << positionals[ARG_INPUT] << std::endl;
        return ERROR_JSON_PARSE_FAILED;
    }
    // multi-file mode covers several kits, so the whitelist argument must be a directory
    if (IsRegularFile(positionals[ARG_LEGAL_DIR])) {
        std::cerr << "multi-file mode requires a whitelist DIRECTORY, got file: " <<
            positionals[ARG_LEGAL_DIR] << std::endl;
        return ERROR_PARAM_INVALID;
    }
    config.legalDir = positionals[ARG_LEGAL_DIR];
    config.resultFile = positionals[ARG_RESULT_FILE];
    return CHECK_OK;
}

// scan-dir branch of ResolveCheckInputs: kit ownership comes from the whitelist dir
// (no checklist.json needed).
CheckErrorCode ResolveScanDirModeInputs(const char* programName,
    const std::vector<std::string>& positionals, CheckConfig& config)
{
    if (positionals.size() != ARG_COUNT) {
        PrintUsage(programName);
        return ERROR_PARAM_INVALID;
    }
    if (IsRegularFile(positionals[ARG_LEGAL_DIR])) {
        std::cerr << "scan-dir mode requires a whitelist DIRECTORY, got file: " <<
            positionals[ARG_LEGAL_DIR] << std::endl;
        return ERROR_PARAM_INVALID;
    }
    CheckErrorCode err = ResolveScanDirInputs(positionals[ARG_INPUT], positionals[ARG_LEGAL_DIR], config);
    if (err != CHECK_OK) {
        return err;
    }
    config.moduleSoDir = positionals[ARG_INPUT];
    config.legalDir = positionals[ARG_LEGAL_DIR];
    config.resultFile = positionals[ARG_RESULT_FILE];
    return CHECK_OK;
}

// single-file branch of ResolveCheckInputs: <kit> <so module> <legal dir | file> <result> ...
CheckErrorCode ResolveSingleFileInputs(const char* programName,
    const std::vector<std::string>& positionals, CheckConfig& config)
{
    if (positionals.size() != SF_ARG_COUNT) {
        PrintUsage(programName);
        return ERROR_PARAM_INVALID;
    }
    config.kitFileMap[positionals[ARG_INPUT]] = { positionals[SF_ARG_SO_MODULE] };
    config.legalDir = positionals[SF_ARG_LEGAL];
    // single-file mode also accepts a whitelist json file directly; a directory is
    // resolved via the {legalDir}/{kit}.json convention
    config.whitelistIsFile = IsRegularFile(config.legalDir);
    config.resultFile = positionals[SF_ARG_RESULT_FILE];
    return CHECK_OK;
}

// Positional args -> CheckConfig (kit->files map, whitelist location, result file),
// merging the ignore sources given via --ignore-file and the optional module map.
CheckErrorCode ResolveCheckInputs(const char* programName,
                                  const std::vector<std::string>& positionals,
                                  const std::vector<std::string>& ignoreSources,
                                  const std::string& moduleMapFile,
                                  CheckConfig& config)
{
    // --module-map first: the scan-dir claiming consults the map, so it must be parsed
    // before the mode resolution. Bad maps are a hard error (the caller asked for
    // map-driven behavior, silently degrading to name-convention mode would hide the
    // mapping gaps).
    if (!moduleMapFile.empty()) {
        config.moduleMapFile = moduleMapFile;
        CheckErrorCode mapErr = ParseModuleMap(config.moduleMapFile, config.moduleMap);
        if (mapErr != CHECK_OK) {
            std::cerr << "parse module map failed: " << config.moduleMapFile << std::endl;
            return mapErr;
        }
    }
    if (EndsWith(positionals[ARG_INPUT], ".json")) {
        CheckErrorCode multiErr = ResolveMultiFileInputs(programName, positionals, config);
        if (multiErr != CHECK_OK) {
            return multiErr;
        }
    } else if (IsDirectory(positionals[ARG_INPUT])) {
        CheckErrorCode scanErr = ResolveScanDirModeInputs(programName, positionals, config);
        if (scanErr != CHECK_OK) {
            return scanErr;
        }
    } else {
        CheckErrorCode singleErr = ResolveSingleFileInputs(programName, positionals, config);
        if (singleErr != CHECK_OK) {
            return singleErr;
        }
    }
    // The mixed-ABC fallback needs a .so dir: mirror the GetNativeModulePath platform defaults
    // (/system/lib64/module on device, ./module on a LINUX host).
    if (config.moduleSoDir.empty()) {
#if defined(OHOS_PLATFORM)
        config.moduleSoDir = "/system/lib64/module";
#else
        config.moduleSoDir = "./module";
#endif
    }
    config.unknownFile = DeriveSiblingJsonPath(config.resultFile, "unknown");
    config.unsupportedFile = DeriveSiblingJsonPath(config.resultFile, "unsupported");
    // --ignore-file sources (the only ignore entry point)
    for (const std::string& src : ignoreSources) {
        LoadIgnoreSource(src, config.ignoreList, config.ignoreInheritMap);
    }
    return CHECK_OK;
}

// Overwrites whitelist entry kit fields with the checklist key so the kit-level Match check
// passes despite case mismatches; the WARN keeps the data-quality issue traceable.
void NormalizeKitField(std::vector<ApiMeta>& legalList, const std::string& kit)
{
    bool normalized = false;
    for (ApiMeta& entry : legalList) {
        if (!entry.kit.empty() && entry.kit != kit) {
            entry.kit = kit;
            normalized = true;
        }
    }
    if (normalized) {
        LogWarn() << "kit field normalized (checklist key != whitelist entry kit, "
                     "fix checklist.json to match exactly): entries now use kit '" << kit << "'";
    }
}

// Loads and normalizes this kit's whitelist; returns false (skip kit) on missing/invalid data.
bool LoadKitWhitelist(const CheckConfig& config, const std::string& kit,
                      CheckSummary& summary, std::vector<ApiMeta>& legalList,
                      std::map<std::string, std::string>& inheritMap)
{
    // legalDir is either a file (used directly) or a dir resolved as {dir}/{kit}.json
    std::string legalJson = config.whitelistIsFile ? config.legalDir :
        FindWhitelistFile(config.legalDir, kit);
    if (ParseLegalApiList(legalJson, legalList, inheritMap) != CHECK_OK) {
        LogWarn() << "kit whitelist missing or invalid: " << legalJson << ", skip kit " << kit;
        if (kit == DEPRECATED_KIT_NAME) {
            return false; // warn only; the caller handles the counters
        }
        if (summary.firstError == CHECK_OK) {
            summary.firstError = ERROR_FILE_OPEN_FAILED;
        }
        return false;
    }
    NormalizeKitField(legalList, kit);
    LogInfo() << "kit " << kit << ": " << legalList.size() << " entry(ies)";
    return true;
}

struct RouteContext {
    std::string kit;
    std::string file;
    bool deprecatedKit = false;
    CheckSummary* summary = nullptr;
    std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>* illegalMap = nullptr;
    std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>* deprecatedMap = nullptr;
    std::vector<std::pair<std::string, std::string>>* unsupportedModules = nullptr;
};

// Routes module results: current kits feed illegalMap/summary, the deprecated bucket feeds
// deprecatedMap (kept separate from firstError and the exit code). Unsupported judgements
// (module map says the binary is not requireNapi-loadable) are collected for the
// unsupported-module report and excluded from the pass/flag/fail counters.
void RouteModuleResult(CheckErrorCode ret, const ModuleCheckResult& result, RouteContext& ctx)
{
    const std::vector<ApiMeta>& illegalList = result.illegalList;
    const size_t exposedCount = result.exposedCount;
    CheckSummary& summary = *ctx.summary;
    if (result.unsupported) {
        summary.moduleUnsupported++;
        ctx.unsupportedModules->push_back({NormalizeModuleName(ctx.file),
                                           result.unsupportedReason});
        return;
    }
    if (ctx.deprecatedKit) {
        summary.depApiExposed += exposedCount;
        if (ret == CHECK_OK) {
            summary.depModulePassed++;
        } else if (!illegalList.empty()) {
            summary.depModuleFlagged++;
            summary.depApiFlagged += illegalList.size();
            (*ctx.deprecatedMap)[ctx.kit][ctx.file] = illegalList;
        } else {
            summary.depModuleFailed++;
        }
        return;
    }
    summary.apiExposed += exposedCount;
    if (ret == CHECK_OK) {
        summary.filePassed++;
        return;
    }
    summary.firstError = (summary.firstError == CHECK_OK) ? ret : summary.firstError;
    if (ret == ERROR_ILLEGAL_API_EXPOSED && !illegalList.empty()) {
        summary.fileFlagged++;
        summary.apiIllegal += illegalList.size();
    } else {
        summary.fileFailed++;
    }
    if (!illegalList.empty()) {
        (*ctx.illegalMap)[ctx.kit][ctx.file] = illegalList;
    }
}

void RunKitChecks(napi_env env, const CheckConfig& config, CheckSummary& summary,
                  std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& illegalMap,
                  std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& deprecatedMap,
                  std::vector<std::pair<std::string, std::string>>& runtimeUnsupported)
{
    for (const auto& [kit, files] : config.kitFileMap) {
        const bool deprecatedKit = (kit == DEPRECATED_KIT_NAME);
        // Parse the whitelist once per kit (ignore list as the base).
        std::vector<ApiMeta> legalList = config.ignoreList;
        std::map<std::string, std::string> inheritMap = config.ignoreInheritMap;
        if (!LoadKitWhitelist(config, kit, summary, legalList, inheritMap)) {
            if (deprecatedKit) {
                summary.depModuleFailed += files.size();
            } else {
                summary.fileFailed += files.size();
            }
            continue;
        }

        LegalIndex legalIndex;
        BuildLegalIndex(legalList, legalIndex);
        std::set<std::string> expandableNames;
        CollectExpandableNames(legalList, expandableNames);

        KitContext kitCtx;
        kitCtx.legalIndex = &legalIndex;
        kitCtx.inheritMap = &inheritMap;
        kitCtx.expandableNames = &expandableNames;
        kitCtx.soDir = config.moduleSoDir;
        kitCtx.moduleMap = config.moduleMap.Empty() ? nullptr : &config.moduleMap;

        for (const auto& file : files) {
            if (config.skipModules.count(NormalizeModuleName(file)) > 0) {
                LogInfo() << "skip module (user request): " << kit << ": " << file;
                summary.moduleSkipped++;
                continue;
            }
            LogInfo() << "CheckSingleFile: " << kit << ": " << file;
            ModuleCheckResult result;
            CheckErrorCode ret = CheckSingleFile(env, kitCtx, kit, file, result);
            RouteContext routeCtx;
            routeCtx.kit = kit;
            routeCtx.file = file;
            routeCtx.deprecatedKit = deprecatedKit;
            routeCtx.summary = &summary;
            routeCtx.illegalMap = &illegalMap;
            routeCtx.deprecatedMap = &deprecatedMap;
            routeCtx.unsupportedModules = &runtimeUnsupported;
            RouteModuleResult(ret, result, routeCtx);
            // Checkpoint after each module (atomic write): a native crash mid-run keeps every
            // result checked so far; the last log line marks the crash point. The unknown
            // and unsupported saves apply the same omit-when-empty rule as the final output
            // (the skip lives in the savers), so all paths behave identically.
            SaveIllegalApiToJson(config.resultFile, illegalMap, deprecatedMap);
            SaveUnknownModulesToJson(config.unknownFile, config.unknownModules);
            std::vector<std::pair<std::string, std::string>> unsupportedSoFar = config.unsupportedModules;
            unsupportedSoFar.insert(unsupportedSoFar.end(), runtimeUnsupported.begin(),
                                    runtimeUnsupported.end());
            SaveUnsupportedModulesToJson(config.unsupportedFile, unsupportedSoFar);
            LogDebug() << "checkpoint written: " << config.resultFile;
            if (!config.unknownModules.empty()) {
                LogDebug() << "checkpoint written: " << config.unknownFile;
            }
            if (!unsupportedSoFar.empty()) {
                LogDebug() << "checkpoint written: " << config.unsupportedFile;
            }
        }
    }
}

void PrintSummary(const CheckSummary& summary, size_t unknownCount, size_t unsupportedCount)
{
    std::cout << "===== 检查完成 =====" << std::endl;
    std::cout << "模块: " << summary.filePassed << " 全部合法 / " << summary.fileFlagged <<
        " 检出非法 / " << summary.fileFailed << " 失败未检 / " << summary.moduleSkipped <<
        " 跳过 / " << summary.moduleUnsupported << " 无法检视" << std::endl;
    std::cout << "导出API: " << summary.apiExposed << " 条, 合法 " <<
        (summary.apiExposed - summary.apiIllegal) << " / 非法 " << summary.apiIllegal << std::endl;
    // deprecated bucket exists only when scan-dir mapped modules onto the deprecated whitelist
    if (summary.depModulePassed + summary.depModuleFlagged + summary.depModuleFailed > 0) {
        std::cout << "废弃桶: " << summary.depModulePassed << " 通过 / " <<
            summary.depModuleFlagged << " 检出 / " << summary.depModuleFailed <<
            " 失败, 导出API " << summary.depApiExposed << " 条, 合法 " <<
            (summary.depApiExposed - summary.depApiFlagged) << " / 检出 " <<
            summary.depApiFlagged << " 条(快照失配)" << std::endl;
    }
    std::cout << "未登记模块: " << unknownCount << std::endl;
    std::cout << "无法检视模块: " << unsupportedCount <<
        " (映射二进制非requireNapi或缺失, 详见unsupported报告)" << std::endl;
    std::cout << "==================" << std::endl;
}

void PrintDeprecatedResult(const std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& deprecatedMap)
{
    if (deprecatedMap.empty()) {
        return;
    }
    std::cout << "===== 废弃模块结果（快照失配，需结合继任者白名单人工研判）=====" << std::endl;
    for (const auto& [kit, fileMap] : deprecatedMap) {
        for (const auto& [file, deprecatedList] : fileMap) {
            for (const auto& apiMeta : deprecatedList) {
                std::cout << "[deprecated] " << kit << " | " << file << " | " << apiMeta.apiText <<
                    " | " << apiMeta.apiType << " | " << apiMeta.className << std::endl;
            }
        }
    }
}

// Prints illegal APIs line by one (hdc shell echoes stdout), then the deprecated and
// unknown-module sections, and archives the result json plus the derived unknown-module
// and unsupported-module jsons. Returns the illegal-API total (deprecated-bucket entries
// excluded).
size_t PrintIllegalResult(const std::string& resultFile, const std::string& unknownFile,
                          const std::string& unsupportedFile,
                          const std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& illegalMap,
                          const std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& deprecatedMap,
                          const std::vector<std::string>& unknownModules,
                          const std::vector<std::pair<std::string, std::string>>& unsupportedModules)
{
    size_t illegalTotal = 0;
    for (const auto& [kit, fileMap] : illegalMap) {
        for (const auto& [file, illegalList] : fileMap) {
            for (const auto& apiMeta : illegalList) {
                illegalTotal++;
                std::cout << "[illegal] " << kit << " | " << file << " | " << apiMeta.apiText <<
                    " | " << apiMeta.apiType << " | " << apiMeta.className << std::endl;
            }
        }
    }
    PrintDeprecatedResult(deprecatedMap);
    for (const std::string& name : unknownModules) {
        std::cout << "[unknown-module] " << name << std::endl;
    }
    for (const auto& [name, reason] : unsupportedModules) {
        std::cout << "[unsupported-module] " << name << " | " << reason << std::endl;
    }
    SaveIllegalApiToJson(resultFile, illegalMap, deprecatedMap);
    LogInfo() << "非法API报告已保存: " << resultFile;
    // omit-when-empty: no unknown.json when there is nothing to report (SaveUnknownModulesToJson)
    SaveUnknownModulesToJson(unknownFile, unknownModules);
    if (!unknownModules.empty()) {
        LogInfo() << "未登记模块报告已保存: " << unknownFile;
    }
    SaveUnsupportedModulesToJson(unsupportedFile, unsupportedModules);
    if (!unsupportedModules.empty()) {
        LogInfo() << "无法检视模块报告已保存: " << unsupportedFile;
    }
    return illegalTotal;
}

void ParseSkipModules(const std::vector<std::string>& skipArgs, std::set<std::string>& skipModules)
{
    for (const std::string& list : skipArgs) {
        std::istringstream listStream(list);
        std::string item;
        while (std::getline(listStream, item, ',')) {
            std::string normalized = NormalizeModuleName(item);
            if (!normalized.empty()) {
                skipModules.insert(normalized);
            }
        }
    }
}

// Real entry point: main() only forwards at the bottom (the linker fixes the name "main").
int CheckApiMain(int argc, char* argv[])
{
    std::cout << "===== API 非法暴露校验工具 =====" << std::endl;
    if (argc < MIN_ARGC) {
        PrintUsage(argv[0]);
        return ERROR_PARAM_INVALID;
    }

    std::vector<std::string> positionals;
    std::vector<std::string> ignoreSources; // --ignore-file sources (files/dirs)
    std::vector<std::string> skipArgs;      // --skip-module lists (comma-separated)
    std::string moduleMapFile;              // --module-map json (empty when absent)
    CheckConfig config;
    if (!ParseCommandLine(argc, argv, positionals, ignoreSources, skipArgs, moduleMapFile)) {
        return ERROR_PARAM_INVALID;
    }
    if (positionals.empty()) {
        PrintUsage(argv[0]);
        return ERROR_PARAM_INVALID;
    }
    ParseSkipModules(skipArgs, config.skipModules);
    CheckErrorCode inputErr = ResolveCheckInputs(argv[0], positionals, ignoreSources,
                                                 moduleMapFile, config);
    if (inputErr != CHECK_OK) {
        return inputErr;
    }

    napi_env env = CreateArkEnv();
    if (env == nullptr) {
        std::cerr << "CreateArkEnv failed" << std::endl;
        return ERROR_JS_ENGINE_INIT;
    }

    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env, &scope);

    std::map<std::string, std::map<std::string, std::vector<ApiMeta>>> illegalMap;
    std::map<std::string, std::map<std::string, std::vector<ApiMeta>>> deprecatedMap;
    std::vector<std::pair<std::string, std::string>> runtimeUnsupported;
    CheckSummary summary;
    RunKitChecks(env, config, summary, illegalMap, deprecatedMap, runtimeUnsupported);

    napi_close_handle_scope(env, scope);
    DestroyArkEnv(env);

    // Unsupported modules come from two fill points: scan-dir claiming (modules never
    // entering the check loop) and check-time judgement (multi/single-file mode). Merge
    // both, keeping scan order first.
    std::vector<std::pair<std::string, std::string>> unsupportedModules = config.unsupportedModules;
    unsupportedModules.insert(unsupportedModules.end(), runtimeUnsupported.begin(),
                              runtimeUnsupported.end());

    PrintIllegalResult(config.resultFile, config.unknownFile, config.unsupportedFile,
                       illegalMap, deprecatedMap, config.unknownModules, unsupportedModules);
    PrintSummary(summary, config.unknownModules.size(), unsupportedModules.size());
    CloseLogFile();

    // The exit code excludes the deprecated bucket and the unsupported judgements: known
    // snapshot mismatch and known non-requireNapi binaries must not drown the main verdict;
    // suspected real findings stay in their dedicated sections/artifacts.
    if (illegalMap.empty() && config.unknownModules.empty() && summary.firstError == CHECK_OK) {
        return CHECK_OK;
    }
    if (summary.firstError != CHECK_OK) {
        return summary.firstError;
    }
    return illegalMap.empty() ? ERROR_UNKNOWN_MODULE_EXPOSED : ERROR_ILLEGAL_API_EXPOSED;
}

} // namespace

int main(int argc, char* argv[])
{
    return CheckApiMain(argc, argv);
}
