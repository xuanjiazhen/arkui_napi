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

#include <napi.h>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <algorithm>
#include <regex>
#include <cctype>

namespace {

constexpr int DEFAULT_DECIMALS = 2;
constexpr int DEFAULT_PRECISION = 2;
constexpr int MIN_CARD_LENGTH = 13;
constexpr int MAX_CARD_LENGTH = 16;
constexpr int CHAR_ARRAY_SIZE_3 = 3;
constexpr int CHAR_ARRAY_SIZE_4 = 4;
constexpr int BUFFER_SIZE = 100;
constexpr int BASE64_CHAR_COUNT = 64;
constexpr int HEX_ENCODE_SIZE = 4;
constexpr int HEX_DECODE_SIZE = 3;
constexpr unsigned char BASE64_MASK_FC = 0xfc;
constexpr unsigned char BASE64_MASK_03 = 0x03;
constexpr unsigned char BASE64_MASK_F0 = 0xf0;
constexpr unsigned char BASE64_MASK_0F = 0x0f;
constexpr unsigned char BASE64_MASK_C0 = 0xc0;
constexpr unsigned char BASE64_MASK_3F = 0x3f;
constexpr unsigned char BASE64_MASK_30 = 0x30;
constexpr unsigned char BASE64_MASK_0F_VAL = 0xf;
constexpr unsigned char BASE64_MASK_3C = 0x3c;
constexpr unsigned char BASE64_MASK_03_VAL = 0x03;
constexpr int BASE64_SHIFT_2 = 2;
constexpr int BASE64_SHIFT_4 = 4;
constexpr int BASE64_SHIFT_6 = 6;
constexpr int NUMBER_1 = 1;
constexpr int NUMBER_2 = 2;
constexpr int NUMBER_3 = 3;
constexpr int NUMBER_4 = 4;
constexpr int NUMBER_6 = 6;
constexpr int NUMBER_8 = 8;
constexpr int NUMBER_9 = 9;
constexpr int NUMBER_26 = 26;
constexpr int NUMBER_30 = 30;
constexpr int NUMBER_32 = 32;
constexpr int NUMBER_100 = 100;
constexpr int NUMBER_1900 = 1900;
constexpr int NUMBER_400 = 400;
constexpr int NUMBER_86400 = 86400;

napi_value ConvertToNumber(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing value to convert");
        return nullptr;
    }

    napi_value result;
    napi_valuetype type;
    status = napi_typeof(env, args[0], &type);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get type");
        return nullptr;
    }

    switch (type) {
        case napi_string: {
            size_t length = 0;
            status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
                return nullptr;
            }
            char* str = new char[length + 1];
            status = napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);
            if (status != napi_ok) {
                delete[] str;
                napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
                return nullptr;
            }
            double value = strtod(str, nullptr);
            delete[] str;
            status = napi_create_number(env, value, &result);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to create number");
                return nullptr;
            }
            break;
        }
        case napi_boolean: {
            bool value = false;
            status = napi_get_value_bool(env, args[0], &value);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to get bool value");
                return nullptr;
            }
            status = napi_create_number(env, value ? 1 : 0, &result);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to create number");
                return nullptr;
            }
            break;
        }
        case napi_number:
            result = args[0];
            break;
        default:
            status = napi_create_number(env, 0, &result);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to create number");
                return nullptr;
            }
            break;
    }

    return result;
}

napi_value ConvertToString(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing value to convert");
        return nullptr;
    }

    napi_value result;
    napi_valuetype type;
    status = napi_typeof(env, args[0], &type);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get type");
        return nullptr;
    }

    switch (type) {
        case napi_number: {
            double value = 0;
            status = napi_get_value_double(env, args[0], &value);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to get double value");
                return nullptr;
            }
            std::stringstream ss;
            ss << value;
            status = napi_create_string_utf8(env, ss.str().c_str(), NAPI_AUTO_LENGTH, &result);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
                return nullptr;
            }
            break;
        }
        case napi_boolean: {
            bool value = false;
            status = napi_get_value_bool(env, args[0], &value);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to get bool value");
                return nullptr;
            }
            status = napi_create_string_utf8(env, value ? "true" : "false", NAPI_AUTO_LENGTH, &result);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
                return nullptr;
            }
            break;
        }
        case napi_string:
            result = args[0];
            break;
        default:
            status = napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &result);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
                return nullptr;
            }
            break;
    }

    return result;
}

