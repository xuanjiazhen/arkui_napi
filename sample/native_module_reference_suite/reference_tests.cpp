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

constexpr size_t K_REFERENCE_CASE_COUNT = 88;
constexpr size_t K_REFERENCE_ARG_COUNT = 2;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr size_t K_VALUE_ARG_INDEX = 0;
constexpr size_t K_LABEL_ARG_INDEX = 1;
constexpr int K_CASE_NUMBER_WIDTH = 2;
constexpr size_t K_NULL_TERMINATOR_SIZE = 1;
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

struct ReferenceCaseSpec {
    const char* name;
    const char* slot;
    int32_t refCount;
    int32_t seed;
    int32_t multiplier;
    int32_t threshold;
};

static const ReferenceCaseSpec K_REFERENCE_CASE_SPECS[] = {
    {
        "referenceCase01",
        "slot01",
        2,
        19,
        4,
        41,
    },
    {
        "referenceCase02",
        "slot02",
        3,
        23,
        5,
        47,
    },
    {
        "referenceCase03",
        "slot03",
        4,
        27,
        6,
        53,
    },
    {
        "referenceCase04",
        "slot04",
        5,
        31,
        7,
        59,
    },
    {
        "referenceCase05",
        "slot05",
        1,
        35,
        8,
        65,
    },
    {
        "referenceCase06",
        "slot06",
        2,
        39,
        9,
        71,
    },
    {
        "referenceCase07",
        "slot07",
        3,
        43,
        3,
        77,
    },
    {
        "referenceCase08",
        "slot08",
        4,
        47,
        4,
        83,
    },
    {
        "referenceCase09",
        "slot09",
        5,
        51,
        5,
        89,
    },
    {
        "referenceCase10",
        "slot10",
        1,
        55,
        6,
        95,
    },
    {
        "referenceCase11",
        "slot11",
        2,
        59,
        7,
        101,
    },
    {
        "referenceCase12",
        "slot12",
        3,
        63,
        8,
        107,
    },
    {
        "referenceCase13",
        "slot13",
        4,
        67,
        9,
        113,
    },
    {
        "referenceCase14",
        "slot14",
        5,
        71,
        3,
        119,
    },
    {
        "referenceCase15",
        "slot15",
        1,
        75,
        4,
        125,
    },
    {
        "referenceCase16",
        "slot16",
        2,
        79,
        5,
        131,
    },
    {
        "referenceCase17",
        "slot17",
        3,
        83,
        6,
        137,
    },
    {
        "referenceCase18",
        "slot18",
        4,
        87,
        7,
        143,
    },
    {
        "referenceCase19",
        "slot19",
        5,
        91,
        8,
        149,
    },
    {
        "referenceCase20",
        "slot20",
        1,
        95,
        9,
        155,
    },
    {
        "referenceCase21",
        "slot21",
        2,
        99,
        3,
        161,
    },
    {
        "referenceCase22",
        "slot22",
        3,
        103,
        4,
        167,
    },
    {
        "referenceCase23",
        "slot23",
        4,
        107,
        5,
        173,
    },
    {
        "referenceCase24",
        "slot24",
        5,
        111,
        6,
        179,
    },
    {
        "referenceCase25",
        "slot25",
        1,
        115,
        7,
        185,
    },
    {
        "referenceCase26",
        "slot26",
        2,
        119,
        8,
        191,
    },
    {
        "referenceCase27",
        "slot27",
        3,
        123,
        9,
        197,
    },
    {
        "referenceCase28",
        "slot28",
        4,
        127,
        3,
        203,
    },
    {
        "referenceCase29",
        "slot29",
        5,
        131,
        4,
        209,
    },
    {
        "referenceCase30",
        "slot30",
        1,
        135,
        5,
        215,
    },
    {
        "referenceCase31",
        "slot31",
        2,
        139,
        6,
        221,
    },
    {
        "referenceCase32",
        "slot32",
        3,
        143,
        7,
        227,
    },
    {
        "referenceCase33",
        "slot33",
        4,
        147,
        8,
        233,
    },
    {
        "referenceCase34",
        "slot34",
        5,
        151,
        9,
        239,
    },
    {
        "referenceCase35",
        "slot35",
        1,
        155,
        3,
        245,
    },
    {
        "referenceCase36",
        "slot36",
        2,
        159,
        4,
        251,
    },
    {
        "referenceCase37",
        "slot37",
        3,
        163,
        5,
        257,
    },
    {
        "referenceCase38",
        "slot38",
        4,
        167,
        6,
        263,
    },
    {
        "referenceCase39",
        "slot39",
        5,
        171,
        7,
        269,
    },
    {
        "referenceCase40",
        "slot40",
        1,
        175,
        8,
        275,
    },
    {
        "referenceCase41",
        "slot41",
        2,
        179,
        9,
        281,
    },
    {
        "referenceCase42",
        "slot42",
        3,
        183,
        3,
        287,
    },
    {
        "referenceCase43",
        "slot43",
        4,
        187,
        4,
        293,
    },
    {
        "referenceCase44",
        "slot44",
        5,
        191,
        5,
        299,
    },
    {
        "referenceCase45",
        "slot45",
        1,
        195,
        6,
        305,
    },
    {
        "referenceCase46",
        "slot46",
        2,
        199,
        7,
        311,
    },
    {
        "referenceCase47",
        "slot47",
        3,
        203,
        8,
        317,
    },
    {
        "referenceCase48",
        "slot48",
        4,
        207,
        9,
        323,
    },
    {
        "referenceCase49",
        "slot49",
        5,
        211,
        3,
        329,
    },
    {
        "referenceCase50",
        "slot50",
        1,
        215,
        4,
        335,
    },
    {
        "referenceCase51",
        "slot51",
        2,
        219,
        5,
        341,
    },
    {
        "referenceCase52",
        "slot52",
        3,
        223,
        6,
        347,
    },
    {
        "referenceCase53",
        "slot53",
        4,
        227,
        7,
        353,
    },
    {
        "referenceCase54",
        "slot54",
        5,
        231,
        8,
        359,
    },
    {
        "referenceCase55",
        "slot55",
        1,
        235,
        9,
        365,
    },
    {
        "referenceCase56",
        "slot56",
        2,
        239,
        3,
        371,
    },
    {
        "referenceCase57",
        "slot57",
        3,
        243,
        4,
        377,
    },
    {
        "referenceCase58",
        "slot58",
        4,
        247,
        5,
        383,
    },
    {
        "referenceCase59",
        "slot59",
        5,
        251,
        6,
        389,
    },
    {
        "referenceCase60",
        "slot60",
        1,
        255,
        7,
        395,
    },
    {
        "referenceCase61",
        "slot61",
        2,
        259,
        8,
        401,
    },
    {
        "referenceCase62",
        "slot62",
        3,
        263,
        9,
        407,
    },
    {
        "referenceCase63",
        "slot63",
        4,
        267,
        3,
        413,
    },
    {
        "referenceCase64",
        "slot64",
        5,
        271,
        4,
        419,
    },
    {
        "referenceCase65",
        "slot65",
        1,
        275,
        5,
        425,
    },
    {
        "referenceCase66",
        "slot66",
        2,
        279,
        6,
        431,
    },
    {
        "referenceCase67",
        "slot67",
        3,
        283,
        7,
        437,
    },
    {
        "referenceCase68",
        "slot68",
        4,
        287,
        8,
        443,
    },
    {
        "referenceCase69",
        "slot69",
        5,
        291,
        9,
        449,
    },
    {
        "referenceCase70",
        "slot70",
        1,
        295,
        3,
        455,
    },
    {
        "referenceCase71",
        "slot71",
        2,
        299,
        4,
        461,
    },
    {
        "referenceCase72",
        "slot72",
        3,
        303,
        5,
        467,
    },
    {
        "referenceCase73",
        "slot73",
        4,
        307,
        6,
        473,
    },
    {
        "referenceCase74",
        "slot74",
        5,
        311,
        7,
        479,
    },
    {
        "referenceCase75",
        "slot75",
        1,
        315,
        8,
        485,
    },
    {
        "referenceCase76",
        "slot76",
        2,
        319,
        9,
        491,
    },
    {
        "referenceCase77",
        "slot77",
        3,
        323,
        3,
        497,
    },
    {
        "referenceCase78",
        "slot78",
        4,
        327,
        4,
        503,
    },
    {
        "referenceCase79",
        "slot79",
        5,
        331,
        5,
        509,
    },
    {
        "referenceCase80",
        "slot80",
        1,
        335,
        6,
        515,
    },
    {
        "referenceCase81",
        "slot81",
        2,
        339,
        7,
        521,
    },
    {
        "referenceCase82",
        "slot82",
        3,
        343,
        8,
        527,
    },
    {
        "referenceCase83",
        "slot83",
        4,
        347,
        9,
        533,
    },
    {
        "referenceCase84",
        "slot84",
        5,
        351,
        3,
        539,
    },
    {
        "referenceCase85",
        "slot85",
        1,
        355,
        4,
        545,
    },
    {
        "referenceCase86",
        "slot86",
        2,
        359,
        5,
        551,
    },
    {
        "referenceCase87",
        "slot87",
        3,
        363,
        6,
        557,
    },
    {
        "referenceCase88",
        "slot88",
        4,
        367,
        7,
        563,
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

napi_value CreateReferenceSummary(
    napi_env env, const ReferenceCaseSpec& spec, const std::string& label, napi_value referencedValue, int32_t score)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "label", label);
    SetNamedInt32(env, result, "score", score);
    SetNamedBool(env, result, "passed", score >= spec.threshold);
    NAPI_CALL(env, napi_set_named_property(env, result, spec.slot, referencedValue));
    return result;
}

