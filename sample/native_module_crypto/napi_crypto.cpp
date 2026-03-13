// Copyright (c) 2023 Huawei Device Co., Ltd.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include "native_api.h"

namespace {

// 哈希函数实现（简化版）
std::string md5(const std::string& input) {
    // 简化的MD5实现，实际项目中应使用标准库
    unsigned int hash = 5381;
    for (char c : input) {
        hash = ((hash << 5) + hash) + c;
    }
    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << hash;
    return ss.str();
}

std::string sha1(const std::string& input) {
    // 简化的SHA1实现
    unsigned int hash = 0x67452301;
    for (char c : input) {
        hash = ((hash << 5) - hash) + c;
    }
    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << hash;
    return ss.str();
}

std::string sha256(const std::string& input) {
    // 简化的SHA256实现
    unsigned long long hash = 0xcbf29ce484222325;
    for (char c : input) {
        hash = (hash ^ (c << 24)) * 0x100000001b3;
    }
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash;
    return ss.str();
}

std::string sha512(const std::string& input) {
    // 简化的SHA512实现
    unsigned long long hash1 = 0x6a09e667f3bcc908;
    unsigned long long hash2 = 0xbb67ae8584caa73b;
    for (char c : input) {
        hash1 = (hash1 ^ (c << 32)) * 0x100000001b3;
        hash2 = (hash2 ^ (c << 16)) * 0x100000001b3;
    }
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << hash1 << hash2;
    return ss.str();
}

// Base64编码解码
const std::string base64_chars = 
             "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
             "abcdefghijklmnopqrstuvwxyz"
             "0123456789+/";

bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

std::string base64_encode(const std::string& input) {
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];
    const char* bytes_to_encode = input.c_str();
    size_t in_len = input.length();
    
    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;
            
            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }
    
    if (i) {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';
        
        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;
        
        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];
        
        while((i++ < 3))
            ret += '=';
    }
    
    return ret;
}

std::string base64_decode(const std::string& input) {
    std::string ret;
    int i = 0;
    int j = 0;
    int in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    
    while (input.length() > in_ && input[in_] != '=') {
        if (!is_base64(input[in_])) {
            return "";
        }
        char_array_4[i++] = input[in_]; in_++;
        if (i ==4) {
            for (i = 0; i <4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
            
            for (i = 0; (i < 3); i++)
                ret += char_array_3[i];
            i = 0;
        }
    }
    
    if (i) {
        for (j = i; j <4; j++)
            char_array_4[j] = 0;
        
        for (j = 0; j <4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);
        
        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
        
        for (j = 0; (j < i - 1); j++) ret += char_array_3[j];
    }
    
    return ret;
}

// URL编码解码
std::string url_encode(const std::string& input) {
    std::stringstream ss;
    ss << std::hex << std::uppercase;
    for (char c : input) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            ss << c;
        } else {
            ss << '%' << std::setw(2) << std::setfill('0') << static_cast<int>(static_cast<unsigned char>(c));
        }
    }
    return ss.str();
}

std::string url_decode(const std::string& input) {
    std::string ret;
    for (size_t i = 0; i < input.length();) {
        if (input[i] == '%') {
            if (i + 2 < input.length()) {
                int value = 0;
                std::stringstream ss;
                ss << std::hex << input.substr(i + 1, 2);
                ss >> value;
                ret += static_cast<char>(value);
                i += 3;
            } else {
                ret += input[i];
                i++;
            }
        } else {
            ret += input[i];
            i++;
        }
    }
    return ret;
}

// 随机数生成
std::string random_bytes(size_t length) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 255);
    std::string result;
    for (size_t i = 0; i < length; i++) {
        result += static_cast<char>(dis(gen));
    }
    return result;
}