napi_value ConvertToBoolean(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing value to convert");
        return nullptr;
    }

    napi_value result;
    napi_valuetype type;
    status = napi_typeof(env, args[0], &type);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get type");
        return nullptr;
    }

    switch (type) {
        case napi_number: {
            double value;
            status = napi_get_value_double(env, args[0], &value);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to get double value");
                return nullptr;
            }
            status = napi_create_boolean(env, value != 0, &result);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to create boolean");
                return nullptr;
            }
            break;
        }
        case napi_string: {
            size_t length = 0;
            status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
                return nullptr;
            }
            char* str = new char[length + 1];
            status = napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);
            if (status != napi_ok) {
                delete[] str;
                napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
                return nullptr;
            }
            std::string s = str;
            delete[] str;
            bool value = (s == "true" || s == "1" || s == "yes" || s == "y" || s == "on");
            status = napi_create_boolean(env, value, &result);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to create boolean");
                return nullptr;
            }
            break;
        }
        case napi_boolean:
            result = args[0];
            break;
        default:
            status = napi_create_boolean(env, false, &result);
            if (status != napi_ok) {
                napi_throw_error(env, "NAPI_ERROR", "Failed to create boolean");
                return nullptr;
            }
            break;
    }

    return result;
}

napi_value ValidateEmail(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing email to validate");
        return nullptr;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
        return nullptr;
    }
    char* email = new char[length + 1];
    status = napi_get_value_string_utf8(env, args[0], email, length + 1, nullptr);
    if (status != napi_ok) {
        delete[] email;
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    std::regex emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    bool valid = std::regex_match(email, emailRegex);
    delete[] email;

    napi_value result;
    status = napi_create_boolean(env, valid, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create boolean");
        return nullptr;
    }
    return result;
}

napi_value ValidateURL(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing URL to validate");
        return nullptr;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
        return nullptr;
    }
    char* url = new char[length + 1];
    status = napi_get_value_string_utf8(env, args[0], url, length + 1, nullptr);
    if (status != napi_ok) {
        delete[] url;
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    std::regex urlRegex("^(https?:\\/\\/)?([\\da-z.-]+)\\.([a-z.]{2,6})([/\\w .-]*)*\\/?$");
    bool valid = std::regex_match(url, urlRegex);
    delete[] url;

    napi_value result;
    status = napi_create_boolean(env, valid, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create boolean");
        return nullptr;
    }
    return result;
}

napi_value ValidatePhoneNumber(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing phone number to validate");
        return nullptr;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
        return nullptr;
    }
    char* phone = new char[length + 1];
    status = napi_get_value_string_utf8(env, args[0], phone, length + 1, nullptr);
    if (status != napi_ok) {
        delete[] phone;
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    std::regex phoneRegex("^\\+?[1-9][0-9]{7,14}$");
    bool valid = std::regex_match(phone, phoneRegex);
    delete[] phone;

    napi_value result;
    status = napi_create_boolean(env, valid, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create boolean");
        return nullptr;
    }
    return result;
}

napi_value ValidateCreditCard(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing credit card number to validate");
        return nullptr;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
        return nullptr;
    }
    char* card = new char[length + 1];
    status = napi_get_value_string_utf8(env, args[0], card, length + 1, nullptr);
    if (status != napi_ok) {
        delete[] card;
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    std::string cardNumber = card;
    cardNumber.erase(std::remove(cardNumber.begin(), cardNumber.end(), ' '), cardNumber.end());
    cardNumber.erase(std::remove(cardNumber.begin(), cardNumber.end(), '-'), cardNumber.end());
    delete[] card;

    int sum = 0;
    bool isEven = false;
    for (int i = static_cast<int>(cardNumber.length()) - 1; i >= 0; i--) {
        int digit = cardNumber[i] - '0';
        if (isEven) {
            digit *= 2;
            if (digit > 9) {
                digit -= 9;
            }
        }
        sum += digit;
        isEven = !isEven;
    }

    bool valid = (sum % 10 == 0) &&
                 (cardNumber.length() >= MIN_CARD_LENGTH) &&
                 (cardNumber.length() <= MAX_CARD_LENGTH);

    napi_value result;
    status = napi_create_boolean(env, valid, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create boolean");
        return nullptr;
    }
    return result;
}

