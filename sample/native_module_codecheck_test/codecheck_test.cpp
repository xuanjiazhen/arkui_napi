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

#include "codecheck_test.h"
#include <cstring>
#include <cstdlib>

// G.NAM.03-CPP: function naming violation (snake_case instead of PascalCase)
int calculate_sum(int a, int b)
{
    return a + b;
}

// G.FMT.11-CPP: missing braces around single-line if/for/while/else
int get_max_value(int* arr, int size)
{
    if (arr == NULL)
        return -1;

    int maxVal = arr[0];
    for (int i = 1; i < size; i++)
        if (arr[i] > maxVal)
            maxVal = arr[i];

    if (size > 10)
        maxVal = maxVal * 2;
    else
        maxVal = maxVal + 1;

    return maxVal;
}

// G.EXP.35-CPP: using NULL instead of nullptr
// G.FMT.04-CPP: multiple declarations on one line
void process_data(const char* input, char* output, int maxLen)
{
    int i = 0, j = 0, count = 0;
    char* temp = NULL;
    void* ptr = NULL;

    if (input == NULL || output == NULL)
        return;

    temp = (char*)malloc(maxLen);
    if (temp == NULL)
        return;

    // G.FMT.05-CPP: line too long
    for (i = 0; i < maxLen && input[i] != '\0'; i++) { temp[i] = input[i]; count++; if (count > 100) break; }

    temp[i] = '\0';

    // G.FMT.11-CPP: missing braces
    for (j = 0; j < i; j++)
        output[j] = temp[j];
    output[j] = '\0';

    free(temp);
}

// G.FMT.05-CPP: line too long
void veryLongFunctionNameThatExceedsTheMaximumAllowedLineLengthForOpenHarmonyCodeStyleCheckAndShouldBeWrappedToMultipleLines(int parameterOne, int parameterTwo, int parameterThree, int parameterFour)
{
    int result = parameterOne + parameterTwo + parameterThree + parameterFour;
    (void)result;
}

// G.FUN.01-CPP: function too long (intentionally verbose)
static napi_value JsProcessAll(napi_env env, napi_callback_info info)
{
    constexpr size_t maxArgs = 1;
    size_t argc = maxArgs;
    napi_value args[maxArgs] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < maxArgs)
        return nullptr;

    constexpr size_t bufSize = 256;
    char buf[bufSize] = {0};
    size_t len = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], buf, bufSize, &len));

    char output[bufSize] = {0};
    process_data(buf, output, bufSize);

    int arr[] = {1, 5, 3, 9, 2, 7, 4, 8, 6, 0};
    int maxVal = get_max_value(arr, 10);

    int sum = calculate_sum(maxVal, len);

    char finalBuf[bufSize] = {0};
    int written = snprintf(finalBuf, bufSize, "%s_%d_%d", output, maxVal, sum);
    if (written < 0)
        return nullptr;

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, finalBuf, strlen(finalBuf), &result));
    return result;
}

static napi_value CodecheckTestInit(napi_env env, napi_value exports)
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
    .nm_register_func = CodecheckTestInit,
    .nm_modname = "codechecktest",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void CodecheckTestRegisterModule(void)
{
    napi_module_register(&g_codecheckModule);
}
