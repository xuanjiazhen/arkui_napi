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

constexpr size_t K_CRYPTO_CASE_COUNT = 88;
constexpr size_t K_CRYPTO_ARG_COUNT = 1;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr size_t K_BUFFER_ARG_INDEX = 0;
constexpr int K_CASE_NUMBER_WIDTH = 2;

struct CryptoCaseSpec {
    const char* name;
    int32_t stride;
    int32_t seed;
    int32_t mask;
    int32_t threshold;
};

struct CryptoSummaryData {
    const char* name;
    int32_t checksum;
    int32_t score;
    const uint8_t* data;
    size_t length;
    int32_t mask;
    bool passed;
};

static const CryptoCaseSpec K_CRYPTO_CASE_SPECS[] = {
    {
"cryptoCase01",
2,
26,
4,
40,
},
{
"cryptoCase02",
3,
33,
5,
49,
},
{
"cryptoCase03",
4,
40,
6,
58,
},
{
"cryptoCase04",
5,
47,
7,
67,
},
{
"cryptoCase05",
1,
54,
8,
76,
},
{
"cryptoCase06",
2,
61,
9,
85,
},
{
"cryptoCase07",
3,
68,
10,
94,
},
{
"cryptoCase08",
4,
75,
11,
103,
},
{
"cryptoCase09",
5,
82,
12,
112,
},
{
"cryptoCase10",
1,
89,
13,
121,
},
{
"cryptoCase11",
2,
96,
3,
130,
},
{
"cryptoCase12",
3,
103,
4,
139,
},
{
"cryptoCase13",
4,
110,
5,
148,
},
{
"cryptoCase14",
5,
117,
6,
157,
},
{
"cryptoCase15",
1,
124,
7,
166,
},
{
"cryptoCase16",
2,
131,
8,
175,
},
{
"cryptoCase17",
3,
138,
9,
184,
},
{
"cryptoCase18",
4,
145,
10,
193,
},
{
"cryptoCase19",
5,
152,
11,
202,
},
{
"cryptoCase20",
1,
159,
12,
211,
},
{
"cryptoCase21",
2,
166,
13,
220,
},
{
"cryptoCase22",
3,
173,
3,
229,
},
{
"cryptoCase23",
4,
180,
4,
238,
},
{
"cryptoCase24",
5,
187,
5,
247,
},
{
"cryptoCase25",
1,
194,
6,
256,
},
{
"cryptoCase26",
2,
201,
7,
265,
},
{
"cryptoCase27",
3,
208,
8,
274,
},
{
"cryptoCase28",
4,
215,
9,
283,
},
{
"cryptoCase29",
5,
222,
10,
292,
},
{
"cryptoCase30",
1,
229,
11,
301,
},
{
"cryptoCase31",
2,
236,
12,
310,
},
{
"cryptoCase32",
3,
243,
13,
319,
},
{
"cryptoCase33",
4,
250,
3,
328,
},
{
"cryptoCase34",
5,
257,
4,
337,
},
{
"cryptoCase35",
1,
264,
5,
346,
},
{
"cryptoCase36",
2,
271,
6,
355,
},
{
"cryptoCase37",
3,
278,
7,
364,
},
{
"cryptoCase38",
4,
285,
8,
373,
},
{
"cryptoCase39",
5,
292,
9,
382,
},
{
"cryptoCase40",
1,
299,
10,
391,
},
{
"cryptoCase41",
2,
306,
11,
400,
},
{
"cryptoCase42",
3,
313,
12,
409,
},
{
"cryptoCase43",
4,
320,
13,
418,
},
{
"cryptoCase44",
5,
327,
3,
427,
},
{
"cryptoCase45",
1,
334,
4,
436,
},
{
"cryptoCase46",
2,
341,
5,
445,
},
{
"cryptoCase47",
3,
348,
6,
454,
},
{
"cryptoCase48",
4,
355,
7,
463,
},
{
"cryptoCase49",
5,
362,
8,
472,
},
{
"cryptoCase50",
1,
369,
9,
481,
},
{
"cryptoCase51",
2,
376,
10,
490,
},
{
"cryptoCase52",
3,
383,
11,
499,
},
{
"cryptoCase53",
4,
390,
12,
508,
},
{
"cryptoCase54",
5,
397,
13,
517,
},
{
"cryptoCase55",
1,
404,
3,
526,
},
{
"cryptoCase56",
2,
411,
4,
535,
},
{
"cryptoCase57",
3,
418,
5,
544,
},
{
"cryptoCase58",
4,
425,
6,
553,
},
{
"cryptoCase59",
5,
432,
7,
562,
},
{
"cryptoCase60",
1,
439,
8,
571,
},
{
"cryptoCase61",
2,
446,
9,
580,
},
{
"cryptoCase62",
3,
453,
10,
589,
},
{
"cryptoCase63",
4,
460,
11,
598,
},
{
"cryptoCase64",
5,
467,
12,
607,
},
{
"cryptoCase65",
1,
474,
13,
616,
},
{
"cryptoCase66",
2,
481,
3,
625,
},
{
"cryptoCase67",
3,
488,
4,
634,
},
{
"cryptoCase68",
4,
495,
5,
643,
},
{
"cryptoCase69",
5,
502,
6,
652,
},
{
"cryptoCase70",
1,
509,
7,
661,
},
{
"cryptoCase71",
2,
516,
8,
670,
},
{
"cryptoCase72",
3,
523,
9,
679,
},
{
"cryptoCase73",
4,
530,
10,
688,
},
{
"cryptoCase74",
5,
537,
11,
697,
},
{
"cryptoCase75",
1,
544,
12,
706,
},
{
"cryptoCase76",
2,
551,
13,
715,
},
{
"cryptoCase77",
3,
558,
3,
724,
},
{
"cryptoCase78",
4,
565,
4,
733,
},
{
"cryptoCase79",
5,
572,
5,
742,
},
{
"cryptoCase80",
1,
579,
6,
751,
},
{
"cryptoCase81",
2,
586,
7,
760,
},
{
"cryptoCase82",
3,
593,
8,
769,
},
{
"cryptoCase83",
4,
600,
9,
778,
},
{
"cryptoCase84",
5,
607,
10,
787,
},
{
"cryptoCase85",
1,
614,
11,
796,
},
{
"cryptoCase86",
2,
621,
12,
805,
},
{
"cryptoCase87",
3,
628,
13,
814,
},
{
"cryptoCase88",
4,
635,
3,
823,
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

size_t GetCaseIndex(void* data)
{
    return static_cast<size_t>(reinterpret_cast<uintptr_t>(data));
}

bool ReadBuffer(napi_env env, napi_value value, uint8_t** data, size_t* length)
{
    bool isBuffer = false;
    if (napi_is_buffer(env, value, &isBuffer) != napi_ok || !isBuffer) {
        napi_throw_type_error(env, nullptr, "buffer is required");
        return false;
    }
    void* rawData = nullptr;
    if (napi_get_buffer_info(env, value, &rawData, length) != napi_ok) {
        return false;
    }
    *data = static_cast<uint8_t*>(rawData);
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

bool SetNamedBuffer(napi_env env, napi_value object, const char* name, const uint8_t* source, size_t length,
    int32_t mask)
{
    void* rawData = nullptr;
    napi_value buffer = nullptr;
    if (napi_create_buffer(env, length, &rawData, &buffer) != napi_ok) {
        return false;
    }
    auto* target = static_cast<uint8_t*>(rawData);
    for (size_t index = 0; index < length; index++) {
        target[index] = static_cast<uint8_t>(source[index] ^ static_cast<uint8_t>(mask));
    }
    return napi_set_named_property(env, object, name, buffer) == napi_ok;
}

napi_value CreateCryptoSummary(napi_env env, const CryptoSummaryData& summary)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", summary.name);
    SetNamedInt32(env, result, "checksum", summary.checksum);
    SetNamedInt32(env, result, "score", summary.score);
    SetNamedBool(env, result, "passed", summary.passed);
    SetNamedBuffer(env, result, "masked", summary.data, summary.length, summary.mask);
    return result;
}

static napi_value RunCryptoCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_CRYPTO_ARG_COUNT;
    napi_value args[K_CRYPTO_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_CRYPTO_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid crypto case");
        return nullptr;
    }
    if (argc < K_CRYPTO_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "buffer is required");
        return nullptr;
    }

    uint8_t* bytes = nullptr;
    size_t length = 0;
    if (!ReadBuffer(env, args[K_BUFFER_ARG_INDEX], &bytes, &length)) {
        return nullptr;
    }

    const auto& spec = K_CRYPTO_CASE_SPECS[caseIndex];
    int32_t checksum = 0;
    for (size_t index = 0; index < length; index += static_cast<size_t>(spec.stride)) {
        checksum += static_cast<int32_t>((bytes[index] + spec.seed) ^ spec.mask);
    }
    const int32_t score = checksum - spec.seed + static_cast<int32_t>(length);
    const CryptoSummaryData summary = {spec.name, checksum, score, bytes, length, spec.mask,
        score >= spec.threshold};
    return CreateCryptoSummary(env, summary);
}

}  // namespace

napi_value RegisterCryptoTests(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_CRYPTO_CASE_COUNT);
    exportNames.reserve(K_CRYPTO_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_CRYPTO_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testCryptoCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunCryptoCase,
            nullptr, nullptr, nullptr, napi_default,
            reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}
