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

constexpr size_t K_NETSERVER_CASE_COUNT = 88;
constexpr size_t K_NETSERVER_ARG_COUNT = 1;
constexpr size_t K_FIRST_CASE_NUMBER = 1;
constexpr size_t K_CONFIG_ARG_INDEX = 0;
constexpr int K_CASE_NUMBER_WIDTH = 2;
constexpr size_t K_NULL_TERMINATOR_SIZE = 1;
constexpr uint32_t K_MODULE_VERSION = 1;
constexpr uint32_t K_NO_MODULE_FLAGS = 0;

struct NetserverCaseSpec {
    const char* name;
    const char* hostPrefix;
    int32_t portBase;
    bool secure;
    int32_t listenerCount;
    int32_t threshold;
};

static const NetserverCaseSpec K_NETSERVER_CASE_SPECS[] = {
    {
        "netserverCase01",
        "host01",
        8001,
        false,
        3,
        40,
    },
    {
        "netserverCase02",
        "host02",
        8002,
        true,
        4,
        43,
    },
    {
        "netserverCase03",
        "host03",
        8003,
        false,
        5,
        46,
    },
    {
        "netserverCase04",
        "host04",
        8004,
        true,
        6,
        49,
    },
    {
        "netserverCase05",
        "host05",
        8005,
        false,
        7,
        52,
    },
    {
        "netserverCase06",
        "host06",
        8006,
        true,
        8,
        55,
    },
    {
        "netserverCase07",
        "host07",
        8007,
        false,
        2,
        58,
    },
    {
        "netserverCase08",
        "host08",
        8008,
        true,
        3,
        61,
    },
    {
        "netserverCase09",
        "host09",
        8009,
        false,
        4,
        64,
    },
    {
        "netserverCase10",
        "host10",
        8010,
        true,
        5,
        67,
    },
    {
        "netserverCase11",
        "host11",
        8000,
        false,
        6,
        70,
    },
    {
        "netserverCase12",
        "host12",
        8001,
        true,
        7,
        73,
    },
    {
        "netserverCase13",
        "host13",
        8002,
        false,
        8,
        76,
    },
    {
        "netserverCase14",
        "host14",
        8003,
        true,
        2,
        79,
    },
    {
        "netserverCase15",
        "host15",
        8004,
        false,
        3,
        82,
    },
    {
        "netserverCase16",
        "host16",
        8005,
        true,
        4,
        85,
    },
    {
        "netserverCase17",
        "host17",
        8006,
        false,
        5,
        88,
    },
    {
        "netserverCase18",
        "host18",
        8007,
        true,
        6,
        91,
    },
    {
        "netserverCase19",
        "host19",
        8008,
        false,
        7,
        94,
    },
    {
        "netserverCase20",
        "host20",
        8009,
        true,
        8,
        97,
    },
    {
        "netserverCase21",
        "host21",
        8010,
        false,
        2,
        100,
    },
    {
        "netserverCase22",
        "host22",
        8000,
        true,
        3,
        103,
    },
    {
        "netserverCase23",
        "host23",
        8001,
        false,
        4,
        106,
    },
    {
        "netserverCase24",
        "host24",
        8002,
        true,
        5,
        109,
    },
    {
        "netserverCase25",
        "host25",
        8003,
        false,
        6,
        112,
    },
    {
        "netserverCase26",
        "host26",
        8004,
        true,
        7,
        115,
    },
    {
        "netserverCase27",
        "host27",
        8005,
        false,
        8,
        118,
    },
    {
        "netserverCase28",
        "host28",
        8006,
        true,
        2,
        121,
    },
    {
        "netserverCase29",
        "host29",
        8007,
        false,
        3,
        124,
    },
    {
        "netserverCase30",
        "host30",
        8008,
        true,
        4,
        127,
    },
    {
        "netserverCase31",
        "host31",
        8009,
        false,
        5,
        130,
    },
    {
        "netserverCase32",
        "host32",
        8010,
        true,
        6,
        133,
    },
    {
        "netserverCase33",
        "host33",
        8000,
        false,
        7,
        136,
    },
    {
        "netserverCase34",
        "host34",
        8001,
        true,
        8,
        139,
    },
    {
        "netserverCase35",
        "host35",
        8002,
        false,
        2,
        142,
    },
    {
        "netserverCase36",
        "host36",
        8003,
        true,
        3,
        145,
    },
    {
        "netserverCase37",
        "host37",
        8004,
        false,
        4,
        148,
    },
    {
        "netserverCase38",
        "host38",
        8005,
        true,
        5,
        151,
    },
    {
        "netserverCase39",
        "host39",
        8006,
        false,
        6,
        154,
    },
    {
        "netserverCase40",
        "host40",
        8007,
        true,
        7,
        157,
    },
    {
        "netserverCase41",
        "host41",
        8008,
        false,
        8,
        160,
    },
    {
        "netserverCase42",
        "host42",
        8009,
        true,
        2,
        163,
    },
    {
        "netserverCase43",
        "host43",
        8010,
        false,
        3,
        166,
    },
    {
        "netserverCase44",
        "host44",
        8000,
        true,
        4,
        169,
    },
    {
        "netserverCase45",
        "host45",
        8001,
        false,
        5,
        172,
    },
    {
        "netserverCase46",
        "host46",
        8002,
        true,
        6,
        175,
    },
    {
        "netserverCase47",
        "host47",
        8003,
        false,
        7,
        178,
    },
    {
        "netserverCase48",
        "host48",
        8004,
        true,
        8,
        181,
    },
    {
        "netserverCase49",
        "host49",
        8005,
        false,
        2,
        184,
    },
    {
        "netserverCase50",
        "host50",
        8006,
        true,
        3,
        187,
    },
    {
        "netserverCase51",
        "host51",
        8007,
        false,
        4,
        190,
    },
    {
        "netserverCase52",
        "host52",
        8008,
        true,
        5,
        193,
    },
    {
        "netserverCase53",
        "host53",
        8009,
        false,
        6,
        196,
    },
    {
        "netserverCase54",
        "host54",
        8010,
        true,
        7,
        199,
    },
    {
        "netserverCase55",
        "host55",
        8000,
        false,
        8,
        202,
    },
    {
        "netserverCase56",
        "host56",
        8001,
        true,
        2,
        205,
    },
    {
        "netserverCase57",
        "host57",
        8002,
        false,
        3,
        208,
    },
    {
        "netserverCase58",
        "host58",
        8003,
        true,
        4,
        211,
    },
    {
        "netserverCase59",
        "host59",
        8004,
        false,
        5,
        214,
    },
    {
        "netserverCase60",
        "host60",
        8005,
        true,
        6,
        217,
    },
    {
        "netserverCase61",
        "host61",
        8006,
        false,
        7,
        220,
    },
    {
        "netserverCase62",
        "host62",
        8007,
        true,
        8,
        223,
    },
    {
        "netserverCase63",
        "host63",
        8008,
        false,
        2,
        226,
    },
    {
        "netserverCase64",
        "host64",
        8009,
        true,
        3,
        229,
    },
    {
        "netserverCase65",
        "host65",
        8010,
        false,
        4,
        232,
    },
    {
        "netserverCase66",
        "host66",
        8000,
        true,
        5,
        235,
    },
    {
        "netserverCase67",
        "host67",
        8001,
        false,
        6,
        238,
    },
    {
        "netserverCase68",
        "host68",
        8002,
        true,
        7,
        241,
    },
    {
        "netserverCase69",
        "host69",
        8003,
        false,
        8,
        244,
    },
    {
        "netserverCase70",
        "host70",
        8004,
        true,
        2,
        247,
    },
    {
        "netserverCase71",
        "host71",
        8005,
        false,
        3,
        250,
    },
    {
        "netserverCase72",
        "host72",
        8006,
        true,
        4,
        253,
    },
    {
        "netserverCase73",
        "host73",
        8007,
        false,
        5,
        256,
    },
    {
        "netserverCase74",
        "host74",
        8008,
        true,
        6,
        259,
    },
    {
        "netserverCase75",
        "host75",
        8009,
        false,
        7,
        262,
    },
    {
        "netserverCase76",
        "host76",
        8010,
        true,
        8,
        265,
    },
    {
        "netserverCase77",
        "host77",
        8000,
        false,
        2,
        268,
    },
    {
        "netserverCase78",
        "host78",
        8001,
        true,
        3,
        271,
    },
    {
        "netserverCase79",
        "host79",
        8002,
        false,
        4,
        274,
    },
    {
        "netserverCase80",
        "host80",
        8003,
        true,
        5,
        277,
    },
    {
        "netserverCase81",
        "host81",
        8004,
        false,
        6,
        280,
    },
    {
        "netserverCase82",
        "host82",
        8005,
        true,
        7,
        283,
    },
    {
        "netserverCase83",
        "host83",
        8006,
        false,
        8,
        286,
    },
    {
        "netserverCase84",
        "host84",
        8007,
        true,
        2,
        289,
    },
    {
        "netserverCase85",
        "host85",
        8008,
        false,
        3,
        292,
    },
    {
        "netserverCase86",
        "host86",
        8009,
        true,
        4,
        295,
    },
    {
        "netserverCase87",
        "host87",
        8010,
        false,
        5,
        298,
    },
    {
        "netserverCase88",
        "host88",
        8000,
        true,
        6,
        301,
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
        napi_throw_error(env, nullptr, "port property is required");
        return false;
    }
    if (napi_get_named_property(env, object, name, &property) != napi_ok) {
        return false;
    }
    if (napi_get_value_int32(env, property, result) != napi_ok) {
        napi_throw_type_error(env, nullptr, "port must be a number");
        return false;
    }
    return true;
}

