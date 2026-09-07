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
#include <iostream>
#include <map>
#include <set>
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
};

// 单个 kit 的检查上下文（白名单索引与可展开名集合，由 kit 循环构建）
struct KitContext {
    const LegalIndex* legalIndex = nullptr;
    const std::map<std::string, std::string>* inheritMap = nullptr;
    const std::set<std::string>* expandableNames = nullptr;
};

// 检查统计
struct CheckSummary {
    size_t fileChecked = 0;
    size_t fileFailed = 0;
    CheckErrorCode firstError = CHECK_OK;
};

void PrintUsage(const char* name)
{
    std::cerr << "Usage(multi-file):  " << name <<
        " <check list json> <legal dir of kit json> <result file> [ignore list] [options]" << std::endl;
    std::cerr << "Usage(scan-dir):    " << name <<
        " <module so dir> <legal dir of kit json> <result file> [ignore list] [options]" << std::endl;
    std::cerr << "Usage(single-file): " << name <<
        " <kit name> <abc file or so module name> <legal dir of kit json> <result file>" <<
        " [ignore list] [options]" << std::endl;
    std::cerr << "Options:" << std::endl;
    std::cerr << "  --log-level <DEBUG|INFO|WARN|ERROR>   log level, default INFO" << std::endl;
    std::cerr << "  --log-file <file>                     write log to file (WARN/ERROR" <<
        " still printed to stderr)" << std::endl;
    std::cerr << "  --ignore-file <path>...               ignore list file(s)/dir(s), greedy until" <<
        " next option" << std::endl;
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
// 剥 "lib" 前缀与 so 后缀、"_napi" 变体与主形态同名去重；
// 相对目录部分的路径分隔符还原为点号。
// 这是 GetNativeModulePath 正向映射（模块名 tolower + lastDot 切分 + 点转目录）的逆运算。
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
    std::string name = relDir.empty() ? stem : relDir + "." + stem;
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

// 检查单个产物文件。白名单相关结构（legalIndex/inheritMap/expandable）由调用方
// 按 kit 预构建（KitContext），避免同一 kit 多文件时重复解析（ArkUI 2 万条白名单 × 79 文件）。
CheckErrorCode CheckSingleFile(napi_env env,
                               const KitContext& kitCtx,
                               const std::string& kit,
                               const std::string& file,
                               std::vector<ApiMeta>& illegalList)
{
    std::vector<ApiMeta> exposedList;
    CheckErrorCode err = CHECK_OK;
    // apiFileName 形态归一：需求接口示例为声明文件名（@ohos.xxx.d.ts），
    // 真机命令与 checklist 直接给运行时模块名（@ohos.taskpool）——两者统一为
    // 模块名（去掉 .d.ts/.d.ets 声明后缀）后走同一加载链
    std::string moduleName = file;
    if (EndsWith(moduleName, ".d.ts")) {
        moduleName = moduleName.substr(0, moduleName.size() - 5); // 5: strlen(".d.ts")
    } else if (EndsWith(moduleName, ".d.ets")) {
        moduleName = moduleName.substr(0, moduleName.size() - 6); // 6: strlen(".d.ets")
    }
    if (EndsWith(moduleName, ".abc")) {
        err = LoadJSAndGetExposedMeta(env, kit, moduleName, exposedList, kitCtx.expandableNames);
        if (err != CHECK_OK) {
            LOG_ERROR("LoadJSAndGetExposedMeta " << moduleName << " failed");
            return err;
        }
#if defined(OHOS_PLATFORM) || defined(LINUX_PLATFORM)
    } else if (StartsWith(moduleName, "@")) {
        // 模块名（如 @ohos.taskpool）：映射为 requireNapi 参数后走真机同款加载链
        SoLoadParam param;
        param.kit = kit;
        param.displayName = moduleName;
        param.requireName = ModuleNameForRequireNapi(moduleName);
        err = LoadSoAndGetExposedMeta(env, param, exposedList, kitCtx.expandableNames);
        if (err != CHECK_OK) {
            return err;
        }
#else
    } else if (StartsWith(moduleName, "@")) {
        LOG_ERROR("非ohos/linux平台不支持检查 so 模块: " << moduleName);
        return ERROR_PARAM_INVALID;
#endif
    } else {
        LOG_ERROR("当前仅支持abc或HarmonyOS模块名(@ohos./@arkts./@hms./@system.开头): " << moduleName);
        return ERROR_PARAM_INVALID;
    }

    err = CheckIllegalExposed(*kitCtx.legalIndex, exposedList, *kitCtx.inheritMap, illegalList);
    if (err != CHECK_OK) {
        LOG_WARN("检查不通过: 存在非法API暴露: " << moduleName);
        return err;
    }

    LOG_INFO("检查通过: 无非法API暴露: " << moduleName);
    return CHECK_OK;
}

// 解析命令行选项与位置参数。选项非法时打印错误（部分场景含用法）并返回 false。
bool ParseCommandLine(int argc, char* argv[], std::vector<std::string>& positionals,
                      std::vector<std::string>& ignoreSources)
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

// 位置参数 → 检查任务配置（kit→文件映射、白名单位置、结果文件），
// 并合并 --ignore-file 指定的 ignore 来源
CheckErrorCode ResolveCheckInputs(const char* programName,
                                  const std::vector<std::string>& positionals,
                                  const std::vector<std::string>& ignoreSources,
                                  CheckConfig& config)
{
    if (EndsWith(positionals[0], ".json")) {
        // multi file
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
    // --ignore-file 指定的来源（可与位置参数形式的 ignore list 共存）
    for (const std::string& src : ignoreSources) {
        LoadIgnoreSource(src, config.ignoreList, config.ignoreInheritMap);
    }
    return CHECK_OK;
}

// 每个 kit 的检查编排：解析白名单（基础为 ignore list）→构建索引→逐文件检查
void RunKitChecks(napi_env env, const CheckConfig& config, CheckSummary& summary,
                  std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& illegalMap)
{
    for (const auto& [kit, files] : config.kitFileMap) {
        // 每个kit解析一次白名单（基础为ignore list），构建索引与可展开名集合
        std::vector<ApiMeta> legalList = config.ignoreList;
        std::map<std::string, std::string> inheritMap = config.ignoreInheritMap;
        // legalDir 为文件时直接用作本 kit 白名单；为目录时按 {dir}/{kit}.json 解析
        std::string legalJson = config.whitelistIsFile ? config.legalDir :
            config.legalDir + "/" + kit + ".json";
        if (ParseLegalApiList(legalJson, legalList, inheritMap) != CHECK_OK) {
            LOG_WARN("kit whitelist missing or invalid: " << legalJson << ", skip " << files.size() <<
                " file(s) of kit " << kit);
            if (summary.firstError == CHECK_OK) {
                summary.firstError = ERROR_FILE_OPEN_FAILED;
            }
            summary.fileFailed += files.size();
            continue;
        }
        LOG_INFO("kit " << kit << ": " << files.size() << " file(s), legal list " <<
            legalList.size() << " entry(ies)");

        LegalIndex legalIndex;
        BuildLegalIndex(legalList, legalIndex);
        std::set<std::string> expandableNames;
        CollectExpandableNames(legalList, expandableNames);

        KitContext kitCtx;
        kitCtx.legalIndex = &legalIndex;
        kitCtx.inheritMap = &inheritMap;
        kitCtx.expandableNames = &expandableNames;

        for (const auto& file : files) {
            LOG_INFO("CheckSingleFile: " << kit << ": " << file);
            std::vector<ApiMeta> illegalList;
            CheckErrorCode ret = CheckSingleFile(env, kitCtx, kit, file, illegalList);
            if (ret == CHECK_OK) {
                summary.fileChecked++;
                continue;
            }
            summary.firstError = (summary.firstError == CHECK_OK) ? ret : summary.firstError;
            summary.fileFailed++;
            if (!illegalList.empty()) {
                illegalMap[kit][file] = illegalList;
            }
        }
    }
}

// 结果输出：私有API清单 stdout 逐行输出（hdc shell 直接回显）+ 未登记模块清单
// + result.json 归档。返回非法API总数。
size_t PrintIllegalResult(const std::string& resultFile,
                          const std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& illegalMap,
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
    for (const std::string& name : unknownModules) {
        std::cout << "[unknown-module] " << name << std::endl;
    }
    SaveIllegalApiToJson(resultFile, illegalMap, unknownModules);
    return illegalTotal;
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
    CheckConfig config;
    if (!ParseCommandLine(argc, argv, positionals, ignoreSources)) {
        return ERROR_PARAM_INVALID;
    }
    if (positionals.empty()) {
        PrintUsage(argv[0]);
        return ERROR_PARAM_INVALID;
    }
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
    CheckSummary summary;
    RunKitChecks(env, config, summary, illegalMap);

    napi_close_handle_scope(env, scope);
    DestroyArkEnv(env);

    // ---------------- 结果输出 ----------------
    size_t illegalTotal = PrintIllegalResult(config.resultFile, illegalMap, config.unknownModules);
    std::cout << "===== 检查完成: " << summary.fileChecked << " 通过, " << summary.fileFailed <<
        " 失败/未检, " << illegalTotal << " 非法API, " << config.unknownModules.size() <<
        " 未登记模块 =====" << std::endl;
    CloseLogFile();

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
