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
    // 符号名与 FindNativeModuleByDisk 同规则：NAPI_<模块名点/斜杠转下划线>_GetABCCode
    std::string symbol = "NAPI_" + requireName + "_GetABCCode";
    for (char& c : symbol) {
        if (c == '.' || c == '/') {
            c = '_';
        }
    }
    auto getAbcCode = reinterpret_cast<GetAbcCodeCallback>(dlsym(handle, symbol.c_str()));
    if (getAbcCode == nullptr) {
        // 混合 so 但符号名不匹配（模块注册名与 requireName 不一致的边缘情况），
        // 尝试小写变体（引擎 moduleKey 在部分路径下 tolower）
        std::string lower = symbol;
        for (char& c : lower) {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        getAbcCode = reinterpret_cast<GetAbcCodeCallback>(dlsym(handle, lower.c_str()));
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

// 执行内嵌 ABC 并取导出对象。多候选记录名（引擎虚拟名 / 模块名 / so 末段名），
// 命中即返回。每次失败清挂起异常，防污染后续候选与模块
bool ExecuteAbcAndGetExports(napi_env env, const std::string& requireName, const std::string& soPath,
                             const char* buf, int len, napi_value& outExports)
{
    NativeEngine* engine = reinterpret_cast<NativeEngine*>(env);
    const EcmaVM* vm = engine->GetEcmaVm();

    // 引擎 LoadNativeModule 的虚拟记录名："lib<name>.z.so/<name>.js"
    std::string virtualName = "lib" + requireName + ".z.so/" + requireName + ".js";
    // so 末段（去 lib 前缀与后缀），对应 hvigor 构建的实际记录名形态
    std::string soStem = soPath.substr(soPath.find_last_of('/') + 1);
    if (soStem.substr(0, 3) == "lib") {
        soStem = soStem.substr(3);
    }
    size_t dot = soStem.find('.');
    if (dot != std::string::npos) {
        soStem = soStem.substr(0, dot);
    }

    const std::string candidates[] = {virtualName, requireName, soStem};
    for (const std::string& recordName : candidates) {
        if (recordName.empty()) {
            continue;
        }
        bool ok = panda::JSNApi::ExecuteModuleFromBuffer(
            const_cast<panda::EcmaVM*>(vm), buf, len, recordName, soPath);
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
        LOG_INFO("ABC executed via recordName: " << recordName << " (so: " << soPath << ")");
        return true;
    }
    return false;
}

// 统一出口：遍历导出
void TraverseAbcExports(napi_env env, const std::string& kit, const std::string& displayName,
                        napi_value exports, std::vector<ApiMeta>& exposedMeta,
                        const std::set<std::string>* expandableNames)
{
    TraverseContext ctx;
    ctx.env = env;
    ctx.kit = kit;
    ctx.file = displayName;
    ctx.expandableNames = expandableNames;
    ctx.outMeta = &exposedMeta;
    TraverseExports(ctx, exports, "");
}

} // namespace

CheckErrorCode LoadAbcFileAndGetExposedMeta(
    napi_env env,
    const std::string& kit,
    const std::string& displayName,
    const std::string& abcFilePath,
    std::vector<ApiMeta>& exposedMeta,
    const std::set<std::string>* expandableNames)
{
    char absBuf[PATH_MAX] = {0};
    if (realpath(abcFilePath.c_str(), absBuf) == nullptr) {
        LOG_WARN("LoadAbcFileAndGetExposedMeta: file not found: " << abcFilePath);
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
        TraverseAbcExports(env, kit, displayName, result, exposedMeta, expandableNames);
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
    std::string lower;
    lower.reserve(requireName.size());
    for (char c : requireName) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
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
    const char* soPostfix = ".so";
#else
    const char* zfix = "";
    const char* soPostfix = ".so";
#endif
    std::string prefix = dirPart.empty() ? soDir : soDir + "/" + dirPart;
    paths.push_back(prefix + "/lib" + stem + zfix + soPostfix);
    paths.push_back(prefix + "/lib" + stem + "_napi" + zfix + soPostfix);
    return paths;
}

CheckErrorCode LoadMixedSoAbcAndGetExposedMeta(
    napi_env env,
    const std::string& kit,
    const std::string& displayName,
    const std::string& requireName,
    const std::string& soPath,
    std::vector<ApiMeta>& exposedMeta,
    const std::set<std::string>* expandableNames)
{
    const char* buf = nullptr;
    int len = 0;
    if (!ExtractAbcFromSo(soPath, requireName, buf, len)) {
        LOG_WARN("ABC extraction failed: " << soPath << " (module: " << displayName << ")");
        return ERROR_JS_EXECUTE_FAILED;
    }
    LOG_INFO("ABC extracted: " << len << " bytes from " << soPath);

    napi_value exports = nullptr;
    if (!ExecuteAbcAndGetExports(env, requireName, soPath, buf, len, exports)) {
        LOG_WARN("ABC execution failed (all record name candidates): " << soPath <<
                 " (module: " << displayName << ")");
        ClearPendingException(env);
        return ERROR_JS_EXECUTE_FAILED;
    }

    TraverseAbcExports(env, kit, displayName, exports, exposedMeta, expandableNames);
    LOG_INFO("mixed so ABC loaded: " << displayName << " (" << exposedMeta.size() << " exports)");
    return CHECK_OK;
}

} // namespace ohos::check
