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

constexpr size_t K_KVSTORE_CASE_COUNT = 88;
constexpr size_t K_KVSTORE_ARG_COUNT = 1;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr size_t K_STORE_ARG_INDEX = 0;
constexpr int K_CASE_NUMBER_WIDTH = 2;
constexpr size_t K_NULL_TERMINATOR_SIZE = 1;
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

struct KvstoreCaseSpec {
    const char* name;
    const char* slot;
    int32_t repeat;
    int32_t seed;
    int32_t step;
    int32_t threshold;
};

static const KvstoreCaseSpec K_KVSTORE_CASE_SPECS[] = {
    {
        "kvstoreCase01",
        "slot01",
        3,
        16,
        5,
        33,
    },
    {
        "kvstoreCase02",
        "slot02",
        4,
        19,
        6,
        37,
    },
    {
        "kvstoreCase03",
        "slot03",
        5,
        22,
        7,
        41,
    },
    {
        "kvstoreCase04",
        "slot04",
        6,
        25,
        8,
        45,
    },
    {
        "kvstoreCase05",
        "slot05",
        2,
        28,
        9,
        49,
    },
    {
        "kvstoreCase06",
        "slot06",
        3,
        31,
        4,
        53,
    },
    {
        "kvstoreCase07",
        "slot07",
        4,
        34,
        5,
        57,
    },
    {
        "kvstoreCase08",
        "slot08",
        5,
        37,
        6,
        61,
    },
    {
        "kvstoreCase09",
        "slot09",
        6,
        40,
        7,
        65,
    },
    {
        "kvstoreCase10",
        "slot10",
        2,
        43,
        8,
        69,
    },
    {
        "kvstoreCase11",
        "slot11",
        3,
        46,
        9,
        73,
    },
    {
        "kvstoreCase12",
        "slot12",
        4,
        49,
        4,
        77,
    },
    {
        "kvstoreCase13",
        "slot13",
        5,
        52,
        5,
        81,
    },
    {
        "kvstoreCase14",
        "slot14",
        6,
        55,
        6,
        85,
    },
    {
        "kvstoreCase15",
        "slot15",
        2,
        58,
        7,
        89,
    },
    {
        "kvstoreCase16",
        "slot16",
        3,
        61,
        8,
        93,
    },
    {
        "kvstoreCase17",
        "slot17",
        4,
        64,
        9,
        97,
    },
    {
        "kvstoreCase18",
        "slot18",
        5,
        67,
        4,
        101,
    },
    {
        "kvstoreCase19",
        "slot19",
        6,
        70,
        5,
        105,
    },
    {
        "kvstoreCase20",
        "slot20",
        2,
        73,
        6,
        109,
    },
    {
        "kvstoreCase21",
        "slot21",
        3,
        76,
        7,
        113,
    },
    {
        "kvstoreCase22",
        "slot22",
        4,
        79,
        8,
        117,
    },
    {
        "kvstoreCase23",
        "slot23",
        5,
        82,
        9,
        121,
    },
    {
        "kvstoreCase24",
        "slot24",
        6,
        85,
        4,
        125,
    },
    {
        "kvstoreCase25",
        "slot25",
        2,
        88,
        5,
        129,
    },
    {
        "kvstoreCase26",
        "slot26",
        3,
        91,
        6,
        133,
    },
    {
        "kvstoreCase27",
        "slot27",
        4,
        94,
        7,
        137,
    },
    {
        "kvstoreCase28",
        "slot28",
        5,
        97,
        8,
        141,
    },
    {
        "kvstoreCase29",
        "slot29",
        6,
        100,
        9,
        145,
    },
    {
        "kvstoreCase30",
        "slot30",
        2,
        103,
        4,
        149,
    },
    {
        "kvstoreCase31",
        "slot31",
        3,
        106,
        5,
        153,
    },
    {
        "kvstoreCase32",
        "slot32",
        4,
        109,
        6,
        157,
    },
    {
        "kvstoreCase33",
        "slot33",
        5,
        112,
        7,
        161,
    },
    {
        "kvstoreCase34",
        "slot34",
        6,
        115,
        8,
        165,
    },
    {
        "kvstoreCase35",
        "slot35",
        2,
        118,
        9,
        169,
    },
    {
        "kvstoreCase36",
        "slot36",
        3,
        121,
        4,
        173,
    },
    {
        "kvstoreCase37",
        "slot37",
        4,
        124,
        5,
        177,
    },
    {
        "kvstoreCase38",
        "slot38",
        5,
        127,
        6,
        181,
    },
    {
        "kvstoreCase39",
        "slot39",
        6,
        130,
        7,
        185,
    },
    {
        "kvstoreCase40",
        "slot40",
        2,
        133,
        8,
        189,
    },
    {
        "kvstoreCase41",
        "slot41",
        3,
        136,
        9,
        193,
    },
    {
        "kvstoreCase42",
        "slot42",
        4,
        139,
        4,
        197,
    },
    {
        "kvstoreCase43",
        "slot43",
        5,
        142,
        5,
        201,
    },
    {
        "kvstoreCase44",
        "slot44",
        6,
        145,
        6,
        205,
    },
    {
        "kvstoreCase45",
        "slot45",
        2,
        148,
        7,
        209,
    },
    {
        "kvstoreCase46",
        "slot46",
        3,
        151,
        8,
        213,
    },
    {
        "kvstoreCase47",
        "slot47",
        4,
        154,
        9,
        217,
    },
    {
        "kvstoreCase48",
        "slot48",
        5,
        157,
        4,
        221,
    },
    {
        "kvstoreCase49",
        "slot49",
        6,
        160,
        5,
        225,
    },
    {
        "kvstoreCase50",
        "slot50",
        2,
        163,
        6,
        229,
    },
    {
        "kvstoreCase51",
        "slot51",
        3,
        166,
        7,
        233,
    },
    {
        "kvstoreCase52",
        "slot52",
        4,
        169,
        8,
        237,
    },
    {
        "kvstoreCase53",
        "slot53",
        5,
        172,
        9,
        241,
    },
    {
        "kvstoreCase54",
        "slot54",
        6,
        175,
        4,
        245,
    },
    {
        "kvstoreCase55",
        "slot55",
        2,
        178,
        5,
        249,
    },
    {
        "kvstoreCase56",
        "slot56",
        3,
        181,
        6,
        253,
    },
    {
        "kvstoreCase57",
        "slot57",
        4,
        184,
        7,
        257,
    },
    {
        "kvstoreCase58",
        "slot58",
        5,
        187,
        8,
        261,
    },
    {
        "kvstoreCase59",
        "slot59",
        6,
        190,
        9,
        265,
    },
    {
        "kvstoreCase60",
        "slot60",
        2,
        193,
        4,
        269,
    },
    {
        "kvstoreCase61",
        "slot61",
        3,
        196,
        5,
        273,
    },
    {
        "kvstoreCase62",
        "slot62",
        4,
        199,
        6,
        277,
    },
    {
        "kvstoreCase63",
        "slot63",
        5,
        202,
        7,
        281,
    },
    {
        "kvstoreCase64",
        "slot64",
        6,
        205,
        8,
        285,
    },
    {
        "kvstoreCase65",
        "slot65",
        2,
        208,
        9,
        289,
    },
    {
        "kvstoreCase66",
        "slot66",
        3,
        211,
        4,
        293,
    },
    {
        "kvstoreCase67",
        "slot67",
        4,
        214,
        5,
        297,
    },
    {
        "kvstoreCase68",
        "slot68",
        5,
        217,
        6,
        301,
    },
    {
        "kvstoreCase69",
        "slot69",
        6,
        220,
        7,
        305,
    },
    {
        "kvstoreCase70",
        "slot70",
        2,
        223,
        8,
        309,
    },
    {
        "kvstoreCase71",
        "slot71",
        3,
        226,
        9,
        313,
    },
    {
        "kvstoreCase72",
        "slot72",
        4,
        229,
        4,
        317,
    },
    {
        "kvstoreCase73",
        "slot73",
        5,
        232,
        5,
        321,
    },
    {
        "kvstoreCase74",
        "slot74",
        6,
        235,
        6,
        325,
    },
    {
        "kvstoreCase75",
        "slot75",
        2,
        238,
        7,
        329,
    },
    {
        "kvstoreCase76",
        "slot76",
        3,
        241,
        8,
        333,
    },
    {
        "kvstoreCase77",
        "slot77",
        4,
        244,
        9,
        337,
    },
    {
        "kvstoreCase78",
        "slot78",
        5,
        247,
        4,
        341,
    },
    {
        "kvstoreCase79",
        "slot79",
        6,
        250,
        5,
        345,
    },
    {
        "kvstoreCase80",
        "slot80",
        2,
        253,
        6,
        349,
    },
    {
        "kvstoreCase81",
        "slot81",
        3,
        256,
        7,
        353,
    },
    {
        "kvstoreCase82",
        "slot82",
        4,
        259,
        8,
        357,
    },
    {
        "kvstoreCase83",
        "slot83",
        5,
        262,
        9,
        361,
    },
    {
        "kvstoreCase84",
        "slot84",
        6,
        265,
        4,
        365,
    },
    {
        "kvstoreCase85",
        "slot85",
        2,
        268,
        5,
        369,
    },
    {
        "kvstoreCase86",
        "slot86",
        3,
        271,
        6,
        373,
    },
    {
        "kvstoreCase87",
        "slot87",
        4,
        274,
        7,
        377,
    },
    {
        "kvstoreCase88",
        "slot88",
        5,
        277,
        8,
        381,
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

bool ReadNamedInt32(napi_env env, napi_value object, const char* name, int32_t* result)
{
    napi_value property = nullptr;
    bool hasProperty = false;
    if (napi_has_named_property(env, object, name, &hasProperty) != napi_ok || !hasProperty) {
        napi_throw_error(env, nullptr, "value property is required");
        return false;
    }
    if (napi_get_named_property(env, object, name, &property) != napi_ok) {
        return false;
    }
    if (napi_get_value_int32(env, property, result) != napi_ok) {
        napi_throw_type_error(env, nullptr, "value must be a number");
        return false;
    }
    return true;
}

bool ReadNamedString(napi_env env, napi_value object, const char* name, std::string* result)
{
    napi_value property = nullptr;
    bool hasProperty = false;
    if (napi_has_named_property(env, object, name, &hasProperty) != napi_ok || !hasProperty) {
        napi_throw_error(env, nullptr, "key property is required");
        return false;
    }
    if (napi_get_named_property(env, object, name, &property) != napi_ok) {
        return false;
    }
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, property, &type) != napi_ok || type != napi_string) {
        napi_throw_type_error(env, nullptr, "key must be a string");
        return false;
    }
    size_t length = 0;
    if (napi_get_value_string_utf8(env, property, nullptr, 0, &length) != napi_ok) {
        return false;
    }
    std::string buffer(length + K_NULL_TERMINATOR_SIZE, '\0');
    if (napi_get_value_string_utf8(env, property, buffer.data(), buffer.size(), &length) != napi_ok) {
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

napi_value CreateKvstoreSummary(
    napi_env env, const KvstoreCaseSpec& spec, const std::string& key, int32_t storedValue, int32_t score)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "key", key);
    SetNamedInt32(env, result, "storedValue", storedValue);
    SetNamedInt32(env, result, "score", score);
    SetNamedBool(env, result, "passed", score >= spec.threshold);
    return result;
}

