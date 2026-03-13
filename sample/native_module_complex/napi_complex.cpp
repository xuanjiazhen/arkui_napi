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
#include <string>
#include <sstream>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

extern const char _binary_complex_js_start[];
extern const char _binary_complex_js_end[];

/*
 * String utilities - Advanced
 */
static napi_value StringToUpperCase(napi_env env, napi_callback_info info)
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

    // Convert to uppercase
    for (size_t i = 0; i < strLen; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] -= 32;
        }
    }

    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &result));

    delete[] str;
    return result;
}

static napi_value StringToLowerCase(napi_env env, napi_callback_info info)
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

    // Convert to lowercase
    for (size_t i = 0; i < strLen; i++) {
        if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] += 32;
        }
    }

    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &result));

    delete[] str;
    return result;
}

static napi_value TrimString(napi_env env, napi_callback_info info)
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

    // Trim whitespace
    size_t start = 0;
    while (start < strLen && (str[start] == ' ' || str[start] == '\t' || str[start] == '\n' || str[start] == '\r')) {
        start++;
    }

    size_t end = strLen - 1;
    while (end >= start && (str[end] == ' ' || str[end] == '\t' || str[end] == '\n' || str[end] == '\r')) {
        end--;
    }

    size_t newLen = end - start + 1;
    char* trimmed = new char[newLen + 1];
    strncpy(trimmed, str + start, newLen);
    trimmed[newLen] = '\0';

    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, trimmed, NAPI_AUTO_LENGTH, &result));

    delete[] str;
    delete[] trimmed;
    return result;
}

static napi_value SplitString(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 2;
    size_t argc = 2;
    napi_value args[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    NAPI_ASSERT(env, valuetype0 == napi_string, "Wrong argument type. String expected.");

    napi_valuetype valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    NAPI_ASSERT(env, valuetype1 == napi_string, "Wrong argument type. Delimiter expected.");

    size_t strLen = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen));

    char* str = new char[strLen + 1];
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], str, strLen + 1, nullptr));

    size_t delimiterLen = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], nullptr, 0, &delimiterLen));

    char* delimiter = new char[delimiterLen + 1];
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], delimiter, delimiterLen + 1, nullptr));

    // Split string
    std::vector<std::string> parts;
    char* token = strtok(str, delimiter);
    while (token != nullptr) {
        parts.push_back(token);
        token = strtok(nullptr, delimiter);
    }

    // Create result array
    napi_value result;
    NAPI_CALL(env, napi_create_array(env, &result));

    for (size_t i = 0; i < parts.size(); i++) {
        napi_value element;
        NAPI_CALL(env, napi_create_string_utf8(env, parts[i].c_str(), NAPI_AUTO_LENGTH, &element));
        NAPI_CALL(env, napi_set_element(env, result, i, element));
    }

    delete[] str;
    delete[] delimiter;
    return result;
}

static napi_value JoinStrings(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 2;
    size_t argc = 2;
    napi_value args[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    NAPI_ASSERT(env, valuetype0 == napi_object, "Wrong argument type. Array expected.");

    bool isArray;
    NAPI_CALL(env, napi_is_array(env, args[0], &isArray));
    NAPI_ASSERT(env, isArray, "Wrong argument type. Array expected.");

    napi_valuetype valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    NAPI_ASSERT(env, valuetype1 == napi_string, "Wrong argument type. Separator expected.");

    uint32_t length;
    NAPI_CALL(env, napi_get_array_length(env, args[0], &length));

    size_t separatorLen = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], nullptr, 0, &separatorLen));

    char* separator = new char[separatorLen + 1];
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], separator, separatorLen + 1, nullptr));

    // Join strings
    std::string resultStr;
    for (uint32_t i = 0; i < length; i++) {
        napi_value element;
        NAPI_CALL(env, napi_get_element(env, args[0], i, &element));

        size_t elemLen = 0;
        NAPI_CALL(env, napi_get_value_string_utf8(env, element, nullptr, 0, &elemLen));

        char* elemStr = new char[elemLen + 1];
        NAPI_CALL(env, napi_get_value_string_utf8(env, element, elemStr, elemLen + 1, nullptr));

        resultStr += elemStr;
        if (i < length - 1) {
            resultStr += separator;
        }

        delete[] elemStr;
    }

    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, resultStr.c_str(), NAPI_AUTO_LENGTH, &result));

    delete[] separator;
    return result;
}

