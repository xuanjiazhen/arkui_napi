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

#include <cstdint>
#include <string>
#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace {

constexpr size_t K_FUNCTION_CASE_COUNT = 88;
constexpr size_t K_FUNCTION_ARG_COUNT = 2;
constexpr size_t K_INVOKE_ARG_COUNT = 2;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr size_t K_CALLBACK_ARG_INDEX = 0;
constexpr size_t K_INPUT_ARG_INDEX = 1;
constexpr size_t K_FIRST_INVOKE_INDEX = 0;
constexpr size_t K_SECOND_INVOKE_INDEX = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;

struct FunctionCaseSpec {
    const char* name;
    int32_t bias;
    int32_t rounds;
    int32_t delta;
    int32_t stride;
    int32_t threshold;
};

static const FunctionCaseSpec K_FUNCTION_CASE_SPECS[] = {
    {
        "functionCase01",
        3,
        2,
        12,
        3,
        31,
    },
    {
        "functionCase02",
        5,
        3,
        15,
        4,
        35,
    },
    {
        "functionCase03",
        7,
        4,
        18,
        5,
        39,
    },
    {
        "functionCase04",
        9,
        5,
        21,
        6,
        43,
    },
    {
        "functionCase05",
        11,
        1,
        24,
        7,
        47,
    },
    {
        "functionCase06",
        13,
        2,
        27,
        8,
        51,
    },
    {
        "functionCase07",
        15,
        3,
        30,
        2,
        55,
    },
    {
        "functionCase08",
        17,
        4,
        33,
        3,
        59,
    },
    {
        "functionCase09",
        19,
        5,
        36,
        4,
        63,
    },
    {
        "functionCase10",
        21,
        1,
        39,
        5,
        67,
    },
    {
        "functionCase11",
        23,
        2,
        42,
        6,
        71,
    },
    {
        "functionCase12",
        25,
        3,
        45,
        7,
        75,
    },
    {
        "functionCase13",
        27,
        4,
        48,
        8,
        79,
    },
    {
        "functionCase14",
        29,
        5,
        51,
        2,
        83,
    },
    {
        "functionCase15",
        31,
        1,
        54,
        3,
        87,
    },
    {
        "functionCase16",
        33,
        2,
        57,
        4,
        91,
    },
    {
        "functionCase17",
        35,
        3,
        60,
        5,
        95,
    },
    {
        "functionCase18",
        37,
        4,
        63,
        6,
        99,
    },
    {
        "functionCase19",
        39,
        5,
        66,
        7,
        103,
    },
    {
        "functionCase20",
        41,
        1,
        69,
        8,
        107,
    },
    {
        "functionCase21",
        43,
        2,
        72,
        2,
        111,
    },
    {
        "functionCase22",
        45,
        3,
        75,
        3,
        115,
    },
    {
        "functionCase23",
        47,
        4,
        78,
        4,
        119,
    },
    {
        "functionCase24",
        49,
        5,
        81,
        5,
        123,
    },
    {
        "functionCase25",
        51,
        1,
        84,
        6,
        127,
    },
    {
        "functionCase26",
        53,
        2,
        87,
        7,
        131,
    },
    {
        "functionCase27",
        55,
        3,
        90,
        8,
        135,
    },
    {
        "functionCase28",
        57,
        4,
        93,
        2,
        139,
    },
    {
        "functionCase29",
        59,
        5,
        96,
        3,
        143,
    },
    {
        "functionCase30",
        61,
        1,
        99,
        4,
        147,
    },
    {
        "functionCase31",
        63,
        2,
        102,
        5,
        151,
    },
    {
        "functionCase32",
        65,
        3,
        105,
        6,
        155,
    },
    {
        "functionCase33",
        67,
        4,
        108,
        7,
        159,
    },
    {
        "functionCase34",
        69,
        5,
        111,
        8,
        163,
    },
    {
        "functionCase35",
        71,
        1,
        114,
        2,
        167,
    },
    {
        "functionCase36",
        73,
        2,
        117,
        3,
        171,
    },
    {
        "functionCase37",
        75,
        3,
        120,
        4,
        175,
    },
    {
        "functionCase38",
        77,
        4,
        123,
        5,
        179,
    },
    {
        "functionCase39",
        79,
        5,
        126,
        6,
        183,
    },
    {
        "functionCase40",
        81,
        1,
        129,
        7,
        187,
    },
    {
        "functionCase41",
        83,
        2,
        132,
        8,
        191,
    },
    {
        "functionCase42",
        85,
        3,
        135,
        2,
        195,
    },
    {
        "functionCase43",
        87,
        4,
        138,
        3,
        199,
    },
    {
        "functionCase44",
        89,
        5,
        141,
        4,
        203,
    },
    {
        "functionCase45",
        91,
        1,
        144,
        5,
        207,
    },
    {
        "functionCase46",
        93,
        2,
        147,
        6,
        211,
    },
    {
        "functionCase47",
        95,
        3,
        150,
        7,
        215,
    },
    {
        "functionCase48",
        97,
        4,
        153,
        8,
        219,
    },
    {
        "functionCase49",
        99,
        5,
        156,
        2,
        223,
    },
    {
        "functionCase50",
        101,
        1,
        159,
        3,
        227,
    },
    {
        "functionCase51",
        103,
        2,
        162,
        4,
        231,
    },
    {
        "functionCase52",
        105,
        3,
        165,
        5,
        235,
    },
    {
        "functionCase53",
        107,
        4,
        168,
        6,
        239,
    },
    {
        "functionCase54",
        109,
        5,
        171,
        7,
        243,
    },
    {
        "functionCase55",
        111,
        1,
        174,
        8,
        247,
    },
    {
        "functionCase56",
        113,
        2,
        177,
        2,
        251,
    },
    {
        "functionCase57",
        115,
        3,
        180,
        3,
        255,
    },
    {
        "functionCase58",
        117,
        4,
        183,
        4,
        259,
    },
    {
        "functionCase59",
        119,
        5,
        186,
        5,
        263,
    },
    {
        "functionCase60",
        121,
        1,
        189,
        6,
        267,
    },
    {
        "functionCase61",
        123,
        2,
        192,
        7,
        271,
    },
    {
        "functionCase62",
        125,
        3,
        195,
        8,
        275,
    },
    {
        "functionCase63",
        127,
        4,
        198,
        2,
        279,
    },
    {
        "functionCase64",
        129,
        5,
        201,
        3,
        283,
    },
    {
        "functionCase65",
        131,
        1,
        204,
        4,
        287,
    },
    {
        "functionCase66",
        133,
        2,
        207,
        5,
        291,
    },
    {
        "functionCase67",
        135,
        3,
        210,
        6,
        295,
    },
    {
        "functionCase68",
        137,
        4,
        213,
        7,
        299,
    },
    {
        "functionCase69",
        139,
        5,
        216,
        8,
        303,
    },
    {
        "functionCase70",
        141,
        1,
        219,
        2,
        307,
    },
    {
        "functionCase71",
        143,
        2,
        222,
        3,
        311,
    },
    {
        "functionCase72",
        145,
        3,
        225,
        4,
        315,
    },
    {
        "functionCase73",
        147,
        4,
        228,
        5,
        319,
    },
    {
        "functionCase74",
        149,
        5,
        231,
        6,
        323,
    },
    {
        "functionCase75",
        151,
        1,
        234,
        7,
        327,
    },
    {
        "functionCase76",
        153,
        2,
        237,
        8,
        331,
    },
    {
        "functionCase77",
        155,
        3,
        240,
        2,
        335,
    },
    {
        "functionCase78",
        157,
        4,
        243,
        3,
        339,
    },
    {
        "functionCase79",
        159,
        5,
        246,
        4,
        343,
    },
    {
        "functionCase80",
        161,
        1,
        249,
        5,
        347,
    },
    {
        "functionCase81",
        163,
        2,
        252,
        6,
        351,
    },
    {
        "functionCase82",
        165,
        3,
        255,
        7,
        355,
    },
    {
        "functionCase83",
        167,
        4,
        258,
        8,
        359,
    },
    {
        "functionCase84",
        169,
        5,
        261,
        2,
        363,
    },
    {
        "functionCase85",
        171,
        1,
        264,
        3,
        367,
    },
    {
        "functionCase86",
        173,
        2,
        267,
        4,
        371,
    },
    {
        "functionCase87",
        175,
        3,
        270,
        5,
        375,
    },
    {
        "functionCase88",
        177,
        4,
        273,
        6,
        379,
    },
};

