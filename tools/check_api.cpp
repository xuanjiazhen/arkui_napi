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

// 检查任务配置：命令行解析后的完整输入（kit→文件映射、白名单位置、ignore 来源）
struct CheckConfig {
    std::map<std::string, std::vector<std::string>> kitFileMap;
    std::string legalDir;
    std::string resultFile;
    bool whitelistIsFile = false; // legalDir 直接给了白名单 json 文件（单文件模式）
    std::vector<ApiMeta> ignoreList;
    std::map<std::string, std::string> ignoreInheritMap; // ignore list 中的继承关系
    std::vector<std::string> unknownModules; // 目录扫描模式：无白名单登记的 so（requireName 形态）
    std::set<std::string> skipModules; // --skip-module 指定的跳过模块（归一化后的模块名）
    std::string moduleSoDir; // 模块 so 目录（scan-dir 的位置参数/平台默认，混合 ABC 备选用）
};

// 废弃模块桶的白名单文件名（数据侧收纳 API 10 及以下废弃旧名模块的桶文件，
// scan-dir 模式按"文件名即 kit"规则，设备上的旧名 so 会全部映射到这个"kit"）。
// 其内容冻结在 API<=8，而旧名 so 背后是与新名 so 共享的现行实现（API 20+），
// 用冻结快照校验现行实现必然大面积失配（真机实测 340 条 flag 中 191 条为
// 快照失配噪音、仅 85 条为形态层差异）。对该桶的结果单独归类为
// DeprecatedModules：不混入 IllegalApis、不计入失败、不影响退出码——真发现
// （全库零声明条目，如 RdbPredicatesV9）仍会呈现在该段供人工研判。
// 根治需数据侧提供新旧模块名别名表做继任者白名单并集（分析报告方案 A）。
constexpr const char* DEPRECATED_KIT_NAME = "API10LessDeprecatedModules";

// 单个 kit 的检查上下文（白名单索引与可展开名集合，由 kit 循环构建）
struct KitContext {
    const LegalIndex* legalIndex = nullptr;
    const std::map<std::string, std::string>* inheritMap = nullptr;
    const std::set<std::string>* expandableNames = nullptr;
    std::string soDir; // 模块 so 目录（混合模块 ABC 备选通道的 so 定位）
};

// 检查统计：模块级与导出API级双向计数，现行 kit 与废弃桶（DEPRECATED_KIT_
// NAME）通道分开。模块级三分：全部合法 / 检出非法 / 失败未检（加载失败、
// 参数非法、白名单缺失）。API 级：遍历产出条数与其中非法条数（合法 =
// exposed - illegal；被忽略属性——内置名/数字键/_napiwrapper——不产条目，
// 不计入 exposed）。
struct CheckSummary {
    // 现行 kit 通道
    size_t filePassed = 0;   // 模块：全部导出匹配合法
    size_t fileFlagged = 0;  // 模块：存在非法API暴露
    size_t fileFailed = 0;   // 模块：加载失败/参数非法/未检
    size_t apiExposed = 0;   // 导出API：遍历条数
    size_t apiIllegal = 0;   // 导出API：非法条数
    // 废弃桶通道（快照失配，独立呈现，不计入上方与退出码）
    size_t depModulePassed = 0;
    size_t depModuleFlagged = 0;
    size_t depModuleFailed = 0;
    size_t depApiExposed = 0;
    size_t depApiFlagged = 0;
    size_t moduleSkipped = 0; // --skip-module 命中跳过的模块数
    CheckErrorCode firstError = CHECK_OK;
};