/*
 * Math utilities - Advanced
 */
static napi_value GCD(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 2;
    size_t argc = 2;
    napi_value args[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    NAPI_ASSERT(env, valuetype0 == napi_number, "Wrong argument type. Number expected.");

    napi_valuetype valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    NAPI_ASSERT(env, valuetype1 == napi_number, "Wrong argument type. Number expected.");

    double value0;
    NAPI_CALL(env, napi_get_value_double(env, args[0], &value0));

    double value1;
    NAPI_CALL(env, napi_get_value_double(env, args[1], &value1));

    int a = static_cast<int>(value0);
    int b = static_cast<int>(value1);

    // Calculate GCD using Euclidean algorithm
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }

    napi_value result;
    NAPI_CALL(env, napi_create_number(env, a, &result));
    return result;
}

static napi_value LCM(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 2;
    size_t argc = 2;
    napi_value args[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    NAPI_ASSERT(env, valuetype0 == napi_number, "Wrong argument type. Number expected.");

    napi_valuetype valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    NAPI_ASSERT(env, valuetype1 == napi_number, "Wrong argument type. Number expected.");

    double value0;
    NAPI_CALL(env, napi_get_value_double(env, args[0], &value0));

    double value1;
    NAPI_CALL(env, napi_get_value_double(env, args[1], &value1));

    int a = static_cast<int>(value0);
    int b = static_cast<int>(value1);

    // Calculate GCD first
    int gcd = a;
    int temp = b;
    while (temp != 0) {
        int t = temp;
        temp = gcd % temp;
        gcd = t;
    }

    // Calculate LCM
    int lcm = (a * b) / gcd;

    napi_value result;
    NAPI_CALL(env, napi_create_number(env, lcm, &result));
    return result;
}

static napi_value IsPerfectNumber(napi_env env, napi_callback_info info)
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
    NAPI_ASSERT(env, n > 0, "Number must be positive");

    // Check if perfect number
    int sum = 0;
    for (int i = 1; i < n; i++) {
        if (n % i == 0) {
            sum += i;
        }
    }

    bool isPerfect = (sum == n);

    napi_value result;
    NAPI_CALL(env, napi_create_boolean(env, isPerfect, &result));
    return result;
}

static napi_value GeneratePrimeNumbers(napi_env env, napi_callback_info info)
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
    NAPI_ASSERT(env, n > 0, "Number must be positive");

    // Generate prime numbers up to n using Sieve of Eratosthenes
    std::vector<bool> isPrime(n + 1, true);
    isPrime[0] = isPrime[1] = false;

    for (int i = 2; i * i <= n; i++) {
        if (isPrime[i]) {
            for (int j = i * i; j <= n; j += i) {
                isPrime[j] = false;
            }
        }
    }

    // Collect prime numbers
    std::vector<int> primes;
    for (int i = 2; i <= n; i++) {
        if (isPrime[i]) {
            primes.push_back(i);
        }
    }

    // Create result array
    napi_value result;
    NAPI_CALL(env, napi_create_array(env, &result));

    for (size_t i = 0; i < primes.size(); i++) {
        napi_value element;
        NAPI_CALL(env, napi_create_number(env, primes[i], &element));
        NAPI_CALL(env, napi_set_element(env, result, i, element));
    }

    return result;
}

/*
 * Array utilities - Advanced
 */
