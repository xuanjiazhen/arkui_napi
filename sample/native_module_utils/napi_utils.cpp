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
#include <cstring>
#include <ctime>
#include <vector>
#include <algorithm>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

extern const char _binary_utils_js_start[];
extern const char _binary_utils_js_end[];

/*
 * String utilities
 */
static napi_value ReverseString(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 1;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == napi_string, "Wrong argument type. String expected.");

    size_t strLen = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen));

    char* str = new char[strLen + 1];
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], str, strLen + 1, nullptr));

    // Reverse the string
    for (size_t i = 0; i < strLen / 2; i++) {
        std::swap(str[i], str[strLen - i - 1]);
    }

    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &result));

    delete[] str;
    return result;
}

static napi_value IsPalindrome(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 1;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == napi_string, "Wrong argument type. String expected.");

    size_t strLen = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen));

    char* str = new char[strLen + 1];
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], str, strLen + 1, nullptr));

    // Check if palindrome
    bool isPalindrome = true;
    for (size_t i = 0; i < strLen / 2; i++) {
        if (str[i] != str[strLen - i - 1]) {
            isPalindrome = false;
            break;
        }
    }

    napi_value result;
    NAPI_CALL(env, napi_create_boolean(env, isPalindrome, &result));

    delete[] str;
    return result;
}

static napi_value CountCharacters(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 1;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == napi_string, "Wrong argument type. String expected.");

    size_t strLen = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen));

    char* str = new char[strLen + 1];
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], str, strLen + 1, nullptr));

    // Count character frequencies
    int counts[256] = { 0 };
    for (size_t i = 0; i < strLen; i++) {
        counts[(unsigned char)str[i]]++;
    }

    // Create result object
    napi_value result;
    NAPI_CALL(env, napi_create_object(env, &result));

    for (int i = 0; i < 256; i++) {
        if (counts[i] > 0) {
            char c[2] = { (char)i, '\0' };
            napi_value key;
            NAPI_CALL(env, napi_create_string_utf8(env, c, NAPI_AUTO_LENGTH, &key));
            napi_value value;
            NAPI_CALL(env, napi_create_number(env, counts[i], &value));
            NAPI_CALL(env, napi_set_property(env, result, key, value));
        }
    }

    delete[] str;
    return result;
}

/*
 * Math utilities
 */
static napi_value Factorial(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 1;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == napi_number, "Wrong argument type. Number expected.");

    double value;
    NAPI_CALL(env, napi_get_value_double(env, args[0], &value));

    int n = static_cast<int>(value);
    NAPI_ASSERT(env, n >= 0, "Factorial is only defined for non-negative integers");

    long long result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }

    napi_value ret;
    NAPI_CALL(env, napi_create_number(env, result, &ret));
    return ret;
}

static napi_value Fibonacci(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 1;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == napi_number, "Wrong argument type. Number expected.");

    double value;
    NAPI_CALL(env, napi_get_value_double(env, args[0], &value));

    int n = static_cast<int>(value);
    NAPI_ASSERT(env, n >= 0, "Fibonacci is only defined for non-negative integers");

    if (n == 0) {
        napi_value ret;
        NAPI_CALL(env, napi_create_number(env, 0, &ret));
        return ret;
    }

    long long a = 0, b = 1;
    for (int i = 2; i <= n; i++) {
        long long temp = b;
        b = a + b;
        a = temp;
    }

    napi_value ret;
    NAPI_CALL(env, napi_create_number(env, b, &ret));
    return ret;
}

static napi_value IsPrime(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 1;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == napi_number, "Wrong argument type. Number expected.");

    double value;
    NAPI_CALL(env, napi_get_value_double(env, args[0], &value));

    int n = static_cast<int>(value);
    NAPI_ASSERT(env, n >= 2, "Prime numbers are greater than or equal to 2");

    bool isPrime = true;
    for (int i = 2; i * i <= n; i++) {
        if (n % i == 0) {
            isPrime = false;
            break;
        }
    }

    napi_value ret;
    NAPI_CALL(env, napi_create_boolean(env, isPrime, &ret));
    return ret;
}

/*
 * Array utilities
 */
static napi_value SortArray(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 1;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == napi_object, "Wrong argument type. Array expected.");

    bool isArray;
    NAPI_CALL(env, napi_is_array(env, args[0], &isArray));
    NAPI_ASSERT(env, isArray, "Wrong argument type. Array expected.");

    uint32_t length;
    NAPI_CALL(env, napi_get_array_length(env, args[0], &length));

    std::vector<double> values;
    values.reserve(length);

    for (uint32_t i = 0; i < length; i++) {
        napi_value element;
        NAPI_CALL(env, napi_get_element(env, args[0], i, &element));

        double value;
        NAPI_CALL(env, napi_get_value_double(env, element, &value));
        values.push_back(value);
    }

    // Sort the array
    std::sort(values.begin(), values.end());

    // Create result array
    napi_value result;
    NAPI_CALL(env, napi_create_array(env, &result));

    for (uint32_t i = 0; i < length; i++) {
        napi_value element;
        NAPI_CALL(env, napi_create_number(env, values[i], &element));
        NAPI_CALL(env, napi_set_element(env, result, i, element));
    }

    return result;
}

