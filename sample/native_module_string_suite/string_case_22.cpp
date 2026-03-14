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

constexpr uint32_t K_EXTRA_DIGEST_SEED = 131;
constexpr size_t K_STRING_CASE_22_ARG_COUNT = 1;

struct Utf8ScenarioSpec {
    const char* label;
    const char* prefix;
    const char* suffix;
    const char* marker;
    int32_t repeat;
};

constexpr std::array<Utf8ScenarioSpec, 12> K_UTF8_SCENARIOS = {{
    {"utf8-window-01", "A<", ">z", "M1", 1},
    {"utf8-window-02", "B[", "]y", "R2", 2},
    {"utf8-window-03", "C(", ")x", "S3", 3},
    {"utf8-window-04", "D{", "}w", "T4", 4},
    {"utf8-window-05", "E|", "|v", "U5", 2},
    {"utf8-window-06", "F/", "\\u", "V6", 5},
    {"utf8-window-07", "G+", "+t", "W7", 3},
    {"utf8-window-08", "H=", "=s", "X8", 4},
    {"utf8-window-09", "I~", "~r", "Y9", 1},
    {"utf8-window-10", "J%", "%q", "Z0", 2},
    {"utf8-window-11", "K!", "!p", "Q1", 3},
    {"utf8-window-12", "L?", "?o", "P2", 4},
}};

std::string RepeatMarker(const std::string& marker, int32_t repeat)
{
    std::string output;
    output.reserve(static_cast<size_t>(repeat) * marker.size());
    for (int32_t index = 0; index < repeat; index++) {
        output += marker;
    }
    return output;
}

uint32_t CountUppercaseLetters(const std::string& value)
{
    uint32_t count = 0;
    for (unsigned char current : value) {
        if (std::isupper(current) != 0) {
            count++;
        }
    }
    return count;
}

uint32_t CountDigits(const std::string& value)
{
    uint32_t count = 0;
    for (unsigned char current : value) {
        if (std::isdigit(current) != 0) {
            count++;
        }
    }
    return count;
}

uint32_t ComputeRollingDigest(const std::string& value, uint32_t seed)
{
    uint32_t digest = seed;
    for (unsigned char current : value) {
        digest = digest * K_EXTRA_DIGEST_SEED + current;
    }
    return digest;
}

napi_value CreateScenarioDetail(napi_env env, const Utf8ScenarioSpec& spec, const std::string& text, uint32_t digest,
    uint32_t uppercaseCount, uint32_t digitCount, bool lengthMatched)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "label", spec.label);
    StringSuite::SetNamedString(env, result, "text", text);
    StringSuite::SetNamedUInt32(env, result, "byteLength", static_cast<uint32_t>(text.size()));
    StringSuite::SetNamedUInt32(env, result, "uppercaseCount", uppercaseCount);
    StringSuite::SetNamedUInt32(env, result, "digitCount", digitCount);
    StringSuite::SetNamedUInt32(env, result, "digest", digest);
    StringSuite::SetNamedBool(env, result, "lengthMatched", lengthMatched);
    return result;
}

static napi_value RunStringCase22(napi_env env, napi_callback_info info)
{
    size_t argc = K_STRING_CASE_22_ARG_COUNT;
    napi_value args[K_STRING_CASE_22_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < K_STRING_CASE_22_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "source is required");
        return nullptr;
    }

    std::string source;
    if (!StringSuite::ReadUtf8(env, args[0], "source must be a string", &source)) {
        return nullptr;
    }

    napi_value details = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, K_UTF8_SCENARIOS.size(), &details));

    uint32_t totalBytes = 0;
    uint32_t totalUppercase = 0;
    uint32_t totalDigits = 0;
    uint32_t combinedDigest = 0;
    uint32_t longestBytes = 0;
    bool allLengthMatched = true;
    for (size_t index = 0; index < K_UTF8_SCENARIOS.size(); index++) {
        const auto& spec = K_UTF8_SCENARIOS[index];
        const std::string repeatedMarker = RepeatMarker(spec.marker, spec.repeat);
        const std::string text =
            std::string(spec.prefix) + source + ":" + spec.suffix + "|" + repeatedMarker + "|" + spec.label;
        const uint32_t expectedLength = static_cast<uint32_t>(
            std::string(spec.prefix).size() + source.size() + std::string(spec.suffix).size() + repeatedMarker.size() +
            std::string(spec.label).size() + 3);
        const bool lengthMatched = expectedLength == text.size();
        const uint32_t uppercaseCount = CountUppercaseLetters(text);
        const uint32_t digitCount = CountDigits(text);
        const uint32_t digest = ComputeRollingDigest(text, static_cast<uint32_t>(index + 1));

        totalBytes += static_cast<uint32_t>(text.size());
        totalUppercase += uppercaseCount;
        totalDigits += digitCount;
        combinedDigest ^= digest;
        longestBytes = std::max(longestBytes, static_cast<uint32_t>(text.size()));
        allLengthMatched = allLengthMatched && lengthMatched;

        napi_value item =
            CreateScenarioDetail(env, spec, text, digest, uppercaseCount, digitCount, lengthMatched);
        NAPI_CALL(env, napi_set_element(env, details, index, item));
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "name", "stringCase22");
    StringSuite::SetNamedString(env, result, "source", source);
    StringSuite::SetNamedUInt32(env, result, "sampleCount", static_cast<uint32_t>(K_UTF8_SCENARIOS.size()));
    StringSuite::SetNamedUInt32(env, result, "totalBytes", totalBytes);
    StringSuite::SetNamedUInt32(env, result, "longestBytes", longestBytes);
    StringSuite::SetNamedUInt32(env, result, "uppercaseCount", totalUppercase);
    StringSuite::SetNamedUInt32(env, result, "digitCount", totalDigits);
    StringSuite::SetNamedUInt32(env, result, "combinedDigest", combinedDigest);
    StringSuite::SetNamedBool(env, result, "allLengthMatched", allLengthMatched);
    StringSuite::SetNamedBool(env, result, "passed",
        allLengthMatched && totalBytes >= longestBytes && combinedDigest != 0U);
    StringSuite::SetNamedValue(env, result, "samples", details);
    return result;
}

}  // namespace

void RegisterStringCase22(std::vector<std::string>* exportNames, std::vector<napi_property_descriptor>* descriptors)
{
    StringSuite::AppendStringDescriptor(exportNames, descriptors, "testStringCase22", RunStringCase22);
}