void PrintUsage(const char* name)
{
    std::cerr << "Usage(multi-file):  " << name <<
        " <check list json> <legal dir of kit json> <result file> [ignore list] [options]" << std::endl;
    std::cerr << "Usage(scan-dir):    " << name <<
        " <module so dir> <legal dir of kit json> <result file> [ignore list] [options]" << std::endl;
    std::cerr << "Usage(single-file): " << name <<
        " <kit name> <so module name> <legal dir of kit json> <result file>" <<
        " [ignore list] [options]" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  --log-level <DEBUG|INFO|WARN|ERROR>   log level, default INFO" << std::endl;
    std::cerr << "  --log-file <file>                     write log to file (WARN/ERROR" <<
        " still printed to stderr)" << std::endl;
    std::cerr << "  --ignore-file <path>...               ignore list file(s)/dir(s), greedy until" <<
        " next option" << std::endl;
    std::cerr << "  --skip-module <name[,name...]>        skip checking given module(s), e.g." <<
        " @ohos.util (crash isolation); summary shows skip count" << std::endl;
    std::cerr << "Note: options should be placed after positional arguments." << std::endl;
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

// 模块名归一：剥声明文件后缀（@ohos.x.d.ts / @ohos.x.d.ets -> @ohos.x）。
// checklist 值、scan-dir 反查名、--skip-module 参数三种来源统一后比较
std::string NormalizeModuleName(const std::string& file)
{
    std::string moduleName = file;
    if (EndsWith(moduleName, ".d.ts")) {
        moduleName = moduleName.substr(0, moduleName.size() - 5); // 5: strlen(".d.ts")
    } else if (EndsWith(moduleName, ".d.ets")) {
        moduleName = moduleName.substr(0, moduleName.size() - 6); // 6: strlen(".d.ets")
    }
    return moduleName;
}

std::string ToLower(const std::string& s)
{
    std::string out = s;
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return out;
}

// 解析 kit 白名单文件路径：直接路径优先（零开销）；不存在时在白名单目录内
// 做大小写不敏感兜底——checklist 的 kit 键与白名单文件名大小写常不一致
// （实测 DrmKit vs DRMKit.json、Penkit vs PenKit.json、DeskTopExtensionKit vs
// DesktopExtensionKit.json、WearEngine vs WearEngineKit.json，4 个 kit 整体
// 被跳过）。仍未命中则返回原路径，由 ParseLegalApiList 的 open failed 报错。
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
        LOG_WARN("kit whitelist resolved case-insensitively: " << kit << " -> " << found);
        return found;
    }
    return direct;
}

// 载入一个 ignore 来源：json 文件，或目录（目录下所有 .json 逐个解析合并）
void LoadIgnoreSource(const std::string& path,
                      std::vector<ApiMeta>& ignoreList,
                      std::map<std::string, std::string>& ignoreInheritMap)
{
    if (IsDirectory(path)) {
        DIR* dir = opendir(path.c_str());
        if (dir == nullptr) {
            LOG_WARN("opendir failed: " << path);
            return;
        }
        struct dirent* entry = nullptr;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (EndsWith(name, ".json")) {
                std::string child = path + "/" + name;
                LOG_INFO("load ignore file: " << child);
                ParseLegalApiList(child, ignoreList, ignoreInheritMap);
            }
        }
        closedir(dir);
        return;
    }
    LOG_INFO("load ignore file: " << path);
    if (ParseLegalApiList(path, ignoreList, ignoreInheritMap) != CHECK_OK) {
        LOG_WARN("parse ignore file failed: " << path);
    }
}

// so 文件名 → requireName 形态模块名（全小写）：
// 剥 "lib" 前缀与 so 后缀、"_napi" 变体与主形态同名去重。
// 这是 GetNativeModulePath 正向映射（模块名 tolower + 点号全部转目录）的
// 逆运算：目录部分（relDir）的每一级分隔符都要还原为点号——
// "hms/core/ar" + "libarengine.z.so" 的 requireName 是 "hms.core.ar.arengine"
// 而不是 "hms/core/ar.arengine"。旧实现只拼不转，多级目录模块反查全部失败
// （实测 737 个"未登记模块"中 hms/、arkts/ 等多级目录全部中招）。
void SoFileToModuleName(const std::string& relDir, const std::string& fileName,
                        std::set<std::string>& names)
{
    const std::string libPrefix = "lib";
    if (!StartsWith(fileName, libPrefix)) {
        return;
    }
    std::string stem = fileName.substr(libPrefix.size());
    if (EndsWith(stem, ".z.so")) {
        stem = stem.substr(0, stem.size() - 5); // 5: strlen(".z.so")
    } else if (EndsWith(stem, ".so")) {
        stem = stem.substr(0, stem.size() - 3); // 3: strlen(".so")
    } else {
        return;
    }
    const std::string napiSuffix = "_napi";
    if (EndsWith(stem, napiSuffix)) {
        stem = stem.substr(0, stem.size() - napiSuffix.size());
    }
    if (stem.empty()) {
        return;
    }
    std::string dottedRelDir = relDir;
    std::replace(dottedRelDir.begin(), dottedRelDir.end(), '/', '.');
    std::string name = dottedRelDir.empty() ? stem : dottedRelDir + "." + stem;
    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
    names.insert(name);
}