int random_int(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

// HMAC实现
std::string hmac(const std::string& message, const std::string& key, const std::string& (*hash_func)(const std::string&)) {
    std::string padded_key = key;
    if (padded_key.length() > 64) {
        padded_key = hash_func(padded_key);
    }
    if (padded_key.length() < 64) {
        padded_key.append(64 - padded_key.length(), '\0');
    }
    
    std::string ipad(64, '\x36');
    std::string opad(64, '\x5c');
    
    for (size_t i = 0; i < 64; i++) {
        ipad[i] ^= padded_key[i];
        opad[i] ^= padded_key[i];
    }
    
    std::string hash1 = hash_func(ipad + message);
    return hash_func(opad + hash1);
}

std::string hmac_md5(const std::string& message, const std::string& key) {
    return hmac(message, key, md5);
}

std::string hmac_sha1(const std::string& message, const std::string& key) {
    return hmac(message, key, sha1);
}

std::string hmac_sha256(const std::string& message, const std::string& key) {
    return hmac(message, key, sha256);
}

// 密码生成和验证
std::string generate_salt(size_t length) {
    const std::string chars = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()_+";
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, chars.length() - 1);
    std::string salt;
    for (size_t i = 0; i < length; i++) {
        salt += chars[dis(gen)];
    }
    return salt;
}

std::string hash_password(const std::string& password, const std::string& salt) {
    return sha256(password + salt);
}

bool verify_password(const std::string& password, const std::string& salt, const std::string& hash) {
    return hash_password(password, salt) == hash;
}

// UUID生成
std::string generate_uuid() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    const char* hex_chars = "0123456789abcdef";
    std::string uuid;
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            uuid += '-';
        } else if (i == 14) {
            uuid += '4';
        } else if (i == 19) {
            uuid += hex_chars[dis(gen) & 0x3 | 0x8];
        } else {
            uuid += hex_chars[dis(gen)];
        }
    }
    return uuid;
}

bool validate_uuid(const std::string& uuid) {
    if (uuid.length() != 36) return false;
    for (int i = 0; i < 36; i++) {
        if (i == 8 || i == 13 || i == 18 || i == 23) {
            if (uuid[i] != '-') return false;
        } else {
            if (!isxdigit(uuid[i])) return false;
        }
    }
    return true;
}

// 安全比较
bool secure_compare(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) return false;
    bool result = true;
    for (size_t i = 0; i < a.length(); i++) {
        result &= (a[i] == b[i]);
    }
    return result;
}

// 时间戳签名
std::string sign_timestamp(long long timestamp, const std::string& secret) {
    std::string message = std::to_string(timestamp);
    return hmac_sha256(message, secret);
}

bool verify_timestamp(long long timestamp, const std::string& signature, const std::string& secret, int tolerance) {
    std::string expected = sign_timestamp(timestamp, secret);
    if (!secure_compare(expected, signature)) return false;
    // 检查时间戳是否在容忍范围内
    long long now = std::chrono::system_clock::now().time_since_epoch().count() / 1000;
    return std::abs(now - timestamp) <= tolerance;
}

// 数据完整性
std::string generate_checksum(const std::string& data) {
    return md5(data);
}

bool verify_checksum(const std::string& data, const std::string& checksum) {
    return generate_checksum(data) == checksum;
}

// 简单的加密解密（用于演示）
std::string encrypt(const std::string& data, const std::string& key) {
    std::string result;
    for (size_t i = 0; i < data.length(); i++) {
        result += data[i] ^ key[i % key.length()];
    }
    return base64_encode(result);
}

std::string decrypt(const std::string& data, const std::string& key) {
    std::string decoded = base64_decode(data);
    std::string result;
    for (size_t i = 0; i < decoded.length(); i++) {
        result += decoded[i] ^ key[i % key.length()];
    }
    return result;
}

// 密钥管理
std::string generate_key(size_t length) {
    return random_bytes(length);
}

std::string derive_key(const std::string& password, const std::string& salt, size_t iterations) {
    std::string key = password + salt;
    for (size_t i = 0; i < iterations; i++) {
        key = sha256(key);
    }
    return key;
}

// 证书相关（简化版）
std::string generate_certificate(const std::string& subject, const std::string& issuer) {
    std::string cert = "-----BEGIN CERTIFICATE-----\n";
    cert += "Subject: " + subject + "\n";
    cert += "Issuer: " + issuer + "\n";
    cert += "Valid From: " + std::to_string(std::chrono::system_clock::now().time_since_epoch().count() / 1000) + "\n";
    cert += "Valid Until: " + std::to_string((std::chrono::system_clock::now() + std::chrono::hours(24 * 365)).time_since_epoch().count() / 1000) + "\n";
    cert += "-----END CERTIFICATE-----";
    return cert;
}

