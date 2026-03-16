/**
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

#include "codecheck_test2.h"
#include <cstring>
#include <cstdlib>

namespace {
constexpr int SIZE_THRESHOLD = 10;
constexpr int MULTIPLIER = 2;
constexpr int COPY_LIMIT = 100;
constexpr size_t MAX_ALLOC_SIZE = 1024 * 1024;
} // namespace

int CalculateSum(int a, int b)
{
    return a + b;
}

int GetMaxValue(int* arr, int size)
{
    if (arr == nullptr) {
        return -1;
    }

    int maxVal = arr[0];
    for (int i = 1; i < size; i++) {
        if (arr[i] > maxVal) {
            maxVal = arr[i];
        }
    }

    if (size > SIZE_THRESHOLD) {
        maxVal = maxVal * MULTIPLIER;
    } else {
        maxVal = maxVal + 1;
    }

    return maxVal;
}

void ProcessData(const char* input, char* output, int maxLen)
{
    int i = 0;
    int j = 0;
    int count = 0;
    char* temp = nullptr;

    if (input == nullptr || output == nullptr) {
        return;
    }

    if (maxLen <= 0 || static_cast<size_t>(maxLen) > MAX_ALLOC_SIZE) {
        return;
    }

    temp = static_cast<char*>(malloc(maxLen));
    if (temp == nullptr) {
        return;
    }

    for (i = 0; i < maxLen && input[i] != '\0'; i++) {
        temp[i] = input[i];
        count++;
        if (count > COPY_LIMIT) {
            break;
        }
    }

    temp[i] = '\0';

    for (j = 0; j < i; j++) {
        output[j] = temp[j];
    }
    output[j] = '\0';

    free(temp);
}

void ProcessAllParameters(int parameterOne,
                          int parameterTwo,
                          int parameterThree,
                          int parameterFour)
{
    int result = parameterOne + parameterTwo +
                 parameterThree + parameterFour;
    (void)result;
}

static napi_value JsProcessAll(napi_env env, napi_callback_info info)
{
    constexpr size_t maxArgs = 1;
    size_t argc = maxArgs;
    napi_value args[maxArgs] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < maxArgs) {
        return nullptr;
    }

    constexpr size_t bufSize = 256;
    char buf[bufSize] = {0};
    size_t len = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], buf, bufSize, &len));

    char output[bufSize] = {0};
    ProcessData(buf, output, bufSize);

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, output, strlen(output), &result));
    return result;
}

static napi_value CodecheckTest2Init(napi_env env, napi_value exports)
{
    static napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("processAll", JsProcessAll),
    };

    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

static napi_module g_codecheckModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = CodecheckTest2Init,
    .nm_modname = "codechecktest2",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void CodecheckTest2RegisterModule(void)
{
    napi_module_register(&g_codecheckModule);
}