napi_value FormatNumber(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing number to format");
        return nullptr;
    }

    double number = 0.0;
    status = napi_get_value_double(env, args[0], &number);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get double value");
        return nullptr;
    }

    int decimals = DEFAULT_DECIMALS;
    if (argc > 1) {
        status = napi_get_value_int32(env, args[1], &decimals);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to get int32 value");
            return nullptr;
        }
    }

    std::stringstream ss;
    ss.precision(decimals);
    ss << std::fixed << number;

    napi_value result;
    status = napi_create_string_utf8(env, ss.str().c_str(), NAPI_AUTO_LENGTH, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    return result;
}

napi_value FormatCurrency(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value args[3];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing amount to format");
        return nullptr;
    }

    double amount = 0.0;
    status = napi_get_value_double(env, args[0], &amount);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get double value");
        return nullptr;
    }

    std::string currency = "USD";
    if (argc > 1) {
        size_t length = 0;
        status = napi_get_value_string_utf8(env, args[1], nullptr, 0, &length);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
            return nullptr;
        }
        char* str = new char[length + 1];
        status = napi_get_value_string_utf8(env, args[1], str, length + 1, nullptr);
        if (status != napi_ok) {
            delete[] str;
            napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
            return nullptr;
        }
        currency = str;
        delete[] str;
    }

    std::string locale = "en-US";
    if (argc > 2) {
        size_t length = 0;
        status = napi_get_value_string_utf8(env, args[2], nullptr, 0, &length);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
            return nullptr;
        }
        char* str = new char[length + 1];
        status = napi_get_value_string_utf8(env, args[2], str, length + 1, nullptr);
        if (status != napi_ok) {
            delete[] str;
            napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
            return nullptr;
        }
        locale = str;
        delete[] str;
    }

    std::stringstream ss;
    ss.precision(DEFAULT_PRECISION);
    ss << std::fixed << amount;

    std::string resultStr = currency + " " + ss.str();

    napi_value result;
    status = napi_create_string_utf8(env, resultStr.c_str(), NAPI_AUTO_LENGTH, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    return result;
}

napi_value FormatDate(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    time_t now = time(nullptr);
    if (argc > 0) {
        double timestamp = 0.0;
        status = napi_get_value_double(env, args[0], &timestamp);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to get double value");
            return nullptr;
        }
        now = static_cast<time_t>(timestamp);
    }

    std::string format = "%Y-%m-%d %H:%M:%S";
    if (argc > 1) {
        size_t length = 0;
        status = napi_get_value_string_utf8(env, args[1], nullptr, 0, &length);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
            return nullptr;
        }
        char* str = new char[length + 1];
        status = napi_get_value_string_utf8(env, args[1], str, length + 1, nullptr);
        if (status != napi_ok) {
            delete[] str;
            napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
            return nullptr;
        }
        format = str;
        delete[] str;
    }

    std::tm localTime = {};
    std::tm* pLocalTime = localtime(&now);
    if (pLocalTime != nullptr) {
        localTime = *pLocalTime;
    }
    char buffer[BUFFER_SIZE];
    size_t resultLen = strftime(buffer, sizeof(buffer), format.c_str(), &localTime);
    if (resultLen == 0) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to format date");
        return nullptr;
    }

    napi_value result;
    status = napi_create_string_utf8(env, buffer, NAPI_AUTO_LENGTH, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    return result;
}