static napi_value RunKvstoreCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_KVSTORE_ARG_COUNT;
    napi_value args[K_KVSTORE_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_KVSTORE_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid kvstore case");
        return nullptr;
    }
    if (argc < K_KVSTORE_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "store object is required");
        return nullptr;
    }

    std::string key;
    int32_t value = 0;
    if (!ReadNamedString(env, args[K_STORE_ARG_INDEX], "key", &key)) {
        return nullptr;
    }
    if (!ReadNamedInt32(env, args[K_STORE_ARG_INDEX], "value", &value)) {
        return nullptr;
    }

    const auto& spec = K_KVSTORE_CASE_SPECS[caseIndex];
    napi_value mirror = nullptr;
    NAPI_CALL(env, napi_create_object(env, &mirror));
    for (int32_t step = 0; step < spec.repeat; step++) {
        napi_value item = nullptr;
        NAPI_CALL(env, napi_create_int32(env, value + spec.seed + step * spec.step, &item));
        NAPI_CALL(env, napi_set_named_property(env, mirror, spec.slot, item));
    }
    int32_t score = value + spec.seed + spec.repeat * spec.step + static_cast<int32_t>(key.size());
    return CreateKvstoreSummary(env, spec, key, value, score);
}

}  // namespace

static napi_value InitKvstoreSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_KVSTORE_CASE_COUNT);
    exportNames.reserve(K_KVSTORE_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_KVSTORE_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testKvstoreCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunKvstoreCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_kvstoreSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitKvstoreSuite,
    .nm_modname = "kvstore_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterKvstoreSuiteModule(void)
{
    napi_module_register(&g_kvstoreSuiteModule);
}
