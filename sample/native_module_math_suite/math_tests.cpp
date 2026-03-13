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

constexpr size_t kMathCaseCount = 18;
constexpr size_t kMathArgCount = 2;
constexpr size_t kFirstCaseNumber = 1;
constexpr int kCaseNumberWidth = 2;
constexpr size_t kNameBufferSize = 32;
constexpr int32_t kBiasStep = 1;
constexpr int32_t kMultiplierCycle = 5;
constexpr int32_t kShiftStep = 2;
constexpr int32_t kShiftOffset = 11;
constexpr int32_t kLoopCycle = 4;
constexpr int32_t kLoopOffset = 2;
constexpr uint32_t kModuleVersion = 1;
constexpr uint32_t kNoModuleFlags = 0;

struct MathCaseSpec {
    std::string name;
    int32_t bias;
    int32_t multiplier;
    int32_t shift;
    int32_t loop;
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

MathCaseSpec GetMathCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + kFirstCaseNumber;
    return {
        BuildIndexedName("mathCase", caseNumber),
        static_cast<int32_t>(caseNumber) * kBiasStep,
        static_cast<int32_t>(caseNumber % kMultiplierCycle) + 1,
        static_cast<int32_t>(caseNumber) * kShiftStep - kShiftOffset,
        static_cast<int32_t>(caseNumber % kLoopCycle) + kLoopOffset,
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

bool SetNamedString(napi_env env, napi_value object, const char* name, const std::string& value)
{
    napi_value napiValue = nullptr;
    if (napi_create_string_utf8(env, value.c_str(), value.size(), &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

napi_value CreateMathSummary(
    napi_env env, const std::string& name, int32_t left, int32_t right, int32_t actual, int32_t expected)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", name);
    SetNamedInt32(env, result, "left", left);
    SetNamedInt32(env, result, "right", right);
    SetNamedInt32(env, result, "actual", actual);
    SetNamedInt32(env, result, "expected", expected);
    SetNamedBool(env, result, "passed", actual == expected);
    return result;
}

static napi_value RunMathCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = kMathArgCount;
    napi_value args[kMathArgCount] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= kMathCaseCount) {
        napi_throw_error(env, nullptr, "invalid math case");
        return nullptr;
    }

    int32_t left = 0;
    int32_t right = 0;
    if (argc < kMathArgCount) {
        napi_throw_type_error(env, nullptr, "left and right are required");
        return nullptr;
    }
    if (!ReadInt32(env, args[0], "left must be a number", &left)) {
        return nullptr;
    }
    if (!ReadInt32(env, args[1], "right must be a number", &right)) {
        return nullptr;
    }

    const auto spec = GetMathCaseSpec(caseIndex);
    int32_t actual = left;
    for (int32_t step = 0; step < spec.loop; step++) {
        actual += spec.bias + step;
    }
    actual = actual * spec.multiplier + right + spec.shift;

    int32_t expected = left;
    for (int32_t step = 0; step < spec.loop; step++) {
        expected += spec.bias + step;
    }
    expected = expected * spec.multiplier + right + spec.shift;

    return CreateMathSummary(env, spec.name, left, right, actual, expected);
}

}  // namespace

static napi_value InitMathSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(kMathCaseCount);
    exportNames.reserve(kMathCaseCount);
    for (size_t caseIndex = 0; caseIndex < kMathCaseCount; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testMathCase", caseIndex + kFirstCaseNumber));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunMathCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_mathSuiteModule = {
    .nm_version = kModuleVersion,
    .nm_flags = kNoModuleFlags,
    .nm_filename = nullptr,
    .nm_register_func = InitMathSuite,
    .nm_modname = "math_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterMathSuiteModule(void) { napi_module_register(&g_mathSuiteModule); }