napi_value Base64Encode(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string to encode");
        return nullptr;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
        return nullptr;
    }
    char* str = new char[length + 1];
    status = napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);
    if (status != napi_ok) {
        delete[] str;
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    const char base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int i = 0;
    int j = 0;
    unsigned char charArray3[CHAR_ARRAY_SIZE_3];
    unsigned char charArray4[CHAR_ARRAY_SIZE_4];

    while (length--) {
        charArray3[i++] = *(str++);
        if (i == CHAR_ARRAY_SIZE_3) {
            charArray4[0] = (charArray3[0] & BASE64_MASK_FC) >> BASE64_SHIFT_2;
            charArray4[1] = ((charArray3[0] & BASE64_MASK_03) << BASE64_SHIFT_4) +
                            ((charArray3[1] & BASE64_MASK_F0) >> BASE64_SHIFT_4);
            charArray4[2] = ((charArray3[1] & BASE64_MASK_0F) << BASE64_SHIFT_2) +
                            ((charArray3[2] & BASE64_MASK_C0) >> BASE64_SHIFT_6);
            charArray4[3] = charArray3[2] & BASE64_MASK_3F;

            for (i = 0; i < CHAR_ARRAY_SIZE_4; i++) {
                result += base64Chars[charArray4[i]];
            }
            i = 0;
        }
    }

    if (i > 0) {
        for (j = i; j < CHAR_ARRAY_SIZE_3; j++) {
            charArray3[j] = '\0';
        }

        charArray4[0] = (charArray3[0] & BASE64_MASK_FC) >> BASE64_SHIFT_2;
        charArray4[1] = ((charArray3[0] & BASE64_MASK_03) << BASE64_SHIFT_4) +
                        ((charArray3[1] & BASE64_MASK_F0) >> BASE64_SHIFT_4);
        charArray4[2] = ((charArray3[1] & BASE64_MASK_0F) << BASE64_SHIFT_2) +
                        ((charArray3[2] & BASE64_MASK_C0) >> BASE64_SHIFT_6);

        for (j = 0; j < i + 1; j++) {
            result += base64Chars[charArray4[j]];
        }

        while (i++ < CHAR_ARRAY_SIZE_3) {
            result += '=';
        }
    }

    delete[] str;

    napi_value resultValue;
    status = napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    return resultValue;
}

napi_value Base64Decode(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string to decode");
        return nullptr;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
        return nullptr;
    }
    char* str = new char[length + 1];
    status = napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);
    if (status != napi_ok) {
        delete[] str;
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    const char base64Chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int i = 0;
    int j = 0;
    int inputIndex = 0;
    unsigned char charArray4[CHAR_ARRAY_SIZE_4];
    unsigned char charArray3[CHAR_ARRAY_SIZE_3];

    while (length-- && (str[inputIndex] != '=') &&
           (std::isalnum(static_cast<unsigned char>(str[inputIndex])) || str[inputIndex] == '+' || str[inputIndex] == '/')) {
        charArray4[i++] = str[inputIndex];
        inputIndex++;
        if (i == CHAR_ARRAY_SIZE_4) {
            for (i = 0; i < CHAR_ARRAY_SIZE_4; i++) {
                const char* charPos = std::strchr(base64Chars, static_cast<char>(charArray4[i]));
                charArray4[i] = static_cast<unsigned char>(charPos - base64Chars);
            }

            charArray3[0] = (charArray4[0] << BASE64_SHIFT_2) +
                            ((charArray4[1] & BASE64_MASK_30) >> BASE64_SHIFT_4);
            charArray3[1] = ((charArray4[1] & BASE64_MASK_0F_VAL) << BASE64_SHIFT_4) +
                            ((charArray4[2] & BASE64_MASK_3C) >> BASE64_SHIFT_2);
            charArray3[2] = ((charArray4[2] & BASE64_MASK_03_VAL) << BASE64_SHIFT_6) + charArray4[3];

            for (i = 0; i < CHAR_ARRAY_SIZE_3; i++) {
                result += static_cast<char>(charArray3[i]);
            }
            i = 0;
        }
    }

    if (i > 0) {
        for (j = i; j < CHAR_ARRAY_SIZE_4; j++) {
            charArray4[j] = 0;
        }

        for (j = 0; j < CHAR_ARRAY_SIZE_4; j++) {
            charArray4[j] = static_cast<unsigned char>(std::strchr(base64Chars, static_cast<char>(charArray4[j])) - base64Chars);
        }

        charArray3[0] = (charArray4[0] << BASE64_SHIFT_2) +
                        ((charArray4[1] & BASE64_MASK_30) >> BASE64_SHIFT_4);
        charArray3[1] = ((charArray4[1] & BASE64_MASK_0F_VAL) << BASE64_SHIFT_4) +
                        ((charArray4[2] & BASE64_MASK_3C) >> BASE64_SHIFT_2);

        for (j = 0; j < i - 1; j++) {
            result += static_cast<char>(charArray3[j]);
        }
    }

    delete[] str;

    napi_value resultValue;
    status = napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    return resultValue;
}

