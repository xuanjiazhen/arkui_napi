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

#include "load_so.h"

#include <cstdint>
#include <set>
#include <string>
#include <vector>

#include "check_helper.h"
#include "jsnapi.h"
#include "native_engine.h"
#include "native_utils.h"

namespace ohos::check {
namespace {

// ArkNativeEngine 构造时注入的 requireNapi 全局函数名，
// PREVIEW 构建下注册名为 requireNapiPreview
constexpr const char* REQUIRE_NAPI_NAMES[] = { "requireNapi", "requireNapiPreview" };

bool GetRequireNapiFunction(napi_env env, napi_value global, napi_value& outFunc)
{
    for (const char* name : REQUIRE_NAPI_NAMES) {
        napi_value func = nullptr;
        if (napi_get_named_property(env, global, name, &func) != napi_ok) {
            continue;
        }
        napi_valuetype type;
        if (napi_typeof(env, func, &type) == napi_ok && type == napi_function) {
            outFunc = func;
            return true;
        }
    }
    return false;
}

// 调用 requireNapi 加载模块并取回导出对象。失败时打印错误、清理挂起异常并
// 返回 nullptr（含两种失败返回形态的拦截）。
napi_value GetModuleExports(napi_env env, const std::string& displayName,
                            const std::string& requireName)
{
    NativeEngine *engine = reinterpret_cast<NativeEngine *>(env);
    const EcmaVM *vm = engine->GetEcmaVm();

    napi_status status;
    napi_value global = nullptr;
    status = napi_get_global(env, &global);
    if (status != napi_ok) {
        LOG_ERROR("napi_get_global failed: " << displayName);
        return nullptr;
    }

    napi_value requireNapi = nullptr;
    if (!GetRequireNapiFunction(env, global, requireNapi)) {
        LOG_ERROR("requireNapi not found on global object: " << displayName);
        return nullptr;
    }

    napi_value soName = nullptr;
    status = napi_create_string_utf8(env, requireName.c_str(), requireName.size(), &soName);
    if (status != napi_ok) {
        LOG_ERROR("napi_create_string_utf8 failed: " << displayName);
        return nullptr;
    }

    napi_value exports = nullptr;
    static const uint32_t argc = 1;
    napi_value args[argc] = { soName };
    status = napi_call_function(env, global, requireNapi, argc, args, &exports);
    if (status != napi_ok) {
        LOG_ERROR("napi_call_function requireNapi failed: " << displayName);
        ClearPendingException(env);
        return nullptr;
    }

    // requireNapi 失败的两种返回形态都要拦截，否则会把失败信息对象当导出面遍历：
    //  1. undefined（参数为空等场景）；
    //  2. NativeModuleFailureInfo 对象（模块不存在的常规失败形态，
    //     ArkNativeEngine::RequireNapi 失败分支创建，自有属性为空，
    //     若不拦截会静默"检查通过"——设备全量扫描的假阴性来源）。
    auto exportsVal = LocalValueFromJsValue(exports);
    if (exportsVal->IsUndefined()) {
        LOG_ERROR("requireNapi returned undefined, module load failed: " << displayName);
        LOG_ERROR("hint: module search path is /system/lib[64]/module on device,"
                  " ./module/lib<module>.so relative to cwd on host");
        ClearPendingException(env);
        return nullptr;
    }
    if (exportsVal->IsNativeModuleFailureInfoObject(vm)) {
        LOG_ERROR("requireNapi returned NativeModuleFailureInfo, module load failed: " << displayName);
        ClearPendingException(env);
        return nullptr;
    }
    return exports;
}

} // namespace

std::string ModuleNameForRequireNapi(const std::string& file)
{
    if (file.empty() || file[0] != '@') {
        return file;
    }
    // 与 ArkNativeEngine::GetOhmurl 的 NATIVE_MODULE 特例集保持一致
    // （ark_native_engine.cpp:110）：这批模块保留全名参与 so 路径拼接。
    static const std::set<std::string> nativeModules = {
        "system.app", "ohos.app", "system.router", "ohos.curves",
        "ohos.matrix4", "system.matrix4", "system.curves",
    };
    std::string name = file.substr(1); // 去掉 '@'
    if (nativeModules.count(name) > 0) {
        return name;
    }
    // 通用规则：去掉首个 '.' 之前的命名空间段
    // （与 GetOhmurl 的 '@ohos:xxx.yyy' + GetStrippedModuleName 链一致）
    size_t pos = file.find('.');
    if (pos == std::string::npos || pos + 1 >= file.size()) {
        return name;
    }
    return file.substr(pos + 1);
}

CheckErrorCode LoadSoAndGetExposedMeta(napi_env env,
                                       const SoLoadParam& param,
                                       std::vector<ApiMeta>& exposedMeta,
                                       const std::set<std::string>* expandableNames)
{
    if (param.requireName.empty()) {
        return ERROR_PARAM_INVALID;
    }

    napi_value exports = GetModuleExports(env, param.displayName, param.requireName);
    if (exports == nullptr) {
        return ERROR_JS_EXECUTE_FAILED;
    }

    // 遍历导出
    TraverseContext ctx;
    ctx.env = env;
    ctx.kit = param.kit;
    ctx.file = param.displayName;
    ctx.expandableNames = expandableNames;
    ctx.outMeta = &exposedMeta;
    TraverseExports(ctx, exports, "");

    return CHECK_OK;
}

}
