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

#include "load_abc.h"

#include <climits>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <sys/stat.h>

#include "ark_native_engine.h"
#include "check_helper.h"
#include "jsnapi.h"
#include "native_utils.h"

namespace ohos::check {
namespace {

// GetABCCode 回调签名（与 NativeModuleManager::GetJSCodeCallback 一致）
using GetAbcCodeCallback = void (*)(const char** buf, int* len);

// 符号名与 FindNativeModuleByDisk 同规则：NAPI_<模块名点/斜杠转下划线>_GetABCCode
std::string BuildAbcCodeSymbolName(const std::string& requireName)
{
    std::string symbol = "NAPI_" + requireName + "_GetABCCode";
    for (char& c : symbol) {
        if (c == '.' || c == '/') {
            c = '_';
        }
    }
    return symbol;
}

std::string ToLowerAscii(const std::string& input)
{
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return out;
}

// dlopen 混合 so 提取 ABC：返回 buffer 指针与长度。
// RTLD_NOW|RTLD_LOCAL：立即解析符号（需要 dlsym 可靠）、不污染全局符号表
bool ExtractAbcFromSo(const std::string& soPath, const std::string& requireName,
                      const char*& outBuf, int& outLen)
{
    void* handle = dlopen(soPath.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (handle == nullptr) {
        LOG_WARN("dlopen failed for ABC extraction: " << soPath << ", dlerror: " <<
                 (dlerror() != nullptr ? dlerror() : "unknown"));
        return false;
    }
    std::string symbol = BuildAbcCodeSymbolName(requireName);
    auto getAbcCode = reinterpret_cast<GetAbcCodeCallback>(dlsym(handle, symbol.c_str()));
    if (getAbcCode == nullptr) {
        // 混合 so 但符号名不匹配（模块注册名与 requireName 不一致的边缘情况），
        // 尝试小写变体（引擎 moduleKey 在部分路径下 tolower）
        getAbcCode = reinterpret_cast<GetAbcCodeCallback>(dlsym(handle, ToLowerAscii(symbol).c_str()));
    }
    if (getAbcCode == nullptr) {
        dlclose(handle);
        return false;
    }
    outBuf = nullptr;
    outLen = 0;
    getAbcCode(&outBuf, &outLen);
    if (outBuf == nullptr || outLen <= 0) {
        dlclose(handle);
        return false;
    }
    // 不 dlclose：buffer 指向 so 的静态数据段，关闭句柄会导致悬垂
    return true;
}

// ABC 执行的候选记录名（引擎虚拟名 / 模块名 / so 末段名），
// 按 hvigor 构建产物与引擎 LoadNativeModule 的虚拟名规则覆盖各形态
std::vector<std::string> BuildRecordNameCandidates(const std::string& requireName, const std::string& soPath)
{
    std::vector<std::string> candidates;
    // 引擎 LoadNativeModule 的虚拟记录名："lib<name>.z.so/<name>.js"
    candidates.push_back("lib" + requireName + ".z.so/" + requireName + ".js");
    candidates.push_back(requireName);
    // so 末段（去 lib 前缀与后缀），对应 hvigor 构建的实际记录名形态
    std::string soStem = soPath.substr(soPath.find_last_of('/') + 1);
    if (soStem.substr(0, 3) == "lib") {
        soStem = soStem.substr(3);
    }
    size_t dot = soStem.find('.');
    if (dot != std::string::npos) {
        soStem = soStem.substr(0, dot);
    }
    if (!soStem.empty()) {
        candidates.push_back(soStem);
    }
    return candidates;
}

// 执行内嵌 ABC 并取导出对象。每次失败清挂起异常，防污染后续候选与模块
bool ExecuteAbcAndGetExports(napi_env env, const AbcLoadParam& param,
                             const char* buf, int len, napi_value& outExports)
{
    NativeEngine* engine = reinterpret_cast<NativeEngine*>(env);
    const EcmaVM* vm = engine->GetEcmaVm();
    for (const std::string& recordName : BuildRecordNameCandidates(param.requireName, param.soPath)) {
        bool ok = panda::JSNApi::ExecuteModuleFromBuffer(
            const_cast<panda::EcmaVM*>(vm), buf, len, recordName, param.soPath);
        if (!ok) {
            ClearPendingException(env);
            LOG_DEBUG("ExecuteModuleFromBuffer failed, recordName: " << recordName);
            continue;
        }
        Local<panda::ObjectRef> obj =
            panda::JSNApi::GetExportObjectFromBuffer(const_cast<panda::EcmaVM*>(vm), recordName, "default");
        ClearPendingException(env);
        if (obj->IsUndefined() || obj->IsNull()) {
            LOG_DEBUG("GetExportObjectFromBuffer empty, recordName: " << recordName);
            continue;
        }
        outExports = JsValueFromLocalValue(obj);
        LOG_INFO("ABC executed via recordName: " << recordName << " (so: " << param.soPath << ")");
        return true;
    }
    return false;
}

// 统一出口：遍历导出
void TraverseAbcExports(napi_env env, const AbcLoadParam& param,
                        napi_value exports, std::vector<ApiMeta>& exposedMeta)
{
    TraverseContext ctx;
    ctx.env = env;
    ctx.kit = param.kit;
    ctx.file = param.displayName;
    ctx.expandableNames = param.expandableNames;
    ctx.outMeta = &exposedMeta;
    TraverseExports(ctx, exports, "");
}

} // namespace

CheckErrorCode LoadAbcFileAndGetExposedMeta(
    napi_env env,
    const AbcLoadParam& param,
    std::vector<ApiMeta>& exposedMeta)
{
    char absBuf[PATH_MAX] = {0};
    if (realpath(param.abcFilePath.c_str(), absBuf) == nullptr) {
        LOG_WARN("LoadAbcFileAndGetExposedMeta: file not found: " << param.abcFilePath);
        return ERROR_FILE_OPEN_FAILED;
    }
    std::string absPath = absBuf;

    NativeEngine* engine = reinterpret_cast<NativeEngine*>(env);
    const EcmaVM* vm = engine->GetEcmaVm();

    // 入口候选（es2abc 产物形态）：
    // 1) 绝对路径（未 merge 单文件：记录名与文件路径一致）
    // 2) 去扩展名基名（--merge-abc：记录名为输出文件名去扩展名）
    std::string baseName = absPath.substr(absPath.find_last_of('/') + 1);
    size_t dotPos = baseName.find_last_of('.');
    std::string mergedEntry = (dotPos == std::string::npos) ? baseName : baseName.substr(0, dotPos);
    const std::string candidates[] = {absPath, mergedEntry};

    for (const std::string& entry : candidates) {
        if (entry.empty()) {
            continue;
        }
        bool ret = panda::JSNApi::ExecuteForAbsolutePath(vm, absPath, entry);
        if (!ret) {
            ClearPendingException(env);
            continue;
        }
        Local<panda::ObjectRef> obj =
            panda::JSNApi::GetExportsFromFile(const_cast<panda::EcmaVM*>(vm), absPath, entry);
        ClearPendingException(env);
        if (obj->IsUndefined()) {
            continue;
        }
        napi_value result = JsValueFromLocalValue(obj);
        TraverseAbcExports(env, param, result, exposedMeta);
        LOG_INFO("abc file executed: " << absPath << " (entry: " << entry << ", " <<
                 exposedMeta.size() << " exports)");
        return CHECK_OK;
    }

    LOG_ERROR("LoadAbcFileAndGetExposedMeta failed: " << absPath);
    return ERROR_JS_EXECUTE_FAILED;
}

bool SoHasEmbeddedAbc(const std::string& soPath)
{
    // 只读文件字节做子串搜索（与 strings GetABCCode 同理），不执行任何代码。
    // 分块读取防大 so 一次载入
    std::ifstream ifs(soPath, std::ios::binary);
    if (!ifs.is_open()) {
        return false;
    }
    static const char marker[] = "GetABCCode";
    static const size_t markerLen = sizeof(marker) - 1;
    std::string tail;
    char chunk[8192];
    while (ifs.read(chunk, sizeof(chunk)) || ifs.gcount() > 0) {
        std::string data(chunk, static_cast<size_t>(ifs.gcount()));
        // 拼接尾部（标记跨块边界）：保留上一块末尾 markerLen-1 字节
        std::string combined = tail + data;
        if (combined.find(marker) != std::string::npos) {
            return true;
        }
        if (combined.size() > markerLen) {
            tail = combined.substr(combined.size() - markerLen + 1);
        } else {
            tail = combined;
        }
        if (!ifs) {
            break;
        }
    }
    return false;
}

std::vector<std::string> CandidateSoPaths(const std::string& soDir, const std::string& requireName)
{
    std::vector<std::string> paths;
    if (soDir.empty() || requireName.empty()) {
        return paths;
    }
    // 镜像 GetNativeModulePath：全小写、末段做 so 名、其余段点转目录
    std::string lower = ToLowerAscii(requireName);
    size_t lastDot = lower.find_last_of('.');
    std::string stem = (lastDot == std::string::npos) ? lower : lower.substr(lastDot + 1);
    std::string dirPart = (lastDot == std::string::npos) ? "" : lower.substr(0, lastDot);
    for (char& c : dirPart) {
        if (c == '.') {
            c = '/';
        }
    }
    // 平台后缀（与引擎一致：设备 .z.so，宿主 LINUX 无 z）
#if defined(OHOS_PLATFORM)
    const char* zfix = ".z";
#else
    const char* zfix = "";
#endif
    std::string prefix = dirPart.empty() ? soDir : soDir + "/" + dirPart;
    paths.push_back(prefix + "/lib" + stem + zfix + ".so");
    paths.push_back(prefix + "/lib" + stem + "_napi" + zfix + ".so");
    return paths;
}

CheckErrorCode LoadMixedSoAbcAndGetExposedMeta(
    napi_env env,
    const AbcLoadParam& param,
    std::vector<ApiMeta>& exposedMeta)
{
    const char* buf = nullptr;
    int len = 0;
    if (!ExtractAbcFromSo(param.soPath, param.requireName, buf, len)) {
        LOG_WARN("ABC extraction failed: " << param.soPath << " (module: " << param.displayName << ")");
        return ERROR_JS_EXECUTE_FAILED;
    }
    LOG_INFO("ABC extracted: " << len << " bytes from " << param.soPath);

    napi_value exports = nullptr;
    if (!ExecuteAbcAndGetExports(env, param, buf, len, exports)) {
        LOG_WARN("ABC execution failed (all record name candidates): " << param.soPath <<
                 " (module: " << param.displayName << ")");
        ClearPendingException(env);
        return ERROR_JS_EXECUTE_FAILED;
    }

    TraverseAbcExports(env, param, exports, exposedMeta);
    LOG_INFO("mixed so ABC loaded: " << param.displayName << " (" << exposedMeta.size() << " exports)");
    return CHECK_OK;
}

} // namespace ohos::check
