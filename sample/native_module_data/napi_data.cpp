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

namespace {

// Data conversion functions
napi_value ConvertToNumber(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing value to convert");
        return nullptr;
    }

    napi_value result;
    napi_valuetype type;
    napi_typeof(env, args[0], &type);

    switch (type) {
        case napi_string: {
            size_t length = 0;
            char* str = nullptr;
            napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
            str = new char[length + 1];
            napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);
            double value = strtod(str, nullptr);
            delete[] str;
            napi_create_number(env, value, &result);
            break;
        }
        case napi_boolean: {
            bool value = false;
            napi_get_value_bool(env, args[0], &value);
            napi_create_number(env, value ? 1 : 0, &result);
            break;
        }
        case napi_number:
            result = args[0];
            break;
        default:
            napi_create_number(env, 0, &result);
            break;
    }

    return result;
}

napi_value ConvertToString(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing value to convert");
        return nullptr;
    }

    napi_value result;
    napi_valuetype type;
    napi_typeof(env, args[0], &type);

    switch (type) {
        case napi_number: {
            double value = 0;
            napi_get_value_double(env, args[0], &value);
            std::stringstream ss;
            ss << value;
            napi_create_string_utf8(env, ss.str().c_str(), NAPI_AUTO_LENGTH, &result);
            break;
        }
        case napi_boolean: {
            bool value = false;
            napi_get_value_bool(env, args[0], &value);
            napi_create_string_utf8(env, value ? "true" : "false", NAPI_AUTO_LENGTH, &result);
            break;
        }
        case napi_string:
            result = args[0];
            break;
        default:
            napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &result);
            break;
    }

    return result;
}

napi_value ConvertToBoolean(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing value to convert");
        return nullptr;
    }

    napi_value result;
    napi_valuetype type;
    napi_typeof(env, args[0], &type);

    switch (type)
    {
        case napi_number:
        {
            double value;
            napi_get_value_double(env, args[0], &value);
            napi_create_boolean(env, value != 0, &result);
        }
        break;
        case napi_string:
        {
            size_t length;
            char* str;
            napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
            str = new char[length + 1];
            napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);
            std::string s = str;
            delete[] str;
            bool value = (s == "true" || s == "1" || s == "yes" || s == "y" || s == "on");
            napi_create_boolean(env, value, &result);
        }
        break;
        case napi_boolean:
            result = args[0];
            break;
        default:
            napi_create_boolean(env, false, &result);
            break;
    }

    return result;
}

// Data validation functions
napi_value ValidateEmail(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing email to validate");
        return nullptr;
    }

    size_t length;
    char* email;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    email = new char[length + 1];
    napi_get_value_string_utf8(env, args[0], email, length + 1, nullptr);

    std::regex emailRegex("^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\\.[a-zA-Z]{2,}$");
    bool valid = std::regex_match(email, emailRegex);
    delete[] email;

    napi_value result;
    napi_create_boolean(env, valid, &result);
    return result;
}

napi_value ValidateURL(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing URL to validate");
        return nullptr;
    }

    size_t length;
    char* url;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    url = new char[length + 1];
    napi_get_value_string_utf8(env, args[0], url, length + 1, nullptr);

    std::regex urlRegex("^(https?:\\/\\/)?([\\da-z.-]+)\\.([a-z.]{2,6})([/\\w .-]*)*\\/?$");
    bool valid = std::regex_match(url, urlRegex);
    delete[] url;

    napi_value result;
    napi_create_boolean(env, valid, &result);
    return result;
}

napi_value ValidatePhoneNumber(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing phone number to validate");
        return nullptr;
    }

    size_t length;
    char* phone;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    phone = new char[length + 1];
    napi_get_value_string_utf8(env, args[0], phone, length + 1, nullptr);

    std::regex phoneRegex("^\\+?[1-9][0-9]{7,14}$");
    bool valid = std::regex_match(phone, phoneRegex);
    delete[] phone;

    napi_value result;
    napi_create_boolean(env, valid, &result);
    return result;
}