static napi_value FilterArray(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 2;
    size_t argc = 2;
    napi_value args[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    NAPI_ASSERT(env, valuetype0 == napi_object, "Wrong argument type. Array expected.");

    bool isArray;
    NAPI_CALL(env, napi_is_array(env, args[0], &isArray));
    NAPI_ASSERT(env, isArray, "Wrong argument type. Array expected.");

    napi_valuetype valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    NAPI_ASSERT(env, valuetype1 == napi_function, "Wrong argument type. Function expected.");

    uint32_t length;
    NAPI_CALL(env, napi_get_array_length(env, args[0], &length));

    // Filter array
    std::vector<napi_value> filtered;
    for (uint32_t i = 0; i < length; i++) {
        napi_value element;
        NAPI_CALL(env, napi_get_element(env, args[0], i, &element));

        // Call filter function
        napi_value argv[1] = { element };
        napi_value result;
        NAPI_CALL(env, napi_call_function(env, nullptr, args[1], 1, argv, &result));

        bool keep;
        NAPI_CALL(env, napi_get_value_bool(env, result, &keep));

        if (keep) {
            filtered.push_back(element);
        }
    }

    // Create result array
    napi_value result;
    NAPI_CALL(env, napi_create_array(env, &result));

    for (size_t i = 0; i < filtered.size(); i++) {
        NAPI_CALL(env, napi_set_element(env, result, i, filtered[i]));
    }

    return result;
}

static napi_value MapArray(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 2;
    size_t argc = 2;
    napi_value args[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    NAPI_ASSERT(env, valuetype0 == napi_object, "Wrong argument type. Array expected.");

    bool isArray;
    NAPI_CALL(env, napi_is_array(env, args[0], &isArray));
    NAPI_ASSERT(env, isArray, "Wrong argument type. Array expected.");

    napi_valuetype valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    NAPI_ASSERT(env, valuetype1 == napi_function, "Wrong argument type. Function expected.");

    uint32_t length;
    NAPI_CALL(env, napi_get_array_length(env, args[0], &length));

    // Map array
    napi_value result;
    NAPI_CALL(env, napi_create_array(env, &result));

    for (uint32_t i = 0; i < length; i++) {
        napi_value element;
        NAPI_CALL(env, napi_get_element(env, args[0], i, &element));

        // Call map function
        napi_value argv[1] = { element };
        napi_value mappedValue;
        NAPI_CALL(env, napi_call_function(env, nullptr, args[1], 1, argv, &mappedValue));

        NAPI_CALL(env, napi_set_element(env, result, i, mappedValue));
    }

    return result;
}

static napi_value ReduceArray(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 2;
    size_t argc = 3;
    napi_value args[3] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    NAPI_ASSERT(env, valuetype0 == napi_object, "Wrong argument type. Array expected.");

    bool isArray;
    NAPI_CALL(env, napi_is_array(env, args[0], &isArray));
    NAPI_ASSERT(env, isArray, "Wrong argument type. Array expected.");

    napi_valuetype valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    NAPI_ASSERT(env, valuetype1 == napi_function, "Wrong argument type. Function expected.");

    uint32_t length;
    NAPI_CALL(env, napi_get_array_length(env, args[0], &length));

    napi_value accumulator;
    if (argc >= 3) {
        accumulator = args[2];
    } else {
        NAPI_CALL(env, napi_get_element(env, args[0], 0, &accumulator));
    }

    // Reduce array
    uint32_t startIndex = (argc >= 3) ? 0 : 1;
    for (uint32_t i = startIndex; i < length; i++) {
        napi_value element;
        NAPI_CALL(env, napi_get_element(env, args[0], i, &element));

        // Call reduce function
        napi_value argv[2] = { accumulator, element };
        NAPI_CALL(env, napi_call_function(env, nullptr, args[1], 2, argv, &accumulator));
    }

    return accumulator;
}

static napi_value RemoveDuplicates(napi_env env, napi_callback_info info)
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

    // Remove duplicates
    std::vector<napi_value> unique;
    for (uint32_t i = 0; i < length; i++) {
        napi_value element;
        NAPI_CALL(env, napi_get_element(env, args[0], i, &element));

        bool isDuplicate = false;
        for (size_t j = 0; j < unique.size(); j++) {
            bool equal;
            NAPI_CALL(env, napi_strict_equals(env, element, unique[j], &equal));
            if (equal) {
                isDuplicate = true;
                break;
            }
        }

        if (!isDuplicate) {
            unique.push_back(element);
        }
    }

    // Create result array
    napi_value result;
    NAPI_CALL(env, napi_create_array(env, &result));

    for (size_t i = 0; i < unique.size(); i++) {
        NAPI_CALL(env, napi_set_element(env, result, i, unique[i]));
    }

    return result;
}

/*
 * Date utilities - Advanced
 */
static napi_value GetDateComponents(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 0;
    size_t argc = 0;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr));

    time_t now = time(nullptr);
    struct tm* localTime = localtime(&now);

    // Create result object
    napi_value result;
    NAPI_CALL(env, napi_create_object(env, &result));

    // Set date components
    napi_value year;
    NAPI_CALL(env, napi_create_number(env, localTime->tm_year + 1900, &year));
    NAPI_CALL(env, napi_set_named_property(env, result, "year", year));

    napi_value month;
    NAPI_CALL(env, napi_create_number(env, localTime->tm_mon + 1, &month));
    NAPI_CALL(env, napi_set_named_property(env, result, "month", month));

    napi_value day;
    NAPI_CALL(env, napi_create_number(env, localTime->tm_mday, &day));
    NAPI_CALL(env, napi_set_named_property(env, result, "day", day));

    napi_value hour;
    NAPI_CALL(env, napi_create_number(env, localTime->tm_hour, &hour));
    NAPI_CALL(env, napi_set_named_property(env, result, "hour", hour));

    napi_value minute;
    NAPI_CALL(env, napi_create_number(env, localTime->tm_min, &minute));
    NAPI_CALL(env, napi_set_named_property(env, result, "minute", minute));

    napi_value second;
    NAPI_CALL(env, napi_create_number(env, localTime->tm_sec, &second));
    NAPI_CALL(env, napi_set_named_property(env, result, "second", second));

    napi_value weekday;
    NAPI_CALL(env, napi_create_number(env, localTime->tm_wday, &weekday));
    NAPI_CALL(env, napi_set_named_property(env, result, "weekday", weekday));

    return result;
}

