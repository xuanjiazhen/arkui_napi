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

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace {

constexpr size_t kBufferCaseCount = 21;
constexpr size_t kBufferArgCount = 1;
constexpr size_t kFirstCaseNumber = 1;
constexpr int kCaseNumberWidth = 2;
constexpr size_t kNameBufferSize = 32;
constexpr int32_t kSeedStep = 5;
constexpr int32_t kStrideCycle = 4;
constexpr int32_t kMaskOffset = 16;
constexpr uint32_t kModuleVersion = 1;
constexpr uint32_t kNoModuleFlags = 0;

struct BufferCaseSpec {
    std::string name;
    int32_t seed;
    int32_t stride;
    int32_t mask;
};

std::string BuildIndexedName(const char* prefix, size_t caseNumber)
{
    char buffer[kNameBufferSize] = {};
    if (std::snprintf(buffer, sizeof(buffer), "%s%0*zu", prefix, kCaseNumberWidth, caseNumber) < 0) {
        return prefix;
    }
    return buffer;
}

size_t GetCaseIndex(void* data) { return static_cast<size_t>(reinterpret_cast<uintptr_t>(data)); }

BufferCaseSpec GetBufferCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + kFirstCaseNumber;
    return {
        BuildIndexedName("bufferCase", caseNumber),
        static_cast<int32_t>(caseNumber) * kSeedStep,
        static_cast<int32_t>(caseNumber % kStrideCycle) + 1,
        static_cast<int32_t>(caseNumber) + kMaskOffset,
    };
}

bool ReadBuffer(napi_env env, napi_value value, uint8_t** data, size_t* length)
{
    bool isBuffer = false;
    if (napi_is_buffer(env, value, &isBuffer) != napi_ok || !isBuffer) {
        napi_throw_type_error(env, nullptr, "value must be a Buffer");
        return false;
    }

    void* rawData = nullptr;
    if (napi_get_buffer_info(env, value, &rawData, length) != napi_ok) {
        return false;
    }
    *data = static_cast<uint8_t*>(rawData);
    return true;
}

bool CreateModifiedCopy(napi_env env, const uint8_t* source, size_t length, int32_t mask, napi_value* result)
{
    void* rawData = nullptr;
    if (napi_create_buffer(env, length, &rawData, result) != napi_ok) {
        return false;
    }
    auto* target = static_cast<uint8_t*>(rawData);
    for (size_t index = 0; index < length; index++) {
        target[index] = static_cast<uint8_t>(source[index] ^ mask);
    }
    return true;
}

bool SetNamedInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int32(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedString(napi_env env, napi_value object, const char* name, const std::string& value)
{
    napi_value napiValue = nullptr;
    if (napi_create_string_utf8(env, value.c_str(), value.size(), &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

napi_value CreateBufferSummary(napi_env env, const std::string& name, int32_t length, int32_t checksum, napi_value copy)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", name);
    SetNamedInt32(env, result, "length", length);
    SetNamedInt32(env, result, "checksum", checksum);
    NAPI_CALL(env, napi_set_named_property(env, result, "copy", copy));
    return result;
}

static napi_value RunBufferCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = kBufferArgCount;
    napi_value args[kBufferArgCount] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= kBufferCaseCount) {
        napi_throw_error(env, nullptr, "invalid buffer case");
        return nullptr;
    }

    uint8_t* bytes = nullptr;
    size_t length = 0;
    if (argc < kBufferArgCount) {
        napi_throw_type_error(env, nullptr, "buffer is required");
        return nullptr;
    }
    if (!ReadBuffer(env, args[0], &bytes, &length)) {
        return nullptr;
    }

    const auto spec = GetBufferCaseSpec(caseIndex);
    int32_t checksum = 0;
    for (size_t itemIndex = 0; itemIndex < length; itemIndex += static_cast<size_t>(spec.stride)) {
        checksum += static_cast<int32_t>((bytes[itemIndex] + spec.seed) ^ spec.mask);
    }

    napi_value copy = nullptr;
    if (!CreateModifiedCopy(env, bytes, length, spec.mask, &copy)) {
        return nullptr;
    }

    return CreateBufferSummary(env, spec.name, static_cast<int32_t>(length), checksum, copy);
}

}  // namespace

static napi_value InitBufferSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(kBufferCaseCount);
    exportNames.reserve(kBufferCaseCount);
    for (size_t caseIndex = 0; caseIndex < kBufferCaseCount; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testBufferCase", caseIndex + kFirstCaseNumber));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunBufferCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_bufferSuiteModule = {
    .nm_version = kModuleVersion,
    .nm_flags = kNoModuleFlags,
    .nm_filename = nullptr,
    .nm_register_func = InitBufferSuite,
    .nm_modname = "buffer_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterBufferSuiteModule(void)
{
    napi_module_register(&g_bufferSuiteModule);
}
