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

constexpr size_t K_STORAGE_CASE_COUNT = 88;
constexpr size_t K_STORAGE_ARG_COUNT = 1;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr size_t K_LIST_ARG_INDEX = 0;
constexpr int K_CASE_NUMBER_WIDTH = 2;
constexpr int32_t K_ODD_MASK = 1;
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

struct StorageCaseSpec {
    const char* name;
    int32_t stride;
    int32_t quota;
    int32_t offset;
    int32_t multiplier;
    int32_t threshold;
};

static const StorageCaseSpec K_STORAGE_CASE_SPECS[] = {
    {
        "storageCase01",
        3,
        24,
        2,
        17,
        46,
    },
    {
        "storageCase02",
        4,
        29,
        3,
        21,
        53,
    },
    {
        "storageCase03",
        5,
        34,
        4,
        25,
        60,
    },
    {
        "storageCase04",
        6,
        39,
        5,
        29,
        67,
    },
    {
        "storageCase05",
        2,
        44,
        6,
        33,
        74,
    },
    {
        "storageCase06",
        3,
        49,
        1,
        37,
        81,
    },
    {
        "storageCase07",
        4,
        54,
        2,
        41,
        88,
    },
    {
        "storageCase08",
        5,
        59,
        3,
        45,
        95,
    },
    {
        "storageCase09",
        6,
        64,
        4,
        49,
        102,
    },
    {
        "storageCase10",
        2,
        69,
        5,
        53,
        109,
    },
    {
        "storageCase11",
        3,
        74,
        6,
        57,
        116,
    },
    {
        "storageCase12",
        4,
        79,
        1,
        61,
        123,
    },
    {
        "storageCase13",
        5,
        84,
        2,
        65,
        130,
    },
    {
        "storageCase14",
        6,
        89,
        3,
        69,
        137,
    },
    {
        "storageCase15",
        2,
        94,
        4,
        73,
        144,
    },
    {
        "storageCase16",
        3,
        99,
        5,
        77,
        151,
    },
    {
        "storageCase17",
        4,
        104,
        6,
        81,
        158,
    },
    {
        "storageCase18",
        5,
        109,
        1,
        85,
        165,
    },
    {
        "storageCase19",
        6,
        114,
        2,
        89,
        172,
    },
    {
        "storageCase20",
        2,
        119,
        3,
        93,
        179,
    },
    {
        "storageCase21",
        3,
        124,
        4,
        97,
        186,
    },
    {
        "storageCase22",
        4,
        129,
        5,
        101,
        193,
    },
    {
        "storageCase23",
        5,
        134,
        6,
        105,
        200,
    },
    {
        "storageCase24",
        6,
        139,
        1,
        109,
        207,
    },
    {
        "storageCase25",
        2,
        144,
        2,
        113,
        214,
    },
    {
        "storageCase26",
        3,
        149,
        3,
        117,
        221,
    },
    {
        "storageCase27",
        4,
        154,
        4,
        121,
        228,
    },
    {
        "storageCase28",
        5,
        159,
        5,
        125,
        235,
    },
    {
        "storageCase29",
        6,
        164,
        6,
        129,
        242,
    },
    {
        "storageCase30",
        2,
        169,
        1,
        133,
        249,
    },
    {
        "storageCase31",
        3,
        174,
        2,
        137,
        256,
    },
    {
        "storageCase32",
        4,
        179,
        3,
        141,
        263,
    },
    {
        "storageCase33",
        5,
        184,
        4,
        145,
        270,
    },
    {
        "storageCase34",
        6,
        189,
        5,
        149,
        277,
    },
    {
        "storageCase35",
        2,
        194,
        6,
        153,
        284,
    },
    {
        "storageCase36",
        3,
        199,
        1,
        157,
        291,
    },
    {
        "storageCase37",
        4,
        204,
        2,
        161,
        298,
    },
    {
        "storageCase38",
        5,
        209,
        3,
        165,
        305,
    },
    {
        "storageCase39",
        6,
        214,
        4,
        169,
        312,
    },
    {
        "storageCase40",
        2,
        219,
        5,
        173,
        319,
    },
    {
        "storageCase41",
        3,
        224,
        6,
        177,
        326,
    },
    {
        "storageCase42",
        4,
        229,
        1,
        181,
        333,
    },
    {
        "storageCase43",
        5,
        234,
        2,
        185,
        340,
    },
    {
        "storageCase44",
        6,
        239,
        3,
        189,
        347,
    },
    {
        "storageCase45",
        2,
        244,
        4,
        193,
        354,
    },
    {
        "storageCase46",
        3,
        249,
        5,
        197,
        361,
    },
    {
        "storageCase47",
        4,
        254,
        6,
        201,
        368,
    },
    {
        "storageCase48",
        5,
        259,
        1,
        205,
        375,
    },
    {
        "storageCase49",
        6,
        264,
        2,
        209,
        382,
    },
    {
        "storageCase50",
        2,
        269,
        3,
        213,
        389,
    },
    {
        "storageCase51",
        3,
        274,
        4,
        217,
        396,
    },
    {
        "storageCase52",
        4,
        279,
        5,
        221,
        403,
    },
    {
        "storageCase53",
        5,
        284,
        6,
        225,
        410,
    },
    {
        "storageCase54",
        6,
        289,
        1,
        229,
        417,
    },
    {
        "storageCase55",
        2,
        294,
        2,
        233,
        424,
    },
    {
        "storageCase56",
        3,
        299,
        3,
        237,
        431,
    },
    {
        "storageCase57",
        4,
        304,
        4,
        241,
        438,
    },
    {
        "storageCase58",
        5,
        309,
        5,
        245,
        445,
    },
    {
        "storageCase59",
        6,
        314,
        6,
        249,
        452,
    },
    {
        "storageCase60",
        2,
        319,
        1,
        253,
        459,
    },
    {
        "storageCase61",
        3,
        324,
        2,
        257,
        466,
    },
    {
        "storageCase62",
        4,
        329,
        3,
        261,
        473,
    },
    {
        "storageCase63",
        5,
        334,
        4,
        265,
        480,
    },
    {
        "storageCase64",
        6,
        339,
        5,
        269,
        487,
    },
    {
        "storageCase65",
        2,
        344,
        6,
        273,
        494,
    },
    {
        "storageCase66",
        3,
        349,
        1,
        277,
        501,
    },
    {
        "storageCase67",
        4,
        354,
        2,
        281,
        508,
    },
    {
        "storageCase68",
        5,
        359,
        3,
        285,
        515,
    },
    {
        "storageCase69",
        6,
        364,
        4,
        289,
        522,
    },
    {
        "storageCase70",
        2,
        369,
        5,
        293,
        529,
    },
    {
        "storageCase71",
        3,
        374,
        6,
        297,
        536,
    },
    {
        "storageCase72",
        4,
        379,
        1,
        301,
        543,
    },
    {
        "storageCase73",
        5,
        384,
        2,
        305,
        550,
    },
    {
        "storageCase74",
        6,
        389,
        3,
        309,
        557,
    },
    {
        "storageCase75",
        2,
        394,
        4,
        313,
        564,
    },
    {
        "storageCase76",
        3,
        399,
        5,
        317,
        571,
    },
    {
        "storageCase77",
        4,
        404,
        6,
        321,
        578,
    },
    {
        "storageCase78",
        5,
        409,
        1,
        325,
        585,
    },
    {
        "storageCase79",
        6,
        414,
        2,
        329,
        592,
    },
    {
        "storageCase80",
        2,
        419,
        3,
        333,
        599,
    },
    {
        "storageCase81",
        3,
        424,
        4,
        337,
        606,
    },
    {
        "storageCase82",
        4,
        429,
        5,
        341,
        613,
    },
    {
        "storageCase83",
        5,
        434,
        6,
        345,
        620,
    },
    {
        "storageCase84",
        6,
        439,
        1,
        349,
        627,
    },
    {
        "storageCase85",
        2,
        444,
        2,
        353,
        634,
    },
    {
        "storageCase86",
        3,
        449,
        3,
        357,
        641,
    },
    {
        "storageCase87",
        4,
        454,
        4,
        361,
        648,
    },
    {
        "storageCase88",
        5,
        459,
        5,
        365,
        655,
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

bool ReadInt32(napi_env env, napi_value value, int32_t* result)
{
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, value, &type) != napi_ok || type != napi_number) {
        return false;
    }
    return napi_get_value_int32(env, value, result) == napi_ok;
}