bool ReadNamedString(napi_env env, napi_value object, const char* name, std::string* result)
{
    napi_value property = nullptr;
    bool hasProperty = false;
    if (napi_has_named_property(env, object, name, &hasProperty) != napi_ok || !hasProperty) {
        napi_throw_error(env, nullptr, "host property is required");
        return false;
    }
    if (napi_get_named_property(env, object, name, &property) != napi_ok) {
        return false;
    }
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, property, &type) != napi_ok || type != napi_string) {
        napi_throw_type_error(env, nullptr, "host must be a string");
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

napi_value CreateNetserverSummary(napi_env env, const NetserverCaseSpec& spec, const std::string& host, int32_t port,
    const std::string& endpoint, int32_t score)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    SetNamedString(env, result, "name", spec.name);
    SetNamedString(env, result, "host", host);
    SetNamedInt32(env, result, "port", port);
    SetNamedString(env, result, "endpoint", endpoint);
    SetNamedBool(env, result, "secure", spec.secure);
    SetNamedInt32(env, result, "score", score);
    SetNamedBool(env, result, "passed", score >= spec.threshold);
    return result;
}

static napi_value RunNetserverCase(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    size_t argc = K_NETSERVER_ARG_COUNT;
    napi_value args[K_NETSERVER_ARG_COUNT] = {nullptr};
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, &data));

    const size_t caseIndex = GetCaseIndex(data);
    if (caseIndex >= K_NETSERVER_CASE_COUNT) {
        napi_throw_error(env, nullptr, "invalid netserver case");
        return nullptr;
    }
    if (argc < K_NETSERVER_ARG_COUNT) {
        napi_throw_type_error(env, nullptr, "config object is required");
        return nullptr;
    }

    std::string host;
    int32_t port = 0;
    if (!ReadNamedString(env, args[K_CONFIG_ARG_INDEX], "host", &host)) {
        return nullptr;
    }
    if (!ReadNamedInt32(env, args[K_CONFIG_ARG_INDEX], "port", &port)) {
        return nullptr;
    }

    const auto& spec = K_NETSERVER_CASE_SPECS[caseIndex];
    const int32_t normalizedPort = port + spec.portBase;
    std::string endpoint = spec.secure ? "https://" : "http://";
    endpoint += spec.hostPrefix;
    endpoint += ".";
    endpoint += host;
    endpoint += ":";
    endpoint += std::to_string(normalizedPort);
    const int32_t score =
        normalizedPort + static_cast<int32_t>(endpoint.size()) + spec.listenerCount * (spec.secure ? 3 : 2);
    return CreateNetserverSummary(env, spec, host, normalizedPort, endpoint, score);
}

}  // namespace

static napi_value InitNetserverSuite(napi_env env, napi_value exports)
{
    std::vector<std::string> exportNames;
    std::vector<napi_property_descriptor> descriptors(K_NETSERVER_CASE_COUNT);
    exportNames.reserve(K_NETSERVER_CASE_COUNT);
    for (size_t caseIndex = 0; caseIndex < K_NETSERVER_CASE_COUNT; caseIndex++) {
        exportNames.emplace_back(BuildIndexedName("testNetserverCase", caseIndex + K_FIRST_CASE_NUMBER));
        descriptors[caseIndex] = napi_property_descriptor{exportNames.back().c_str(), nullptr, RunNetserverCase,
            nullptr, nullptr, nullptr, napi_default, reinterpret_cast<void*>(static_cast<uintptr_t>(caseIndex))};
    }
    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_netserverSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitNetserverSuite,
    .nm_modname = "netserver_suite",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterNetserverSuiteModule(void)
{
    napi_module_register(&g_netserverSuiteModule);
}
