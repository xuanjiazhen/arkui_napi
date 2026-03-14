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

constexpr uint32_t K_SEARCH_DIGEST_FACTOR = 223;
constexpr size_t K_STRING_CASE_29_ARG_COUNT = 1;

struct SearchScenarioSpec {
    const char* label;
    const char* needle;
    const char* left;
    const char* right;
};

constexpr std::array<SearchScenarioSpec, 10> K_SEARCH_SCENARIOS = {{
    {"search-01", "ab", "alpha", "beta"},
    {"search-02", "or", "north", "shore"},
    {"search-03", "de", "delta", "edge"},
    {"search-04", "ra", "trace", "radar"},
    {"search-05", "fi", "field", "finish"},
    {"search-06", "le", "level", "angle"},
    {"search-07", "si", "signal", "sigma"},
    {"search-08", "co", "core", "cobalt"},
    {"search-09", "ma", "matrix", "gamma"},
    {"search-10", "ve", "vector", "silver"},
}};

uint32_t ComputeSearchDigest(const std::string& value, uint32_t seed)
{
    uint32_t digest = seed;
    for (unsigned char current : value) {
        digest = digest * K_SEARCH_DIGEST_FACTOR + current;
    }
    return digest;
}

uint32_t CountNeedleHits(const std::string& haystack, const std::string& needle)
{
    if (needle.empty()) {
        return 0;
    }

    uint32_t count = 0;
    size_t position = haystack.find(needle);
    while (position != std::string::npos) {
        count++;
        position = haystack.find(needle, position + needle.size());
    }
    return count;
}

napi_value CreateSearchDetail(napi_env env, const SearchScenarioSpec& spec, const std::string& haystack,
    int32_t firstIndex, uint32_t hitCount, uint32_t digest, bool found)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "label", spec.label);
    StringSuite::SetNamedString(env, result, "haystack", haystack);
    StringSuite::SetNamedInt32(env, result, "firstIndex", firstIndex);
    StringSuite::SetNamedUInt32(env, result, "hitCount", hitCount);
    StringSuite::SetNamedUInt32(env, result, "digest", digest);
    StringSuite::SetNamedBool(env, result, "found", found);
    return result;
}

static napi_value RunStringCase29(napi_env env, napi_callback_info info)
{
    size_t argc = K_STRING_CASE_29_ARG_COUNT;
    napi_value args[K_STRING_CASE_29_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < K_STRING_CASE_29_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "source is required");
        return nullptr;
    }

    std::string source;
    if (!StringSuite::ReadUtf8(env, args[0], "source must be a string", &source)) {
        return nullptr;
    }

    napi_value details = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, K_SEARCH_SCENARIOS.size(), &details));

    int32_t positionSum = 0;
    uint32_t totalHits = 0;
    uint32_t digestMix = 0;
    bool allFound = true;
    for (size_t index = 0; index < K_SEARCH_SCENARIOS.size(); index++) {
        const auto& spec = K_SEARCH_SCENARIOS[index];
        const std::string haystack =
            std::string(spec.left) + "|" + source + "|" + spec.right + "|" + spec.needle + "|" + source;
        const int32_t firstIndex = static_cast<int32_t>(haystack.find(spec.needle));
        const uint32_t hitCount = CountNeedleHits(haystack, spec.needle);
        const uint32_t digest = ComputeSearchDigest(haystack, static_cast<uint32_t>(index + 61));
        const bool found = firstIndex >= 0;

        positionSum += firstIndex;
        totalHits += hitCount;
        digestMix ^= digest;
        allFound = allFound && found;

        napi_value item = CreateSearchDetail(env, spec, haystack, firstIndex, hitCount, digest, found);
        NAPI_CALL(env, napi_set_element(env, details, index, item));
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "name", "stringCase29");
    StringSuite::SetNamedString(env, result, "source", source);
    StringSuite::SetNamedUInt32(env, result, "scenarioCount", static_cast<uint32_t>(K_SEARCH_SCENARIOS.size()));
    StringSuite::SetNamedInt32(env, result, "positionSum", positionSum);
    StringSuite::SetNamedUInt32(env, result, "totalHits", totalHits);
    StringSuite::SetNamedUInt32(env, result, "digestMix", digestMix);
    StringSuite::SetNamedBool(env, result, "allFound", allFound);
    StringSuite::SetNamedBool(env, result, "passed", allFound && totalHits >= static_cast<uint32_t>(K_SEARCH_SCENARIOS.size()));
    StringSuite::SetNamedValue(env, result, "details", details);
    return result;
}

}  // namespace

void RegisterStringCase29(std::vector<std::string>* exportNames, std::vector<napi_property_descriptor>* descriptors)
{
    StringSuite::AppendStringDescriptor(exportNames, descriptors, "testStringCase29", RunStringCase29);
}
