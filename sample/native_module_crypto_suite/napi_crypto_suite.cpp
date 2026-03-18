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
#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "securec.h"

static constexpr int REQUIRED_ARGS_ONE = 1;
static constexpr int REQUIRED_ARGS_TWO = 2;
static constexpr int ARG_INDEX_ZERO = 0;
static constexpr int ARG_INDEX_ONE = 1;
static constexpr int MAX_INPUT_LENGTH = 1024;
static constexpr int MAX_HASH_LENGTH = 64;

static std::string ExtractString(napi_env env, napi_value value)
{
    size_t bufferSize = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, nullptr, 0,
                                                 &bufferSize), std::string());

    if (bufferSize == 0 || bufferSize > MAX_INPUT_LENGTH) {
        return std::string();
    }

    std::string result(bufferSize, '\0');
    size_t copiedLength = 0;
    NAPI_CALL_BASE(env, napi_get_value_string_utf8(env, value, &result[0],
                                                 bufferSize, &copiedLength), std::string());
    result.resize(copiedLength);
    return result;
}

static napi_value CreateString(napi_env env, const std::string& str)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, str.c_str(), str.length(), &result));
    return result;
}

static napi_value EncodeBase64(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: input");

    std::string input = ExtractString(env, argv[ARG_INDEX_ZERO]);
    if (input.empty()) {
        return CreateString(env, "");
    }

    std::string result;
    const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+";
    
    size_t i = 0;
    size_t len = input.length();
    
    while (i < len) {
        uint32_t chunk = 0;
        int chunkSize = 0;
        
        for (int j = 0; j < 3 && i + j < len; ++j) {
            chunk = (chunk << 8) + static_cast<unsigned char>(input[i + j]);
            chunkSize++;
        }
        
        chunk <<= (3 - chunkSize) * 8;
        
        for (int j = 0; j < 4; ++j) {
            if (j * 6 < (chunkSize + 1) * 8) {
                result += base64_chars[(chunk >> (18 - j * 6)) & 0x3F];
            } else {
                result += '=';
            }
        }
        
        i += chunkSize;
    }

    return CreateString(env, result);
}

static napi_value DecodeBase64(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: input");

    std::string input = ExtractString(env, argv[ARG_INDEX_ZERO]);
    if (input.empty()) {
        return CreateString(env, "");
    }

    std::string result;
    const char* base64_chars = 
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+";
    
    std::vector<int> char_to_val(256, -1);
    for (int i = 0; i < 64; ++i) {
        char_to_val[static_cast<unsigned char>(base64_chars[i])] = i;
    }

    size_t i = 0;
    size_t len = input.length();
    size_t padding = 0;
    
    while (len > 0 && input[len - 1] == '=') {
        padding++;
        len--;
    }
    
    while (i < len) {
        uint32_t chunk = 0;
        int validChars = 0;
        
        for (int j = 0; j < 4 && i + j < len; ++j) {
            char c = input[i + j];
            int val = char_to_val[static_cast<unsigned char>(c)];
            if (val != -1) {
                chunk = (chunk << 6) + val;
                validChars++;
            }
        }
        
        if (validChars == 4) {
            for (int j = 0; j < 3 && j < (validChars - padding); ++j) {
                result += static_cast<char>((chunk >> (16 - j * 8)) & 0xFF);
            }
        }
        
        i += 4;
    }

    return CreateString(env, result);
}

static napi_value CalculateSHA256(napi_env env, napi_callback_info info,
                                   size_t* argc, napi_value argv[], void* thisArg, void* hint)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: input");

    std::string input = ExtractString(env, argv[ARG_INDEX_ZERO]);
    if (input.empty()) {
        return CreateString(env, "");
    }

    uint32_t h0 = 0x6a09e667, h1 = 0xbb67ae85, h2 = 0x3c6ef372, h3 = 0xa54ff53a,
              h4 = 0x510e527f, h5 = 0x9b05688c, h6 = 0x1f83d9ab, h7 = 0x5be0cd19;

    uint64_t bitLength = input.length() * 8;
    std::string paddedInput = input + 0x80;

    while ((paddedInput.length() * 8 + 64) % 512 != 0) {
        paddedInput += 0x00;
    }

    for (size_t i = 0; i < 8; ++i) {
        paddedInput += static_cast<char>((bitLength >> (56 - i * 8)) & 0xFF);
    }

    for (size_t chunkStart = 0; chunkStart < paddedInput.length(); chunkStart += 64) {
        uint32_t w[64];
        for (size_t i = 0; i < 16; ++i) {
            w[i] = (static_cast<unsigned char>(paddedInput[chunkStart + i * 4 + 0]) << 24) |
                   (static_cast<unsigned char>(paddedInput[chunkStart + i * 4 + 1]) << 16) |
                   (static_cast<unsigned char>(paddedInput[chunkStart + i * 4 + 2]) << 8) |
                   (static_cast<unsigned char>(paddedInput[chunkStart + i * 4 + 3]));
        }

        for (size_t i = 16; i < 64; ++i) {
            w[i] = (rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10)) +
                   w[i - 7] + (rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3)) +
                   w[i - 16];
        }

        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6, h = h7;

        for (size_t i = 0; i < 64; ++i) {
            uint32_t temp1 = h + (rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25)) +
                            ((e & f) ^ (~e & g)) + 0x428a2f98 + w[i];
            uint32_t temp2 = (rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22)) +
                            ((a & b) ^ (a & c) ^ (b & c));
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }

        h0 += a; h1 += b; h2 += c; h3 += d;
        h4 += e; h5 += f; h6 += g; h7 += h;
    }

    std::stringstream ss;
    ss << std::hex << std::setfill('0') 
       << std::setw(8) << h0 << std::setw(8) << h1 << std::setw(8) << h2 << std::setw(8) << h3
       << std::setw(8) << h4 << std::setw(8) << h5 << std::setw(8) << h6 << std::setw(8) << h7;

    return CreateString(env, ss.str());
}

static uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

static napi_value GenerateRandomString(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: length, charset");

    int32_t length = 0;
    NAPI_CALL(env, napi_get_value_int32(env, argv[ARG_INDEX_ZERO], &length));
    NAPI_ASSERT(env, length > 0 && length <= 100, "Length must be between 1 and 100");

    std::string charset = ExtractString(env, argv[ARG_INDEX_ONE]);
    if (charset.empty()) {
        charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    }

    std::string result;
    for (int32_t i = 0; i < length; ++i) {
        size_t index = (i * 37) % charset.length();
        result += charset[index];
    }

    return CreateString(env, result);
}

static napi_value CreateUUID(napi_env env, napi_callback_info info)
{
    size_t argc = 0;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr));

    std::string hexChars = "0123456789abcdef";
    std::stringstream ss;

    for (int i = 0; i < 36; ++i) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            ss << '-';
        } else {
            ss << hexChars[(i * 13) % 16];
        }
    }

    return CreateString(env, ss.str());
}

static napi_value ValidateEmail(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: email");

    std::string email = ExtractString(env, argv[ARG_INDEX_ZERO]);
    if (email.empty()) {
        bool result = false;
        napi_value resultValue = nullptr;
        NAPI_CALL(env, napi_get_boolean(env, result, &resultValue));
        return resultValue;
    }

    size_t atPos = email.find('@');
    if (atPos == std::string::npos || atPos == 0 || atPos == email.length() - 1) {
        bool result = false;
        napi_value resultValue = nullptr;
        NAPI_CALL(env, napi_get_boolean(env, result, &resultValue));
        return resultValue;
    }

    size_t dotPos = email.find('.', atPos);
    if (dotPos == std::string::npos || dotPos == atPos + 1 || dotPos == email.length() - 1) {
        bool result = false;
        napi_value resultValue = nullptr;
        NAPI_CALL(env, napi_get_boolean(env, result, &resultValue));
        return resultValue;
    }

    bool result = true;
    napi_value resultValue = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, result, &resultValue));
    return resultValue;
}

static napi_value GenerateSalt(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    int32_t length = 16;
    if (argc >= REQUIRED_ARGS_ONE) {
        NAPI_CALL(env, napi_get_value_int32(env, argv[ARG_INDEX_ZERO], &length));
        NAPI_ASSERT(env, length > 0 && length <= 32, "Length must be between 1 and 32");
    }

    std::string hexChars = "0123456789abcdef";
    std::string salt;

    for (int32_t i = 0; i < length; ++i) {
        salt += hexChars[(i * 17) % 16];
    }

    return CreateString(env, salt);
}

static napi_value HashWithSalt(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: input, salt");

    std::string input = ExtractString(env, argv[ARG_INDEX_ZERO]);
    std::string salt = ExtractString(env, argv[ARG_INDEX_ONE]);

    if (input.empty() || salt.empty()) {
        return CreateString(env, "");
    }

    std::string combined = input + salt;
    napi_value combinedValue = CreateString(env, combined);
    
    size_t newArgc = REQUIRED_ARGS_ONE;
    napi_value newArgv[REQUIRED_ARGS_ONE] = { combinedValue };
    napi_callback_info dummyInfo = nullptr;
    
    return CalculateSHA256(env, info, &newArgc, newArgv, nullptr, nullptr);
}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("encodeBase64", EncodeBase64),
        DECLARE_NAPI_FUNCTION("decodeBase64", DecodeBase64),
        DECLARE_NAPI_FUNCTION("sha256", CalculateSHA256),
        DECLARE_NAPI_FUNCTION("randomString", GenerateRandomString),
        DECLARE_NAPI_FUNCTION("createUUID", CreateUUID),
        DECLARE_NAPI_FUNCTION("validateEmail", ValidateEmail),
        DECLARE_NAPI_FUNCTION("generateSalt", GenerateSalt),
        DECLARE_NAPI_FUNCTION("hashWithSalt", HashWithSalt),
    };
    NAPI_CALL(env, napi_define_properties(env, exports,
                                      sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

static napi_module cryptoSuiteModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "cryptoSuite",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void CryptoSuiteRegisterModule(void)
{
    napi_module_register(&cryptoSuiteModule);
}