napi_value ValidateCreditCard(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing credit card number to validate");
        return nullptr;
    }

    size_t length;
    char* card;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    card = new char[length + 1];
    napi_get_value_string_utf8(env, args[0], card, length + 1, nullptr);

    // Remove spaces and dashes
    std::string cardNumber = card;
    cardNumber.erase(std::remove(cardNumber.begin(), cardNumber.end(), ' '), cardNumber.end());
    cardNumber.erase(std::remove(cardNumber.begin(), cardNumber.end(), '-'), cardNumber.end());
    delete[] card;

    // Luhn algorithm
    int sum = 0;
    bool isEven = false;
    for (int i = cardNumber.length() - 1; i >= 0; i--)
    {
        int digit = cardNumber[i] - '0';
        if (isEven)
        {
            digit *= 2;
            if (digit > 9)
            {
                digit -= 9;
            }
        }
        sum += digit;
        isEven = !isEven;
    }

    bool valid = (sum % 10 == 0) && (cardNumber.length() >= 13) && (cardNumber.length() <= 16);

    napi_value result;
    napi_create_boolean(env, valid, &result);
    return result;
}

// Data formatting functions
napi_value FormatNumber(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing number to format");
        return nullptr;
    }

    double number;
    napi_get_value_double(env, args[0], &number);

    int decimals = 2;
    if (argc > 1)
    {
        napi_get_value_int32(env, args[1], &decimals);
    }

    std::stringstream ss;
    ss.precision(decimals);
    ss << std::fixed << number;

    napi_value result;
    napi_create_string_utf8(env, ss.str().c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

napi_value FormatCurrency(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value args[3];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing amount to format");
        return nullptr;
    }

    double amount;
    napi_get_value_double(env, args[0], &amount);

    std::string currency = "USD";
    if (argc > 1)
    {
        size_t length;
        char* str;
        napi_get_value_string_utf8(env, args[1], nullptr, 0, &length);
        str = new char[length + 1];
        napi_get_value_string_utf8(env, args[1], str, length + 1, nullptr);
        currency = str;
        delete[] str;
    }

    std::string locale = "en-US";
    if (argc > 2)
    {
        size_t length;
        char* str;
        napi_get_value_string_utf8(env, args[2], nullptr, 0, &length);
        str = new char[length + 1];
        napi_get_value_string_utf8(env, args[2], str, length + 1, nullptr);
        locale = str;
        delete[] str;
    }

    std::stringstream ss;
    ss.precision(2);
    ss << std::fixed << amount;

    std::string resultStr = currency + " " + ss.str();

    napi_value result;
    napi_create_string_utf8(env, resultStr.c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

napi_value FormatDate(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    time_t now = time(nullptr);
    if (argc > 0)
    {
        double timestamp;
        napi_get_value_double(env, args[0], &timestamp);
        now = static_cast<time_t>(timestamp);
    }

    std::string format = "%Y-%m-%d %H:%M:%S";
    if (argc > 1)
    {
        size_t length;
        char* str;
        napi_get_value_string_utf8(env, args[1], nullptr, 0, &length);
        str = new char[length + 1];
        napi_get_value_string_utf8(env, args[1], str, length + 1, nullptr);
        format = str;
        delete[] str;
    }

    struct tm* localTime = localtime(&now);
    char buffer[100];
    strftime(buffer, sizeof(buffer), format.c_str(), localTime);

    napi_value result;
    napi_create_string_utf8(env, buffer, NAPI_AUTO_LENGTH, &result);
    return result;
}

// Data encoding functions
napi_value Base64Encode(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string to encode");
        return nullptr;
    }

    size_t length;
    char* str;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    str = new char[length + 1];
    napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);

    const char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (length--)
    {
        char_array_3[i++] = *(str++);
        if (i == 3)
        {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for (i = 0; i < 4; i++)
            {
                result += base64_chars[char_array_4[i]];
            }
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 3; j++)
        {
            char_array_3[j] = '\0';
        }

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);

        for (j = 0; j < i + 1; j++)
        {
            result += base64_chars[char_array_4[j]];
        }

        while (i++ < 3)
        {
            result += '=';
        }
    }

    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

