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

// scan-dir 模式回归样例：@hms.* 全名形态的三级目录模块。
// requireNapi 参数为 "hms.core.ar.hmsdemo"（保留 @ 后全名，与
// ModuleNameForRequireNapi 的 @hms 规则一致），so 需布局在
// module/hms/core/ar/libhmsdemo.so。旧实现把 @hms. 剥成首段
// （"core.ar.hmsdemo" -> module/core/ar/，路径不存在），模块永远无法
// 映射与加载；本样例与 scandirdemo 一起锁定该回归。
static napi_value Start(napi_env env, napi_callback_info /* info */)
{
    napi_value ret = nullptr;
    napi_get_boolean(env, true, &ret);
    return ret;
}

static napi_value ModuleInit(napi_env env, napi_value exports)
{
    napi_value start = nullptr;
    napi_create_function(env, "start", NAPI_AUTO_LENGTH, Start, nullptr, &start);
    napi_set_named_property(env, exports, "start", start);

    napi_value version = nullptr;
    napi_create_uint32(env, 1, &version);
    napi_set_named_property(env, exports, "version", version);
    return exports;
}

// 标准node-api模块声明
static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = ModuleInit,
    .nm_modname = "hms.core.ar.hmsdemo",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterCheckApiHmsDemoModule(void)
{
    napi_module_register(&demoModule);
}
