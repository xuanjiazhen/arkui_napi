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

constexpr uint32_t K_MIRROR_DIGEST_FACTOR = 173;
constexpr size_t K_STRING_CASE_24_ARG_COUNT = 1;

struct MirrorScenarioSpec {
    const char* label;
    const char* head;
    const char* tail;
    const char* pivot;
    size_t window;
};

constexpr std::array<MirrorScenarioSpec, 9> K_MIRROR_SCENARIOS = {{
    {"mirror-01", "oak", "stone", "@@", 2},
    {"mirror-02", "pine", "stream", "##", 3},
    {"mirror-03", "maple", "shore", "$$", 4},
    {"mirror-04", "cedar", "field", "&&", 5},
    {"mirror-05", "birch", "north", "**", 6},
    {"mirror-06", "elm", "south", "==", 3},
    {"mirror-07", "spruce", "east", "++", 4},
    {"mirror-08", "fir", "west", "!!", 5},
    {"mirror-09", "ash", "center", "%%", 2},
}};

std::string ReverseString(const std::string& value) { return std::string(value.rbegin(), value.rend()); }

uint32_t CountSymmetryMatches(const std::string& value)
{
    uint32_t matches = 0;
    for (size_t left = 0; left < value.size() / 2; left++) {
        if (value[left] == value[value.size() - left - 1]) {
            matches++;
        }
    }
    return matches;
}

uint32_t CountVowels(const std::string& value)
{
    uint32_t count = 0;
    for (unsigned char current : value) {
        const unsigned char lowered = static_cast<unsigned char>(std::tolower(current));
        if (lowered == 'a' || lowered == 'e' || lowered == 'i' || lowered == 'o' || lowered == 'u') {
            count++;
        }
    }
    return count;
}

uint32_t ComputeMirrorDigest(const std::string& value, uint32_t seed)
{
    uint32_t digest = seed;
    for (unsigned char current : value) {
        digest = digest * K_MIRROR_DIGEST_FACTOR + current;
    }
    return digest;
}

napi_value CreateMirrorDetail(napi_env env, const MirrorScenarioSpec& spec, const std::string& composed,
    const std::string& mirrored, uint32_t symmetryMatches, uint32_t vowelCount, uint32_t digest, bool pivotVisible)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "label", spec.label);
    StringSuite::SetNamedString(env, result, "composed", composed);
    StringSuite::SetNamedString(env, result, "mirrored", mirrored);
    StringSuite::SetNamedUInt32(env, result, "symmetryMatches", symmetryMatches);
    StringSuite::SetNamedUInt32(env, result, "vowelCount", vowelCount);
    StringSuite::SetNamedUInt32(env, result, "digest", digest);
    StringSuite::SetNamedBool(env, result, "pivotVisible", pivotVisible);
    return result;
}

static napi_value RunStringCase24(napi_env env, napi_callback_info info)
{
    size_t argc = K_STRING_CASE_24_ARG_COUNT;
    napi_value args[K_STRING_CASE_24_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < K_STRING_CASE_24_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "source is required");
        return nullptr;
    }

    std::string source;
    if (!StringSuite::ReadUtf8(env, args[0], "source must be a string", &source)) {
        return nullptr;
    }

    napi_value details = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, K_MIRROR_SCENARIOS.size(), &details));

    uint32_t totalMatches = 0;
    uint32_t totalVowels = 0;
    uint32_t digestMix = 0;
    uint32_t longestMirror = 0;
    bool allPivotsVisible = true;
    for (size_t index = 0; index < K_MIRROR_SCENARIOS.size(); index++) {
        const auto& spec = K_MIRROR_SCENARIOS[index];
        const std::string composed =
            std::string(spec.head) + ":" + source + std::string(spec.pivot) + spec.tail + ":" + spec.label;
        const size_t window = std::min(spec.window, composed.size());
        const std::string mirrored =
            composed.substr(0, window) + ReverseString(composed) + composed.substr(composed.size() - window);
        const uint32_t symmetryMatches = CountSymmetryMatches(mirrored);
        const uint32_t vowelCount = CountVowels(mirrored);
        const uint32_t digest = ComputeMirrorDigest(mirrored, static_cast<uint32_t>(index + 11));
        const bool pivotVisible = mirrored.find(spec.pivot) != std::string::npos;

        totalMatches += symmetryMatches;
        totalVowels += vowelCount;
        digestMix ^= digest;
        longestMirror = std::max(longestMirror, static_cast<uint32_t>(mirrored.size()));
        allPivotsVisible = allPivotsVisible && pivotVisible;

        napi_value item =
            CreateMirrorDetail(env, spec, composed, mirrored, symmetryMatches, vowelCount, digest, pivotVisible);
        NAPI_CALL(env, napi_set_element(env, details, index, item));
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "name", "stringCase24");
    StringSuite::SetNamedString(env, result, "source", source);
    StringSuite::SetNamedUInt32(env, result, "scenarioCount", static_cast<uint32_t>(K_MIRROR_SCENARIOS.size()));
    StringSuite::SetNamedUInt32(env, result, "totalMatches", totalMatches);
    StringSuite::SetNamedUInt32(env, result, "totalVowels", totalVowels);
    StringSuite::SetNamedUInt32(env, result, "longestMirror", longestMirror);
    StringSuite::SetNamedUInt32(env, result, "digestMix", digestMix);
    StringSuite::SetNamedBool(env, result, "allPivotsVisible", allPivotsVisible);
    StringSuite::SetNamedBool(
        env, result, "passed", allPivotsVisible && totalVowels >= static_cast<uint32_t>(source.size()));
    StringSuite::SetNamedValue(env, result, "details", details);
    return result;
}

}  // namespace

void RegisterStringCase24(std::vector<std::string>* exportNames, std::vector<napi_property_descriptor>* descriptors)
{
    StringSuite::AppendStringDescriptor(exportNames, descriptors, "testStringCase24", RunStringCase24);
}