bool validate_certificate(const std::string& certificate) {
    // 简化的证书验证
    return certificate.find("-----BEGIN CERTIFICATE-----") != std::string::npos &&
           certificate.find("-----END CERTIFICATE-----") != std::string::npos;
}

} // namespace

// MD5哈希
static napi_value MD5(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected input string");
        return nullptr;
    }
    
    size_t inputLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &inputLength);
    char* inputBuffer = new char[inputLength + 1];
    napi_get_value_string_utf8(env, args[0], inputBuffer, inputLength + 1, nullptr);
    std::string input(inputBuffer);
    delete[] inputBuffer;
    
    std::string result = md5(input);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// SHA1哈希
static napi_value SHA1(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected input string");
        return nullptr;
    }
    
    size_t inputLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &inputLength);
    char* inputBuffer = new char[inputLength + 1];
    napi_get_value_string_utf8(env, args[0], inputBuffer, inputLength + 1, nullptr);
    std::string input(inputBuffer);
    delete[] inputBuffer;
    
    std::string result = sha1(input);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// SHA256哈希
static napi_value SHA256(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected input string");
        return nullptr;
    }
    
    size_t inputLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &inputLength);
    char* inputBuffer = new char[inputLength + 1];
    napi_get_value_string_utf8(env, args[0], inputBuffer, inputLength + 1, nullptr);
    std::string input(inputBuffer);
    delete[] inputBuffer;
    
    std::string result = sha256(input);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// SHA512哈希
static napi_value SHA512(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected input string");
        return nullptr;
    }
    
    size_t inputLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &inputLength);
    char* inputBuffer = new char[inputLength + 1];
    napi_get_value_string_utf8(env, args[0], inputBuffer, inputLength + 1, nullptr);
    std::string input(inputBuffer);
    delete[] inputBuffer;
    
    std::string result = sha512(input);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// AES加密（简化版）
static napi_value EncryptAES(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected plaintext and key");
        return nullptr;
    }
    
    size_t plaintextLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &plaintextLength);
    char* plaintextBuffer = new char[plaintextLength + 1];
    napi_get_value_string_utf8(env, args[0], plaintextBuffer, plaintextLength + 1, nullptr);
    std::string plaintext(plaintextBuffer);
    delete[] plaintextBuffer;
    
    size_t keyLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &keyLength);
    char* keyBuffer = new char[keyLength + 1];
    napi_get_value_string_utf8(env, args[1], keyBuffer, keyLength + 1, nullptr);
    std::string key(keyBuffer);
    delete[] keyBuffer;
    
    std::string result = encrypt(plaintext, key);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// AES解密（简化版）
static napi_value DecryptAES(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected ciphertext and key");
        return nullptr;
    }
    
    size_t ciphertextLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &ciphertextLength);
    char* ciphertextBuffer = new char[ciphertextLength + 1];
    napi_get_value_string_utf8(env, args[0], ciphertextBuffer, ciphertextLength + 1, nullptr);
    std::string ciphertext(ciphertextBuffer);
    delete[] ciphertextBuffer;
    
    size_t keyLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &keyLength);
    char* keyBuffer = new char[keyLength + 1];
    napi_get_value_string_utf8(env, args[1], keyBuffer, keyLength + 1, nullptr);
    std::string key(keyBuffer);
    delete[] keyBuffer;
    
    std::string result = decrypt(ciphertext, key);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// DES加密（简化版）
static napi_value EncryptDES(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected plaintext and key");
        return nullptr;
    }
    
    size_t plaintextLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &plaintextLength);
    char* plaintextBuffer = new char[plaintextLength + 1];
    napi_get_value_string_utf8(env, args[0], plaintextBuffer, plaintextLength + 1, nullptr);
    std::string plaintext(plaintextBuffer);
    delete[] plaintextBuffer;
    
    size_t keyLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &keyLength);
    char* keyBuffer = new char[keyLength + 1];
    napi_get_value_string_utf8(env, args[1], keyBuffer, keyLength + 1, nullptr);
    std::string key(keyBuffer);
    delete[] keyBuffer;
    
    std::string result = encrypt(plaintext, key);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// DES解密（简化版）