napi_value Base64Decode(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string to decode");
        return nullptr;
    }

    size_t length;
    char* str;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    str = new char[length + 1];
    napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);

    const char* base64_chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];

    while (length-- && (str[in_] != '=') && isalnum(str[in_]) || str[in_] == '+' || str[in_] == '/')
    {
        char_array_4[i++] = str[in_]; in_++;
        if (i == 4)
        {
            for (i = 0; i < 4; i++)
            {
                char_array_4[i] = strchr(base64_chars, char_array_4[i]) - base64_chars;
            }

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++)
            {
                result += char_array_3[i];
            }
            i = 0;
        }
    }

    if (i)
    {
        for (j = i; j < 4; j++)
        {
            char_array_4[j] = 0;
        }

        for (j = 0; j < 4; j++)
        {
            char_array_4[j] = strchr(base64_chars, char_array_4[j]) - base64_chars;
        }

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);

        for (j = 0; j < i - 1; j++)
        {
            result += char_array_3[j];
        }
    }

    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

napi_value URLEncode(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string to encode");
        return nullptr;
    }

    size_t length;
    char* str;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    str = new char[length + 1];
    napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);

    std::string result;
    for (size_t i = 0; i < length; i++)
    {
        unsigned char c = str[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~')
        {
            result += c;
        }
        else if (c == ' ')
        {
            result += '+';
        }
        else
        {
            char hex[4];
            sprintf(hex, "%%%02X", c);
            result += hex;
        }
    }
    delete[] str;

    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

napi_value URLDecode(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string to decode");
        return nullptr;
    }

    size_t length;
    char* str;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    str = new char[length + 1];
    napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);

    std::string result;
    for (size_t i = 0; i < length; i++)
    {
        if (str[i] == '+')
        {
            result += ' ';
        }
        else if (str[i] == '%' && i + 2 < length)
        {
            char hex[3] = {str[i+1], str[i+2], '\0'};
            int value = strtol(hex, nullptr, 16);
            result += static_cast<char>(value);
            i += 2;
        }
        else
        {
            result += str[i];
        }
    }
    delete[] str;

    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// Array utilities
napi_value ArraySort(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing array to sort");
        return nullptr;
    }

    bool isArray;
    napi_is_array(env, args[0], &isArray);
    if (!isArray)
    {
        napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an array");
        return nullptr;
    }

    uint32_t length;
    napi_get_array_length(env, args[0], &length);

    std::vector<double> values;
    for (uint32_t i = 0; i < length; i++)
    {
        napi_value element;
        napi_get_element(env, args[0], i, &element);
        double value;
        napi_get_value_double(env, element, &value);
        values.push_back(value);
    }

    std::sort(values.begin(), values.end());

    napi_value result;
    napi_create_array(env, &result);
    for (uint32_t i = 0; i < length; i++)
    {
        napi_value element;
        napi_create_number(env, values[i], &element);
        napi_set_element(env, result, i, element);
    }

    return result;
}

napi_value ArrayUnique(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing array to process");
        return nullptr;
    }

    bool isArray;
    napi_is_array(env, args[0], &isArray);
    if (!isArray)
    {
        napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an array");
        return nullptr;
    }

    uint32_t length;
    napi_get_array_length(env, args[0], &length);

    std::vector<double> values;
    std::map<double, bool> seen;
    for (uint32_t i = 0; i < length; i++)
    {
        napi_value element;
        napi_get_element(env, args[0], i, &element);
        double value;
        napi_get_value_double(env, element, &value);
        if (!seen[value])
        {
            values.push_back(value);
            seen[value] = true;
        }
    }

    napi_value result;
    napi_create_array(env, &result);
    for (uint32_t i = 0; i < values.size(); i++)
    {
        napi_value element;
        napi_create_number(env, values[i], &element);
        napi_set_element(env, result, i, element);
    }

    return result;
}

