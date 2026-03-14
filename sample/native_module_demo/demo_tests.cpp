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

constexpr size_t K_DEMO_CASE_COUNT = 88;
constexpr size_t K_DEMO_ARG_COUNT = 1;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr size_t K_RECORD_ARG_INDEX = 0;
constexpr int K_CASE_NUMBER_WIDTH = 2;
constexpr size_t K_NULL_TERMINATOR_SIZE = 1;

struct DemoCaseSpec {
    const char* name;
    const char* tag;
    int32_t repeat;
    int32_t offset;
    int32_t gain;
    int32_t threshold;
};

struct DemoSummaryData {
    const char* name;
    const std::string& title;
    const std::string& decorated;
    int32_t count;
    int32_t score;
    bool passed;
};

static const DemoCaseSpec K_DEMO_CASE_SPECS[] = {
    {
        "demoCase01",
        "demo01",
        3,
        11,
        4,
        26,
    },
    {
        "demoCase02",
        "demo02",
        4,
        15,
        5,
        31,
    },
    {
        "demoCase03",
        "demo03",
        5,
        19,
        6,
        36,
    },
    {
        "demoCase04",
        "demo04",
        6,
        23,
        7,
        41,
    },
    {
        "demoCase05",
        "demo05",
        7,
        27,
        8,
        46,
    },
    {
        "demoCase06",
        "demo06",
        8,
        31,
        3,
        51,
    },
    {
        "demoCase07",
        "demo07",
        9,
        35,
        4,
        56,
    },
    {
        "demoCase08",
        "demo08",
        10,
        39,
        5,
        61,
    },
    {
        "demoCase09",
        "demo09",
        2,
        43,
        6,
        66,
    },
    {
        "demoCase10",
        "demo10",
        3,
        47,
        7,
        71,
    },
    {
        "demoCase11",
        "demo11",
        4,
        51,
        8,
        76,
    },
    {
        "demoCase12",
        "demo12",
        5,
        55,
        3,
        81,
    },
    {
        "demoCase13",
        "demo13",
        6,
        59,
        4,
        86,
    },
    {
        "demoCase14",
        "demo14",
        7,
        63,
        5,
        91,
    },
    {
        "demoCase15",
        "demo15",
        8,
        67,
        6,
        96,
    },
    {
        "demoCase16",
        "demo16",
        9,
        71,
        7,
        101,
    },
    {
        "demoCase17",
        "demo17",
        10,
        75,
        8,
        106,
    },
    {
        "demoCase18",
        "demo18",
        2,
        79,
        3,
        111,
    },
    {
        "demoCase19",
        "demo19",
        3,
        83,
        4,
        116,
    },
    {
        "demoCase20",
        "demo20",
        4,
        87,
        5,
        121,
    },
    {
        "demoCase21",
        "demo21",
        5,
        91,
        6,
        126,
    },
    {
        "demoCase22",
        "demo22",
        6,
        95,
        7,
        131,
    },
    {
        "demoCase23",
        "demo23",
        7,
        99,
        8,
        136,
    },
    {
        "demoCase24",
        "demo24",
        8,
        103,
        3,
        141,
    },
    {
        "demoCase25",
        "demo25",
        9,
        107,
        4,
        146,
    },
    {
        "demoCase26",
        "demo26",
        10,
        111,
        5,
        151,
    },
    {
        "demoCase27",
        "demo27",
        2,
        115,
        6,
        156,
    },
    {
        "demoCase28",
        "demo28",
        3,
        119,
        7,
        161,
    },
    {
        "demoCase29",
        "demo29",
        4,
        123,
        8,
        166,
    },
    {
        "demoCase30",
        "demo30",
        5,
        127,
        3,
        171,
    },
    {
        "demoCase31",
        "demo31",
        6,
        131,
        4,
        176,
    },
    {
        "demoCase32",
        "demo32",
        7,
        135,
        5,
        181,
    },
    {
        "demoCase33",
        "demo33",
        8,
        139,
        6,
        186,
    },
    {
        "demoCase34",
        "demo34",
        9,
        143,
        7,
        191,
    },
    {
        "demoCase35",
        "demo35",
        10,
        147,
        8,
        196,
    },
    {
        "demoCase36",
        "demo36",
        2,
        151,
        3,
        201,
    },
    {
        "demoCase37",
        "demo37",
        3,
        155,
        4,
        206,
    },
    {
        "demoCase38",
        "demo38",
        4,
        159,
        5,
        211,
    },
    {
        "demoCase39",
        "demo39",
        5,
        163,
        6,
        216,
    },
    {
        "demoCase40",
        "demo40",
        6,
        167,
        7,
        221,
    },
    {
        "demoCase41",
        "demo41",
        7,
        171,
        8,
        226,
    },
    {
        "demoCase42",
        "demo42",
        8,
        175,
        3,
        231,
    },
    {
        "demoCase43",
        "demo43",
        9,
        179,
        4,
        236,
    },
    {
        "demoCase44",
        "demo44",
        10,
        183,
        5,
        241,
    },
    {
        "demoCase45",
        "demo45",
        2,
        187,
        6,
        246,
    },
    {
        "demoCase46",
        "demo46",
        3,
        191,
        7,
        251,
    },
    {
        "demoCase47",
        "demo47",
        4,
        195,
        8,
        256,
    },
    {
        "demoCase48",
        "demo48",
        5,
        199,
        3,
        261,
    },
    {
        "demoCase49",
        "demo49",
        6,
        203,
        4,
        266,
    },
    {
        "demoCase50",
        "demo50",
        7,
        207,
        5,
        271,
    },
    {
        "demoCase51",
        "demo51",
        8,
        211,
        6,
        276,
    },
    {
        "demoCase52",
        "demo52",
        9,
        215,
        7,
        281,
    },
    {
        "demoCase53",
        "demo53",
        10,
        219,
        8,
        286,
    },
    {
        "demoCase54",
        "demo54",
        2,
        223,
        3,
        291,
    },
    {
        "demoCase55",
        "demo55",
        3,
        227,
        4,
        296,
    },
    {
        "demoCase56",
        "demo56",
        4,
        231,
        5,
        301,
    },
    {
        "demoCase57",
        "demo57",
        5,
        235,
        6,
        306,
    },
    {
        "demoCase58",
        "demo58",
        6,
        239,
        7,
        311,
    },
    {
        "demoCase59",
        "demo59",
        7,
        243,
        8,
        316,
    },
    {
        "demoCase60",
        "demo60",
        8,
        247,
        3,
        321,
    },
    {
        "demoCase61",
        "demo61",
        9,
        251,
        4,
        326,
    },
    {
        "demoCase62",
        "demo62",
        10,
        255,
        5,
        331,
    },
    {
        "demoCase63",
        "demo63",
        2,
        259,
        6,
        336,
    },
    {
        "demoCase64",
        "demo64",
        3,
        263,
        7,
        341,
    },
    {
        "demoCase65",
        "demo65",
        4,
        267,
        8,
        346,
    },
    {
        "demoCase66",
        "demo66",
        5,
        271,
        3,
        351,
    },
    {
        "demoCase67",
        "demo67",
        6,
        275,
        4,
        356,
    },
    {
        "demoCase68",
        "demo68",
        7,
        279,
        5,
        361,
    },
    {
        "demoCase69",
        "demo69",
        8,
        283,
        6,
        366,
    },
    {
        "demoCase70",
        "demo70",
        9,
        287,
        7,
        371,
    },
    {
        "demoCase71",
        "demo71",
        10,
        291,
        8,
        376,
    },
    {
        "demoCase72",
        "demo72",
        2,
        295,
        3,
        381,
    },
    {
        "demoCase73",
        "demo73",
        3,
        299,
        4,
        386,
    },
    {
        "demoCase74",
        "demo74",
        4,
        303,
        5,
        391,
    },
    {
        "demoCase75",
        "demo75",
        5,
        307,
        6,
        396,
    },
    {
        "demoCase76",
        "demo76",
        6,
        311,
        7,
        401,
    },
    {
        "demoCase77",
        "demo77",
        7,
        315,
        8,
        406,
    },
    {
        "demoCase78",
        "demo78",
        8,
        319,
        3,
        411,
    },
    {
        "demoCase79",
        "demo79",
        9,
        323,
        4,
        416,
    },
    {
        "demoCase80",
        "demo80",
        10,
        327,
        5,
        421,
    },
    {
        "demoCase81",
        "demo81",
        2,
        331,
        6,
        426,
    },
    {
        "demoCase82",
        "demo82",
        3,
        335,
        7,
        431,
    },
    {
        "demoCase83",
        "demo83",
        4,
        339,
        8,
        436,
    },
    {
        "demoCase84",
        "demo84",
        5,
        343,
        3,
        441,
    },
    {
        "demoCase85",
        "demo85",
        6,
        347,
        4,
        446,
    },
    {
        "demoCase86",
        "demo86",
        7,
        351,
        5,
        451,
    },
    {
        "demoCase87",
        "demo87",
        8,
        355,
        6,
        456,
    },
    {
        "demoCase88",
        "demo88",
        9,
        359,
        7,
        461,
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

bool ReadUtf8(napi_env env, napi_value value, const char* message, std::string* result)
{
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, value, &type) != napi_ok) {
        return false;
    }
    if (type != napi_string) {
        napi_throw_type_error(env, nullptr, message);
        return false;
    }
    size_t length = 0;
    if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok) {
        return false;
    }
    std::string buffer(length + K_NULL_TERMINATOR_SIZE, '\0');
    if (napi_get_value_string_utf8(env, value, buffer.data(), buffer.size(), &length) != napi_ok) {
        return false;
    }
    buffer.resize(length);
    *result = buffer;
    return true;
}

