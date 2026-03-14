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

constexpr size_t K_JSON_CASE_COUNT = 88;
constexpr size_t K_JSON_ARG_COUNT = 1;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr size_t K_INPUT_ARG_INDEX = 0;
constexpr int K_CASE_NUMBER_WIDTH = 2;
constexpr size_t K_NULL_TERMINATOR_SIZE = 1;

struct JsonCaseSpec {
    const char* name;
    const char* field;
    const char* tag;
    int32_t delta;
    int32_t repeat;
    int32_t threshold;
};

static const JsonCaseSpec K_JSON_CASE_SPECS[] = {
    {
        "jsonCase01",
        "field01",
        "tag01",
        8,
        3,
        24,
    },
    {
        "jsonCase02",
        "field02",
        "tag02",
        11,
        4,
        31,
    },
    {
        "jsonCase03",
        "field03",
        "tag03",
        14,
        5,
        38,
    },
    {
        "jsonCase04",
        "field04",
        "tag04",
        17,
        6,
        45,
    },
    {
        "jsonCase05",
        "field05",
        "tag05",
        20,
        2,
        52,
    },
    {
        "jsonCase06",
        "field06",
        "tag06",
        23,
        3,
        59,
    },
    {
        "jsonCase07",
        "field07",
        "tag07",
        26,
        4,
        66,
    },
    {
        "jsonCase08",
        "field08",
        "tag08",
        29,
        5,
        73,
    },
    {
        "jsonCase09",
        "field09",
        "tag09",
        32,
        6,
        80,
    },
    {
        "jsonCase10",
        "field10",
        "tag10",
        35,
        2,
        87,
    },
    {
        "jsonCase11",
        "field11",
        "tag11",
        38,
        3,
        94,
    },
    {
        "jsonCase12",
        "field12",
        "tag12",
        41,
        4,
        101,
    },
    {
        "jsonCase13",
        "field13",
        "tag13",
        44,
        5,
        108,
    },
    {
        "jsonCase14",
        "field14",
        "tag14",
        47,
        6,
        115,
    },
    {
        "jsonCase15",
        "field15",
        "tag15",
        50,
        2,
        122,
    },
    {
        "jsonCase16",
        "field16",
        "tag16",
        53,
        3,
        129,
    },
    {
        "jsonCase17",
        "field17",
        "tag17",
        56,
        4,
        136,
    },
    {
        "jsonCase18",
        "field18",
        "tag18",
        59,
        5,
        143,
    },
    {
        "jsonCase19",
        "field19",
        "tag19",
        62,
        6,
        150,
    },
    {
        "jsonCase20",
        "field20",
        "tag20",
        65,
        2,
        157,
    },
    {
        "jsonCase21",
        "field21",
        "tag21",
        68,
        3,
        164,
    },
    {
        "jsonCase22",
        "field22",
        "tag22",
        71,
        4,
        171,
    },
    {
        "jsonCase23",
        "field23",
        "tag23",
        74,
        5,
        178,
    },
    {
        "jsonCase24",
        "field24",
        "tag24",
        77,
        6,
        185,
    },
    {
        "jsonCase25",
        "field25",
        "tag25",
        80,
        2,
        192,
    },
    {
        "jsonCase26",
        "field26",
        "tag26",
        83,
        3,
        199,
    },
    {
        "jsonCase27",
        "field27",
        "tag27",
        86,
        4,
        206,
    },
    {
        "jsonCase28",
        "field28",
        "tag28",
        89,
        5,
        213,
    },
    {
        "jsonCase29",
        "field29",
        "tag29",
        92,
        6,
        220,
    },
    {
        "jsonCase30",
        "field30",
        "tag30",
        95,
        2,
        227,
    },
    {
        "jsonCase31",
        "field31",
        "tag31",
        98,
        3,
        234,
    },
    {
        "jsonCase32",
        "field32",
        "tag32",
        101,
        4,
        241,
    },
    {
        "jsonCase33",
        "field33",
        "tag33",
        104,
        5,
        248,
    },
    {
        "jsonCase34",
        "field34",
        "tag34",
        107,
        6,
        255,
    },
    {
        "jsonCase35",
        "field35",
        "tag35",
        110,
        2,
        262,
    },
    {
        "jsonCase36",
        "field36",
        "tag36",
        113,
        3,
        269,
    },
    {
        "jsonCase37",
        "field37",
        "tag37",
        116,
        4,
        276,
    },
    {
        "jsonCase38",
        "field38",
        "tag38",
        119,
        5,
        283,
    },
    {
        "jsonCase39",
        "field39",
        "tag39",
        122,
        6,
        290,
    },
    {
        "jsonCase40",
        "field40",
        "tag40",
        125,
        2,
        297,
    },
    {
        "jsonCase41",
        "field41",
        "tag41",
        128,
        3,
        304,
    },
    {
        "jsonCase42",
        "field42",
        "tag42",
        131,
        4,
        311,
    },
    {
        "jsonCase43",
        "field43",
        "tag43",
        134,
        5,
        318,
    },
    {
        "jsonCase44",
        "field44",
        "tag44",
        137,
        6,
        325,
    },
    {
        "jsonCase45",
        "field45",
        "tag45",
        140,
        2,
        332,
    },
    {
        "jsonCase46",
        "field46",
        "tag46",
        143,
        3,
        339,
    },
    {
        "jsonCase47",
        "field47",
        "tag47",
        146,
        4,
        346,
    },
    {
        "jsonCase48",
        "field48",
        "tag48",
        149,
        5,
        353,
    },
    {
        "jsonCase49",
        "field49",
        "tag49",
        152,
        6,
        360,
    },
    {
        "jsonCase50",
        "field50",
        "tag50",
        155,
        2,
        367,
    },
    {
        "jsonCase51",
        "field51",
        "tag51",
        158,
        3,
        374,
    },
    {
        "jsonCase52",
        "field52",
        "tag52",
        161,
        4,
        381,
    },
    {
        "jsonCase53",
        "field53",
        "tag53",
        164,
        5,
        388,
    },
    {
        "jsonCase54",
        "field54",
        "tag54",
        167,
        6,
        395,
    },
    {
        "jsonCase55",
        "field55",
        "tag55",
        170,
        2,
        402,
    },
    {
        "jsonCase56",
        "field56",
        "tag56",
        173,
        3,
        409,
    },
    {
        "jsonCase57",
        "field57",
        "tag57",
        176,
        4,
        416,
    },
    {
        "jsonCase58",
        "field58",
        "tag58",
        179,
        5,
        423,
    },
    {
        "jsonCase59",
        "field59",
        "tag59",
        182,
        6,
        430,
    },
    {
        "jsonCase60",
        "field60",
        "tag60",
        185,
        2,
        437,
    },
    {
        "jsonCase61",
        "field61",
        "tag61",
        188,
        3,
        444,
    },
    {
        "jsonCase62",
        "field62",
        "tag62",
        191,
        4,
        451,
    },
    {
        "jsonCase63",
        "field63",
        "tag63",
        194,
        5,
        458,
    },
    {
        "jsonCase64",
        "field64",
        "tag64",
        197,
        6,
        465,
    },
    {
        "jsonCase65",
        "field65",
        "tag65",
        200,
        2,
        472,
    },
    {
        "jsonCase66",
        "field66",
        "tag66",
        203,
        3,
        479,
    },
    {
        "jsonCase67",
        "field67",
        "tag67",
        206,
        4,
        486,
    },
    {
        "jsonCase68",
        "field68",
        "tag68",
        209,
        5,
        493,
    },
    {
        "jsonCase69",
        "field69",
        "tag69",
        212,
        6,
        500,
    },
    {
        "jsonCase70",
        "field70",
        "tag70",
        215,
        2,
        507,
    },
    {
        "jsonCase71",
        "field71",
        "tag71",
        218,
        3,
        514,
    },
    {
        "jsonCase72",
        "field72",
        "tag72",
        221,
        4,
        521,
    },
    {
        "jsonCase73",
        "field73",
        "tag73",
        224,
        5,
        528,
    },
    {
        "jsonCase74",
        "field74",
        "tag74",
        227,
        6,
        535,
    },
    {
        "jsonCase75",
        "field75",
        "tag75",
        230,
        2,
        542,
    },
    {
        "jsonCase76",
        "field76",
        "tag76",
        233,
        3,
        549,
    },
    {
        "jsonCase77",
        "field77",
        "tag77",
        236,
        4,
        556,
    },
    {
        "jsonCase78",
        "field78",
        "tag78",
        239,
        5,
        563,
    },
    {
        "jsonCase79",
        "field79",
        "tag79",
        242,
        6,
        570,
    },
    {
        "jsonCase80",
        "field80",
        "tag80",
        245,
        2,
        577,
    },
    {
        "jsonCase81",
        "field81",
        "tag81",
        248,
        3,
        584,
    },
    {
        "jsonCase82",
        "field82",
        "tag82",
        251,
        4,
        591,
    },
    {
        "jsonCase83",
        "field83",
        "tag83",
        254,
        5,
        598,
    },
    {
        "jsonCase84",
        "field84",
        "tag84",
        257,
        6,
        605,
    },
    {
        "jsonCase85",
        "field85",
        "tag85",
        260,
        2,
        612,
    },
    {
        "jsonCase86",
        "field86",
        "tag86",
        263,
        3,
        619,
    },
    {
        "jsonCase87",
        "field87",
        "tag87",
        266,
        4,
        626,
    },
    {
        "jsonCase88",
        "field88",
        "tag88",
        269,
        5,
        633,
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

napi_value CreateJsonSummary(napi_env env, const JsonCaseSpec& spec, const std::string& raw, const std::string& text,
                             int32_t score)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "raw", raw);
    SetNamedString(env, result, "text", text);
    SetNamedInt32(env, result, "score", score);
    SetNamedBool(env, result, "passed", score >= spec.threshold);
    return result;
}