static napi_value FormatDate(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 1;
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    NAPI_ASSERT(env, valuetype0 == napi_string, "Wrong argument type. Format string expected.");

    size_t formatLen = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], nullptr, 0, &formatLen));

    char* format = new char[formatLen + 1];
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], format, formatLen + 1, nullptr));

    time_t now = time(nullptr);
    struct tm* localTime = localtime(&now);

    char buffer[256];
    strftime(buffer, sizeof(buffer), format, localTime);

    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, buffer, NAPI_AUTO_LENGTH, &result));

    delete[] format;
    return result;
}

/*
 * Simple data structures
 */
// Simple linked list implementation
struct ListNode {
    int value;
    ListNode* next;
    ListNode(int val) : value(val), next(nullptr) {}
};

static napi_value CreateLinkedList(napi_env env, napi_callback_info info)
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

    // Create linked list
    ListNode* head = nullptr;
    ListNode* tail = nullptr;

    for (uint32_t i = 0; i < length; i++) {
        napi_value element;
        NAPI_CALL(env, napi_get_element(env, args[0], i, &element));

        double value;
        NAPI_CALL(env, napi_get_value_double(env, element, &value));

        ListNode* newNode = new ListNode(static_cast<int>(value));
        if (!head) {
            head = newNode;
            tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
    }

    // Convert linked list to array
    std::vector<int> values;
    ListNode* current = head;
    while (current) {
        values.push_back(current->value);
        ListNode* temp = current;
        current = current->next;
        delete temp;
    }

    // Create result array
    napi_value result;
    NAPI_CALL(env, napi_create_array(env, &result));

    for (size_t i = 0; i < values.size(); i++) {
        napi_value element;
        NAPI_CALL(env, napi_create_number(env, values[i], &element));
        NAPI_CALL(env, napi_set_element(env, result, i, element));
    }

    return result;
}

/*
 * Simple encryption utilities
 */