std::string BuildIndexedName(const char* prefix, size_t caseNumber)
{
    std::string suffix = std::to_string(caseNumber);
    if (suffix.size() < static_cast<size_t>(K_CASE_NUMBER_WIDTH)) {
        suffix.insert(0, static_cast<std::string::size_type>(K_CASE_NUMBER_WIDTH - suffix.size()), '0');
    }
    return std::string(prefix) + suffix;
}

size_t GetCaseIndex(void* data) { return static_cast<size_t>(reinterpret_cast<uintptr_t>(data)); }

bool ReadFunction(napi_env env, napi_value value)
{
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, value, &type) != napi_ok) {
        return false;
    }
    if (type != napi_function) {
        napi_throw_type_error(env, nullptr, "callback must be a function");
        return false;
    }
    return true;
}

bool ReadInt32(napi_env env, napi_value value, const char* message, int32_t* result)
{
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, value, &type) != napi_ok) {
        return false;
    }
    if (type != napi_number) {
        napi_throw_type_error(env, nullptr, message);
        return false;
    }
    return napi_get_value_int32(env, value, result) == napi_ok;
}

bool SetNamedInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int32(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedBool(napi_env env, napi_value object, const char* name, bool value)
{
    napi_value napiValue = nullptr;
    if (napi_get_boolean(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedString(napi_env env, napi_value object, const char* name, const char* value)
{
    napi_value napiValue = nullptr;
    if (napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

napi_value CreateFunctionSummary(napi_env env, const FunctionCaseSpec& spec, int32_t input, napi_value outputs,
                                 int32_t total)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedInt32(env, result, "input", input);
    SetNamedInt32(env, result, "total", total);
    SetNamedBool(env, result, "passed", total >= spec.threshold);
    NAPI_CALL(env, napi_set_named_property(env, result, "outputs", outputs));
    return result;
}

static napi_value RunFunctionCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_FUNCTION_ARG_COUNT;
    napi_value args[K_FUNCTION_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_FUNCTION_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid function case");
        return nullptr;
    }
    if (argc < K_FUNCTION_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "callback and input are required");
        return nullptr;
    }
    if (!ReadFunction(env, args[K_CALLBACK_ARG_INDEX])) {
        return nullptr;
    }

    int32_t input = 0;
    if (!ReadInt32(env, args[K_INPUT_ARG_INDEX], "input must be a number", &input)) {
        return nullptr;
    }

    const auto& spec = K_FUNCTION_CASE_SPECS[caseIndex];
    napi_value undefined = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &undefined));
    napi_value outputs = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, static_cast<size_t>(spec.rounds), &outputs));
    int32_t total = 0;
    for (int32_t round = 0; round < spec.rounds; round++) {
        napi_value invokeArgs[K_INVOKE_ARG_COUNT] = {nullptr};
        napi_value callbackResult = nullptr;
        NAPI_CALL(env,
                  napi_create_int32(env, input + spec.bias + round * spec.stride, &invokeArgs[K_FIRST_INVOKE_INDEX]));
        NAPI_CALL(env, napi_create_int32(env, spec.delta + round, &invokeArgs[K_SECOND_INVOKE_INDEX]));
        if (napi_call_function(env, undefined, args[K_CALLBACK_ARG_INDEX], K_INVOKE_ARG_COUNT, invokeArgs,
                               &callbackResult) != napi_ok) {
            return nullptr;
        }
        NAPI_CALL(env, napi_set_element(env, outputs, static_cast<uint32_t>(round), callbackResult));
        total += input + spec.bias + spec.delta + round * spec.stride;
    }
    return CreateFunctionSummary(env, spec, input, outputs, total);
}

}  // namespace

napi_value RegisterFunctionTests(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_FUNCTION_CASE_COUNT);
    exportNames.reserve(K_FUNCTION_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_FUNCTION_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testFunctionCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(),
                                                          nullptr,
                                                          RunFunctionCase,
                                                          nullptr,
                                                          nullptr,
                                                          nullptr,
                                                          napi_default,
                                                          reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}
