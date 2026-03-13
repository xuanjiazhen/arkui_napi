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

constexpr size_t kErrorCaseCount = 20;
constexpr size_t kErrorArgCount = 2;
constexpr size_t kFirstCaseNumber = 1;
constexpr int kCaseNumberWidth = 2;
constexpr size_t kNameBufferSize = 32;
constexpr size_t kNullTerminatorSize = 1;
constexpr int32_t kParityMask = 1;
constexpr int32_t kMinimumValueOffset = 18;
constexpr uint32_t kModuleVersion = 1;
constexpr uint32_t kNoModuleFlags = 0;

struct ErrorCaseSpec {
    std::string name;
    int32_t minValue;
    int32_t parity;
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

ErrorCaseSpec GetErrorCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + kFirstCaseNumber;
    return {
        BuildIndexedName("errorCase", caseNumber),
        static_cast<int32_t>(caseNumber) - kMinimumValueOffset,
        static_cast<int32_t>(caseNumber % 2),
    };
}

bool ReadInt32(napi_env env, napi_value value, const char* message, int32_t* result)
{
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, value, &type) != napi_ok) {
        return false;
    }
    if (type != napi_number) {
        napi_throw_type_error(env, nullptr, message);
        return false;
    }
    return napi_get_value_int32(env, value, result) == napi_ok;
}

bool ReadUtf8(napi_env env, napi_value value, const char* message, std::string* result)
{
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, value, &type) != napi_ok) {
        return false;
    }
    if (type != napi_string) {
        napi_throw_type_error(env, nullptr, message);
        return false;
    }

    size_t length = 0;
    if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok) {
        return false;
    }
    std::string buffer(length + kNullTerminatorSize, '\0');
    if (napi_get_value_string_utf8(env, value, buffer.data(), buffer.size(), &length) != napi_ok) {
        return false;
    }
    buffer.resize(length);
    *result = buffer;
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

napi_value CreateErrorSummary(napi_env env, const std::string& name, int32_t value, const std::string& text)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", name);
    SetNamedInt32(env, result, "value", value);
    SetNamedString(env, result, "text", text);
    return result;
}

static napi_value RunErrorCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = kErrorArgCount;
    napi_value args[kErrorArgCount] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= kErrorCaseCount) {
        napi_throw_error(env, nullptr, "invalid error case");
        return nullptr;
    }

    int32_t value = 0;
    std::string text;
    if (argc < kErrorArgCount) {
        napi_throw_type_error(env, nullptr, "value and text are required");
        return nullptr;
    }
    if (!ReadInt32(env, args[0], "value must be a number", &value)) {
        return nullptr;
    }
    if (!ReadUtf8(env, args[1], "text must be a string", &text)) {
        return nullptr;
    }

    const auto spec = GetErrorCaseSpec(caseIndex);
    if (value < spec.minValue) {
        napi_throw_range_error(env, nullptr, spec.name.c_str());
        return nullptr;
    }
    if ((value & kParityMask) != spec.parity) {
        napi_throw_error(env, nullptr, "parity check failed");
        return nullptr;
    }
    if (text.empty()) {
        napi_throw_error(env, nullptr, "text must not be empty");
        return nullptr;
    }

    return CreateErrorSummary(env, spec.name, value, text);
}

}  // namespace

static napi_value InitErrorSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(kErrorCaseCount);
    exportNames.reserve(kErrorCaseCount);
    for (size_t caseIndex = 0; caseIndex < kErrorCaseCount; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testErrorCase", caseIndex + kFirstCaseNumber));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunErrorCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_errorSuiteModule = {
    .nm_version = kModuleVersion,
    .nm_flags = kNoModuleFlags,
    .nm_filename = nullptr,
    .nm_register_func = InitErrorSuite,
    .nm_modname = "error_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterErrorSuiteModule(void)
{
    napi_module_register(&g_errorSuiteModule);
}