napi_value ArraySum(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing array to sum");
        return nullptr;
    }

    bool isArray;
    napi_is_array(env, args[0], &isArray);
    if (!isArray)
    {
        napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an array");
        return nullptr;
    }

    uint32_t length;
    napi_get_array_length(env, args[0], &length);

    double sum = 0;
    for (uint32_t i = 0; i < length; i++)
    {
        napi_value element;
        napi_get_element(env, args[0], i, &element);
        double value;
        napi_get_value_double(env, element, &value);
        sum += value;
    }

    napi_value result;
    napi_create_number(env, sum, &result);
    return result;
}

napi_value ArrayAverage(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing array to average");
        return nullptr;
    }

    bool isArray;
    napi_is_array(env, args[0], &isArray);
    if (!isArray)
    {
        napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an array");
        return nullptr;
    }

    uint32_t length;
    napi_get_array_length(env, args[0], &length);

    if (length == 0)
    {
        napi_value result;
        napi_create_number(env, 0, &result);
        return result;
    }

    double sum = 0;
    for (uint32_t i = 0; i < length; i++)
    {
        napi_value element;
        napi_get_element(env, args[0], i, &element);
        double value;
        napi_get_value_double(env, element, &value);
        sum += value;
    }

    napi_value result;
    napi_create_number(env, sum / length, &result);
    return result;
}

// Object utilities
napi_value ObjectKeys(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing object to process");
        return nullptr;
    }

    bool isObject;
    napi_is_object(env, args[0], &isObject);
    if (!isObject)
    {
        napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an object");
        return nullptr;
    }

    napi_value keys;
    napi_create_array(env, &keys);
    uint32_t index = 0;

    napi_value propNames;
    napi_get_property_names(env, args[0], &propNames);
    uint32_t length;
    napi_get_array_length(env, propNames, &length);

    for (uint32_t i = 0; i < length; i++)
    {
        napi_value key;
        napi_get_element(env, propNames, i, &key);
        napi_set_element(env, keys, index++, key);
    }

    return keys;
}

napi_value ObjectValues(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing object to process");
        return nullptr;
    }

    bool isObject;
    napi_is_object(env, args[0], &isObject);
    if (!isObject)
    {
        napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an object");
        return nullptr;
    }

    napi_value values;
    napi_create_array(env, &values);
    uint32_t index = 0;

    napi_value propNames;
    napi_get_property_names(env, args[0], &propNames);
    uint32_t length;
    napi_get_array_length(env, propNames, &length);

    for (uint32_t i = 0; i < length; i++)
    {
        napi_value key;
        napi_get_element(env, propNames, i, &key);
        napi_value value;
        napi_get_property(env, args[0], key, &value);
        napi_set_element(env, values, index++, value);
    }

    return values;
}

napi_value ObjectEntries(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing object to process");
        return nullptr;
    }

    bool isObject;
    napi_is_object(env, args[0], &isObject);
    if (!isObject)
    {
        napi_throw_error(env, "ERR_INVALID_TYPE", "Argument must be an object");
        return nullptr;
    }

    napi_value entries;
    napi_create_array(env, &entries);
    uint32_t index = 0;

    napi_value propNames;
    napi_get_property_names(env, args[0], &propNames);
    uint32_t length;
    napi_get_array_length(env, propNames, &length);

    for (uint32_t i = 0; i < length; i++)
    {
        napi_value key;
        napi_get_element(env, propNames, i, &key);
        napi_value value;
        napi_get_property(env, args[0], key, &value);
        napi_value entry;
        napi_create_array(env, &entry);
        napi_set_element(env, entry, 0, key);
        napi_set_element(env, entry, 1, value);
        napi_set_element(env, entries, index++, entry);
    }

    return entries;
}

