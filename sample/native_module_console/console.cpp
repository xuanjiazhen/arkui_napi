/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <string>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace console_module {

struct TimerData {
    std::chrono::steady_clock::time_point startTime;
    std::string label;
};

static std::map<std::string, TimerData> g_timers;
static std::mutex g_timersMutex;

static std::string ValueToString(napi_env env, napi_value value)
{
    napi_valuetype type;
    napi_status status = napi_typeof(env, value, &type);
    if (status != napi_ok) {
        return "[unknown]";
    }

    switch (type) {
        case napi_undefined:
            return "undefined";
        case napi_null:
            return "null";
        case napi_boolean: {
            bool boolValue;
            if (napi_get_value_bool(env, value, &boolValue) == napi_ok) {
                return boolValue ? "true" : "false";
            }
            return "[boolean]";
        }
        case napi_number: {
            double numValue;
            if (napi_get_value_double(env, value, &numValue) == napi_ok) {
                std::ostringstream oss;
                oss << numValue;
                return oss.str();
            }
            return "[number]";
        }
        case napi_string: {
            size_t length;
            if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) == napi_ok) {
                std::string str(length, '\0');
                napi_get_value_string_utf8(env, value, &str[0], length + 1, &length);
                return str;
            }
            return "[string]";
        }
        case napi_object: {
            napi_value toString;
            if (napi_get_named_property(env, value, "toString", &toString) != napi_ok) {
                return "[object]";
            }

            napi_valuetype toStringType;
            if (napi_typeof(env, toString, &toStringType) != napi_ok || toStringType != napi_function) {
                return "[object]";
            }

            napi_value result;
            if (napi_call_function(env, value, toString, 0, nullptr, &result) != napi_ok) {
                return "[object]";
            }

            size_t length;
            if (napi_get_value_string_utf8(env, result, nullptr, 0, &length) != napi_ok) {
                return "[object]";
            }

            std::string str(length, '\0');
            napi_get_value_string_utf8(env, result, &str[0], length + 1, &length);
            return str;
        }
        case napi_function:
            return "[function]";
        case napi_symbol:
            return "[symbol]";
        case napi_bigint:
            return "[bigint]";
        case napi_external:
            return "[external]";
        default:
            return "[unknown]";
    }
}

static void PrintLog(const std::string& prefix, const std::string& message)
{
    std::cout << prefix << message << std::endl;
}

static napi_value Log(napi_env env, napi_callback_info info)
{
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr);

    if (argc == 0) {
        std::cout << std::endl;
        return nullptr;
    }

    std::vector<napi_value> args(argc);
    napi_get_cb_info(env, info, &argc, args.data(), nullptr, nullptr);

    std::ostringstream oss;
    for (size_t i = 0; i < argc; ++i) {
        if (i > 0) {
            oss << " ";
        }
        oss << ValueToString(env, args[i]);
    }

    PrintLog("", oss.str());
    return nullptr;
}

static napi_value Debug(napi_env env, napi_callback_info info)
{
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr);

    if (argc == 0) {
        std::cout << "[DEBUG]" << std::endl;
        return nullptr;
    }

    std::vector<napi_value> args(argc);
    napi_get_cb_info(env, info, &argc, args.data(), nullptr, nullptr);

    std::ostringstream oss;
    for (size_t i = 0; i < argc; ++i) {
        if (i > 0) {
            oss << " ";
        }
        oss << ValueToString(env, args[i]);
    }

    PrintLog("[DEBUG] ", oss.str());
    return nullptr;
}

static napi_value Info(napi_env env, napi_callback_info info)
{
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr);

    if (argc == 0) {
        std::cout << "[INFO]" << std::endl;
        return nullptr;
    }

    std::vector<napi_value> args(argc);
    napi_get_cb_info(env, info, &argc, args.data(), nullptr, nullptr);

    std::ostringstream oss;
    for (size_t i = 0; i < argc; ++i) {
        if (i > 0) {
            oss << " ";
        }
        oss << ValueToString(env, args[i]);
    }

    PrintLog("[INFO] ", oss.str());
    return nullptr;
}