static napi_value GetMax(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 1;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == napi_object, "Wrong argument type. Array expected.");

    bool isArray;
    NAPI_CALL(env, napi_is_array(env, args[0], &isArray));
    NAPI_ASSERT(env, isArray, "Wrong argument type. Array expected.");

    uint32_t length;
    NAPI_CALL(env, napi_get_array_length(env, args[0], &length));
    NAPI_ASSERT(env, length > 0, "Array must not be empty");

    double maxValue = -1e100;
    for (uint32_t i = 0; i < length; i++) {
        napi_value element;
        NAPI_CALL(env, napi_get_element(env, args[0], i, &element));

        double value;
        NAPI_CALL(env, napi_get_value_double(env, element, &value));
        if (value > maxValue) {
            maxValue = value;
        }
    }

    napi_value result;
    NAPI_CALL(env, napi_create_number(env, maxValue, &result));
    return result;
}

static napi_value GetMin(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 1;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == napi_object, "Wrong argument type. Array expected.");

    bool isArray;
    NAPI_CALL(env, napi_is_array(env, args[0], &isArray));
    NAPI_ASSERT(env, isArray, "Wrong argument type. Array expected.");

    uint32_t length;
    NAPI_CALL(env, napi_get_array_length(env, args[0], &length));
    NAPI_ASSERT(env, length > 0, "Array must not be empty");

    double minValue = 1e100;
    for (uint32_t i = 0; i < length; i++) {
        napi_value element;
        NAPI_CALL(env, napi_get_element(env, args[0], i, &element));

        double value;
        NAPI_CALL(env, napi_get_value_double(env, element, &value));
        if (value < minValue) {
            minValue = value;
        }
    }

    napi_value result;
    NAPI_CALL(env, napi_create_number(env, minValue, &result));
    return result;
}

static napi_value SumArray(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 1;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == napi_object, "Wrong argument type. Array expected.");

    bool isArray;
    NAPI_CALL(env, napi_is_array(env, args[0], &isArray));
    NAPI_ASSERT(env, isArray, "Wrong argument type. Array expected.");

    uint32_t length;
    NAPI_CALL(env, napi_get_array_length(env, args[0], &length));

    double sum = 0.0;
    for (uint32_t i = 0; i < length; i++) {
        napi_value element;
        NAPI_CALL(env, napi_get_element(env, args[0], i, &element));

        double value;
        NAPI_CALL(env, napi_get_value_double(env, element, &value));
        sum += value;
    }

    napi_value result;
    NAPI_CALL(env, napi_create_number(env, sum, &result));
    return result;
}

/*
 * Time utilities
 */
static napi_value GetTimestamp(napi_env env, napi_callback_info info)
{
    time_t now = time(nullptr);
    napi_value result;
    NAPI_CALL(env, napi_create_number(env, now, &result));
    return result;
}

static napi_value FormatTime(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 0;
    size_t argc = 0;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr));

    time_t now = time(nullptr);
    struct tm* localTime = localtime(&now);

    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", localTime);

    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, buffer, NAPI_AUTO_LENGTH, &result));
    return result;
}

/*
 * Simple encryption utilities
 */
static napi_value SimpleHash(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 1;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == napi_string, "Wrong argument type. String expected.");

    size_t strLen = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen));

    char* str = new char[strLen + 1];
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], str, strLen + 1, nullptr));

    // Simple hash function (not secure, just for demonstration)
    unsigned int hash = 5381;
    for (size_t i = 0; i < strLen; i++) {
        hash = ((hash << 5) + hash) + str[i];
    }

    char hashStr[16];
    sprintf(hashStr, "%08X", hash);

    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, hashStr, NAPI_AUTO_LENGTH, &result));

    delete[] str;
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
        // String utilities
        DECLARE_NAPI_FUNCTION("reverseString", ReverseString),
        DECLARE_NAPI_FUNCTION("isPalindrome", IsPalindrome),
        DECLARE_NAPI_FUNCTION("countCharacters", CountCharacters),
        // Math utilities
        DECLARE_NAPI_FUNCTION("factorial", Factorial),
        DECLARE_NAPI_FUNCTION("fibonacci", Fibonacci),
        DECLARE_NAPI_FUNCTION("isPrime", IsPrime),
        // Array utilities
        DECLARE_NAPI_FUNCTION("sortArray", SortArray),
        DECLARE_NAPI_FUNCTION("getMax", GetMax),
        DECLARE_NAPI_FUNCTION("getMin", GetMin),
        DECLARE_NAPI_FUNCTION("sumArray", SumArray),
        // Time utilities
        DECLARE_NAPI_FUNCTION("getTimestamp", GetTimestamp),
        DECLARE_NAPI_FUNCTION("formatTime", FormatTime),
        // Encryption utilities
        DECLARE_NAPI_FUNCTION("simpleHash", SimpleHash),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

extern "C" __attribute__((visibility("default"))) void NAPI_utils_GetJSCode(const char** buf, int* bufLen)
{
    if (buf != nullptr) {
        *buf = _binary_utils_js_start;
    }

    if (bufLen != nullptr) {
        *bufLen = _binary_utils_js_end - _binary_utils_js_start;
    }
}

/*
 * Module define
 */
static napi_module utilsModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "utils",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};
/*
 * Module register function
 */
extern "C" __attribute__((constructor)) void UtilsRegisterModule(void)
{
    napi_module_register(&utilsModule);
}