static napi_value DecryptDES(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected ciphertext and key");
        return nullptr;
    }
    
    size_t ciphertextLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &ciphertextLength);
    char* ciphertextBuffer = new char[ciphertextLength + 1];
    napi_get_value_string_utf8(env, args[0], ciphertextBuffer, ciphertextLength + 1, nullptr);
    std::string ciphertext(ciphertextBuffer);
    delete[] ciphertextBuffer;
    
    size_t keyLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &keyLength);
    char* keyBuffer = new char[keyLength + 1];
    napi_get_value_string_utf8(env, args[1], keyBuffer, keyLength + 1, nullptr);
    std::string key(keyBuffer);
    delete[] keyBuffer;
    
    std::string result = decrypt(ciphertext, key);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// Base64编码
static napi_value Base64Encode(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected input string");
        return nullptr;
    }
    
    size_t inputLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &inputLength);
    char* inputBuffer = new char[inputLength + 1];
    napi_get_value_string_utf8(env, args[0], inputBuffer, inputLength + 1, nullptr);
    std::string input(inputBuffer);
    delete[] inputBuffer;
    
    std::string result = base64_encode(input);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// Base64解码
static napi_value Base64Decode(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected input string");
        return nullptr;
    }
    
    size_t inputLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &inputLength);
    char* inputBuffer = new char[inputLength + 1];
    napi_get_value_string_utf8(env, args[0], inputBuffer, inputLength + 1, nullptr);
    std::string input(inputBuffer);
    delete[] inputBuffer;
    
    std::string result = base64_decode(input);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// URL编码
static napi_value UrlEncode(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected input string");
        return nullptr;
    }
    
    size_t inputLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &inputLength);
    char* inputBuffer = new char[inputLength + 1];
    napi_get_value_string_utf8(env, args[0], inputBuffer, inputLength + 1, nullptr);
    std::string input(inputBuffer);
    delete[] inputBuffer;
    
    std::string result = url_encode(input);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// URL解码
static napi_value UrlDecode(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected input string");
        return nullptr;
    }
    
    size_t inputLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &inputLength);
    char* inputBuffer = new char[inputLength + 1];
    napi_get_value_string_utf8(env, args[0], inputBuffer, inputLength + 1, nullptr);
    std::string input(inputBuffer);
    delete[] inputBuffer;
    
    std::string result = url_decode(input);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 随机字节
static napi_value RandomBytes(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected length");
        return nullptr;
    }
    
    size_t length;
    napi_get_value_uint32(env, args[0], &length);
    
    std::string result = random_bytes(length);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 随机整数
static napi_value RandomInt(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected min and max");
        return nullptr;
    }
    
    int min, max;
    napi_get_value_int32(env, args[0], &min);
    napi_get_value_int32(env, args[1], &max);
    
    int result = random_int(min, max);
    napi_value resultValue;
    napi_create_int32(env, result, &resultValue);
    return resultValue;
}

// HMAC MD5
static napi_value HmacMD5(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected message and key");
        return nullptr;
    }
    
    size_t messageLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &messageLength);
    char* messageBuffer = new char[messageLength + 1];
    napi_get_value_string_utf8(env, args[0], messageBuffer, messageLength + 1, nullptr);
    std::string message(messageBuffer);
    delete[] messageBuffer;
    
    size_t keyLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &keyLength);
    char* keyBuffer = new char[keyLength + 1];
    napi_get_value_string_utf8(env, args[1], keyBuffer, keyLength + 1, nullptr);
    std::string key(keyBuffer);
    delete[] keyBuffer;
    
    std::string result = hmac_md5(message, key);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// HMAC SHA1
static napi_value HmacSHA1(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected message and key");
        return nullptr;
    }
    
    size_t messageLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &messageLength);
    char* messageBuffer = new char[messageLength + 1];
    napi_get_value_string_utf8(env, args[0], messageBuffer, messageLength + 1, nullptr);
    std::string message(messageBuffer);
    delete[] messageBuffer;
    
    size_t keyLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &keyLength);
    char* keyBuffer = new char[keyLength + 1];
    napi_get_value_string_utf8(env, args[1], keyBuffer, keyLength + 1, nullptr);
    std::string key(keyBuffer);
    delete[] keyBuffer;
    
    std::string result = hmac_sha1(message, key);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// HMAC SHA256