napi_value URLEncode(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string to encode");
        return nullptr;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
        return nullptr;
    }
    char* str = new char[length + 1];
    status = napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);
    if (status != napi_ok) {
        delete[] str;
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    std::string result;
    for (size_t idx = 0; idx < length; idx++) {
        unsigned char c = static_cast<unsigned char>(str[idx]);
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            result += c;
        } else if (c == ' ') {
            result += '+';
        } else {
            char hex[HEX_ENCODE_SIZE];
            snprintf(hex, sizeof(hex), "%%%02X", c);
            result += hex;
        }
    }
    delete[] str;

    napi_value resultValue;
    status = napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    return resultValue;
}

napi_value URLDecode(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string to decode");
        return nullptr;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
        return nullptr;
    }
    char* str = new char[length + 1];
    status = napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);
    if (status != napi_ok) {
        delete[] str;
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    std::string result;
    for (size_t i = 0; i < length; i++) {
        if (str[i] == '+') {
            result += ' ';
        } else if (str[i] == '%' && i + 2 < length) {
            char hex[HEX_DECODE_SIZE] = {str[i + 1], str[i + 2], '\0'};
            int value = strtol(hex, nullptr, 16);
            result += static_cast<char>(value);
            i += 2;
        } else {
            result += str[i];
        }
    }
    delete[] str;

    napi_value resultValue;
    status = napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    return resultValue;
}

napi_value ArraySort(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing array to sort");
        return nullptr;
    }

    bool isArray = false;
    status = napi_is_array(env, args[0], &isArray);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to check array");
        return nullptr;
    }
    if (!isArray) {
        napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an array");
        return nullptr;
    }

    uint32_t length = 0;
    status = napi_get_array_length(env, args[0], &length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get array length");
        return nullptr;
    }

    std::vector<double> values;
    for (uint32_t i = 0; i < length; i++) {
        napi_value element;
        status = napi_get_element(env, args[0], i, &element);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to get element");
            return nullptr;
        }
        double value = 0.0;
        status = napi_get_value_double(env, element, &value);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to get double value");
            return nullptr;
        }
        values.push_back(value);
    }

    std::sort(values.begin(), values.end());

    napi_value result;
    status = napi_create_array(env, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create array");
        return nullptr;
    }
    for (uint32_t i = 0; i < length; i++) {
        napi_value element;
        status = napi_create_number(env, values[i], &element);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to create number");
            return nullptr;
        }
        status = napi_set_element(env, result, i, element);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to set element");
            return nullptr;
        }
    }

    return result;
}

napi_value ArrayUnique(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing array to process");
        return nullptr;
    }

    bool isArray = false;
    status = napi_is_array(env, args[0], &isArray);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to check array");
        return nullptr;
    }
    if (!isArray) {
        napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an array");
        return nullptr;
    }

    uint32_t length = 0;
    status = napi_get_array_length(env, args[0], &length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get array length");
        return nullptr;
    }

    std::vector<double> values;
    std::map<double, bool> seen;
    for (uint32_t i = 0; i < length; i++) {
        napi_value element;
        status = napi_get_element(env, args[0], i, &element);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to get element");
            return nullptr;
        }
        double value = 0.0;
        status = napi_get_value_double(env, element, &value);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to get double value");
            return nullptr;
        }
        if (!seen[value]) {
            values.push_back(value);
            seen[value] = true;
        }
    }

    napi_value result;
    status = napi_create_array(env, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create array");
        return nullptr;
    }
    for (size_t i = 0; i < values.size(); i++) {
        napi_value element;
        status = napi_create_number(env, values[i], &element);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to create number");
            return nullptr;
        }
        status = napi_set_element(env, result, i, element);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to set element");
            return nullptr;
        }
    }

    return result;
}