static napi_value RunReferenceCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_REFERENCE_ARG_COUNT;
    napi_value args[K_REFERENCE_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_REFERENCE_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid reference case");
        return nullptr;
    }
    if (argc < K_REFERENCE_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "value and label are required");
        return nullptr;
    }

    std::string label;
    if (!ReadUtf8(env, args[K_LABEL_ARG_INDEX], "label must be a string", &label)) {
        return nullptr;
    }

    const auto& spec = K_REFERENCE_CASE_SPECS[caseIndex];
    napi_ref reference = nullptr;
    NAPI_CALL(
        env, napi_create_reference(env, args[K_VALUE_ARG_INDEX], static_cast<uint32_t>(spec.refCount), &reference));
    napi_value referencedValue = nullptr;
    NAPI_CALL(env, napi_get_reference_value(env, reference, &referencedValue));
    NAPI_CALL(env, napi_delete_reference(env, reference));
    const int32_t score =
        static_cast<int32_t>(label.size()) * spec.multiplier + spec.seed + static_cast<int32_t>(spec.refCount);
    return CreateReferenceSummary(env, spec, label, referencedValue, score);
}

}  // namespace

static napi_value InitReferenceSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_REFERENCE_CASE_COUNT);
    exportNames.reserve(K_REFERENCE_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_REFERENCE_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testReferenceCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunReferenceCase,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_referenceSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitReferenceSuite,
    .nm_modname = "reference_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterReferenceSuiteModule(void)
{
    napi_module_register(&g_referenceSuiteModule);
}