static napi_value CaesarCipher(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 2;
    size_t argc = 2;
    napi_value args[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    NAPI_ASSERT(env, valuetype0 == napi_string, "Wrong argument type. String expected.");

    napi_valuetype valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    NAPI_ASSERT(env, valuetype1 == napi_number, "Wrong argument type. Shift expected.");

    size_t strLen = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen));

    char* str = new char[strLen + 1];
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], str, strLen + 1, nullptr));

    double shiftValue;
    NAPI_CALL(env, napi_get_value_double(env, args[1], &shiftValue));
    int shift = static_cast<int>(shiftValue) % 26;
    if (shift < 0) shift += 26;

    // Apply Caesar cipher
    for (size_t i = 0; i < strLen; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            str[i] = 'a' + (str[i] - 'a' + shift) % 26;
        } else if (str[i] >= 'A' && str[i] <= 'Z') {
            str[i] = 'A' + (str[i] - 'A' + shift) % 26;
        }
    }

    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &result));

    delete[] str;
    return result;
}

static napi_value VigenereCipher(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 2;
    size_t argc = 2;
    napi_value args[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));
    NAPI_ASSERT(env, valuetype0 == napi_string, "Wrong argument type. String expected.");

    napi_valuetype valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));
    NAPI_ASSERT(env, valuetype1 == napi_string, "Wrong argument type. Key expected.");

    size_t strLen = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen));

    char* str = new char[strLen + 1];
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], str, strLen + 1, nullptr));

    size_t keyLen = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], nullptr, 0, &keyLen));

    char* key = new char[keyLen + 1];
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], key, keyLen + 1, nullptr));

    // Apply Vigenere cipher
    for (size_t i = 0, j = 0; i < strLen; i++) {
        if (str[i] >= 'a' && str[i] <= 'z') {
            int keyShift = key[j % keyLen] - 'a';
            if (keyShift < 0) keyShift += 26;
            str[i] = 'a' + (str[i] - 'a' + keyShift) % 26;
            j++;
        } else if (str[i] >= 'A' && str[i] <= 'Z') {
            int keyShift = key[j % keyLen] - 'A';
            if (keyShift < 0) keyShift += 26;
            str[i] = 'A' + (str[i] - 'A' + keyShift) % 26;
            j++;
        }
    }

    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, str, NAPI_AUTO_LENGTH, &result));

    delete[] str;
    delete[] key;
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
        DECLARE_NAPI_FUNCTION("toUpperCase", StringToUpperCase),
        DECLARE_NAPI_FUNCTION("toLowerCase", StringToLowerCase),
        DECLARE_NAPI_FUNCTION("trim", TrimString),
        DECLARE_NAPI_FUNCTION("split", SplitString),
        DECLARE_NAPI_FUNCTION("join", JoinStrings),
        // Math utilities
        DECLARE_NAPI_FUNCTION("gcd", GCD),
        DECLARE_NAPI_FUNCTION("lcm", LCM),
        DECLARE_NAPI_FUNCTION("isPerfectNumber", IsPerfectNumber),
        DECLARE_NAPI_FUNCTION("generatePrimes", GeneratePrimeNumbers),
        // Array utilities
        DECLARE_NAPI_FUNCTION("filter", FilterArray),
        DECLARE_NAPI_FUNCTION("map", MapArray),
        DECLARE_NAPI_FUNCTION("reduce", ReduceArray),
        DECLARE_NAPI_FUNCTION("removeDuplicates", RemoveDuplicates),
        // Date utilities
        DECLARE_NAPI_FUNCTION("getDateComponents", GetDateComponents),
        DECLARE_NAPI_FUNCTION("formatDate", FormatDate),
        // Data structures
        DECLARE_NAPI_FUNCTION("createLinkedList", CreateLinkedList),
        // Encryption utilities
        DECLARE_NAPI_FUNCTION("caesarCipher", CaesarCipher),
        DECLARE_NAPI_FUNCTION("vigenereCipher", VigenereCipher),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

extern "C" __attribute__((visibility("default"))) void NAPI_complex_GetJSCode(const char** buf, int* bufLen)
{
    if (buf != nullptr) {
        *buf = _binary_complex_js_start;
    }

    if (bufLen != nullptr) {
        *bufLen = _binary_complex_js_end - _binary_complex_js_start;
    }
}

/*
 * Module define
 */
static napi_module complexModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "complex",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};
/*
 * Module register function
 */
extern "C" __attribute__((constructor)) void ComplexRegisterModule(void)
{
    napi_module_register(&complexModule);
}