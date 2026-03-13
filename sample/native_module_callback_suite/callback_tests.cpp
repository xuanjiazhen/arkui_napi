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

constexpr size_t kCallbackCaseCount = 20;
constexpr size_t kCallbackArgCount = 2;
constexpr size_t kCallbackInvokeArgCount = 3;
constexpr size_t kFirstCaseNumber = 1;
constexpr int kCaseNumberWidth = 2;
constexpr size_t kNameBufferSize = 32;
constexpr int32_t kRightMultiplier = 2;
constexpr int32_t kCallsCycle = 4;
constexpr uint32_t kModuleVersion = 1;
constexpr uint32_t kNoModuleFlags = 0;

struct CallbackCaseSpec {
    std::string name;
    int32_t left;
    int32_t right;
    int32_t calls;
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

CallbackCaseSpec GetCallbackCaseSpec(size_t caseIndex)
{
    const size_t caseNumber = caseIndex + kFirstCaseNumber;
    return {
        BuildIndexedName("callbackCase", caseNumber),
        static_cast<int32_t>(caseNumber),
        static_cast<int32_t>(caseNumber) * kRightMultiplier,
        static_cast<int32_t>(caseNumber % kCallsCycle) + 1,
    };
}

bool ReadFunction(napi_env env, napi_value value)
{
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, value, &type) != napi_ok) {
        return false;
    }
    if (type != napi_function) {
        napi_throw_type_error(env, nullptr, "callback must be a function");
        return false;
    }
    return true;
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

bool CallFunction(napi_env env, napi_value callback, size_t argc, napi_value* argv, napi_value* result)
{
    napi_value undefined = nullptr;
    if (napi_get_undefined(env, &undefined) != napi_ok) {
        return false;
    }
    return napi_call_function(env, undefined, callback, argc, argv, result) == napi_ok;
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

napi_value CreateCallbackSummary(napi_env env, const std::string& name, int32_t calls, napi_value outputs)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", name);
    SetNamedInt32(env, result, "calls", calls);
    NAPI_CALL(env, napi_set_named_property(env, result, "outputs", outputs));
    return result;
}

static napi_value RunCallbackCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = kCallbackArgCount;
    napi_value args[kCallbackArgCount] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= kCallbackCaseCount) {
        napi_throw_error(env, nullptr, "invalid callback case");
        return nullptr;
    }

    int32_t input = 0;
    if (argc < kCallbackArgCount) {
        napi_throw_type_error(env, nullptr, "callback and input are required");
        return nullptr;
    }
    if (!ReadFunction(env, args[0])) {
        return nullptr;
    }
    if (!ReadInt32(env, args[1], "input must be a number", &input)) {
        return nullptr;
    }

    const auto spec = GetCallbackCaseSpec(caseIndex);
    napi_value outputs = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, static_cast<size_t>(spec.calls), &outputs));
    for (int32_t callIndex = 0; callIndex < spec.calls; callIndex++) {
        napi_value callbackArgs[kCallbackInvokeArgCount] = {nullptr};
        napi_value callbackResult = nullptr;
        NAPI_CALL(env, napi_create_int32(env, input + spec.left + callIndex, &callbackArgs[0]));
        NAPI_CALL(env, napi_create_int32(env, spec.right + callIndex, &callbackArgs[1]));
        NAPI_CALL(env, napi_create_int32(env, callIndex, &callbackArgs[2]));
        if (!CallFunction(env, args[0], kCallbackInvokeArgCount, callbackArgs, &callbackResult)) {
            return nullptr;
        }
        NAPI_CALL(env, napi_set_element(env, outputs, static_cast<uint32_t>(callIndex), callbackResult));
    }

    return CreateCallbackSummary(env, spec.name, spec.calls, outputs);
}

}  // namespace

static napi_value InitCallbackSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(kCallbackCaseCount);
    exportNames.reserve(kCallbackCaseCount);
    for (size_t caseIndex = 0; caseIndex < kCallbackCaseCount; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testCallbackCase", caseIndex + kFirstCaseNumber));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunCallbackCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_callbackSuiteModule = {
    .nm_version = kModuleVersion,
    .nm_flags = kNoModuleFlags,
    .nm_filename = nullptr,
    .nm_register_func = InitCallbackSuite,
    .nm_modname = "callback_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterCallbackSuiteModule(void)
{
    napi_module_register(&g_callbackSuiteModule);
}