// 递归枚举模块目录（如 /system/lib64/module）下全部 so，产出 requireName 集合。
// relDir 为当前层相对目录（顶层为空串），避免 full 路径反复拼接与递归栈膨胀。
void EnumerateSoModules(const std::string& moduleDir, const std::string& relDir,
                        std::set<std::string>& names)
{
    std::string full = relDir.empty() ? moduleDir : moduleDir + "/" + relDir;
    DIR* dir = opendir(full.c_str());
    if (dir == nullptr) {
        LOG_WARN("opendir failed: " << full);
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
            EnumerateSoModules(moduleDir, childRel, names);
        } else if (S_ISREG(st.st_mode)) {
            SoFileToModuleName(relDir, name, names);
        }
    }
    closedir(dir);
}

// 目录扫描模式输入组装：枚举模块目录得 requireName 集合 → 遍历白名单目录
// {kit}.json 提取规范模块名 → 复用 K5 映射（ModuleNameForRequireNapi）换算
// requireName 做大小写不敏感反查。命中者进 kitFileMap（保留规范名大小写供
// file 坐标匹配），未命中者进 unknownModules（无公开 API 登记的 so）。
// kit 取自白名单 json 文件名，与 RunKitChecks 的 {legalDir}/{kit}.json 约定一致。
CheckErrorCode ResolveScanDirInputs(const std::string& moduleDir, const std::string& legalDir,
                                    CheckConfig& config)
{
    std::set<std::string> requireNames;
    EnumerateSoModules(moduleDir, "", requireNames);
    LOG_INFO("scan-dir: " << requireNames.size() << " module(s) under " << moduleDir);
    if (requireNames.empty()) {
        std::cerr << "no module so found under: " << moduleDir << std::endl;
        return ERROR_PARAM_INVALID;
    }

    DIR* dir = opendir(legalDir.c_str());
    if (dir == nullptr) {
        std::cerr << "open whitelist dir failed: " << legalDir << std::endl;
        return ERROR_FILE_OPEN_FAILED;
    }
    std::set<std::string> mapped; // 已归属的 requireName（防同一模块跨 kit 重复检查）
    struct dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        std::string name = entry->d_name;
        if (!EndsWith(name, ".json")) {
            continue;
        }
        std::string kit = name.substr(0, name.size() - 5); // 5: strlen(".json")
        std::set<std::string> canonicalNames;
        if (CollectModuleNames(legalDir + "/" + name, canonicalNames) != CHECK_OK) {
            LOG_WARN("scan-dir: skip invalid whitelist: " << name);
            continue;
        }
        for (const std::string& canonical : canonicalNames) {
            std::string key = ModuleNameForRequireNapi(canonical);
            std::transform(key.begin(), key.end(), key.begin(), ::tolower);
            if (requireNames.count(key) > 0 && mapped.insert(key).second) {
                config.kitFileMap[kit].push_back(canonical);
            }
        }
    }
    closedir(dir);

    for (const std::string& name : requireNames) {
        if (mapped.count(name) == 0) {
            config.unknownModules.push_back(name);
        }
    }
    LOG_INFO("scan-dir: " << config.kitFileMap.size() << " kit(s), " <<
        config.unknownModules.size() << " unknown module(s)");
    return CHECK_OK;
}

// 模块级隔离守卫说明见 ModuleCheckGuard 定义处
struct ModuleCheckResult {
    std::vector<ApiMeta> exposedList;
    std::vector<ApiMeta> illegalList;
    size_t exposedCount = 0;
};

// 单模块加载上下文：CheckSingleFile 分支加载函数的公共输入
struct ModuleLoadInfo {
    std::string kit;
    std::string file;       // checklist/命令行原始值（.abc displayName 用）
    std::string moduleName; // 归一化后的模块名（或 .abc 路径）
};