// String utilities
napi_value StringToUpper(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string to convert");
        return nullptr;
    }

    size_t length;
    char* str;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    str = new char[length + 1];
    napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);

    std::string s = str;
    delete[] str;
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);

    napi_value result;
    napi_create_string_utf8(env, s.c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

napi_value StringToLower(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string to convert");
        return nullptr;
    }

    size_t length;
    char* str;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    str = new char[length + 1];
    napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);

    std::string s = str;
    delete[] str;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    napi_value result;
    napi_create_string_utf8(env, s.c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

napi_value StringTrim(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string to trim");
        return nullptr;
    }

    size_t length;
    char* str;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &length);
    str = new char[length + 1];
    napi_get_value_string_utf8(env, args[0], str, length + 1, nullptr);

    std::string s = str;
    delete[] str;
    size_t start = s.find_first_not_of(" \t\n\r\f\v");
    if (start == std::string::npos)
    {
        s = "";
    }
    else
    {
        size_t end = s.find_last_not_of(" \t\n\r\f\v");
        s = s.substr(start, end - start + 1);
    }

    napi_value result;
    napi_create_string_utf8(env, s.c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

napi_value StringSplit(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 2)
    {
        napi_throw_error(env, "ERR_INVALID_ARGS", "Missing string or delimiter");
        return nullptr;
    }

    size_t strLength;
    char* str;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLength);
    str = new char[strLength + 1];
    napi_get_value_string_utf8(env, args[0], str, strLength + 1, nullptr);

    size_t delimiterLength;
    char* delimiter;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &delimiterLength);
    delimiter = new char[delimiterLength + 1];
    napi_get_value_string_utf8(env, args[1], delimiter, delimiterLength + 1, nullptr);

    std::string s = str;
    std::string delim = delimiter;
    delete[] str;
    delete[] delimiter;

    napi_value result;
    napi_create_array(env, &result);
    uint32_t index = 0;

    size_t pos = 0;
    while ((pos = s.find(delim)) != std::string::npos)
    {
        std::string token = s.substr(0, pos);
        napi_value tokenValue;
        napi_create_string_utf8(env, token.c_str(), NAPI_AUTO_LENGTH, &tokenValue);
        napi_set_element(env, result, index++, tokenValue);
        s.erase(0, pos + delim.length());
    }

    if (!s.empty())
    {
        napi_value tokenValue;
        napi_create_string_utf8(env, s.c_str(), NAPI_AUTO_LENGTH, &tokenValue);
        napi_set_element(env, result, index++, tokenValue);
    }

    return result;
}

} // namespace

extern "C" {

napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor descriptors[] = {
        // Conversion functions
        { "convertToNumber", nullptr, ConvertToNumber, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "convertToString", nullptr, ConvertToString, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "convertToBoolean", nullptr, ConvertToBoolean, nullptr, nullptr, nullptr, napi_default, nullptr },
        
        // Validation functions
        { "validateEmail", nullptr, ValidateEmail, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "validateURL", nullptr, ValidateURL, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "validatePhoneNumber", nullptr, ValidatePhoneNumber, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "validateCreditCard", nullptr, ValidateCreditCard, nullptr, nullptr, nullptr, napi_default, nullptr },
        
        // Formatting functions
        { "formatNumber", nullptr, FormatNumber, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "formatCurrency", nullptr, FormatCurrency, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "formatDate", nullptr, FormatDate, nullptr, nullptr, nullptr, napi_default, nullptr },
        
        // Encoding functions
        { "base64Encode", nullptr, Base64Encode, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "base64Decode", nullptr, Base64Decode, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "urlEncode", nullptr, URLEncode, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "urlDecode", nullptr, URLDecode, nullptr, nullptr, nullptr, napi_default, nullptr },
        
        // Array utilities
        { "arraySort", nullptr, ArraySort, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "arrayUnique", nullptr, ArrayUnique, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "arraySum", nullptr, ArraySum, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "arrayAverage", nullptr, ArrayAverage, nullptr, nullptr, nullptr, napi_default, nullptr },
        
        // Object utilities
        { "objectKeys", nullptr, ObjectKeys, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "objectValues", nullptr, ObjectValues, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "objectEntries", nullptr, ObjectEntries, nullptr, nullptr, nullptr, napi_default, nullptr },
        
        // String utilities
        { "stringToUpper", nullptr, StringToUpper, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "stringToLower", nullptr, StringToLower, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "stringTrim", nullptr, StringTrim, nullptr, nullptr, nullptr, napi_default, nullptr },
        { "stringSplit", nullptr, StringSplit, nullptr, nullptr, nullptr, napi_default, nullptr },
    };

    napi_define_properties(env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
    return exports;
}

NAPI_MODULE(native_module_data, Init)

}