static napi_value RunJsonCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_JSON_ARG_COUNT;
    napi_value args[K_JSON_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_JSON_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid json case");
        return nullptr;
    }
    if (argc < K_JSON_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "json text is required");
        return nullptr;
    }

    std::string raw;
    if (!ReadUtf8(env, args[K_INPUT_ARG_INDEX], "json text must be a string", &raw)) {
        return nullptr;
    }

    const auto& spec = K_JSON_CASE_SPECS[caseIndex];
    napi_value parsed = nullptr;
    if (!CallJsonMethod(env, "parse", args[K_INPUT_ARG_INDEX], &parsed)) {
        napi_throw_error(env, nullptr, "invalid json text");
        return nullptr;
    }
    napi_value marker = nullptr;
    napi_value count = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, spec.tag, NAPI_AUTO_LENGTH, &marker));
    NAPI_CALL(env, napi_create_int32(env, spec.delta, &count));
    NAPI_CALL(env, napi_set_named_property(env, parsed, spec.field, marker));
    NAPI_CALL(env, napi_set_named_property(env, parsed, "count", count));

    napi_value output = nullptr;
    if (!CallJsonMethod(env, "stringify", parsed, &output)) {
        napi_throw_error(env, nullptr, "stringify failed");
        return nullptr;
    }
    std::string text;
    if (!ReadUtf8(env, output, "stringify failed", &text)) {
        return nullptr;
    }

    int32_t score = spec.delta;
    for (int32_t step = 0; step < spec.repeat; step++) {
        score += static_cast<int32_t>(text.size()) + step;
    }
    return CreateJsonSummary(env, spec, raw, text, score);
}

}  // namespace

napi_value RegisterJsonTests(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_JSON_CASE_COUNT);
    exportNames.reserve(K_JSON_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_JSON_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testJsonCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(),
                                                          nullptr,
                                                          RunJsonCase,
                                                          nullptr,
                                                          nullptr,
                                                          nullptr,
                                                          napi_default,
                                                          reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}