// 加载并遍历独立 .abc 文件（CheckSingleFile 的 abc 分支提取）
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

// requireNapi 加载 + 混合 so ABC 备选（CheckSingleFile 的 so 分支提取）
CheckErrorCode LoadSoExposed(napi_env env, const KitContext& kitCtx,
                              const ModuleLoadInfo& info, ModuleCheckResult& result)
{
    SoLoadParam param;
    param.kit = info.kit;
    param.displayName = info.moduleName;
    param.requireName = ModuleNameForRequireNapi(info.moduleName);
    CheckErrorCode err = LoadSoAndGetExposedMeta(env, param, result.exposedList, kitCtx.expandableNames);
    if (err == CHECK_OK) {
        return CHECK_OK;
    }
    // 混合 so（内嵌 ArkTS ABC）备选通道：requireNapi 走引擎 LoadArkModule
    // 在裸进程缺运行时上下文会失败（真机 103 个混合模块即此因）——
    // 直接 dlopen so 提取 ABC 字节码、多候选记录名执行
    AbcLoadParam abcParam;
    abcParam.kit = info.kit;
    abcParam.displayName = info.moduleName;
    abcParam.requireName = param.requireName;
    abcParam.expandableNames = kitCtx.expandableNames;
    for (const std::string& soPath : CandidateSoPaths(kitCtx.soDir, param.requireName)) {
        if (!IsRegularFile(soPath) || !SoHasEmbeddedAbc(soPath)) {
            continue;
        }
        abcParam.soPath = soPath;
        err = LoadMixedSoAbcAndGetExposedMeta(env, abcParam, result.exposedList);
        if (err == CHECK_OK) {
            return CHECK_OK;
        }
    }
    return err;
}

// 检查单个产物文件。白名单相关结构（legalIndex/inheritMap/expandable）由调用方
// 按 kit 预构建（KitContext），避免同一 kit 多文件时重复解析（ArkUI 2 万条白名单 × 79 文件）。
CheckErrorCode CheckSingleFile(napi_env env,
                               const KitContext& kitCtx,
                               const std::string& kit,
                               const std::string& file,
                               ModuleCheckResult& result)
{
    // 模块级隔离：进入时清残留挂起异常并开独立 handle scope，退出时复原。
    // 此前所有模块共用一个 scope 且从不清异常，单个模块遍历导出时 getter
    // 抛出的异常会永久污染 env，后续模块的 requireNapi 查找（napi_get_named_
    // property 带 NAPI_PREAMBLE）全部短路失败（真机 515/597 连续误报）。
    ModuleCheckGuard guard(env);

    result.exposedCount = 0;
    CheckErrorCode err = CHECK_OK;
    // apiFileName 形态归一（见 NormalizeModuleName）：声明文件名与运行时模块名统一
    ModuleLoadInfo info;
    info.kit = kit;
    info.file = file;
    info.moduleName = NormalizeModuleName(file);
    if (EndsWith(info.moduleName, ".abc")) {
        err = LoadAbcExposed(env, kitCtx, info, result);
#if defined(OHOS_PLATFORM) || defined(LINUX_PLATFORM)
    } else if (StartsWith(info.moduleName, "@")) {
        err = LoadSoExposed(env, kitCtx, info, result);
#else
    } else if (StartsWith(info.moduleName, "@")) {
        LOG_ERROR("非ohos/linux平台不支持检查 so 模块: " << info.moduleName);
        return ERROR_PARAM_INVALID;
#endif
    } else {
        LOG_ERROR("当前仅支持HarmonyOS模块名(@ohos./@arkts./@hms./@system.开头): " << info.moduleName);
        return ERROR_PARAM_INVALID;
    }
    if (err != CHECK_OK) {
        return err;
    }

    // 遍历成功：记录该模块导出条数（供统计：合法 = exposed - illegal）
    result.exposedCount = result.exposedList.size();

    err = CheckIllegalExposed(*kitCtx.legalIndex, result.exposedList, *kitCtx.inheritMap,
                              result.illegalList);
    const size_t nExposed = result.exposedList.size();
    const size_t nIllegal = result.illegalList.size();
    if (err != CHECK_OK) {
        LOG_WARN("检查不通过: 存在非法API暴露: " << info.moduleName << " (导出 " << nExposed <<
                 " 条, 合法 " << (nExposed - nIllegal) << ", 非法 " << nIllegal << ")");
        return err;
    }
    LOG_INFO("检查通过: 无非法API暴露: " << info.moduleName << " (导出 " << nExposed << " 条)");
    return CHECK_OK;
}

