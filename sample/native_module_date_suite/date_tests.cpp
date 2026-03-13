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

constexpr size_t kDateCaseCount = 28;
constexpr size_t kDateArgCount = 1;
constexpr size_t kFirstCaseNumber = 1;
constexpr int kCaseNumberWidth = 2;
constexpr size_t kNameBufferSize = 32;
constexpr int32_t kShiftHourStep = 3;
constexpr int32_t kBucketCycle = 6;
constexpr int32_t kHoursPerDay = 24;
constexpr double kSecondsPerMinute = 60.0;
constexpr double kMinutesPerHour = 60.0;
constexpr double kMillisecondsPerSecond = 1000.0;
constexpr uint32_t kModuleVersion = 1;
constexpr uint32_t kNoModuleFlags = 0;

struct DateCaseSpec {
    std::string name;
    int32_t shiftHours;
    int32_t bucket;
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

DateCaseSpec GetDateCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + kFirstCaseNumber;
    return {
        BuildIndexedName("dateCase", caseNumber),
        static_cast<int32_t>(caseNumber) * kShiftHourStep,
        static_cast<int32_t>(caseNumber % kBucketCycle),
    };
}

bool ReadDate(napi_env env, napi_value value, double* result)
{
    if (napi_get_date_value(env, value, result) != napi_ok) {
        napi_throw_type_error(env, nullptr, "value must be a Date");
        return false;
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

bool SetNamedBool(napi_env env, napi_value object, const char* name, bool value)
{
    napi_value napiValue = nullptr;
    if (napi_get_boolean(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedDouble(napi_env env, napi_value object, const char* name, double value)
{
    napi_value napiValue = nullptr;
    if (napi_create_double(env, value, &napiValue) != napi_ok) {
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

napi_value CreateDateSummary(
    napi_env env, const std::string& name, double originalMs, double shiftedMs, int32_t bucket, bool matchesBucket)
{
    napi_value result = nullptr;
    napi_value shiftedDate = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    NAPI_CALL(env, napi_create_date(env, shiftedMs, &shiftedDate));
    SetNamedString(env, result, "name", name);
    SetNamedDouble(env, result, "originalMs", originalMs);
    SetNamedDouble(env, result, "shiftedMs", shiftedMs);
    SetNamedInt32(env, result, "bucket", bucket);
    SetNamedBool(env, result, "matchesBucket", matchesBucket);
    NAPI_CALL(env, napi_set_named_property(env, result, "shiftedDate", shiftedDate));
    return result;
}

static napi_value RunDateCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = kDateArgCount;
    napi_value args[kDateArgCount] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= kDateCaseCount) {
        napi_throw_error(env, nullptr, "invalid date case");
        return nullptr;
    }

    double originalMs = 0;
    if (argc < kDateArgCount) {
        napi_throw_type_error(env, nullptr, "date is required");
        return nullptr;
    }
    if (!ReadDate(env, args[0], &originalMs)) {
        return nullptr;
    }

    const auto spec = GetDateCaseSpec(caseIndex);
    const double millisecondsPerHour = kMinutesPerHour * kSecondsPerMinute * kMillisecondsPerSecond;
    const double shiftedMs = originalMs + static_cast<double>(spec.shiftHours) * millisecondsPerHour;
    const int32_t bucket = static_cast<int32_t>(shiftedMs / millisecondsPerHour) % kHoursPerDay;

    return CreateDateSummary(env, spec.name, originalMs, shiftedMs, bucket, bucket % kBucketCycle == spec.bucket);
}

}  // namespace

static napi_value InitDateSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(kDateCaseCount);
    exportNames.reserve(kDateCaseCount);
    for (size_t caseIndex = 0; caseIndex < kDateCaseCount; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testDateCase", caseIndex + kFirstCaseNumber));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunDateCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_dateSuiteModule = {
    .nm_version = kModuleVersion,
    .nm_flags = kNoModuleFlags,
    .nm_filename = nullptr,
    .nm_register_func = InitDateSuite,
    .nm_modname = "date_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterDateSuiteModule(void) { napi_module_register(&g_dateSuiteModule); }
