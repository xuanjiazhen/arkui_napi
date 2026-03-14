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
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

#include "string_suite_common.h"

namespace {

constexpr uint32_t K_SEGMENT_DIGEST_FACTOR = 167;
constexpr size_t K_STRING_CASE_23_ARG_COUNT = 1;

struct TokenScenarioSpec {
    const char* label;
    const char* prefix;
    const char* infix;
    const char* suffix;
    char separator;
};

constexpr std::array<TokenScenarioSpec, 10> K_TOKEN_SCENARIOS = {{
    {"segment-01", "north", "alpha", "end", '/'},
    {"segment-02", "south", "bravo", "tail", '-'},
    {"segment-03", "east", "charlie", "done", '+'},
    {"segment-04", "west", "delta", "core", '#'},
    {"segment-05", "upper", "echo", "leaf", ':'},
    {"segment-06", "lower", "foxtrot", "gate", ';'},
    {"segment-07", "digit", "golf", "node", '='},
    {"segment-08", "plain", "hotel", "zone", ','},
    {"segment-09", "mixed", "india", "ring", '%'},
    {"segment-10", "final", "juliet", "peak", '^'},
}};

std::string NormalizeTokenString(const std::string& value)
{
    std::string normalized;
    normalized.reserve(value.size());
    bool previousUnderscore = false;
    for (unsigned char current : value) {
        if (std::isalnum(current) != 0) {
            normalized.push_back(static_cast<char>(std::tolower(current)));
            previousUnderscore = false;
            continue;
        }
        if (!previousUnderscore) {
            normalized.push_back('_');
            previousUnderscore = true;
        }
    }
    while (!normalized.empty() && normalized.back() == '_') {
        normalized.pop_back();
    }
    return normalized;
}

uint32_t CountSegments(const std::string& value)
{
    if (value.empty()) {
        return 0;
    }

    uint32_t count = 1;
    for (char current : value) {
        if (current == '_') {
            count++;
        }
    }
    return count;
}

uint32_t CountCharacter(const std::string& value, char target)
{
    return static_cast<uint32_t>(std::count(value.begin(), value.end(), target));
}

uint32_t ComputeSegmentDigest(const std::string& value, uint32_t seed)
{
    uint32_t digest = seed;
    for (unsigned char current : value) {
        digest = digest * K_SEGMENT_DIGEST_FACTOR + current;
    }
    return digest;
}

napi_value CreateTokenDetail(napi_env env, const TokenScenarioSpec& spec, const std::string& composed,
    const std::string& normalized, uint32_t segmentCount, uint32_t underscoreCount, uint32_t digest, bool matched)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "label", spec.label);
    StringSuite::SetNamedString(env, result, "composed", composed);
    StringSuite::SetNamedString(env, result, "normalized", normalized);
    StringSuite::SetNamedUInt32(env, result, "segmentCount", segmentCount);
    StringSuite::SetNamedUInt32(env, result, "underscoreCount", underscoreCount);
    StringSuite::SetNamedUInt32(env, result, "digest", digest);
    StringSuite::SetNamedBool(env, result, "matched", matched);
    return result;
}

static napi_value RunStringCase23(napi_env env, napi_callback_info info)
{
    size_t argc = K_STRING_CASE_23_ARG_COUNT;
    napi_value args[K_STRING_CASE_23_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < K_STRING_CASE_23_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "source is required");
        return nullptr;
    }

    std::string source;
    if (!StringSuite::ReadUtf8(env, args[0], "source must be a string", &source)) {
        return nullptr;
    }

    napi_value details = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, K_TOKEN_SCENARIOS.size(), &details));

    uint32_t totalSegments = 0;
    uint32_t totalUnderscores = 0;
    uint32_t digestMix = 0;
    uint32_t longestNormalized = 0;
    bool allMatched = true;
    for (size_t index = 0; index < K_TOKEN_SCENARIOS.size(); index++) {
        const auto& spec = K_TOKEN_SCENARIOS[index];
        const std::string composed = std::string(spec.prefix) + spec.separator + source + spec.separator + spec.infix +
                                     spec.separator + spec.suffix + spec.separator + spec.label;
        const std::string normalized = NormalizeTokenString(composed);
        const uint32_t segmentCount = CountSegments(normalized);
        const uint32_t underscoreCount = CountCharacter(normalized, '_');
        const uint32_t digest = ComputeSegmentDigest(normalized, static_cast<uint32_t>(index + 5));
        const bool matched = normalized.find(source.empty() ? "" : NormalizeTokenString(source)) != std::string::npos;

        totalSegments += segmentCount;
        totalUnderscores += underscoreCount;
        digestMix ^= digest;
        longestNormalized = std::max(longestNormalized, static_cast<uint32_t>(normalized.size()));
        allMatched = allMatched && matched && segmentCount == underscoreCount + 1;

        napi_value item =
            CreateTokenDetail(env, spec, composed, normalized, segmentCount, underscoreCount, digest, matched);
        NAPI_CALL(env, napi_set_element(env, details, index, item));
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "name", "stringCase23");
    StringSuite::SetNamedString(env, result, "source", source);
    StringSuite::SetNamedUInt32(env, result, "scenarioCount", static_cast<uint32_t>(K_TOKEN_SCENARIOS.size()));
    StringSuite::SetNamedUInt32(env, result, "totalSegments", totalSegments);
    StringSuite::SetNamedUInt32(env, result, "totalUnderscores", totalUnderscores);
    StringSuite::SetNamedUInt32(env, result, "longestNormalized", longestNormalized);
    StringSuite::SetNamedUInt32(env, result, "digestMix", digestMix);
    StringSuite::SetNamedBool(env, result, "allMatched", allMatched);
    StringSuite::SetNamedBool(
        env, result, "passed", allMatched && totalSegments >= static_cast<uint32_t>(K_TOKEN_SCENARIOS.size()));
    StringSuite::SetNamedValue(env, result, "details", details);
    return result;
}

}  // namespace

void RegisterStringCase23(std::vector<std::string>* exportNames, std::vector<napi_property_descriptor>* descriptors)
{
    StringSuite::AppendStringDescriptor(exportNames, descriptors, "testStringCase23", RunStringCase23);
}
