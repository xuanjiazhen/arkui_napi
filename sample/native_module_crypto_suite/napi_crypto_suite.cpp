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
#include <vector>
#include <algorithm>
#include <sstream>

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "securec.h"

static constexpr int REQUIRED_ARGS_ONE = 1;
static constexpr int REQUIRED_ARGS_TWO = 2;
static constexpr int ARG_INDEX_ZERO = 0;
static constexpr int ARG_INDEX_ONE = 1;
static constexpr int MAX_INPUT_LENGTH = 1024;
static constexpr int MAX_SALT_LENGTH = 32;
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

static uint32_t rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

static uint32_t sigma0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

static uint32_t sigma1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

static uint32_t Sigma0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

static uint32_t Sigma1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

static uint32_t Ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

static uint32_t Maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
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
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
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
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    
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

static napi_value CalculateSHA256(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: input");

    std::string input = ExtractString(env, argv[ARG_INDEX_ZERO]);
    if (input.empty()) {
        return CreateString(env, "");
    }

    std::vector<uint32_t> hash(8, 0);
    const uint32_t round_constants[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    uint32_t h0 = 0x6a09e667, h1 = 0xbb67ae85, h2 = 0x3c6ef372, h3 = 0xa54ff53a,
              h4 = 0x510e527f, h5 = 0x9b05688c, h6 = 0x1f83d9ab, h7 = 0x5be0cd19;

    std::string paddedInput = input;
    uint64_t bitLength = input.length() * 8;
    
    paddedInput.push_back(0x80);
    while ((paddedInput.length() * 8 + 64) % 512 != 0) {
        paddedInput.push_back(0x00);
    }

    for (size_t i = 0; i < 8; ++i) {
        paddedInput.push_back((bitLength >> (56 - i * 8)) & 0xFF);
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
            w[i] = sigma1(w[i - 2]) + w[i - 7] + sigma0(w[i - 15]) + w[i - 16];
        }

        uint32_t a = h0, b = h1, c = h2, d = h3, e = h4, f = h5, g = h6, h = h7;

        for (size_t i = 0; i < 64; ++i) {
            uint32_t temp1 = h + Sigma1(e) + Ch(e, f, g) + round_constants[i] + w[i];
            uint32_t temp2 = Sigma0(a) + Maj(a, b, c);
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

static napi_value GenerateMD5(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: input");

    std::string input = ExtractString(env, argv[ARG_INDEX_ZERO]);
    if (input.empty()) {
        return CreateString(env, "");
    }

    std::vector<uint32_t> buffer(16, 0);
    uint32_t a = 0x67452301, b = 0xefcdab89, c = 0x98badcfe, d = 0x10325476;
    uint32_t aa = 0, bb = 0, cc = 0, dd = 0;

    size_t i = 0;
    while (i < input.length()) {
        for (size_t j = 0; j < 16 && i + j < input.length(); ++j) {
            buffer[j] = (static_cast<unsigned char>(input[i + j]) << 24) |
                        (static_cast<unsigned char>(input[i + j]) << 16) |
                        (static_cast<unsigned char>(input[i + j]) << 8) |
                        (static_cast<unsigned char>(input[i + j]));
        }
        i += 16;
    }

    uint32_t F = (d & b) | (~d & c);
    uint32_t G = (c & d) | (~c & b);
    uint32_t H = (b & d) | (~b & c);
    uint32_t I = b;

    for (size_t k = 0; k < 64; ++k) {
        uint32_t shift = 0;
        uint32_t temp = 0;

        switch (k) {
            case 0: case 4: case 8: case 12: case 16: case 20: case 24: case 28:
            case 32: case 36: case 40: case 44: case 48: case 52: case 56: case 60:
                shift = (k % 16) * 4;
                temp = F + a + buffer[k % 16] + 0xd76aa478 + (k < 16 ? 7 : k < 32 ? 12 : k < 48 ? 17 : 22);
                break;
            default:
                shift = ((k % 4) * 4) + ((k / 4) % 4);
                temp = G + a + buffer[k % 16] + 0xe8c7b756 + (k < 16 ? 5 : k < 32 ? 9 : k < 48 ? 14 : 20);
                break;
        }

        temp = (temp << shift) | (temp >> (32 - shift));
        d = c;
        c = b;
        b = a;
        a = temp;
    }

    aa += a; bb += b; cc += c; dd += d;

    std::stringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << aa << std::setw(8) << bb << std::setw(8) << cc << std::setw(8) << dd;

    return CreateString(env, ss.str());
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
        NAPI_ASSERT(env, length > 0 && length <= MAX_SALT_LENGTH, "Length must be between 1 and 32");
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
    return CalculateSHA256(env, [](napi_env env, napi_callback_info info) -> napi_value {
        napi_value result = nullptr;
        NAPI_CALL_BASE(env, napi_get_cb_info(env, info, nullptr, nullptr, nullptr, nullptr), nullptr);
        return result;
    });
}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("encodeBase64", EncodeBase64),
        DECLARE_NAPI_FUNCTION("decodeBase64", DecodeBase64),
        DECLARE_NAPI_FUNCTION("sha256", CalculateSHA256),
        DECLARE_NAPI_FUNCTION("md5", GenerateMD5),
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
