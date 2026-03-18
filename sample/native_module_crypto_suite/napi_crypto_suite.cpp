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
static constexpr int BASE64_CHAR_COUNT = 64;
static constexpr int BASE64_CHUNK_SIZE = 4;
static constexpr int BASE64_BYTE_COUNT = 3;
static constexpr int BASE64_BYTE_BITS = 8;
static constexpr int BASE64_CHAR_BITS = 6;
static constexpr int SHA256_CHUNK_BYTES = 64;
static constexpr int SHA256_CHUNK_WORDS = 16;
static constexpr int SHA256_ROUNDS = 64;
static constexpr int SHA256_WORD_BYTES = 4;
static constexpr int SHA256_WORD_BITS = 32;
static constexpr int HEX_WIDTH = 8;
static constexpr int UUID_LENGTH = 36;
static constexpr int UUID_POSITION_1 = 8;
static constexpr int UUID_POSITION_2 = 13;
static constexpr int UUID_POSITION_3 = 18;
static constexpr int UUID_POSITION_4 = 23;
static constexpr int UUID_MOD_BASE = 16;
static constexpr int UUID_MULTIPLIER = 13;
static constexpr int DEFAULT_SALT_LENGTH = 16;
static constexpr int MAX_SALT_LENGTH = 32;
static constexpr int SALT_MULTIPLIER = 17;
static constexpr int RAND_MULTIPLIER = 37;
static constexpr int EMAIL_MAX_LENGTH = 255;
static constexpr int CHUNK_ALIGNMENT_BITS = 512;
static constexpr int INITIAL_PADDING = 0x80;
static constexpr int ZERO_PADDING = 0x00;
static constexpr int SHA256_CONSTANT_0 = 0x428a2f98;
static constexpr int ROTATION_BITS_2 = 2;
static constexpr int ROTATION_BITS_6 = 6;
static constexpr int ROTATION_BITS_7 = 7;
static constexpr int ROTATION_BITS_10 = 10;
static constexpr int ROTATION_BITS_11 = 11;
static constexpr int ROTATION_BITS_13 = 13;
static constexpr int ROTATION_BITS_17 = 17;
static constexpr int ROTATION_BITS_18 = 18;
static constexpr int ROTATION_BITS_19 = 19;
static constexpr int ROTATION_BITS_22 = 22;
static constexpr int ROTATION_BITS_25 = 25;
static constexpr int VECTOR_SIZE = 8;
static constexpr int BUFFER_SIZE = 50;

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
    const char* base64Chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+";
    
    size_t i = 0;
    size_t len = input.length();
    
    while (i < len) {
        uint32_t chunk = 0;
        int chunkSize = 0;
        
        for (int j = 0; j < BASE64_BYTE_COUNT && i + j < len; ++j) {
            chunk = (chunk << BASE64_BYTE_BITS) + static_cast<unsigned char>(input[i + j]);
            chunkSize++;
        }

        chunk <<= (BASE64_BYTE_COUNT - chunkSize) * BASE64_BYTE_BITS;

        for (int j = 0; j < BASE64_CHUNK_SIZE; ++j) {
            if (j * BASE64_CHAR_BITS < (chunkSize + 1) * BASE64_BYTE_BITS) {
                result += base64Chars[(chunk >> (BASE64_BYTE_BITS * 3 - j * BASE64_CHAR_BITS)) & 0x3F];
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
    const char* base64Chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+";
    
    std::vector<int> charToVal(256, -1);
    for (int i = 0; i < BASE64_CHAR_COUNT; ++i) {
        charToVal[static_cast<unsigned char>(base64Chars[i])] = i;
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
        
        for (int j = 0; j < BASE64_CHUNK_SIZE && i + j < len; ++j) {
            char c = input[i + j];
            int val = charToVal[static_cast<unsigned char>(c)];
            if (val != -1) {
                chunk = (chunk << BASE64_CHAR_BITS) + val;
                validChars++;
            }
        }

        if (validChars == BASE64_CHUNK_SIZE) {
            for (int j = 0; j < BASE64_BYTE_COUNT && j < (validChars - padding); ++j) {
                result += static_cast<char>((chunk >> (BASE64_BYTE_BITS * 2 - j * BASE64_BYTE_BITS)) & 0xFF);
            }
        }

        i += BASE64_CHUNK_SIZE;
    }

    return CreateString(env, result);
}

static std::string CalculateSHA256Internal(const std::string& input)
{
    uint32_t h0 = 0x6a09e667;
    uint32_t h1 = 0xbb67ae85;
    uint32_t h2 = 0x3c6ef32;
    uint32_t h3 = 0xa54ff53a;
    uint32_t h4 = 0x51527f;
    uint32_t h5 = 0x9b05688c;
    uint32_t h6 = 0x1f83d9ab;
    uint32_t h7 = 0x5be0cd19;

    std::string paddedInput = PreparePadding(input);

    for (size_t chunkStart = 0; chunkStart < paddedInput.length(); chunkStart += SHA256_CHUNK_BYTES) {
        uint32_t w[SHA256_ROUNDS];
        InitializeWorkArray(w, paddedInput, chunkStart);
        ExtendWorkArray(w);

        for (size_t i = 0; i < SHA256_ROUNDS; ++i) {
            ProcessCompressionRound(h0, h1, h2, h3, h4, h5, h6, h7, w, i);
        }
    }

    return FormatHashOutput(h0, h1, h2, h3, h4, h5, h6, h7);
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

    std::string hashOutput = CalculateSHA256Internal(input);

    return CreateString(env, hashOutput);
}

static uint32_t Rotr(uint32_t x, uint32_t n)
{
    return (x >> n) | (x << (SHA256_WORD_BITS - n));
}

static std::string PreparePadding(const std::string& input)
{
    uint64_t bitLength = input.length() * BASE64_BYTE_BITS;
    std::string paddedInput = input + INITIAL_PADDING;

    while ((paddedInput.length() * BASE64_BYTE_BITS + SHA256_CHUNK_BYTES) % CHUNK_ALIGNMENT_BITS != 0) {
        paddedInput += ZERO_PADDING;
    }

    for (size_t i = 0; i < SHA256_WORD_BYTES; ++i) {
        paddedInput += static_cast<char>((bitLength >> (BASE64_BYTE_BITS * SHA256_WORD_BYTES - i * BASE64_BYTE_BITS)) & 0xFF);
    }

    return paddedInput;
}

static void InitializeWorkArray(uint32_t w[], const std::string& paddedInput, size_t chunkStart)
{
    for (size_t i = 0; i < SHA256_CHUNK_WORDS; ++i) {
        uint32_t byte0 = static_cast<unsigned char>(paddedInput[chunkStart + i * SHA256_WORD_BYTES + 0]) << 24;
        uint32_t byte1 = static_cast<unsigned char>(paddedInput[chunkStart + i * SHA256_WORD_BYTES + 1]) << 16;
        uint32_t byte2 = static_cast<unsigned char>(paddedInput[chunkStart + i * SHA256_WORD_BYTES + 2]) << 8;
        uint32_t byte3 = static_cast<unsigned char>(paddedInput[chunkStart + i * SHA256_WORD_BYTES + 3]);
        w[i] = byte0 | byte1 | byte2 | byte3;
    }
}

static void ExtendWorkArray(uint32_t w[])
{
    for (size_t i = SHA256_CHUNK_WORDS; i < SHA256_ROUNDS; ++i) {
        w[i] = (Rotr(w[i - 2], ROTATION_BITS_17) ^ Rotr(w[i - 2], ROTATION_BITS_19) ^ (w[i - 2] >> ROTATION_BITS_10)) +
               w[i - 7] + (Rotr(w[i - 15], ROTATION_BITS_7) ^ Rotr(w[i - 15], ROTATION_BITS_18) ^ (w[i - 15] >> ROTATION_BITS_3)) +
               w[i - 16];
    }
}

static void ProcessCompressionRound(uint32_t& h0, uint32_t& h1, uint32_t& h2, uint32_t& h3,
                               uint32_t& h4, uint32_t& h5, uint32_t& h6, uint32_t& h7,
                               const uint32_t w[], size_t roundIndex)
{
    uint32_t a = h0;
    uint32_t b = h1;
    uint32_t c = h2;
    uint32_t d = h3;
    uint32_t e = h4;
    uint32_t f = h5;
    uint32_t g = h6;
    uint32_t h = h7;

    uint32_t temp1 = h + (Rotr(e, ROTATION_BITS_6) ^ Rotr(e, ROTATION_BITS_11) ^ Rotr(e, ROTATION_BITS_25)) +
                       ((e & f) ^ (~e & g)) + SHA256_CONSTANT_0 + w[roundIndex];
    uint32_t temp2 = (Rotr(a, ROTATION_BITS_2) ^ Rotr(a, ROTATION_BITS_13) ^ Rotr(a, ROTATION_BITS_22)) +
                       ((a & b) ^ (a & c) ^ (b & c));

    h = g;
    g = f;
    f = e;
    e = d + temp1;
    d = c;
    c = b;
    b = a;
    a = temp1 + temp2;

    h0 += a;
    h1 += b;
    h2 += c;
    h3 += d;
    h4 += e;
    h5 += f;
    h6 += g;
    h7 += h;
}

static std::string FormatHashOutput(uint32_t h0, uint32_t h1, uint32_t h2, uint32_t h3,
                              uint32_t h4, uint32_t h5, uint32_t h6, uint32_t h7)
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << 
          std::setw(HEX_WIDTH) << h0 << std::setw(HEX_WIDTH) << h1 << 
          std::setw(HEX_WIDTH) << h2 << std::setw(HEX_WIDTH) << h3 << 
          std::setw(HEX_WIDTH) << h4 << std::setw(HEX_WIDTH) << h5 << 
          std::setw(HEX_WIDTH) << h6 << std::setw(HEX_WIDTH) << h7;
    return ss.str();
}

static napi_value GenerateRandomString(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: length, charset");

    int32_t length = 0;
    NAPI_CALL(env, napi_get_value_int32(env, argv[ARG_INDEX_ZERO], &length));
    NAPI_ASSERT(env, length > 0 && length <= MAX_INPUT_LENGTH, "Length must be between 1 and 100");

    std::string charset = ExtractString(env, argv[ARG_INDEX_ONE]);
    if (charset.empty()) {
        charset = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    }

    std::string result;
    for (int32_t i = 0; i < length; ++i) {
        size_t index = (i * RAND_MULTIPLIER) % charset.length();
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

    for (int i = 0; i < UUID_LENGTH; ++i) {
        if (i == UUID_POSITION_1 || i == UUID_POSITION_2 || i == UUID_POSITION_3 || i == UUID_POSITION_4) {
            ss << '-';
        } else {
            ss << hexChars[(i * UUID_MULTIPLIER) % UUID_MOD_BASE];
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

    int32_t length = DEFAULT_SALT_LENGTH;
    if (argc >= REQUIRED_ARGS_ONE) {
        NAPI_CALL(env, napi_get_value_int32(env, argv[ARG_INDEX_ZERO], &length));
        NAPI_ASSERT(env, length > 0 && length <= MAX_SALT_LENGTH, "Length must be between 1 and 32");
    }

    std::string hexChars = "0123456789abcdef";
    std::string salt;

    for (int32_t i = 0; i < length; ++i) {
        salt += hexChars[(i * SALT_MULTIPLIER) % UUID_MOD_BASE];
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
    std::string hashOutput = CalculateSHA256Internal(combined);

    return CreateString(env, hashOutput);
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
