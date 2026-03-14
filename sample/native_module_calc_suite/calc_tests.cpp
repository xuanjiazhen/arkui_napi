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

constexpr size_t K_CALC_CASE_COUNT = 96;
constexpr size_t K_CALC_ARG_COUNT = 2;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr size_t K_LEFT_ARG_INDEX = 0;
constexpr size_t K_RIGHT_ARG_INDEX = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

struct CalcCaseSpec {
    const char* name;
    int32_t multiplier;
    int32_t divider;
    int32_t offset;
    int32_t loop;
    int32_t threshold;
};

static const CalcCaseSpec K_CALC_CASE_SPECS[] = {
    {
        "calcCase01",
        2,
        3,
        -14,
        4,
        20,
    },
    {
        "calcCase02",
        3,
        4,
        -11,
        5,
        31,
    },
    {
        "calcCase03",
        4,
        5,
        -8,
        6,
        42,
    },
    {
        "calcCase04",
        5,
        6,
        -5,
        7,
        53,
    },
    {
        "calcCase05",
        6,
        7,
        -2,
        3,
        64,
    },
    {
        "calcCase06",
        7,
        8,
        1,
        4,
        75,
    },
    {
        "calcCase07",
        8,
        2,
        4,
        5,
        86,
    },
    {
        "calcCase08",
        9,
        3,
        7,
        6,
        97,
    },
    {
        "calcCase09",
        10,
        4,
        10,
        7,
        108,
    },
    {
        "calcCase10",
        11,
        5,
        13,
        3,
        119,
    },
    {
        "calcCase11",
        12,
        6,
        16,
        4,
        130,
    },
    {
        "calcCase12",
        13,
        7,
        19,
        5,
        141,
    },
    {
        "calcCase13",
        14,
        8,
        22,
        6,
        152,
    },
    {
        "calcCase14",
        15,
        2,
        25,
        7,
        163,
    },
    {
        "calcCase15",
        16,
        3,
        28,
        3,
        174,
    },
    {
        "calcCase16",
        17,
        4,
        31,
        4,
        185,
    },
    {
        "calcCase17",
        18,
        5,
        34,
        5,
        196,
    },
    {
        "calcCase18",
        19,
        6,
        37,
        6,
        207,
    },
    {
        "calcCase19",
        20,
        7,
        40,
        7,
        218,
    },
    {
        "calcCase20",
        21,
        8,
        43,
        3,
        229,
    },
    {
        "calcCase21",
        22,
        2,
        46,
        4,
        240,
    },
    {
        "calcCase22",
        23,
        3,
        49,
        5,
        251,
    },
    {
        "calcCase23",
        24,
        4,
        52,
        6,
        262,
    },
    {
        "calcCase24",
        25,
        5,
        55,
        7,
        273,
    },
    {
        "calcCase25",
        26,
        6,
        58,
        3,
        284,
    },
    {
        "calcCase26",
        27,
        7,
        61,
        4,
        295,
    },
    {
        "calcCase27",
        28,
        8,
        64,
        5,
        306,
    },
    {
        "calcCase28",
        29,
        2,
        67,
        6,
        317,
    },
    {
        "calcCase29",
        30,
        3,
        70,
        7,
        328,
    },
    {
        "calcCase30",
        31,
        4,
        73,
        3,
        339,
    },
    {
        "calcCase31",
        32,
        5,
        76,
        4,
        350,
    },
    {
        "calcCase32",
        33,
        6,
        79,
        5,
        361,
    },
    {
        "calcCase33",
        34,
        7,
        82,
        6,
        372,
    },
    {
        "calcCase34",
        35,
        8,
        85,
        7,
        383,
    },
    {
        "calcCase35",
        36,
        2,
        88,
        3,
        394,
    },
    {
        "calcCase36",
        37,
        3,
        91,
        4,
        405,
    },
    {
        "calcCase37",
        38,
        4,
        94,
        5,
        416,
    },
    {
        "calcCase38",
        39,
        5,
        97,
        6,
        427,
    },
    {
        "calcCase39",
        40,
        6,
        100,
        7,
        438,
    },
    {
        "calcCase40",
        41,
        7,
        103,
        3,
        449,
    },
    {
        "calcCase41",
        42,
        8,
        106,
        4,
        460,
    },
    {
        "calcCase42",
        43,
        2,
        109,
        5,
        471,
    },
    {
        "calcCase43",
        44,
        3,
        112,
        6,
        482,
    },
    {
        "calcCase44",
        45,
        4,
        115,
        7,
        493,
    },
    {
        "calcCase45",
        46,
        5,
        118,
        3,
        504,
    },
    {
        "calcCase46",
        47,
        6,
        121,
        4,
        515,
    },
    {
        "calcCase47",
        48,
        7,
        124,
        5,
        526,
    },
    {
        "calcCase48",
        49,
        8,
        127,
        6,
        537,
    },
    {
        "calcCase49",
        50,
        2,
        130,
        7,
        548,
    },
    {
        "calcCase50",
        51,
        3,
        133,
        3,
        559,
    },
    {
        "calcCase51",
        52,
        4,
        136,
        4,
        570,
    },
    {
        "calcCase52",
        53,
        5,
        139,
        5,
        581,
    },
    {
        "calcCase53",
        54,
        6,
        142,
        6,
        592,
    },
    {
        "calcCase54",
        55,
        7,
        145,
        7,
        603,
    },
    {
        "calcCase55",
        56,
        8,
        148,
        3,
        614,
    },
    {
        "calcCase56",
        57,
        2,
        151,
        4,
        625,
    },
    {
        "calcCase57",
        58,
        3,
        154,
        5,
        636,
    },
    {
        "calcCase58",
        59,
        4,
        157,
        6,
        647,
    },
    {
        "calcCase59",
        60,
        5,
        160,
        7,
        658,
    },
    {
        "calcCase60",
        61,
        6,
        163,
        3,
        669,
    },
    {
        "calcCase61",
        62,
        7,
        166,
        4,
        680,
    },
    {
        "calcCase62",
        63,
        8,
        169,
        5,
        691,
    },
    {
        "calcCase63",
        64,
        2,
        172,
        6,
        702,
    },
    {
        "calcCase64",
        65,
        3,
        175,
        7,
        713,
    },
    {
        "calcCase65",
        66,
        4,
        178,
        3,
        724,
    },
    {
        "calcCase66",
        67,
        5,
        181,
        4,
        735,
    },
    {
        "calcCase67",
        68,
        6,
        184,
        5,
        746,
    },
    {
        "calcCase68",
        69,
        7,
        187,
        6,
        757,
    },
    {
        "calcCase69",
        70,
        8,
        190,
        7,
        768,
    },
    {
        "calcCase70",
        71,
        2,
        193,
        3,
        779,
    },
    {
        "calcCase71",
        72,
        3,
        196,
        4,
        790,
    },
    {
        "calcCase72",
        73,
        4,
        199,
        5,
        801,
    },
    {
        "calcCase73",
        74,
        5,
        202,
        6,
        812,
    },
    {
        "calcCase74",
        75,
        6,
        205,
        7,
        823,
    },
    {
        "calcCase75",
        76,
        7,
        208,
        3,
        834,
    },
    {
        "calcCase76",
        77,
        8,
        211,
        4,
        845,
    },
    {
        "calcCase77",
        78,
        2,
        214,
        5,
        856,
    },
    {
        "calcCase78",
        79,
        3,
        217,
        6,
        867,
    },
    {
        "calcCase79",
        80,
        4,
        220,
        7,
        878,
    },
    {
        "calcCase80",
        81,
        5,
        223,
        3,
        889,
    },
    {
        "calcCase81",
        82,
        6,
        226,
        4,
        900,
    },
    {
        "calcCase82",
        83,
        7,
        229,
        5,
        911,
    },
    {
        "calcCase83",
        84,
        8,
        232,
        6,
        922,
    },
    {
        "calcCase84",
        85,
        2,
        235,
        7,
        933,
    },
    {
        "calcCase85",
        86,
        3,
        238,
        3,
        944,
    },
    {
        "calcCase86",
        87,
        4,
        241,
        4,
        955,
    },
    {
        "calcCase87",
        88,
        5,
        244,
        5,
        966,
    },
    {
        "calcCase88",
        89,
        6,
        247,
        6,
        977,
    },
    {
        "calcCase89",
        90,
        7,
        250,
        7,
        988,
    },
    {
        "calcCase90",
        91,
        8,
        253,
        3,
        999,
    },
    {
        "calcCase91",
        92,
        2,
        256,
        4,
        1010,
    },
    {
        "calcCase92",
        93,
        3,
        259,
        5,
        1021,
    },
    {
        "calcCase93",
        94,
        4,
        262,
        6,
        1032,
    },
    {
        "calcCase94",
        95,
        5,
        265,
        7,
        1043,
    },
    {
        "calcCase95",
        96,
        6,
        268,
        3,
        1054,
    },
    {
        "calcCase96",
        97,
        7,
        271,
        4,
        1065,
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

napi_value CreateCalcSummary(
    napi_env env, const CalcCaseSpec& spec, int32_t left, int32_t right, int32_t total, int32_t normalized)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedInt32(env, result, "left", left);
    SetNamedInt32(env, result, "right", right);
    SetNamedInt32(env, result, "total", total);
    SetNamedInt32(env, result, "normalized", normalized);
    SetNamedBool(env, result, "passed", normalized <= spec.threshold);
    return result;
}

static napi_value RunCalcCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_CALC_ARG_COUNT;
    napi_value args[K_CALC_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_CALC_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid calc case");
        return nullptr;
    }
    if (argc < K_CALC_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "left and right are required");
        return nullptr;
    }

    int32_t left = 0;
    int32_t right = 0;
    if (!ReadInt32(env, args[K_LEFT_ARG_INDEX], "left must be a number", &left)) {
        return nullptr;
    }
    if (!ReadInt32(env, args[K_RIGHT_ARG_INDEX], "right must be a number", &right)) {
        return nullptr;
    }

    const auto& spec = K_CALC_CASE_SPECS[caseIndex];
    int32_t total = left * spec.multiplier + right + spec.offset;
    for (int32_t step = 0; step < spec.loop; step++) {
        total += step + spec.divider;
    }
    const int32_t normalized = total / spec.divider;
    return CreateCalcSummary(env, spec, left, right, total, normalized);
}

}  // namespace

static napi_value InitCalcSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_CALC_CASE_COUNT);
    exportNames.reserve(K_CALC_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_CALC_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testCalcCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunCalcCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_calcSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitCalcSuite,
    .nm_modname = "calc_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterCalcSuiteModule(void) { napi_module_register(&g_calcSuiteModule); }
