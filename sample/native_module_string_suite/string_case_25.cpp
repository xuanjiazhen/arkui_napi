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
#include <string>
#include <vector>

#include "string_suite_common.h"

namespace {

constexpr uint32_t K_GRID_DIGEST_FACTOR = 181;
constexpr size_t K_STRING_CASE_25_ARG_COUNT = 1;

struct AffixScenarioSpec {
    const char* label;
    const char* prefix;
    const char* suffix;
    const char* needle;
};

constexpr std::array<AffixScenarioSpec, 10> K_AFFIX_SCENARIOS = {{
    {"grid-01", "left", "right", "ri"},
    {"grid-02", "front", "back", "ba"},
    {"grid-03", "begin", "finish", "fi"},
    {"grid-04", "inner", "outer", "ou"},
    {"grid-05", "solid", "liquid", "li"},
    {"grid-06", "quick", "steady", "te"},
    {"grid-07", "alpha", "omega", "om"},
    {"grid-08", "amber", "violet", "vi"},
    {"grid-09", "delta", "sigma", "si"},
    {"grid-10", "quiet", "loud", "lo"},
}};

uint32_t ComputeGridDigest(const std::string& value, uint32_t seed)
{
    uint32_t digest = seed;
    for (unsigned char current : value) {
        digest = digest * K_GRID_DIGEST_FACTOR + current;
    }
    return digest;
}

uint32_t CountOccurrences(const std::string& value, const std::string& needle)
{
    if (needle.empty()) {
        return 0;
    }

    uint32_t count = 0;
    size_t position = value.find(needle);
    while (position != std::string::npos) {
        count++;
        position = value.find(needle, position + 1);
    }
    return count;
}

napi_value CreateGridDetail(napi_env env, const AffixScenarioSpec& spec, const std::string& text, int32_t firstNeedle,
    int32_t lastNeedle, uint32_t occurrenceCount, uint32_t digest, bool sourceVisible)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "label", spec.label);
    StringSuite::SetNamedString(env, result, "text", text);
    StringSuite::SetNamedInt32(env, result, "firstNeedle", firstNeedle);
    StringSuite::SetNamedInt32(env, result, "lastNeedle", lastNeedle);
    StringSuite::SetNamedUInt32(env, result, "occurrenceCount", occurrenceCount);
    StringSuite::SetNamedUInt32(env, result, "digest", digest);
    StringSuite::SetNamedBool(env, result, "sourceVisible", sourceVisible);
    return result;
}

static napi_value RunStringCase25(napi_env env, napi_callback_info info)
{
    size_t argc = K_STRING_CASE_25_ARG_COUNT;
    napi_value args[K_STRING_CASE_25_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < K_STRING_CASE_25_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "source is required");
        return nullptr;
    }

    std::string source;
    if (!StringSuite::ReadUtf8(env, args[0], "source must be a string", &source)) {
        return nullptr;
    }

    napi_value details = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, K_AFFIX_SCENARIOS.size(), &details));

    uint32_t totalOccurrences = 0;
    uint32_t digestMix = 0;
    int32_t firstPositionSum = 0;
    int32_t lastPositionSum = 0;
    bool everySourceVisible = true;
    for (size_t index = 0; index < K_AFFIX_SCENARIOS.size(); index++) {
        const auto& spec = K_AFFIX_SCENARIOS[index];
        const std::string text = std::string(spec.prefix) + "|" + source + "|" + spec.suffix + "|" + source + "|" +
                                 spec.needle + "|" + spec.label;
        const int32_t firstNeedle = static_cast<int32_t>(text.find(spec.needle));
        const int32_t lastNeedle = static_cast<int32_t>(text.rfind(spec.needle));
        const uint32_t occurrenceCount = CountOccurrences(text, spec.needle);
        const uint32_t digest = ComputeGridDigest(text, static_cast<uint32_t>(index + 21));
        const bool sourceVisible = source.empty() || text.find(source) != std::string::npos;

        totalOccurrences += occurrenceCount;
        digestMix ^= digest;
        firstPositionSum += firstNeedle;
        lastPositionSum += lastNeedle;
        everySourceVisible = everySourceVisible && sourceVisible;

        napi_value item =
            CreateGridDetail(env, spec, text, firstNeedle, lastNeedle, occurrenceCount, digest, sourceVisible);
        NAPI_CALL(env, napi_set_element(env, details, index, item));
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "name", "stringCase25");
    StringSuite::SetNamedString(env, result, "source", source);
    StringSuite::SetNamedUInt32(env, result, "scenarioCount", static_cast<uint32_t>(K_AFFIX_SCENARIOS.size()));
    StringSuite::SetNamedUInt32(env, result, "totalOccurrences", totalOccurrences);
    StringSuite::SetNamedInt32(env, result, "firstPositionSum", firstPositionSum);
    StringSuite::SetNamedInt32(env, result, "lastPositionSum", lastPositionSum);
    StringSuite::SetNamedUInt32(env, result, "digestMix", digestMix);
    StringSuite::SetNamedBool(env, result, "everySourceVisible", everySourceVisible);
    StringSuite::SetNamedBool(env, result, "passed",
        everySourceVisible && totalOccurrences >= static_cast<uint32_t>(K_AFFIX_SCENARIOS.size()));
    StringSuite::SetNamedValue(env, result, "details", details);
    return result;
}

}  // namespace

void RegisterStringCase25(std::vector<std::string>* exportNames, std::vector<napi_property_descriptor>* descriptors)
{
    StringSuite::AppendStringDescriptor(exportNames, descriptors, "testStringCase25", RunStringCase25);
}