static napi_value HmacSHA256(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected message and key");
        return nullptr;
    }
    
    size_t messageLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &messageLength);
    char* messageBuffer = new char[messageLength + 1];
    napi_get_value_string_utf8(env, args[0], messageBuffer, messageLength + 1, nullptr);
    std::string message(messageBuffer);
    delete[] messageBuffer;
    
    size_t keyLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &keyLength);
    char* keyBuffer = new char[keyLength + 1];
    napi_get_value_string_utf8(env, args[1], keyBuffer, keyLength + 1, nullptr);
    std::string key(keyBuffer);
    delete[] keyBuffer;
    
    std::string result = hmac_sha256(message, key);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 生成盐值
static napi_value GenerateSalt(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected length");
        return nullptr;
    }
    
    size_t length;
    napi_get_value_uint32(env, args[0], &length);
    
    std::string result = generate_salt(length);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 哈希密码
static napi_value HashPassword(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected password and salt");
        return nullptr;
    }
    
    size_t passwordLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &passwordLength);
    char* passwordBuffer = new char[passwordLength + 1];
    napi_get_value_string_utf8(env, args[0], passwordBuffer, passwordLength + 1, nullptr);
    std::string password(passwordBuffer);
    delete[] passwordBuffer;
    
    size_t saltLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &saltLength);
    char* saltBuffer = new char[saltLength + 1];
    napi_get_value_string_utf8(env, args[1], saltBuffer, saltLength + 1, nullptr);
    std::string salt(saltBuffer);
    delete[] saltBuffer;
    
    std::string result = hash_password(password, salt);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 验证密码
static napi_value VerifyPassword(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 3) {
        napi_throw_error(env, "Invalid arguments", "Expected password, salt and hash");
        return nullptr;
    }
    
    size_t passwordLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &passwordLength);
    char* passwordBuffer = new char[passwordLength + 1];
    napi_get_value_string_utf8(env, args[0], passwordBuffer, passwordLength + 1, nullptr);
    std::string password(passwordBuffer);
    delete[] passwordBuffer;
    
    size_t saltLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &saltLength);
    char* saltBuffer = new char[saltLength + 1];
    napi_get_value_string_utf8(env, args[1], saltBuffer, saltLength + 1, nullptr);
    std::string salt(saltBuffer);
    delete[] saltBuffer;
    
    size_t hashLength;
    napi_get_value_string_utf8(env, args[2], nullptr, 0, &hashLength);
    char* hashBuffer = new char[hashLength + 1];
    napi_get_value_string_utf8(env, args[2], hashBuffer, hashLength + 1, nullptr);
    std::string hash(hashBuffer);
    delete[] hashBuffer;
    
    bool result = verify_password(password, salt, hash);
    napi_value resultValue;
    napi_create_boolean(env, result, &resultValue);
    return resultValue;
}

// 生成UUID
static napi_value GenerateUUID(napi_env env, napi_callback_info info) {
    std::string result = generate_uuid();
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 验证UUID
static napi_value ValidateUUID(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected UUID string");
        return nullptr;
    }
    
    size_t uuidLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &uuidLength);
    char* uuidBuffer = new char[uuidLength + 1];
    napi_get_value_string_utf8(env, args[0], uuidBuffer, uuidLength + 1, nullptr);
    std::string uuid(uuidBuffer);
    delete[] uuidBuffer;
    
    bool result = validate_uuid(uuid);
    napi_value resultValue;
    napi_create_boolean(env, result, &resultValue);
    return resultValue;
}

// 安全比较
static napi_value SecureCompare(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected two strings");
        return nullptr;
    }
    
    size_t str1Length;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &str1Length);
    char* str1Buffer = new char[str1Length + 1];
    napi_get_value_string_utf8(env, args[0], str1Buffer, str1Length + 1, nullptr);
    std::string str1(str1Buffer);
    delete[] str1Buffer;
    
    size_t str2Length;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &str2Length);
    char* str2Buffer = new char[str2Length + 1];
    napi_get_value_string_utf8(env, args[1], str2Buffer, str2Length + 1, nullptr);
    std::string str2(str2Buffer);
    delete[] str2Buffer;
    
    bool result = secure_compare(str1, str2);
    napi_value resultValue;
    napi_create_boolean(env, result, &resultValue);
    return resultValue;
}