// 解析命令行选项与位置参数。选项非法时打印错误（部分场景含用法）并返回 false。
bool ParseCommandLine(int argc, char* argv[], std::vector<std::string>& positionals,
                      std::vector<std::string>& ignoreSources,
                      std::vector<std::string>& skipModules)
{
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--log-level") {
            LogLevel level = LogLevel::INFO;
            if (i + 1 >= argc || !ParseLogLevel(argv[++i], level)) {
                std::cerr << "invalid --log-level value" << std::endl;
                PrintUsage(argv[0]);
                return false;
            }
            SetLogLevel(level);
        } else if (arg == "--log-file") {
            if (i + 1 >= argc || !SetLogFile(argv[++i])) {
                std::cerr << "invalid --log-file path" << std::endl;
                return false;
            }
        } else if (arg == "--ignore-file") {
            // 贪婪消费后续参数直到下一个选项；只接受"以 .json 结尾或磁盘上存在"的
            // 路径，避免选项与位置参数交错时误吞位置参数（真机命令中选项位于全部
            // 位置参数之后，不受影响）
            bool consumed = false;
            while (i + 1 < argc && argv[i + 1][0] != '-' &&
                   (EndsWith(argv[i + 1], ".json") || FileExists(argv[i + 1]))) {
                ignoreSources.push_back(argv[++i]);
                consumed = true;
            }
            if (!consumed) {
                std::cerr << "--ignore-file requires at least one path" << std::endl;
                PrintUsage(argv[0]);
                return false;
            }
        } else if (arg == "--skip-module") {
            // 崩溃隔离：跳过指定模块（逗号分隔清单，可多次给出）。
            // 值以 '@' 开头，不会与选项混淆；命中与否在汇总行"跳过 N"可见
            if (i + 1 >= argc || argv[i + 1][0] == '-') {
                std::cerr << "--skip-module requires a module name list" << std::endl;
                PrintUsage(argv[0]);
                return false;
            }
            skipModules.push_back(argv[++i]);
        } else if (arg.size() > 1 && arg[0] == '-' && arg[1] == '-') {
            std::cerr << "unknown option: " << arg << std::endl;
            PrintUsage(argv[0]);
            return false;
        } else {
            positionals.push_back(arg);
        }
    }
    return true;
}

// multi-file 模式输入解析（ResolveCheckInputs 的 .json 分支提取）
CheckErrorCode ResolveMultiFileInputs(const char* programName,
                                     const std::vector<std::string>& positionals,
                                     const std::vector<std::string>& ignoreSources,
                                     CheckConfig& config)
{
    if (positionals.size() < 3) {
        PrintUsage(programName);
        return ERROR_PARAM_INVALID;
    }
    if (ParseKitToCheckFiles(positionals[0], config.kitFileMap) != CHECK_OK) {
        std::cerr << "Parse input file fail: " << positionals[0] << std::endl;
        return ERROR_JSON_PARSE_FAILED;
    }
    // 多文件模式覆盖多个 kit，白名单参数必须为目录
    if (IsRegularFile(positionals[1])) {
        std::cerr << "multi-file mode requires a whitelist DIRECTORY, got file: " <<
            positionals[1] << std::endl;
        return ERROR_PARAM_INVALID;
    }
    config.legalDir = positionals[1];
    config.resultFile = positionals[2];
    if (positionals.size() > 3) {
        LoadIgnoreSource(positionals[3], config.ignoreList, config.ignoreInheritMap);
    }
    return CHECK_OK;
}

