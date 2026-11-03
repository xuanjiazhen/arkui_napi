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

// module-map sample: the map classifies @ohos.util.Vector as a framework abc (not
// requireNapi-loadable), while a conventionally-named binary ALSO exists (this so:
// module "util.vector", file module/util/libvector.so). The claim path's conventional
// fallback keeps such a module checkable, so the check-time judgement must defer to
// the load instead of hiding the conventionally-named file (stale-map protection).
// run_scandir_map asserts this module is CHECKED and NOT reported as unsupported.

static napi_value Peek(napi_env env, napi_callback_info /* info */)
{
    napi_value ret = nullptr;
    napi_get_boolean(env, true, &ret);
    return ret;
}

static napi_value ModuleInit(napi_env env, napi_value exports)
{
    napi_value peek = nullptr;
    napi_create_function(env, "peek", NAPI_AUTO_LENGTH, Peek, nullptr, &peek);
    napi_set_named_property(env, exports, "peek", peek);
    return exports;
}

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = ModuleInit,
    .nm_modname = "util.vector",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterCheckApiVectorModule(void)
{
    napi_module_register(&demoModule);
}