bool ReadInt32Array(napi_env env, napi_value value, std::vector<int32_t>* result)
{
    bool isArray = false;
    if (napi_is_array(env, value, &isArray) != napi_ok || !isArray) {
        napi_throw_type_error(env, nullptr, "values must be an array");
        return false;
    }
    uint32_t length = 0;
    if (napi_get_array_length(env, value, &length) != napi_ok) {
        return false;
    }
    result->clear();
    result->reserve(length);
    for (uint32_t index = 0; index < length; index++) {
        napi_value element = nullptr;
        int32_t current = 0;
        if (napi_get_element(env, value, index, &element) != napi_ok || !ReadInt32(env, element, &current)) {
            napi_throw_type_error(env, nullptr, "array elements must be numbers");
            return false;
        }
        result->push_back(current);
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

bool SetNamedString(napi_env env, napi_value object, const char* name, const char* value)
{
    napi_value napiValue = nullptr;
    if (napi_create_string_utf8(env, value, NAPI_AUTO_LENGTH, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

napi_value CreateStorageSummary(
    napi_env env, const StorageCaseSpec& spec, int32_t used, int32_t score, int32_t oddCount)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedInt32(env, result, "used", used);
    SetNamedInt32(env, result, "score", score);
    SetNamedInt32(env, result, "oddCount", oddCount);
    SetNamedBool(env, result, "passed", score <= spec.threshold);
    return result;
}

static napi_value RunStorageCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_STORAGE_ARG_COUNT;
    napi_value args[K_STORAGE_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_STORAGE_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid storage case");
        return nullptr;
    }
    if (argc < K_STORAGE_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "value list is required");
        return nullptr;
    }

    std::vector<int32_t> values;
    if (!ReadInt32Array(env, args[K_LIST_ARG_INDEX], &values)) {
        return nullptr;
    }

    const auto& spec = K_STORAGE_CASE_SPECS[caseIndex];
    int32_t used = 0;
    int32_t oddCount = 0;
    for (size_t index = 0; index < values.size(); index += static_cast<size_t>(spec.stride)) {
        used += (values[index] + spec.offset) * spec.multiplier;
        if ((values[index] & K_ODD_MASK) != 0) {
            oddCount++;
        }
    }
    const int32_t score = spec.quota - used + oddCount * spec.stride;
    return CreateStorageSummary(env, spec, used, score, oddCount);
}

}  // namespace

static napi_value InitStorageSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_STORAGE_CASE_COUNT);
    exportNames.reserve(K_STORAGE_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_STORAGE_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testStorageCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunStorageCase, nullptr,
            nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_storageSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitStorageSuite,
    .nm_modname = "storage_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterStorageSuiteModule(void)
{
    napi_module_register(&g_storageSuiteModule);
}