// 位置参数 → 检查任务配置（kit→文件映射、白名单位置、结果文件），
// 并合并 --ignore-file 指定的 ignore 来源
CheckErrorCode ResolveCheckInputs(const char* programName,
                                  const std::vector<std::string>& positionals,
                                  const std::vector<std::string>& ignoreSources,
                                  CheckConfig& config)
{
    if (EndsWith(positionals[0], ".json")) {
        CheckErrorCode multiErr = ResolveMultiFileInputs(programName, positionals, ignoreSources, config);
        if (multiErr != CHECK_OK) {
            return multiErr;
        }
    } else if (IsDirectory(positionals[0])) {
        // scan-dir：枚举 module 目录（/system/lib64/module）全部 so，
        // kit 归属从白名单目录自派生，checklist.json 不再需要
        if (positionals.size() < 3) {
            PrintUsage(programName);
            return ERROR_PARAM_INVALID;
        }
        if (IsRegularFile(positionals[1])) {
            std::cerr << "scan-dir mode requires a whitelist DIRECTORY, got file: " <<
                positionals[1] << std::endl;
            return ERROR_PARAM_INVALID;
        }
        CheckErrorCode err = ResolveScanDirInputs(positionals[0], positionals[1], config);
        if (err != CHECK_OK) {
            return err;
        }
        config.moduleSoDir = positionals[0];
        config.legalDir = positionals[1];
        config.resultFile = positionals[2];
        if (positionals.size() > 3) {
            LoadIgnoreSource(positionals[3], config.ignoreList, config.ignoreInheritMap);
        }
    } else {
        // single file
        if (positionals.size() < 4) {
            PrintUsage(programName);
            return ERROR_PARAM_INVALID;
        }
        config.kitFileMap[positionals[0]] = { positionals[1] };
        config.legalDir = positionals[2];
        // 需求接口的 apiFilePath 为"合法白名单文件"：单文件模式下允许直接传
        // 白名单 json 文件（该文件即本 kit 的白名单）；传目录则按
        // {legalDir}/{kit}.json 约定解析（真机命令形态）
        config.whitelistIsFile = IsRegularFile(config.legalDir);
        config.resultFile = positionals[3];
        if (positionals.size() > 4) {
            LoadIgnoreSource(positionals[4], config.ignoreList, config.ignoreInheritMap);
        }
    }
    // 非 scan-dir 模式的混合 ABC 备选需要 so 目录：按引擎 GetNativeModulePath
    // 的平台默认前缀（OHOS 设备 /system/lib64/module，宿主 LINUX ./module）
    if (config.moduleSoDir.empty()) {
#if defined(OHOS_PLATFORM)
        config.moduleSoDir = "/system/lib64/module";
#else
        config.moduleSoDir = "./module";
#endif
    }
    // --ignore-file 指定的来源（可与位置参数形式的 ignore list 共存）
    for (const std::string& src : ignoreSources) {
        LoadIgnoreSource(src, config.ignoreList, config.ignoreInheritMap);
    }
    return CHECK_OK;
}

// 每个 kit 的检查编排：解析白名单（基础为 ignore list）→构建索引→逐文件检查
// 每个 kit 的检查编排：解析白名单（基础为 ignore list）→构建索引→逐文件检查。
// 废弃桶 kit（DEPRECATED_KIT_NAME）的结果路由到 deprecatedMap 独立归类，
// 不计入 summary 的通过/失败、不设置 firstError（见常量处注释）。
// kit 归一：白名单条目的 kit 字段（如 DRMKit）与 checklist 键（如 DrmKit）
// 大小写不一致时，Match 的 kit 级校验会全拒——整 kit 伪报。FindWhitelistFile
// 解决了"找得到文件"，这里解决"条目对得上键"：覆写为 kitFileMap 的键。
// 加载链已按 kit 键精确取文件，条目因"来自本文件"而只属于本 kit，
// 覆写不扩大放行面（跨 kit 泄漏由 file 级守卫拦截）。
// 规格要求 checklist 键与白名单文件名完全一致（DESIGN_SPEC.md 2.2.4），
// 大小写错配属数据质量问题——覆写时打 WARN 保持可追溯。
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
        LOG_WARN("kit field normalized (checklist key != whitelist entry kit, "
                 "fix checklist.json to match exactly): entries now use kit '" << kit << "'");
    }
}