napi_value ArraySum(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing array to sum");
        return nullptr;
    }

    bool isArray = false;
    status = napi_is_array(env, args[0], &isArray);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to check array");
        return nullptr;
    }
    if (!isArray) {
        napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an array");
        return nullptr;
    }

    uint32_t length = 0;
    status = napi_get_array_length(env, args[0], &length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get array length");
        return nullptr;
    }

    double sum = 0.0;
    for (uint32_t i = 0; i < length; i++) {
        napi_value element;
        status = napi_get_element(env, args[0], i, &element);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to get element");
            return nullptr;
        }
        double value = 0.0;
        status = napi_get_value_double(env, element, &value);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to get double value");
            return nullptr;
        }
        sum += value;
    }

    napi_value result;
    status = napi_create_number(env, sum, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create number");
        return nullptr;
    }
    return result;
}

napi_value ArrayAverage(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing array to average");
        return nullptr;
    }

    bool isArray = false;
    status = napi_is_array(env, args[0], &isArray);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to check array");
        return nullptr;
    }
    if (!isArray) {
        napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an array");
        return nullptr;
    }

    uint32_t length = 0;
    status = napi_get_array_length(env, args[0], &length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get array length");
        return nullptr;
    }

    if (length == 0) {
        napi_value result;
        status = napi_create_number(env, 0, &result);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to create number");
            return nullptr;
        }
        return result;
    }

    double sum = 0.0;
    for (uint32_t i = 0; i < length; i++) {
        napi_value element;
        status = napi_get_element(env, args[0], i, &element);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to get element");
            return nullptr;
        }
        double value = 0.0;
        status = napi_get_value_double(env, element, &value);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to get double value");
            return nullptr;
        }
        sum += value;
    }

    napi_value result;
    status = napi_create_number(env, sum / length, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create number");
        return nullptr;
    }
    return result;
}

static napi_status ValidateObject(napi_env env, napi_value value, const char* errorMsg)
{
    bool isObject = false;
    napi_status status = napi_is_object(env, value, &isObject);
    if (status != napi_ok) {
        return status;
    }
    if (!isObject) {
        napi_throw_error(env, "ERR_INVALID_TYPE", errorMsg);
        return napi_invalid_arg;
    }
    return napi_ok;
}

static napi_value GetPropertyNames(napi_env env, napi_value obj, uint32_t* length)
{
    napi_value propNames;
    napi_status status = napi_get_property_names(env, obj, &propNames);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get property names");
        return nullptr;
    }
    status = napi_get_array_length(env, propNames, length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get array length");
        return nullptr;
    }
    return propNames;
}

static napi_status FillKeysArray(napi_env env, napi_value result, napi_value propNames, uint32_t length)
{
    uint32_t index = 0;
    for (uint32_t i = 0; i < length; i++) {
        napi_value key;
        napi_status status = napi_get_element(env, propNames, i, &key);
        if (status != napi_ok) {
            return status;
        }
        status = napi_set_element(env, result, index++, key);
        if (status != napi_ok) {
            return status;
        }
    }
    return napi_ok;
}

static napi_status FillValuesArray(napi_env env, napi_value result, napi_value obj, napi_value propNames, uint32_t length)
{
    uint32_t index = 0;
    for (uint32_t i = 0; i < length; i++) {
        napi_value key;
        napi_status status = napi_get_element(env, propNames, i, &key);
        if (status != napi_ok) {
            return status;
        }
        napi_value value;
        status = napi_get_property(env, obj, key, &value);
        if (status != napi_ok) {
            return status;
        }
        status = napi_set_element(env, result, index++, value);
        if (status != napi_ok) {
            return status;
        }
    }
    return napi_ok;
}

