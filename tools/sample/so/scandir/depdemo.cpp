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

#include "napi/native_api.h"

// 方案 B 回归样例：废弃桶模块。so 布局在 module/ability/（单级目录，模拟
// @ohos.ability.* 旧名模块的设备布局），白名单为 API10LessDeprecatedModules.json
// （冻结快照形态：className 用 "unnamed" 占位符）。
// 导出面：oldApi（废弃快照已收录）+ newApi（废弃后新增——快照必然没有）。
// 预期：newApi 条目进入 result.json 的 DeprecatedModules 段，不进 IllegalApis、
// 不影响退出码；若无独立归类，则混入 IllegalApis 且退出码 2001（红态）。
static napi_value OldApi(napi_env env, napi_callback_info /* info */)
{
    napi_value ret = nullptr;
    napi_get_undefined(env, &ret);
    return ret;
}

// 模拟废弃之后共享实现新增的 API（冻结快照不可能收录）
static napi_value NewApi(napi_env env, napi_callback_info /* info */)
{
    napi_value ret = nullptr;
    napi_get_undefined(env, &ret);
    return ret;
}

static napi_value ModuleInit(napi_env env, napi_value exports)
{
    napi_value oldApi = nullptr;
    napi_create_function(env, "oldApi", NAPI_AUTO_LENGTH, OldApi, nullptr, &oldApi);
    napi_set_named_property(env, exports, "oldApi", oldApi);

    napi_value newApi = nullptr;
    napi_create_function(env, "newApi", NAPI_AUTO_LENGTH, NewApi, nullptr, &newApi);
    napi_set_named_property(env, exports, "newApi", newApi);
    return exports;
}

// 标准node-api模块声明
static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = ModuleInit,
    .nm_modname = "ability.depdemo",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterCheckApiDepDemoModule(void)
{
    napi_module_register(&demoModule);
}