static napi_value Warn(napi_env env, napi_callback_info info)
{
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr);

    if (argc == 0) {
        std::cout << "[WARN]" << std::endl;
        return nullptr;
    }

    std::vector<napi_value> args(argc);
    napi_get_cb_info(env, info, &argc, args.data(), nullptr, nullptr);

    std::ostringstream oss;
    for (size_t i = 0; i < argc; ++i) {
        if (i > 0) {
            oss << " ";
        }
        oss << ValueToString(env, args[i]);
    }

    PrintLog("[WARN] ", oss.str());
    return nullptr;
}

static napi_value Error(napi_env env, napi_callback_info info)
{
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr);

    if (argc == 0) {
        std::cout << "[ERROR]" << std::endl;
        return nullptr;
    }

    std::vector<napi_value> args(argc);
    napi_get_cb_info(env, info, &argc, args.data(), nullptr, nullptr);

    std::ostringstream oss;
    for (size_t i = 0; i < argc; ++i) {
        if (i > 0) {
            oss << " ";
        }
        oss << ValueToString(env, args[i]);
    }

    PrintLog("[ERROR] ", oss.str());
    return nullptr;
}

static napi_value Time(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    std::string label = "default";
    if (argc > 0) {
        label = ValueToString(env, args[0]);
    }

    std::lock_guard<std::mutex> lock(g_timersMutex);

    auto it = g_timers.find(label);
    if (it != g_timers.end()) {
        std::cout << "[WARN] Timer '" << label << "' already exists" << std::endl;
    } else {
        TimerData timerData;
        timerData.startTime = std::chrono::steady_clock::now();
        timerData.label = label;
        g_timers[label] = timerData;
        std::cout << label << ": timer started" << std::endl;
    }

    return nullptr;
}

static napi_value TimeEnd(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    std::string label = "default";
    if (argc > 0) {
        label = ValueToString(env, args[0]);
    }

    std::lock_guard<std::mutex> lock(g_timersMutex);

    auto it = g_timers.find(label);
    if (it == g_timers.end()) {
        std::cout << "[WARN] Timer '" << label << "' does not exist" << std::endl;
    } else {
        auto endTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - it->second.startTime).count();
        std::cout << label << ": " << duration << "ms" << std::endl;
        g_timers.erase(it);
    }

    return nullptr;
}

static napi_value TimeLog(napi_env env, napi_callback_info info)
{
    size_t argc = 0;
    napi_get_cb_info(env, info, &argc, nullptr, nullptr, nullptr);

    if (argc == 0) {
        std::cout << "[ERROR] timeLog requires at least one argument" << std::endl;
        return nullptr;
    }

    std::vector<napi_value> args(argc);
    napi_get_cb_info(env, info, &argc, args.data(), nullptr, nullptr);

    std::string label = ValueToString(env, args[0]);

    std::lock_guard<std::mutex> lock(g_timersMutex);

    auto it = g_timers.find(label);
    if (it == g_timers.end()) {
        std::cout << "[WARN] Timer '" << label << "' does not exist" << std::endl;
    } else {
        auto currentTime = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            currentTime - it->second.startTime).count();

        std::ostringstream oss;
        oss << label << ": " << duration << "ms";

        for (size_t i = 1; i < argc; ++i) {
            oss << " " << ValueToString(env, args[i]);
        }

        std::cout << oss.str() << std::endl;
    }

    return nullptr;
}

static napi_value ConsoleInit(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("log", Log),
        DECLARE_NAPI_FUNCTION("debug", Debug),
        DECLARE_NAPI_FUNCTION("info", Info),
        DECLARE_NAPI_FUNCTION("warn", Warn),
        DECLARE_NAPI_FUNCTION("error", Error),
        DECLARE_NAPI_FUNCTION("time", Time),
        DECLARE_NAPI_FUNCTION("timeEnd", TimeEnd),
        DECLARE_NAPI_FUNCTION("timeLog", TimeLog),
    };

    napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
    return exports;
}

} // namespace console_module

EXTERN_C_START
static napi_module consoleModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = console_module::ConsoleInit,
    .nm_modname = "console",
    .nm_priv = nullptr,
    .reserved = { 0 },
};
EXTERN_C_END

extern "C" __attribute__((constructor)) void RegisterConsoleModule(void)
{
    napi_module_register(&consoleModule);
}
