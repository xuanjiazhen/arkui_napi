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

constexpr uint32_t K_RLE_DIGEST_FACTOR = 211;
constexpr size_t K_STRING_CASE_28_ARG_COUNT = 1;

struct RleScenarioSpec {
    const char* label;
    const char* prefix;
    const char* filler;
    uint32_t repeat;
};

constexpr std::array<RleScenarioSpec, 9> K_RLE_SCENARIOS = {{
    {"rle-01", "aa", "north", 2},
    {"rle-02", "bb", "south", 3},
    {"rle-03", "cc", "east", 4},
    {"rle-04", "dd", "west", 5},
    {"rle-05", "ee", "core", 3},
    {"rle-06", "ff", "trace", 2},
    {"rle-07", "gg", "value", 4},
    {"rle-08", "hh", "delta", 3},
    {"rle-09", "ii", "omega", 5},
}};

std::string RepeatBlock(const std::string& value, uint32_t repeat)
{
    std::string result;
    result.reserve(value.size() * repeat);
    for (uint32_t index = 0; index < repeat; index++) {
        result += value;
    }
    return result;
}

std::string EncodeRunLength(const std::string& value)
{
    if (value.empty()) {
        return {};
    }

    std::string encoded;
    size_t index = 0;
    while (index < value.size()) {
        size_t next = index + 1;
        while (next < value.size() && value[next] == value[index]) {
            next++;
        }
        encoded.push_back(value[index]);
        encoded += std::to_string(next - index);
        encoded.push_back('|');
        index = next;
    }
    return encoded;
}

uint32_t ComputeRleDigest(const std::string& value, uint32_t seed)
{
    uint32_t digest = seed;
    for (unsigned char current : value) {
        digest = digest * K_RLE_DIGEST_FACTOR + current;
    }
    return digest;
}

napi_value CreateRleDetail(napi_env env, const RleScenarioSpec& spec, const std::string& expanded,
    const std::string& encoded, uint32_t digest, bool compressed)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "label", spec.label);
    StringSuite::SetNamedString(env, result, "expanded", expanded);
    StringSuite::SetNamedString(env, result, "encoded", encoded);
    StringSuite::SetNamedUInt32(env, result, "expandedLength", static_cast<uint32_t>(expanded.size()));
    StringSuite::SetNamedUInt32(env, result, "encodedLength", static_cast<uint32_t>(encoded.size()));
    StringSuite::SetNamedUInt32(env, result, "digest", digest);
    StringSuite::SetNamedBool(env, result, "compressed", compressed);
    return result;
}

static napi_value RunStringCase28(napi_env env, napi_callback_info info)
{
    size_t argc = K_STRING_CASE_28_ARG_COUNT;
    napi_value args[K_STRING_CASE_28_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < K_STRING_CASE_28_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "source is required");
        return nullptr;
    }

    std::string source;
    if (!StringSuite::ReadUtf8(env, args[0], "source must be a string", &source)) {
        return nullptr;
    }

    napi_value details = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, K_RLE_SCENARIOS.size(), &details));

    uint32_t digestMix = 0;
    uint32_t totalExpanded = 0;
    uint32_t totalEncoded = 0;
    bool allCompressed = true;
    for (size_t index = 0; index < K_RLE_SCENARIOS.size(); index++) {
        const auto& spec = K_RLE_SCENARIOS[index];
        const std::string expanded =
            RepeatBlock(spec.prefix, spec.repeat) + source + RepeatBlock(spec.filler, spec.repeat) + spec.label;
        const std::string encoded = EncodeRunLength(expanded);
        const uint32_t digest = ComputeRleDigest(encoded, static_cast<uint32_t>(index + 51));
        const bool compressed = encoded.size() <= expanded.size() * 2;

        digestMix ^= digest;
        totalExpanded += static_cast<uint32_t>(expanded.size());
        totalEncoded += static_cast<uint32_t>(encoded.size());
        allCompressed = allCompressed && compressed;

        napi_value item = CreateRleDetail(env, spec, expanded, encoded, digest, compressed);
        NAPI_CALL(env, napi_set_element(env, details, index, item));
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "name", "stringCase28");
    StringSuite::SetNamedString(env, result, "source", source);
    StringSuite::SetNamedUInt32(env, result, "scenarioCount", static_cast<uint32_t>(K_RLE_SCENARIOS.size()));
    StringSuite::SetNamedUInt32(env, result, "totalExpanded", totalExpanded);
    StringSuite::SetNamedUInt32(env, result, "totalEncoded", totalEncoded);
    StringSuite::SetNamedUInt32(env, result, "digestMix", digestMix);
    StringSuite::SetNamedBool(env, result, "allCompressed", allCompressed);
    StringSuite::SetNamedBool(env, result, "passed", allCompressed && totalEncoded <= totalExpanded * 2);
    StringSuite::SetNamedValue(env, result, "details", details);
    return result;
}

}  // namespace

void RegisterStringCase28(std::vector<std::string>* exportNames, std::vector<napi_property_descriptor>* descriptors)
{
    StringSuite::AppendStringDescriptor(exportNames, descriptors, "testStringCase28", RunStringCase28);
}
