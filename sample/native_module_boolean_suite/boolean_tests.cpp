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

#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace {

constexpr size_t kBooleanCaseCount = 20;
constexpr size_t kBooleanArgCount = 3;
constexpr size_t kFirstCaseNumber = 1;
constexpr int kCaseNumberWidth = 2;
constexpr size_t kNameBufferSize = 32;
constexpr int32_t kFirstBoolWeight = 4;
constexpr int32_t kSecondBoolWeight = 2;
constexpr int32_t kThirdBoolWeight = 1;
constexpr int32_t kPassThreshold = 3;
constexpr uint32_t kModuleVersion = 1;
constexpr uint32_t kNoModuleFlags = 0;

constexpr std::array<const char*, 4> kRouteNames = {
    "beta",
    "gamma",
    "delta",
    "alpha",
};

struct BooleanCaseSpec {
    std::string name;
    bool invertFirst;
    bool invertSecond;
    bool invertThird;
    const char* route;
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

BooleanCaseSpec GetBooleanCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + kFirstCaseNumber;
    return {
        BuildIndexedName("booleanCase", caseNumber),
        caseIndex % 2 == 0,
        caseIndex % 3 == 2,
        caseIndex % kRouteNames.size() == kRouteNames.size() - 1,
        kRouteNames[caseIndex % kRouteNames.size()],
    };
}

bool ReadBool(napi_env env, napi_value value, const char* message, bool* result)
{
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, value, &type) != napi_ok) {
        return false;
    }
    if (type != napi_boolean) {
        napi_throw_type_error(env, nullptr, message);
        return false;
    }
    return napi_get_value_bool(env, value, result) == napi_ok;
}

bool SetNamedInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int32(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedBool(napi_env env, napi_value object, const char* name, bool value)
{
    napi_value napiValue = nullptr;
    if (napi_get_boolean(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedString(napi_env env, napi_value object, const char* name, const char* value)
{
    napi_value napiValue = nullptr;
    if (napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

napi_value CreateBooleanSummary(napi_env env, const char* name, const char* route, int32_t score, bool passed)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", name);
    SetNamedString(env, result, "route", route);
    SetNamedInt32(env, result, "score", score);
    SetNamedBool(env, result, "passed", passed);
    return result;
}

static napi_value RunBooleanCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = kBooleanArgCount;
    napi_value args[kBooleanArgCount] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= kBooleanCaseCount) {
        napi_throw_error(env, nullptr, "invalid boolean case");
        return nullptr;
    }

    bool first = false;
    bool second = false;
    bool third = false;
    if (argc < kBooleanArgCount) {
        napi_throw_type_error(env, nullptr, "three boolean values are required");
        return nullptr;
    }
    if (!ReadBool(env, args[0], "first must be a boolean", &first)) {
        return nullptr;
    }
    if (!ReadBool(env, args[1], "second must be a boolean", &second)) {
        return nullptr;
    }
    if (!ReadBool(env, args[2], "third must be a boolean", &third)) {
        return nullptr;
    }

    const auto spec = GetBooleanCaseSpec(caseIndex);
    const bool normalizedFirst = spec.invertFirst ? !first : first;
    const bool normalizedSecond = spec.invertSecond ? !second : second;
    const bool normalizedThird = spec.invertThird ? !third : third;
    const int32_t score = static_cast<int32_t>(normalizedFirst) * kFirstBoolWeight +
                          static_cast<int32_t>(normalizedSecond) * kSecondBoolWeight +
                          static_cast<int32_t>(normalizedThird) * kThirdBoolWeight;

    return CreateBooleanSummary(env, spec.name.c_str(), spec.route, score, score >= kPassThreshold);
}

}  // namespace

static napi_value InitBooleanSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(kBooleanCaseCount);
    exportNames.reserve(kBooleanCaseCount);
    for (size_t caseIndex = 0; caseIndex < kBooleanCaseCount; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testBooleanCase", caseIndex + kFirstCaseNumber));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunBooleanCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_booleanSuiteModule = {
    .nm_version = kModuleVersion,
    .nm_flags = kNoModuleFlags,
    .nm_filename = nullptr,
    .nm_register_func = InitBooleanSuite,
    .nm_modname = "boolean_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterBooleanSuiteModule(void)
{
    napi_module_register(&g_booleanSuiteModule);
}
