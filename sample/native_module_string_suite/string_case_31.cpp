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

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

#include "string_suite_common.h"

namespace {

constexpr uint32_t K_WINDOW_DIGEST_FACTOR = 239;
constexpr size_t K_STRING_CASE_31_ARG_COUNT = 1;

struct WindowScenarioSpec {
    const char* label;
    const char* prefix;
    const char* suffix;
    size_t window;
};

constexpr std::array<WindowScenarioSpec, 10> K_WINDOW_SCENARIOS = {{
    {"window-01", "north", "gate", 2},
    {"window-02", "south", "ring", 3},
    {"window-03", "east", "field", 4},
    {"window-04", "west", "shore", 5},
    {"window-05", "inner", "stone", 6},
    {"window-06", "outer", "anchor", 3},
    {"window-07", "delta", "signal", 4},
    {"window-08", "omega", "vector", 5},
    {"window-09", "plain", "silver", 2},
    {"window-10", "rapid", "violet", 3},
}};

std::string BuildWindowTrace(const std::string& value, size_t window)
{
    if (value.empty()) {
        return {};
    }

    const size_t normalizedWindow = std::min(window, value.size());
    std::string trace;
    for (size_t index = 0; index + normalizedWindow <= value.size(); index++) {
        if (!trace.empty()) {
            trace.push_back('|');
        }
        trace += value.substr(index, normalizedWindow);
    }
    return trace;
}

uint32_t ComputeWindowDigest(const std::string& value, uint32_t seed)
{
    uint32_t digest = seed;
    for (unsigned char current : value) {
        digest = digest * K_WINDOW_DIGEST_FACTOR + current;
    }
    return digest;
}

napi_value CreateWindowDetail(napi_env env, const WindowScenarioSpec& spec, const std::string& trace, uint32_t digest,
    uint32_t windowCount, bool sourceVisible)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "label", spec.label);
    StringSuite::SetNamedString(env, result, "trace", trace);
    StringSuite::SetNamedUInt32(env, result, "digest", digest);
    StringSuite::SetNamedUInt32(env, result, "windowCount", windowCount);
    StringSuite::SetNamedBool(env, result, "sourceVisible", sourceVisible);
    return result;
}

static napi_value RunStringCase31(napi_env env, napi_callback_info info)
{
    size_t argc = K_STRING_CASE_31_ARG_COUNT;
    napi_value args[K_STRING_CASE_31_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < K_STRING_CASE_31_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "source is required");
        return nullptr;
    }

    std::string source;
    if (!StringSuite::ReadUtf8(env, args[0], "source must be a string", &source)) {
        return nullptr;
    }

    napi_value details = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, K_WINDOW_SCENARIOS.size(), &details));

    uint32_t digestMix = 0;
    uint32_t totalWindowCount = 0;
    uint32_t longestTrace = 0;
    bool allSourceVisible = true;
    for (size_t index = 0; index < K_WINDOW_SCENARIOS.size(); index++) {
        const auto& spec = K_WINDOW_SCENARIOS[index];
        const std::string text = std::string(spec.prefix) + ":" + source + ":" + spec.suffix + ":" + spec.label;
        const std::string trace = BuildWindowTrace(text, spec.window);
        const uint32_t digest = ComputeWindowDigest(trace, static_cast<uint32_t>(index + 81));
        const uint32_t windowCount =
            trace.empty() ? 0U : static_cast<uint32_t>(std::count(trace.begin(), trace.end(), '|') + 1);
        const bool sourceVisible = source.empty() || trace.find(source) != std::string::npos;

        digestMix ^= digest;
        totalWindowCount += windowCount;
        longestTrace = std::max(longestTrace, static_cast<uint32_t>(trace.size()));
        allSourceVisible = allSourceVisible && sourceVisible;

        napi_value item = CreateWindowDetail(env, spec, trace, digest, windowCount, sourceVisible);
        NAPI_CALL(env, napi_set_element(env, details, index, item));
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "name", "stringCase31");
    StringSuite::SetNamedString(env, result, "source", source);
    StringSuite::SetNamedUInt32(env, result, "scenarioCount", static_cast<uint32_t>(K_WINDOW_SCENARIOS.size()));
    StringSuite::SetNamedUInt32(env, result, "totalWindowCount", totalWindowCount);
    StringSuite::SetNamedUInt32(env, result, "longestTrace", longestTrace);
    StringSuite::SetNamedUInt32(env, result, "digestMix", digestMix);
    StringSuite::SetNamedBool(env, result, "allSourceVisible", allSourceVisible);
    StringSuite::SetNamedBool(env, result, "passed",
        allSourceVisible && totalWindowCount >= static_cast<uint32_t>(K_WINDOW_SCENARIOS.size()));
    StringSuite::SetNamedValue(env, result, "details", details);
    return result;
}

}  // namespace

void RegisterStringCase31(std::vector<std::string>* exportNames, std::vector<napi_property_descriptor>* descriptors)
{
    StringSuite::AppendStringDescriptor(exportNames, descriptors, "testStringCase31", RunStringCase31);
}