// 时间戳签名
static napi_value SignTimestamp(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp and secret");
        return nullptr;
    }
    
    double timestamp;
    napi_get_value_double(env, args[0], &timestamp);
    
    size_t secretLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &secretLength);
    char* secretBuffer = new char[secretLength + 1];
    napi_get_value_string_utf8(env, args[1], secretBuffer, secretLength + 1, nullptr);
    std::string secret(secretBuffer);
    delete[] secretBuffer;
    
    std::string result = sign_timestamp(static_cast<long long>(timestamp), secret);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 验证时间戳签名
static napi_value VerifyTimestamp(napi_env env, napi_callback_info info) {
    size_t argc = 4;
    napi_value args[4];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 4) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp, signature, secret and tolerance");
        return nullptr;
    }
    
    double timestamp;
    napi_get_value_double(env, args[0], &timestamp);
    
    size_t signatureLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &signatureLength);
    char* signatureBuffer = new char[signatureLength + 1];
    napi_get_value_string_utf8(env, args[1], signatureBuffer, signatureLength + 1, nullptr);
    std::string signature(signatureBuffer);
    delete[] signatureBuffer;
    
    size_t secretLength;
    napi_get_value_string_utf8(env, args[2], nullptr, 0, &secretLength);
    char* secretBuffer = new char[secretLength + 1];
    napi_get_value_string_utf8(env, args[2], secretBuffer, secretLength + 1, nullptr);
    std::string secret(secretBuffer);
    delete[] secretBuffer;
    
    int tolerance;
    napi_get_value_int32(env, args[3], &tolerance);
    
    bool result = verify_timestamp(static_cast<long long>(timestamp), signature, secret, tolerance);
    napi_value resultValue;
    napi_create_boolean(env, result, &resultValue);
    return resultValue;
}

// 生成校验和
static napi_value GenerateChecksum(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected data string");
        return nullptr;
    }
    
    size_t dataLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &dataLength);
    char* dataBuffer = new char[dataLength + 1];
    napi_get_value_string_utf8(env, args[0], dataBuffer, dataLength + 1, nullptr);
    std::string data(dataBuffer);
    delete[] dataBuffer;
    
    std::string result = generate_checksum(data);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 验证校验和
static napi_value VerifyChecksum(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected data and checksum");
        return nullptr;
    }
    
    size_t dataLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &dataLength);
    char* dataBuffer = new char[dataLength + 1];
    napi_get_value_string_utf8(env, args[0], dataBuffer, dataLength + 1, nullptr);
    std::string data(dataBuffer);
    delete[] dataBuffer;
    
    size_t checksumLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &checksumLength);
    char* checksumBuffer = new char[checksumLength + 1];
    napi_get_value_string_utf8(env, args[1], checksumBuffer, checksumLength + 1, nullptr);
    std::string checksum(checksumBuffer);
    delete[] checksumBuffer;
    
    bool result = verify_checksum(data, checksum);
    napi_value resultValue;
    napi_create_boolean(env, result, &resultValue);
    return resultValue;
}

// 通用加密
static napi_value Encrypt(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected plaintext and key");
        return nullptr;
    }
    
    size_t plaintextLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &plaintextLength);
    char* plaintextBuffer = new char[plaintextLength + 1];
    napi_get_value_string_utf8(env, args[0], plaintextBuffer, plaintextLength + 1, nullptr);
    std::string plaintext(plaintextBuffer);
    delete[] plaintextBuffer;
    
    size_t keyLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &keyLength);
    char* keyBuffer = new char[keyLength + 1];
    napi_get_value_string_utf8(env, args[1], keyBuffer, keyLength + 1, nullptr);
    std::string key(keyBuffer);
    delete[] keyBuffer;
    
    std::string result = encrypt(plaintext, key);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 通用解密
static napi_value Decrypt(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected ciphertext and key");
        return nullptr;
    }
    
    size_t ciphertextLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &ciphertextLength);
    char* ciphertextBuffer = new char[ciphertextLength + 1];
    napi_get_value_string_utf8(env, args[0], ciphertextBuffer, ciphertextLength + 1, nullptr);
    std::string ciphertext(ciphertextBuffer);
    delete[] ciphertextBuffer;
    
    size_t keyLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &keyLength);
    char* keyBuffer = new char[keyLength + 1];
    napi_get_value_string_utf8(env, args[1], keyBuffer, keyLength + 1, nullptr);
    std::string key(keyBuffer);
    delete[] keyBuffer;
    
    std::string result = decrypt(ciphertext, key);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 生成密钥
