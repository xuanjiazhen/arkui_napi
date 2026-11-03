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

// module-map sample: ONE shared so (module name "util.struct") backing TWO d.ts modules
// (@ohos.util.ArrayList and @ohos.util.HashMap in module_map.json) - the many-to-one case
// the file-name convention cannot express. Each module is checked against the exports of
// this so, and whitelist entries of the sibling module must match through the file alias
// set built from the map (no alias -> the sibling class is a false "illegal" finding).

static napi_value ArrayListAdd(napi_env env, napi_callback_info /* info */)
{
    napi_value ret = nullptr;
    napi_get_boolean(env, true, &ret);
    return ret;
}

static napi_value ArrayListConstructor(napi_env env, napi_callback_info info)
{
    napi_value jsthis = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
    return jsthis;
}

static napi_value HashMapSet(napi_env env, napi_callback_info /* info */)
{
    napi_value ret = nullptr;
    napi_get_boolean(env, true, &ret);
    return ret;
}

static napi_value HashMapConstructor(napi_env env, napi_callback_info info)
{
    napi_value jsthis = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
    return jsthis;
}

static napi_value ModuleInit(napi_env env, napi_value exports)
{
    napi_value arrayList = nullptr;
    napi_property_descriptor arrayListDesc[] = {
        {"add", nullptr, ArrayListAdd, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    napi_define_class(env, "ArrayList", NAPI_AUTO_LENGTH, ArrayListConstructor, nullptr,
                      1, arrayListDesc, &arrayList);
    napi_set_named_property(env, exports, "ArrayList", arrayList);

    napi_value hashMap = nullptr;
    napi_property_descriptor hashMapDesc[] = {
        {"set", nullptr, HashMapSet, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    napi_define_class(env, "HashMap", NAPI_AUTO_LENGTH, HashMapConstructor, nullptr,
                      1, hashMapDesc, &hashMap);
    napi_set_named_property(env, exports, "HashMap", hashMap);
    return exports;
}

static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = ModuleInit,
    .nm_modname = "util.struct",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterCheckApiStructDemoModule(void)
{
    napi_module_register(&demoModule);
}
