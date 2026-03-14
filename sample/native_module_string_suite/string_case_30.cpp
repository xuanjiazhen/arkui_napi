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

constexpr uint32_t K_PIPELINE_DIGEST_FACTOR = 229;
constexpr size_t K_STRING_CASE_30_ARG_COUNT = 1;

struct PipelineScenarioSpec {
    const char* label;
    const char* prefix;
    const char* suffix;
    const char* replaceNeedle;
    const char* replaceValue;
};

constexpr std::array<PipelineScenarioSpec, 8> K_PIPELINE_SCENARIOS = {{
    {"pipe-01", "north", "gate", "a", "A"},
    {"pipe-02", "south", "ring", "e", "E"},
    {"pipe-03", "east", "field", "i", "I"},
    {"pipe-04", "west", "shore", "o", "O"},
    {"pipe-05", "inner", "stone", "u", "U"},
    {"pipe-06", "outer", "signal", "t", "T"},
    {"pipe-07", "delta", "vector", "r", "R"},
    {"pipe-08", "omega", "anchor", "n", "N"},
}};

std::string ReplaceAll(std::string value, const std::string& needle, const std::string& replacement)
{
    if (needle.empty()) {
        return value;
    }

    size_t position = value.find(needle);
    while (position != std::string::npos) {
        value.replace(position, needle.size(), replacement);
        position = value.find(needle, position + replacement.size());
    }
    return value;
}

std::string RotateLeft(const std::string& value, size_t offset)
{
    if (value.empty()) {
        return value;
    }
    const size_t normalizedOffset = offset % value.size();
    return value.substr(normalizedOffset) + value.substr(0, normalizedOffset);
}

uint32_t ComputePipelineDigest(const std::string& value, uint32_t seed)
{
    uint32_t digest = seed;
    for (unsigned char current : value) {
        digest = digest * K_PIPELINE_DIGEST_FACTOR + current;
    }
    return digest;
}

napi_value CreatePipelineDetail(napi_env env, const PipelineScenarioSpec& spec, const std::string& pipelineOutput,
    uint32_t digest, uint32_t outputLength, bool replacementApplied)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "label", spec.label);
    StringSuite::SetNamedString(env, result, "output", pipelineOutput);
    StringSuite::SetNamedUInt32(env, result, "digest", digest);
    StringSuite::SetNamedUInt32(env, result, "outputLength", outputLength);
    StringSuite::SetNamedBool(env, result, "replacementApplied", replacementApplied);
    return result;
}

static napi_value RunStringCase30(napi_env env, napi_callback_info info)
{
    size_t argc = K_STRING_CASE_30_ARG_COUNT;
    napi_value args[K_STRING_CASE_30_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < K_STRING_CASE_30_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "source is required");
        return nullptr;
    }

    std::string source;
    if (!StringSuite::ReadUtf8(env, args[0], "source must be a string", &source)) {
        return nullptr;
    }

    napi_value details = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, K_PIPELINE_SCENARIOS.size(), &details));

    uint32_t digestMix = 0;
    uint32_t totalOutputLength = 0;
    uint32_t longestOutput = 0;
    bool allReplacementApplied = true;
    for (size_t index = 0; index < K_PIPELINE_SCENARIOS.size(); index++) {
        const auto& spec = K_PIPELINE_SCENARIOS[index];
        std::string pipelineOutput = std::string(spec.prefix) + "|" + source + "|" + spec.suffix + "|" + spec.label;
        pipelineOutput = ReplaceAll(pipelineOutput, spec.replaceNeedle, spec.replaceValue);
        pipelineOutput = RotateLeft(pipelineOutput, index + 1);
        pipelineOutput += "#" + std::to_string(index + source.size());
        const uint32_t digest = ComputePipelineDigest(pipelineOutput, static_cast<uint32_t>(index + 71));
        const bool replacementApplied = pipelineOutput.find(spec.replaceValue) != std::string::npos;

        digestMix ^= digest;
        totalOutputLength += static_cast<uint32_t>(pipelineOutput.size());
        longestOutput = std::max(longestOutput, static_cast<uint32_t>(pipelineOutput.size()));
        allReplacementApplied = allReplacementApplied && replacementApplied;

        napi_value item = CreatePipelineDetail(
            env, spec, pipelineOutput, digest, static_cast<uint32_t>(pipelineOutput.size()), replacementApplied);
        NAPI_CALL(env, napi_set_element(env, details, index, item));
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    StringSuite::SetNamedString(env, result, "name", "stringCase30");
    StringSuite::SetNamedString(env, result, "source", source);
    StringSuite::SetNamedUInt32(env, result, "scenarioCount", static_cast<uint32_t>(K_PIPELINE_SCENARIOS.size()));
    StringSuite::SetNamedUInt32(env, result, "totalOutputLength", totalOutputLength);
    StringSuite::SetNamedUInt32(env, result, "longestOutput", longestOutput);
    StringSuite::SetNamedUInt32(env, result, "digestMix", digestMix);
    StringSuite::SetNamedBool(env, result, "allReplacementApplied", allReplacementApplied);
    StringSuite::SetNamedBool(env, result, "passed", allReplacementApplied && totalOutputLength >= longestOutput);
    StringSuite::SetNamedValue(env, result, "details", details);
    return result;
}

}  // namespace

void RegisterStringCase30(std::vector<std::string>* exportNames, std::vector<napi_property_descriptor>* descriptors)
{
    StringSuite::AppendStringDescriptor(exportNames, descriptors, "testStringCase30", RunStringCase30);
}