// 加载并归一化本 kit 的白名单。失败时按 deprecated/现行 分别计数并返回 false
bool LoadKitWhitelist(const CheckConfig& config, const std::string& kit,
                      CheckSummary& summary, std::vector<ApiMeta>& legalList,
                      std::map<std::string, std::string>& inheritMap)
{
    // legalDir 为文件时直接用作本 kit 白名单；为目录时按 {dir}/{kit}.json 解析
    std::string legalJson = config.whitelistIsFile ? config.legalDir :
        FindWhitelistFile(config.legalDir, kit);
    if (ParseLegalApiList(legalJson, legalList, inheritMap) != CHECK_OK) {
        LOG_WARN("kit whitelist missing or invalid: " << legalJson << ", skip kit " << kit);
        if (kit == DEPRECATED_KIT_NAME) {
            return false; // 只告警，计数由调用方处理（保持旧口径）
        }
        if (summary.firstError == CHECK_OK) {
            summary.firstError = ERROR_FILE_OPEN_FAILED;
        }
        return false;
    }
    NormalizeKitField(legalList, kit);
    LOG_INFO("kit " << kit << ": " << legalList.size() << " entry(ies)");
    return true;
}

// 路由上下文：RouteModuleResult 的输入集合（kit 标识 + 白名单归属 + 输出容器）
struct RouteContext {
    std::string kit;
    std::string file;
    bool deprecatedKit = false;
    CheckSummary* summary = nullptr;
    std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>* illegalMap = nullptr;
    std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>* deprecatedMap = nullptr;
};

// 模块结果路由：现行 kit 计入 illegalMap/summary；废弃桶计入 deprecatedMap
// （独立归类，不影响 firstError/退出码）
void RouteModuleResult(CheckErrorCode ret, const ModuleCheckResult& result, RouteContext& ctx)
{
    const std::vector<ApiMeta>& illegalList = result.illegalList;
    const size_t exposedCount = result.exposedCount;
    CheckSummary& summary = *ctx.summary;
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
                  std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& deprecatedMap)
{
    for (const auto& [kit, files] : config.kitFileMap) {
        const bool deprecatedKit = (kit == DEPRECATED_KIT_NAME);
        // 每个kit解析一次白名单（基础为ignore list），构建索引与可展开名集合
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

        for (const auto& file : files) {
            if (config.skipModules.count(NormalizeModuleName(file)) > 0) {
                LOG_INFO("skip module (user request): " << kit << ": " << file);
                summary.moduleSkipped++;
                continue;
            }
            LOG_INFO("CheckSingleFile: " << kit << ": " << file);
            ModuleCheckResult result;
            CheckErrorCode ret = CheckSingleFile(env, kitCtx, kit, file, result);
            RouteContext routeCtx;
            routeCtx.kit = kit;
            routeCtx.file = file;
            routeCtx.deprecatedKit = deprecatedKit;
            routeCtx.summary = &summary;
            routeCtx.illegalMap = &illegalMap;
            routeCtx.deprecatedMap = &deprecatedMap;
            RouteModuleResult(ret, result, routeCtx);
            // 增量落盘：每个已检查模块（含通过/废弃桶）之后原子写盘（临时文件+
            // rename，见 SaveIllegalApiToJson）。全量加载新常态下单模块 Init/遍历
            // 可能 native 崩溃（真机实测死在 @ohos.util 遍历中）——进程死亡时
            // 已检结果不丢，日志尾行即死亡点
            SaveIllegalApiToJson(config.resultFile, illegalMap, deprecatedMap, config.unknownModules);
            LOG_DEBUG("checkpoint written: " << config.resultFile);
        }
    }
}

