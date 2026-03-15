/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "hello.h"
#include <string>

namespace NapiHello {

std::string Greet(const std::string &name)
{
    return "Hello, " + name + "!";
}

int Add(int a, int b)
{
    return a + b;
}

} // namespace NapiHello

static napi_value JsGreet(napi_env env, napi_callback_info info)
{
    constexpr size_t maxArgs = 1;
    size_t argc = maxArgs;
    napi_value args[maxArgs] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < maxArgs) {
        napi_throw_error(env, nullptr, "Expected 1 argument");
        return nullptr;
    }

    constexpr size_t bufSize = 256;
    char buf[bufSize] = {0};
    size_t len = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], buf, bufSize, &len));

    std::string result = NapiHello::Greet(std::string(buf, len));

    napi_value output = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, result.c_str(), result.length(), &output));
    return output;
}

static napi_value JsAdd(napi_env env, napi_callback_info info)
{
    constexpr size_t maxArgs = 2;
    size_t argc = maxArgs;
    napi_value args[maxArgs] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < maxArgs) {
        napi_throw_error(env, nullptr, "Expected 2 arguments");
        return nullptr;
    }

    int32_t a = 0;
    int32_t b = 0;
    NAPI_CALL(env, napi_get_value_int32(env, args[0], &a));
    NAPI_CALL(env, napi_get_value_int32(env, args[1], &b));

    int result = NapiHello::Add(a, b);

    napi_value output = nullptr;
    NAPI_CALL(env, napi_create_int32(env, result, &output));
    return output;
}

static napi_value HelloInit(napi_env env, napi_value exports)
{
    static napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("greet", JsGreet),
        DECLARE_NAPI_FUNCTION("add", JsAdd),
    };

    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

static napi_module g_helloModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = HelloInit,
    .nm_modname = "hello",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void HelloRegisterModule(void)
{
    napi_module_register(&g_helloModule);
}
