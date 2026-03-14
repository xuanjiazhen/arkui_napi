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
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

#include "string_suite_common.h"

namespace {

constexpr uint32_t K_BUCKET_DIGEST_FACTOR = 193;
constexpr size_t K_STRING_CASE_26_ARG_COUNT = 1;

struct BucketScenarioSpec {
    const char* label;
    const char* left;
    const char* right;
    const char* infix;
};

constexpr std::array<BucketScenarioSpec, 10> K_BUCKET_SCENARIOS = {{
    {"bucket-01", "A1", "z9", "mix"},
    {"bucket-02", "B2", "y8", "scan"},
    {"bucket-03", "C3", "x7", "value"},
    {"bucket-04", "D4", "w6", "probe"},
    {"bucket-05", "E5", "v5", "trace"},
    {"bucket-06", "F6", "u4", "shape"},
    {"bucket-07", "G7", "t3", "grain"},
    {"bucket-08", "H8", "s2", "anchor"},
    {"bucket-09", "I9", "r1", "debug"},
    {"bucket-10", "J0", "q0", "signal"},
}};

struct BucketStats {
    uint32_t letters = 0;
    uint32_t digits = 0;
    uint32_t separators = 0;
    uint32_t uppercase = 0;
};

BucketStats ScanBuckets(const std::string& value)
{
    BucketStats stats;
    for (unsigned char current : value) {
        if (std::isalpha(current) != 0) {
            stats.letters++;
            if (std::isupper(current) != 0) {
                stats.uppercase++;
            }
            continue;
        }
        if (std::isdigit(current) != 0) {
            stats.digits++;
            continue;
        }
        stats.separators++;
    }
    return stats;
}

uint32_t ComputeBucketDigest(const std::string& value, uint32_t seed)
{
    uint32_t digest = seed;
    for (unsigned char current : value) {
        digest = digest * K_BUCKET_DIGEST_FACTOR + current;
    }
    return digest;
}

napi_value CreateBucketDetail(
    napi_env env, const BucketScenarioSpec& spec, const std::string& text, const BucketStats& stats, uint32_t digest)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "label", spec.label);
    StringSuite::SetNamedString(env, result, "text", text);
    StringSuite::SetNamedUInt32(env, result, "letters", stats.letters);
    StringSuite::SetNamedUInt32(env, result, "digits", stats.digits);
    StringSuite::SetNamedUInt32(env, result, "separators", stats.separators);
    StringSuite::SetNamedUInt32(env, result, "uppercase", stats.uppercase);
    StringSuite::SetNamedUInt32(env, result, "digest", digest);
    return result;
}

static napi_value RunStringCase26(napi_env env, napi_callback_info info)
{
    size_t argc = K_STRING_CASE_26_ARG_COUNT;
    napi_value args[K_STRING_CASE_26_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < K_STRING_CASE_26_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "source is required");
        return nullptr;
    }

    std::string source;
    if (!StringSuite::ReadUtf8(env, args[0], "source must be a string", &source)) {
        return nullptr;
    }

    napi_value details = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, K_BUCKET_SCENARIOS.size(), &details));

    BucketStats totalStats;
    uint32_t digestMix = 0;
    bool separatorFloor = true;
    for (size_t index = 0; index < K_BUCKET_SCENARIOS.size(); index++) {
        const auto& spec = K_BUCKET_SCENARIOS[index];
        const std::string text =
            std::string(spec.left) + "|" + source + "|" + spec.infix + "|" + spec.right + "|" + spec.label;
        const BucketStats stats = ScanBuckets(text);
        const uint32_t digest = ComputeBucketDigest(text, static_cast<uint32_t>(index + 31));

        totalStats.letters += stats.letters;
        totalStats.digits += stats.digits;
        totalStats.separators += stats.separators;
        totalStats.uppercase += stats.uppercase;
        digestMix ^= digest;
        separatorFloor = separatorFloor && stats.separators >= 4;

        napi_value item = CreateBucketDetail(env, spec, text, stats, digest);
        NAPI_CALL(env, napi_set_element(env, details, index, item));
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "name", "stringCase26");
    StringSuite::SetNamedString(env, result, "source", source);
    StringSuite::SetNamedUInt32(env, result, "scenarioCount", static_cast<uint32_t>(K_BUCKET_SCENARIOS.size()));
    StringSuite::SetNamedUInt32(env, result, "letters", totalStats.letters);
    StringSuite::SetNamedUInt32(env, result, "digits", totalStats.digits);
    StringSuite::SetNamedUInt32(env, result, "separators", totalStats.separators);
    StringSuite::SetNamedUInt32(env, result, "uppercase", totalStats.uppercase);
    StringSuite::SetNamedUInt32(env, result, "digestMix", digestMix);
    StringSuite::SetNamedBool(env, result, "separatorFloor", separatorFloor);
    StringSuite::SetNamedBool(env, result, "passed", separatorFloor && totalStats.letters > totalStats.digits);
    StringSuite::SetNamedValue(env, result, "details", details);
    return result;
}

}  // namespace

void RegisterStringCase26(std::vector<std::string>* exportNames, std::vector<napi_property_descriptor>* descriptors)
{
    StringSuite::AppendStringDescriptor(exportNames, descriptors, "testStringCase26", RunStringCase26);
}
