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

#include <cstddef>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

extern const char BINARY_HELLO_JS_START[];
extern const char BINARY_HELLO_JS_END[];

/*
 * Sync callback
 */
static napi_value SayHello(napi_env env, napi_callback_info info)
{
    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, "Hello, ArkUI NAPI!", NAPI_AUTO_LENGTH, &result));
    return result;
}

/*
 * function for module exports
 */
static napi_value Init(napi_env env, napi_value exports)
{
    /*
     * Properties define
     */
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("sayHello", SayHello),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

extern "C" __attribute__((visibility("default"))) void NapiHelloGetJsCode(const char** buf, int* bufLen)
{
    if (buf != nullptr) {
        *buf = BINARY_HELLO_JS_START;
    }

    if (bufLen != nullptr) {
        *bufLen = BINARY_HELLO_JS_END - BINARY_HELLO_JS_START;
    }
}

/*
 * Module define
 */
static napi_module helloModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "hello",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};
/*
 * Module register function
 */
extern "C" __attribute__((constructor)) void HelloRegisterModule(void)
{
    napi_module_register(&helloModule);
}