bool ReadNamedString(napi_env env, napi_value object, const char* name, std::string* result)
{
    napi_value property = nullptr;
    bool hasProperty = false;
    if (napi_has_named_property(env, object, name, &hasProperty) != napi_ok || !hasProperty) {
        napi_throw_error(env, nullptr, "title property is required");
        return false;
    }
    if (napi_get_named_property(env, object, name, &property) != napi_ok) {
        return false;
    }
    return ReadUtf8(env, property, "title must be a string", result);
}

bool ReadNamedInt32(napi_env env, napi_value object, const char* name, int32_t* result)
{
    napi_value property = nullptr;
    bool hasProperty = false;
    if (napi_has_named_property(env, object, name, &hasProperty) != napi_ok || !hasProperty) {
        napi_throw_error(env, nullptr, "count property is required");
        return false;
    }
    if (napi_get_named_property(env, object, name, &property) != napi_ok) {
        return false;
    }
    if (napi_get_value_int32(env, property, result) != napi_ok) {
        napi_throw_type_error(env, nullptr, "count must be a number");
        return false;
    }
    return true;
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

bool SetNamedString(napi_env env, napi_value object, const char* name, const std::string& value)
{
    napi_value napiValue = nullptr;
    if (napi_create_string_utf8(env, value.c_str(), value.size(), &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool CallJsonMethod(napi_env env, const char* name, napi_value arg, napi_value* result)
{
    napi_value global = nullptr;
    napi_value json = nullptr;
    napi_value method = nullptr;
    if (napi_get_global(env, &global) != napi_ok) {
        return false;
    }
    if (napi_get_named_property(env, global, "JSON", &json) != napi_ok) {
        return false;
    }
    if (napi_get_named_property(env, json, name, &method) != napi_ok) {
        return false;
    }
    return napi_call_function(env, json, method, 1, &arg, result) == napi_ok;
}

napi_value CreateDemoSummary(napi_env env, const DemoSummaryData& summary)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", summary.name);
    SetNamedString(env, result, "title", summary.title);
    SetNamedString(env, result, "decorated", summary.decorated);
    SetNamedInt32(env, result, "count", summary.count);
    SetNamedInt32(env, result, "score", summary.score);
    SetNamedBool(env, result, "passed", summary.passed);
    return result;
}

static napi_value RunDemoCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_DEMO_ARG_COUNT;
    napi_value args[K_DEMO_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_DEMO_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid demo case");
        return nullptr;
    }
    if (argc < K_DEMO_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "record is required");
        return nullptr;
    }

    std::string title;
    int32_t count = 0;
    if (!ReadNamedString(env, args[K_RECORD_ARG_INDEX], "title", &title)) {
        return nullptr;
    }
    if (!ReadNamedInt32(env, args[K_RECORD_ARG_INDEX], "count", &count)) {
        return nullptr;
    }

    const auto& spec = K_DEMO_CASE_SPECS[caseIndex];
    std::string decorated = spec.tag;
    for (int32_t step = 0; step < spec.repeat; step++) {
        decorated += ":";
        decorated += title;
    }
    const int32_t score = count * spec.gain + spec.offset + static_cast<int32_t>(decorated.size());
    const DemoSummaryData summary = {spec.name, title, decorated, count, score, score >= spec.threshold};
    return CreateDemoSummary(env, summary);
}

}  // namespace

napi_value RegisterDemoTests(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_DEMO_CASE_COUNT);
    exportNames.reserve(K_DEMO_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_DEMO_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testDemoCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(),
                                                          nullptr,
                                                          RunDemoCase,
                                                          nullptr,
                                                          nullptr,
                                                          nullptr,
                                                          napi_default,
                                                          reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}