// 汇总块输出：模块三分 + 导出API 合法/非法 + 废弃桶（有映射时）+ 未登记
void PrintSummary(const CheckSummary& summary, size_t unknownCount)
{
    std::cout << "===== 检查完成 =====" << std::endl;
    std::cout << "模块: " << summary.filePassed << " 全部合法 / " << summary.fileFlagged <<
        " 检出非法 / " << summary.fileFailed << " 失败未检 / " << summary.moduleSkipped <<
        " 跳过" << std::endl;
    std::cout << "导出API: " << summary.apiExposed << " 条, 合法 " <<
        (summary.apiExposed - summary.apiIllegal) << " / 非法 " << summary.apiIllegal << std::endl;
    // 废弃桶通道（快照失配）：仅在 scan-dir 映射到废弃白名单时存在
    if (summary.depModulePassed + summary.depModuleFlagged + summary.depModuleFailed > 0) {
        std::cout << "废弃桶: " << summary.depModulePassed << " 通过 / " <<
            summary.depModuleFlagged << " 检出 / " << summary.depModuleFailed <<
            " 失败, 导出API " << summary.depApiExposed << " 条, 合法 " <<
            (summary.depApiExposed - summary.depApiFlagged) << " / 检出 " <<
            summary.depApiFlagged << " 条(快照失配)" << std::endl;
    }
    std::cout << "未登记模块: " << unknownCount << std::endl;
    std::cout << "==================" << std::endl;
}

// 废弃桶逐行输出（PrintIllegalResult 的 deprecated 段提取）
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

// 结果输出：非法API清单 stdout 逐行输出（hdc shell 直接回显）+ 废弃桶结果段
// （快照失配，单独归类提示人工研判）+ 未登记模块清单 + result.json 归档。
// 返回非法API总数（废弃桶条目不计入，单独统计）。
size_t PrintIllegalResult(const std::string& resultFile,
                          const std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& illegalMap,
                          const std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& deprecatedMap,
                          const std::vector<std::string>& unknownModules)
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
    SaveIllegalApiToJson(resultFile, illegalMap, deprecatedMap, unknownModules);
    LOG_INFO("非法API报告已保存: " << resultFile);
    return illegalTotal;
}

// --skip-module 值拆逗号并归一化入配置
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

// 工具逻辑入口：CLI 解析与 kit 编排的全部逻辑。
// C++ 可执行文件的入口函数名固定为 main（链接器约定，不可更改），
// 故 main 仅为末尾的一行转发，主体在本函数。
int CheckApiMain(int argc, char* argv[])
{
    std::cout << "===== HarmonyOS API 非法暴露校验工具 =====" << std::endl;
    if (argc < 2) {
        PrintUsage(argv[0]);
        return ERROR_PARAM_INVALID;
    }

    // ---------------- 参数解析：位置参数 + 命名选项 ----------------
    std::vector<std::string> positionals;
    std::vector<std::string> ignoreSources;   // --ignore-file 指定的文件/目录
    std::vector<std::string> skipArgs;        // --skip-module 指定的模块清单（逗号分隔）
    CheckConfig config;
    if (!ParseCommandLine(argc, argv, positionals, ignoreSources, skipArgs)) {
        return ERROR_PARAM_INVALID;
    }
    if (positionals.empty()) {
        PrintUsage(argv[0]);
        return ERROR_PARAM_INVALID;
    }
    ParseSkipModules(skipArgs, config.skipModules);
    CheckErrorCode inputErr = ResolveCheckInputs(argv[0], positionals, ignoreSources, config);
    if (inputErr != CHECK_OK) {
        return inputErr;
    }

    // 创建轻量NAPI环境
    napi_env env = CreateArkEnv();
    if (env == nullptr) {
        std::cerr << "CreateArkEnv failed" << std::endl;
        return ERROR_JS_ENGINE_INIT;
    }

    napi_handle_scope scope = nullptr;
    napi_open_handle_scope(env, &scope);

    std::map<std::string, std::map<std::string, std::vector<ApiMeta>>> illegalMap;
    std::map<std::string, std::map<std::string, std::vector<ApiMeta>>> deprecatedMap;
    CheckSummary summary;
    RunKitChecks(env, config, summary, illegalMap, deprecatedMap);

    napi_close_handle_scope(env, scope);
    DestroyArkEnv(env);

    // ---------------- 结果输出 ----------------
    PrintIllegalResult(config.resultFile, illegalMap, deprecatedMap, config.unknownModules);
    PrintSummary(summary, config.unknownModules.size());
    CloseLogFile();

    // 退出码不含废弃桶结果（deprecatedMap）：该通道是已知快照失配，条目量
    // 不应淹没主结论；其中疑似真发现由 DeprecatedModules 段供人工研判
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
