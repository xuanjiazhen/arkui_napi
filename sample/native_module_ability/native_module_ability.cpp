/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include <string>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#define GET_PARAMS(env, info, num) \
    size_t argc = num;             \
    napi_value argv[num];          \
    napi_value thisVar = nullptr;  \
    void* data = nullptr;          \
    napi_get_cb_info(env, info, &argc, argv, &thisVar, &data)

enum TestEnum {
    ONE = 0,
    TWO,
    THREE,
    FOUR
};

/*
 * Constructor
 */
static napi_value DemoJavascriptClassConstructor(napi_env env, napi_callback_info info)
{
    napi_value thisArg = nullptr;
    void* data = nullptr;

    napi_get_cb_info(env, info, nullptr, nullptr, &thisArg, &data);

    napi_value global = nullptr;
    napi_get_global(env, &global);

    return thisArg;
}

/*
 * Class Init
 */
void DemoJavascriptClassInit(napi_env env, napi_value exports)
{
    napi_value one = nullptr;
    napi_value two = nullptr;
    napi_value three = nullptr;
    napi_value four = nullptr;

    napi_create_int32(env, TestEnum::ONE, &one);
    napi_create_int32(env, TestEnum::TWO, &two);
    napi_create_int32(env, TestEnum::THREE, &three);
    napi_create_int32(env, TestEnum::FOUR, &four);

    napi_property_descriptor descriptors[] = {
        DECLARE_NAPI_STATIC_PROPERTY("ONE", one),
        DECLARE_NAPI_STATIC_PROPERTY("TWO", two),
        DECLARE_NAPI_STATIC_PROPERTY("THREE", three),
        DECLARE_NAPI_STATIC_PROPERTY("FOUR", four),
    };

    napi_value result = nullptr;
    napi_define_class(env, "DemoClass", NAPI_AUTO_LENGTH, DemoJavascriptClassConstructor, nullptr,
                      sizeof(descriptors) / sizeof(*descriptors), descriptors, &result);

    napi_set_named_property(env, exports, "DemoClass", result);
}

static napi_value AbilityExport(napi_env env, napi_value exports)
{
    DemoJavascriptClassInit(env, exports);
    return exports;
}

// ability module
static napi_module abilityModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = AbilityExport,
    .nm_modname = "ability",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

// ability module register
extern "C" __attribute__((constructor)) void AbilityRegister()
{
    napi_module_register(&abilityModule);
}