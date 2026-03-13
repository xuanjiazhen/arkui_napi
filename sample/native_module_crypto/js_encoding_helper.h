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

#ifndef FOUNDATION_ACE_NAPI_TEST_NATIVE_MODULE_CRYPTO_JS_ENCODING_HELPER_H
#define FOUNDATION_ACE_NAPI_TEST_NATIVE_MODULE_CRYPTO_JS_ENCODING_HELPER_H

#include "napi/native_api.h"
#include "napi/native_node_api.h"

#include <cstdint>
#include <string>
#include <vector>

namespace EncodingBufferSize {
    constexpr size_t INPUT_BUFFER_SIZE = 2048;
    constexpr size_t OUTPUT_BUFFER_SIZE = 4096;
};

class Base64Helper {
public:
    static std::string Encode(const std::string& input);
    static std::string Decode(const std::string& input);
    static std::string EncodeBytes(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> DecodeToBytes(const std::string& input);

private:
    static const char ENCODE_TABLE[];
    static const int DECODE_TABLE[];
    static constexpr char PADDING = '=';
};

class HexHelper {
public:
    static std::string Encode(const std::string& input);
    static std::string Decode(const std::string& input);
    static std::string EncodeBytes(const std::vector<uint8_t>& input);
    static std::vector<uint8_t> DecodeToBytes(const std::string& input);
    static bool IsValidHex(const std::string& input);

private:
    static const char HEX_CHARS[];
    static int CharToNibble(char c);
};

class HashHelper {
public:
    static uint32_t SimpleHash(const std::string& input);
    static uint32_t DJB2Hash(const std::string& input);
    static uint32_t FNV1aHash(const std::string& input);
    static uint32_t CRC32(const std::string& input);
    static std::string HashToHex(uint32_t hash);
};

class ChecksumHelper {
public:
    static uint8_t XORChecksum(const std::string& input);
    static uint16_t Fletcher16(const std::string& input);
    static uint32_t Adler32(const std::string& input);
};

class ROT13Helper {
public:
    static std::string Transform(const std::string& input);
};

class URLEncodingHelper {
public:
    static std::string Encode(const std::string& input);
    static std::string Decode(const std::string& input);

private:
    static bool ShouldEncode(char c);
};

#endif