static napi_value GenerateKey(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected length");
        return nullptr;
    }
    
    size_t length;
    napi_get_value_uint32(env, args[0], &length);
    
    std::string result = generate_key(length);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 派生密钥
static napi_value DeriveKey(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 3) {
        napi_throw_error(env, "Invalid arguments", "Expected password, salt and iterations");
        return nullptr;
    }
    
    size_t passwordLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &passwordLength);
    char* passwordBuffer = new char[passwordLength + 1];
    napi_get_value_string_utf8(env, args[0], passwordBuffer, passwordLength + 1, nullptr);
    std::string password(passwordBuffer);
    delete[] passwordBuffer;
    
    size_t saltLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &saltLength);
    char* saltBuffer = new char[saltLength + 1];
    napi_get_value_string_utf8(env, args[1], saltBuffer, saltLength + 1, nullptr);
    std::string salt(saltBuffer);
    delete[] saltBuffer;
    
    size_t iterations;
    napi_get_value_uint32(env, args[2], &iterations);
    
    std::string result = derive_key(password, salt, iterations);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 生成证书
static napi_value GenerateCertificate(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected subject and issuer");
        return nullptr;
    }
    
    size_t subjectLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &subjectLength);
    char* subjectBuffer = new char[subjectLength + 1];
    napi_get_value_string_utf8(env, args[0], subjectBuffer, subjectLength + 1, nullptr);
    std::string subject(subjectBuffer);
    delete[] subjectBuffer;
    
    size_t issuerLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &issuerLength);
    char* issuerBuffer = new char[issuerLength + 1];
    napi_get_value_string_utf8(env, args[1], issuerBuffer, issuerLength + 1, nullptr);
    std::string issuer(issuerBuffer);
    delete[] issuerBuffer;
    
    std::string result = generate_certificate(subject, issuer);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 验证证书
static napi_value ValidateCertificate(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected certificate string");
        return nullptr;
    }
    
    size_t certLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &certLength);
    char* certBuffer = new char[certLength + 1];
    napi_get_value_string_utf8(env, args[0], certBuffer, certLength + 1, nullptr);
    std::string cert(certBuffer);
    delete[] certBuffer;
    
    bool result = validate_certificate(cert);
    napi_value resultValue;
    napi_create_boolean(env, result, &resultValue);
    return resultValue;
}

// 模块初始化
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor descriptors[] = {
        {"md5", nullptr, MD5, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sha1", nullptr, SHA1, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sha256", nullptr, SHA256, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sha512", nullptr, SHA512, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"encryptAES", nullptr, EncryptAES, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"decryptAES", nullptr, DecryptAES, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"encryptDES", nullptr, EncryptDES, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"decryptDES", nullptr, DecryptDES, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"base64Encode", nullptr, Base64Encode, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"base64Decode", nullptr, Base64Decode, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"urlEncode", nullptr, UrlEncode, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"urlDecode", nullptr, UrlDecode, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"randomBytes", nullptr, RandomBytes, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"randomInt", nullptr, RandomInt, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"hmacMD5", nullptr, HmacMD5, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"hmacSHA1", nullptr, HmacSHA1, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"hmacSHA256", nullptr, HmacSHA256, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"generateSalt", nullptr, GenerateSalt, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"hashPassword", nullptr, HashPassword, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"verifyPassword", nullptr, VerifyPassword, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"generateUUID", nullptr, GenerateUUID, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"validateUUID", nullptr, ValidateUUID, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"secureCompare", nullptr, SecureCompare, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"signTimestamp", nullptr, SignTimestamp, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"verifyTimestamp", nullptr, VerifyTimestamp, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"generateChecksum", nullptr, GenerateChecksum, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"verifyChecksum", nullptr, VerifyChecksum, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"encrypt", nullptr, Encrypt, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"decrypt", nullptr, Decrypt, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"generateKey", nullptr, GenerateKey, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"deriveKey", nullptr, DeriveKey, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"generateCertificate", nullptr, GenerateCertificate, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"validateCertificate", nullptr, ValidateCertificate, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    
    napi_define_properties(env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
    return exports;
}

NAPI_MODULE(napi_crypto, Init)