static napi_status FillEntriesArray(napi_env env, napi_value result, napi_value obj, napi_value propNames, uint32_t length)
{
    uint32_t index = 0;
    for (uint32_t i = 0; i < length; i++) {
        napi_value key;
        napi_status status = napi_get_element(env, propNames, i, &key);
        if (status != napi_ok) {
            return status;
        }
        napi_value value;
        status = napi_get_property(env, obj, key, &value);
        if (status != napi_ok) {
            return status;
        }
        napi_value entry;
        status = napi_create_array(env, &entry);
        if (status != napi_ok) {
            return status;
        }
        status = napi_set_element(env, entry, 0, key);
        if (status != napi_ok) {
            return status;
        }
        status = napi_set_element(env, entry, 1, value);
        if (status != napi_ok) {
            return status;
        }
        status = napi_set_element(env, result, index++, entry);
        if (status != napi_ok) {
            return status;
        }
    }
    return napi_ok;
}

napi_value ObjectKeys(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing object to process");
        return nullptr;
    }

    status = ValidateObject(env, args[0], "Argument must be an object");
    if (status != napi_ok) {
        return nullptr;
    }

    uint32_t length = 0;
    napi_value propNames = GetPropertyNames(env, args[0], &length);
    if (propNames == nullptr) {
        return nullptr;
    }

    napi_value keys;
    status = napi_create_array(env, &keys);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create array");
        return nullptr;
    }

    status = FillKeysArray(env, keys, propNames, length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to fill array");
        return nullptr;
    }

    return keys;
}

napi_value ObjectValues(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing object to process");
        return nullptr;
    }

    status = ValidateObject(env, args[0], "Argument must be an object");
    if (status != napi_ok) {
        return nullptr;
    }

    uint32_t length = 0;
    napi_value propNames = GetPropertyNames(env, args[0], &length);
    if (propNames == nullptr) {
        return nullptr;
    }

    napi_value values;
    status = napi_create_array(env, &values);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create array");
        return nullptr;
    }

    status = FillValuesArray(env, values, args[0], propNames, length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to fill array");
        return nullptr;
    }

    return values;
}

napi_value ObjectEntries(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing object to process");
        return nullptr;
    }

    status = ValidateObject(env, args[0], "Argument must be an object");
    if (status != napi_ok) {
        return nullptr;
    }

    uint32_t length = 0;
    napi_value propNames = GetPropertyNames(env, args[0], &length);
    if (propNames == nullptr) {
        return nullptr;
    }

    napi_value entries;
    status = napi_create_array(env, &entries);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create array");
        return nullptr;
    }

    status = FillEntriesArray(env, entries, args[0], propNames, length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to fill array");
        return nullptr;
    }

    return entries;
}

napi_value StringToUpper(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string to convert");
        return nullptr;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
        return nullptr;
    }
    char* str = new char[length + 1];
    status = napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);
    if (status != napi_ok) {
        delete[] str;
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    std::string s = str;
    delete[] str;
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);

    napi_value result;
    status = napi_create_string_utf8(env, s.c_str(), NAPI_AUTO_LENGTH, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    return result;
}

napi_value StringToLower(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string to convert");
        return nullptr;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
        return nullptr;
    }
    char* str = new char[length + 1];
    status = napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);
    if (status != napi_ok) {
        delete[] str;
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    std::string s = str;
    delete[] str;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    napi_value result;
    status = napi_create_string_utf8(env, s.c_str(), NAPI_AUTO_LENGTH, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    return result;
}

napi_value StringTrim(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 1) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string to trim");
        return nullptr;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
        return nullptr;
    }
    char* str = new char[length + 1];
    status = napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);
    if (status != napi_ok) {
        delete[] str;
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    std::string s = str;
    delete[] str;
    size_t start = s.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos) {
        s = "";
    } else {
        size_t end = s.find_last_not_of(" \t\n\r\f\v");
        s = s.substr(start, end - start + 1);
    }

    napi_value result;
    status = napi_create_string_utf8(env, s.c_str(), NAPI_AUTO_LENGTH, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
        return nullptr;
    }
    return result;
}

