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

constexpr uint32_t K_PALINDROME_DIGEST_FACTOR = 199;
constexpr size_t K_STRING_CASE_27_ARG_COUNT = 1;

struct PalindromeScenarioSpec {
    const char* label;
    const char* left;
    const char* center;
    const char* right;
};

constexpr std::array<PalindromeScenarioSpec, 8> K_PALINDROME_SCENARIOS = {{
    {"pal-01", "level", "north", "level"},
    {"pal-02", "radar", "south", "radar"},
    {"pal-03", "civic", "east", "civic"},
    {"pal-04", "refer", "west", "refer"},
    {"pal-05", "madam", "core", "madam"},
    {"pal-06", "rotor", "ring", "rotor"},
    {"pal-07", "kayak", "trace", "kayak"},
    {"pal-08", "tenet", "anchor", "tenet"},
}};

std::string NormalizePalindromeText(const std::string& value)
{
    std::string normalized;
    normalized.reserve(value.size());
    for (unsigned char current : value) {
        if (std::isalnum(current) != 0) {
            normalized.push_back(static_cast<char>(std::tolower(current)));
        }
    }
    return normalized;
}

uint32_t CountMirrorMismatches(const std::string& value)
{
    uint32_t mismatches = 0;
    for (size_t left = 0; left < value.size() / 2; left++) {
        if (value[left] != value[value.size() - left - 1]) {
            mismatches++;
        }
    }
    return mismatches;
}

uint32_t ComputePalindromeDigest(const std::string& value, uint32_t seed)
{
    uint32_t digest = seed;
    for (unsigned char current : value) {
        digest = digest * K_PALINDROME_DIGEST_FACTOR + current;
    }
    return digest;
}

napi_value CreatePalindromeDetail(napi_env env, const PalindromeScenarioSpec& spec, const std::string& normalized,
    uint32_t mismatches, uint32_t digest, bool palindromeLike)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "label", spec.label);
    StringSuite::SetNamedString(env, result, "normalized", normalized);
    StringSuite::SetNamedUInt32(env, result, "mismatches", mismatches);
    StringSuite::SetNamedUInt32(env, result, "digest", digest);
    StringSuite::SetNamedBool(env, result, "palindromeLike", palindromeLike);
    return result;
}

static napi_value RunStringCase27(napi_env env, napi_callback_info info)
{
    size_t argc = K_STRING_CASE_27_ARG_COUNT;
    napi_value args[K_STRING_CASE_27_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < K_STRING_CASE_27_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "source is required");
        return nullptr;
    }

    std::string source;
    if (!StringSuite::ReadUtf8(env, args[0], "source must be a string", &source)) {
        return nullptr;
    }

    napi_value details = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, K_PALINDROME_SCENARIOS.size(), &details));

    uint32_t totalMismatches = 0;
    uint32_t digestMix = 0;
    uint32_t longestNormalized = 0;
    bool allPalindromeLike = true;
    for (size_t index = 0; index < K_PALINDROME_SCENARIOS.size(); index++) {
        const auto& spec = K_PALINDROME_SCENARIOS[index];
        const std::string composed =
            std::string(spec.left) + "|" + source + "|" + spec.center + "|" + source + "|" + spec.right;
        const std::string normalized = NormalizePalindromeText(composed);
        const uint32_t mismatches = CountMirrorMismatches(normalized);
        const uint32_t digest = ComputePalindromeDigest(normalized, static_cast<uint32_t>(index + 41));
        const bool palindromeLike = mismatches <= source.size();

        totalMismatches += mismatches;
        digestMix ^= digest;
        longestNormalized = std::max(longestNormalized, static_cast<uint32_t>(normalized.size()));
        allPalindromeLike = allPalindromeLike && palindromeLike;

        napi_value item = CreatePalindromeDetail(env, spec, normalized, mismatches, digest, palindromeLike);
        NAPI_CALL(env, napi_set_element(env, details, index, item));
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "name", "stringCase27");
    StringSuite::SetNamedString(env, result, "source", source);
    StringSuite::SetNamedUInt32(env, result, "scenarioCount", static_cast<uint32_t>(K_PALINDROME_SCENARIOS.size()));
    StringSuite::SetNamedUInt32(env, result, "totalMismatches", totalMismatches);
    StringSuite::SetNamedUInt32(env, result, "longestNormalized", longestNormalized);
    StringSuite::SetNamedUInt32(env, result, "digestMix", digestMix);
    StringSuite::SetNamedBool(env, result, "allPalindromeLike", allPalindromeLike);
    StringSuite::SetNamedBool(env, result, "passed", allPalindromeLike && longestNormalized >= source.size());
    StringSuite::SetNamedValue(env, result, "details", details);
    return result;
}

}  // namespace

void RegisterStringCase27(std::vector<std::string>* exportNames, std::vector<napi_property_descriptor>* descriptors)
{
    StringSuite::AppendStringDescriptor(exportNames, descriptors, "testStringCase27", RunStringCase27);
}
