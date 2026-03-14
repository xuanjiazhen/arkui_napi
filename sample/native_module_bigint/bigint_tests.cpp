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

constexpr size_t K_BIGINT_CASE_COUNT = 88;
constexpr size_t K_BIGINT_ARG_COUNT = 1;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr size_t K_INPUT_ARG_INDEX = 0;
constexpr int K_CASE_NUMBER_WIDTH = 2;
constexpr int32_t K_SHIFT_BASE = 3;
constexpr int32_t K_BIT_SLICE_MASK = 0x7f;

struct BigintCaseSpec {
    const char* name;
    bool useUnsigned;
    uint32_t shift;
    int64_t bias;
    uint64_t scale;
};

struct BigintSummaryData {
    const char* name;
    bool useUnsigned;
    int32_t shift;
    int64_t signedValue;
    uint64_t unsignedValue;
    int64_t mixedValue;
    uint64_t scaledValue;
    bool passed;
};

static const BigintCaseSpec K_BIGINT_CASE_SPECS[] = {
    {
        "bigintCase01",
        false,
        2,
        40LL,
        16ULL,
    },
    {
        "bigintCase02",
        true,
        3,
        57LL,
        21ULL,
    },
    {
        "bigintCase03",
        false,
        4,
        74LL,
        26ULL,
    },
    {
        "bigintCase04",
        true,
        5,
        91LL,
        31ULL,
    },
    {
        "bigintCase05",
        false,
        6,
        108LL,
        36ULL,
    },
    {
        "bigintCase06",
        true,
        7,
        125LL,
        41ULL,
    },
    {
        "bigintCase07",
        false,
        8,
        142LL,
        46ULL,
    },
    {
        "bigintCase08",
        true,
        9,
        159LL,
        51ULL,
    },
    {
        "bigintCase09",
        false,
        10,
        176LL,
        56ULL,
    },
    {
        "bigintCase10",
        true,
        11,
        193LL,
        61ULL,
    },
    {
        "bigintCase11",
        false,
        12,
        210LL,
        66ULL,
    },
    {
        "bigintCase12",
        true,
        13,
        227LL,
        71ULL,
    },
    {
        "bigintCase13",
        false,
        1,
        244LL,
        76ULL,
    },
    {
        "bigintCase14",
        true,
        2,
        261LL,
        81ULL,
    },
    {
        "bigintCase15",
        false,
        3,
        278LL,
        86ULL,
    },
    {
        "bigintCase16",
        true,
        4,
        295LL,
        91ULL,
    },
    {
        "bigintCase17",
        false,
        5,
        312LL,
        96ULL,
    },
    {
        "bigintCase18",
        true,
        6,
        329LL,
        101ULL,
    },
    {
        "bigintCase19",
        false,
        7,
        346LL,
        106ULL,
    },
    {
        "bigintCase20",
        true,
        8,
        363LL,
        111ULL,
    },
    {
        "bigintCase21",
        false,
        9,
        380LL,
        116ULL,
    },
    {
        "bigintCase22",
        true,
        10,
        397LL,
        121ULL,
    },
    {
        "bigintCase23",
        false,
        11,
        414LL,
        126ULL,
    },
    {
        "bigintCase24",
        true,
        12,
        431LL,
        131ULL,
    },
    {
        "bigintCase25",
        false,
        13,
        448LL,
        136ULL,
    },
    {
        "bigintCase26",
        true,
        1,
        465LL,
        141ULL,
    },
    {
        "bigintCase27",
        false,
        2,
        482LL,
        146ULL,
    },
    {
        "bigintCase28",
        true,
        3,
        499LL,
        151ULL,
    },
    {
        "bigintCase29",
        false,
        4,
        516LL,
        156ULL,
    },
    {
        "bigintCase30",
        true,
        5,
        533LL,
        161ULL,
    },
    {
        "bigintCase31",
        false,
        6,
        550LL,
        166ULL,
    },
    {
        "bigintCase32",
        true,
        7,
        567LL,
        171ULL,
    },
    {
        "bigintCase33",
        false,
        8,
        584LL,
        176ULL,
    },
    {
        "bigintCase34",
        true,
        9,
        601LL,
        181ULL,
    },
    {
        "bigintCase35",
        false,
        10,
        618LL,
        186ULL,
    },
    {
        "bigintCase36",
        true,
        11,
        635LL,
        191ULL,
    },
    {
        "bigintCase37",
        false,
        12,
        652LL,
        196ULL,
    },
    {
        "bigintCase38",
        true,
        13,
        669LL,
        201ULL,
    },
    {
        "bigintCase39",
        false,
        1,
        686LL,
        206ULL,
    },
    {
        "bigintCase40",
        true,
        2,
        703LL,
        211ULL,
    },
    {
        "bigintCase41",
        false,
        3,
        720LL,
        216ULL,
    },
    {
        "bigintCase42",
        true,
        4,
        737LL,
        221ULL,
    },
    {
        "bigintCase43",
        false,
        5,
        754LL,
        226ULL,
    },
    {
        "bigintCase44",
        true,
        6,
        771LL,
        231ULL,
    },
    {
        "bigintCase45",
        false,
        7,
        788LL,
        236ULL,
    },
    {
        "bigintCase46",
        true,
        8,
        805LL,
        241ULL,
    },
    {
        "bigintCase47",
        false,
        9,
        822LL,
        246ULL,
    },
    {
        "bigintCase48",
        true,
        10,
        839LL,
        251ULL,
    },
    {
        "bigintCase49",
        false,
        11,
        856LL,
        256ULL,
    },
    {
        "bigintCase50",
        true,
        12,
        873LL,
        261ULL,
    },
    {
        "bigintCase51",
        false,
        13,
        890LL,
        266ULL,
    },
    {
        "bigintCase52",
        true,
        1,
        907LL,
        271ULL,
    },
    {
        "bigintCase53",
        false,
        2,
        924LL,
        276ULL,
    },
    {
        "bigintCase54",
        true,
        3,
        941LL,
        281ULL,
    },
    {
        "bigintCase55",
        false,
        4,
        958LL,
        286ULL,
    },
    {
        "bigintCase56",
        true,
        5,
        975LL,
        291ULL,
    },
    {
        "bigintCase57",
        false,
        6,
        992LL,
        296ULL,
    },
    {
        "bigintCase58",
        true,
        7,
        1009LL,
        301ULL,
    },
    {
        "bigintCase59",
        false,
        8,
        1026LL,
        306ULL,
    },
    {
        "bigintCase60",
        true,
        9,
        1043LL,
        311ULL,
    },
    {
        "bigintCase61",
        false,
        10,
        1060LL,
        316ULL,
    },
    {
        "bigintCase62",
        true,
        11,
        1077LL,
        321ULL,
    },
    {
        "bigintCase63",
        false,
        12,
        1094LL,
        326ULL,
    },
    {
        "bigintCase64",
        true,
        13,
        1111LL,
        331ULL,
    },
    {
        "bigintCase65",
        false,
        1,
        1128LL,
        336ULL,
    },
    {
        "bigintCase66",
        true,
        2,
        1145LL,
        341ULL,
    },
    {
        "bigintCase67",
        false,
        3,
        1162LL,
        346ULL,
    },
    {
        "bigintCase68",
        true,
        4,
        1179LL,
        351ULL,
    },
    {
        "bigintCase69",
        false,
        5,
        1196LL,
        356ULL,
    },
    {
        "bigintCase70",
        true,
        6,
        1213LL,
        361ULL,
    },
    {
        "bigintCase71",
        false,
        7,
        1230LL,
        366ULL,
    },
    {
        "bigintCase72",
        true,
        8,
        1247LL,
        371ULL,
    },
    {
        "bigintCase73",
        false,
        9,
        1264LL,
        376ULL,
    },
    {
        "bigintCase74",
        true,
        10,
        1281LL,
        381ULL,
    },
    {
        "bigintCase75",
        false,
        11,
        1298LL,
        386ULL,
    },
    {
        "bigintCase76",
        true,
        12,
        1315LL,
        391ULL,
    },
    {
        "bigintCase77",
        false,
        13,
        1332LL,
        396ULL,
    },
    {
        "bigintCase78",
        true,
        1,
        1349LL,
        401ULL,
    },
    {
        "bigintCase79",
        false,
        2,
        1366LL,
        406ULL,
    },
    {
        "bigintCase80",
        true,
        3,
        1383LL,
        411ULL,
    },
    {
        "bigintCase81",
        false,
        4,
        1400LL,
        416ULL,
    },
    {
        "bigintCase82",
        true,
        5,
        1417LL,
        421ULL,
    },
    {
        "bigintCase83",
        false,
        6,
        1434LL,
        426ULL,
    },
    {
        "bigintCase84",
        true,
        7,
        1451LL,
        431ULL,
    },
    {
        "bigintCase85",
        false,
        8,
        1468LL,
        436ULL,
    },
    {
        "bigintCase86",
        true,
        9,
        1485LL,
        441ULL,
    },
    {
        "bigintCase87",
        false,
        10,
        1502LL,
        446ULL,
    },
    {
        "bigintCase88",
        true,
        11,
        1519LL,
        451ULL,
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

bool SetNamedBool(napi_env env, napi_value object, const char* name, bool value)
{
    napi_value napiValue = nullptr;
    if (napi_get_boolean(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int32(env, value, &napiValue) != napi_ok) {
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

bool SetNamedBigintInt64(napi_env env, napi_value object, const char* name, int64_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_bigint_int64(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

bool SetNamedBigintUint64(napi_env env, napi_value object, const char* name, uint64_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_bigint_uint64(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

napi_value CreateBigintSummary(napi_env env, const BigintSummaryData& summary)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", summary.name);
    SetNamedBool(env, result, "useUnsigned", summary.useUnsigned);
    SetNamedInt32(env, result, "shift", summary.shift);
    SetNamedBigintInt64(env, result, "signedValue", summary.signedValue);
    SetNamedBigintUint64(env, result, "unsignedValue", summary.unsignedValue);
    SetNamedBigintInt64(env, result, "mixedValue", summary.mixedValue);
    SetNamedBigintUint64(env, result, "scaledValue", summary.scaledValue);
    SetNamedBool(env, result, "passed", summary.passed);
    return result;
}

static napi_value RunBigintCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_BIGINT_ARG_COUNT;
    napi_value args[K_BIGINT_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_BIGINT_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid bigint case");
        return nullptr;
    }
    if (argc < K_BIGINT_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "bigint input is required");
        return nullptr;
    }

    const auto& spec = K_BIGINT_CASE_SPECS[caseIndex];
    bool lossless = false;
    int64_t signedValue = 0;
    uint64_t unsignedValue = 0;
    if (spec.useUnsigned) {
        if (napi_get_value_bigint_uint64(env, args[K_INPUT_ARG_INDEX], &unsignedValue, &lossless) != napi_ok) {
            napi_throw_type_error(env, nullptr, "input must be a bigint");
            return nullptr;
        }
        signedValue = static_cast<int64_t>(unsignedValue & static_cast<uint64_t>(INT64_MAX));
    } else {
        if (napi_get_value_bigint_int64(env, args[K_INPUT_ARG_INDEX], &signedValue, &lossless) != napi_ok) {
            napi_throw_type_error(env, nullptr, "input must be a bigint");
            return nullptr;
        }
        unsignedValue = static_cast<uint64_t>(signedValue >= 0 ? signedValue : -signedValue);
    }

    const int64_t shiftedValue = static_cast<int64_t>(unsignedValue >> (spec.shift % (K_SHIFT_BASE * 8)));
    const int64_t mixedValue = shiftedValue + spec.bias + signedValue;
    const uint64_t scaledValue =
        (unsignedValue + static_cast<uint64_t>(spec.scale)) * static_cast<uint64_t>(caseIndex + K_SHIFT_BASE);
    const uint64_t maskedScaledValue = scaledValue & static_cast<uint64_t>(~K_BIT_SLICE_MASK);
    const BigintSummaryData summary = {spec.name,
                                       spec.useUnsigned,
                                       static_cast<int32_t>(spec.shift),
                                       signedValue,
                                       unsignedValue,
                                       mixedValue,
                                       maskedScaledValue,
                                       maskedScaledValue >= static_cast<uint64_t>(spec.scale)};
    return CreateBigintSummary(env, summary);
}

}  // namespace

napi_value RegisterBigintTests(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_BIGINT_CASE_COUNT);
    exportNames.reserve(K_BIGINT_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_BIGINT_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testBigintCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(),
                                                          nullptr,
                                                          RunBigintCase,
                                                          nullptr,
                                                          nullptr,
                                                          nullptr,
                                                          napi_default,
                                                          reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}