napi_value StringSplit(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2];
    napi_status status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get callback info");
        return nullptr;
    }

    if (argc < 2) {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string or delimiter");
        return nullptr;
    }

    size_t strLength = 0;
    status = napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLength);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
        return nullptr;
    }
    char* str = new char[strLength + 1];
    status = napi_get_value_string_utf8(env, args[0], str, strLength + 1, nullptr);
    if (status != napi_ok) {
        delete[] str;
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    size_t delimiterLength = 0;
    status = napi_get_value_string_utf8(env, args[1], nullptr, 0, &delimiterLength);
    if (status != napi_ok) {
        delete[] str;
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string length");
        return nullptr;
    }
    char* delimiter = new char[delimiterLength + 1];
    status = napi_get_value_string_utf8(env, args[1], delimiter, delimiterLength + 1, nullptr);
    if (status != napi_ok) {
        delete[] str;
        delete[] delimiter;
        napi_throw_error(env, "NAPI_ERROR", "Failed to get string value");
        return nullptr;
    }

    std::string s = str;
    std::string delim = delimiter;
    delete[] str;
    delete[] delimiter;

    napi_value result;
    status = napi_create_array(env, &result);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to create array");
        return nullptr;
    }
    uint32_t index = 0;

    size_t pos = 0;
    while ((pos = s.find(delim)) != std::string::npos) {
        std::string token = s.substr(0, pos);
        napi_value tokenValue;
        status = napi_create_string_utf8(env, token.c_str(), NAPI_AUTO_LENGTH, &tokenValue);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
            return nullptr;
        }
        status = napi_set_element(env, result, index++, tokenValue);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to set element");
            return nullptr;
        }
        s.erase(0, pos + delim.length());
    }

    if (!s.empty()) {
        napi_value tokenValue;
        status = napi_create_string_utf8(env, s.c_str(), NAPI_AUTO_LENGTH, &tokenValue);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to create string");
            return nullptr;
        }
        status = napi_set_element(env, result, index++, tokenValue);
        if (status != napi_ok) {
            napi_throw_error(env, "NAPI_ERROR", "Failed to set element");
            return nullptr;
        }
    }

    return result;
}

} // namespace

extern "C" {

napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor descriptors[] = {
        { "convertToNumber", nullptr, ConvertToNumber, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "convertToString", nullptr, ConvertToString, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "convertToBoolean", nullptr, ConvertToBoolean, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "validateEmail", nullptr, ValidateEmail, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "validateURL", nullptr, ValidateURL, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "validatePhoneNumber", nullptr, ValidatePhoneNumber, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "validateCreditCard", nullptr, ValidateCreditCard, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "formatNumber", nullptr, FormatNumber, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "formatCurrency", nullptr, FormatCurrency, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "formatDate", nullptr, FormatDate, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "base64Encode", nullptr, Base64Encode, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "base64Decode", nullptr, Base64Decode, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "urlEncode", nullptr, URLEncode, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "urlDecode", nullptr, URLDecode, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "arraySort", nullptr, ArraySort, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "arrayUnique", nullptr, ArrayUnique, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "arraySum", nullptr, ArraySum, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "arrayAverage", nullptr, ArrayAverage, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "objectKeys", nullptr, ObjectKeys, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "objectValues", nullptr, ObjectValues, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "objectEntries", nullptr, ObjectEntries, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "stringToUpper", nullptr, StringToUpper, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "stringToLower", nullptr, StringToLower, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "stringTrim", nullptr, StringTrim, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "stringSplit", nullptr, StringSplit, nullptr, nullptr, nullptr, napi_default, nullptr },
    };

    napi_status status = napi_define_properties(env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
    if (status != napi_ok) {
        napi_throw_error(env, "NAPI_ERROR", "Failed to define properties");
        return nullptr;
    }
    return exports;
}
NAPI_MODULE(native_module_data, Init)

}
