/*
 * Copyright (c) 2026 Huawei Device Co., {type: Ltd.
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
#include <unordered_map>

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "hilog/log.h"

namespace {
constexpr int32_t HILOG_MODULE_ID = 0x0001;
constexpr uint32_t MODULE_VERSION = 1;
constexpr uint32_t NO_MODULE_FLAGS = 0;
constexpr uint32_t DEFAULT_EVENT_DELAY_MS = 1000;
constexpr uint32_t MAX_EVENT_NAME_LENGTH = 256;

std::unordered_map<uint64_t, bool> g_eventHandleMap;
uint64_t g_nextHandleId = 1;

void LogInfo(const std::string& message)
{
    HiLogPrint(LOG_CORE, LOG_INFO, HILOG_MODULE_ID, "EventNotification", "%{public}s", message.c_str());
}

void LogError(const std::string& message)
{
    HiLogPrint(LOG_CORE, LOG_ERROR, HILOG_MODULE_ID, "EventNotification", "%{public}s", message.c_str());
}

uint64_t GenerateEventHandle()
{
    return g_nextHandleId++;
}

void MarkEventCompleted(uint64_t handleId)
{
    g_eventHandleMap[handleId] = true;
}

bool IsEventCompleted(uint64_t handleId)
{
    auto it = g_eventHandleMap.find(handleId);
    return it != g_eventHandleMap.end() && it->second;
}

struct EventCallbackData {
    napi_env env;
    napi_ref callbackRef;
    uint64_t handleId;
    std::string eventName;
    int32_t result;
};

void EventCallbackWrapper(void* data)
{
    auto* callbackData = static_cast<EventCallbackData*>(data);
    if (callbackData == nullptr) {
        return;
    }

    napi_handle_scope scope = nullptr;
    if (napi_open_handle_scope(callbackData->env, &scope) != napi_ok) {
        delete callbackData;
        return;
    }

    napi_value callback = nullptr;
    if (napi_get_reference_value(callbackData->env, callbackData->callbackRef, &callback) == napi_ok) {
        napi_value resultValue = nullptr;
        napi_create_int32(callbackData->env, callbackData->result, &resultValue);

        napi_value undefined = nullptr;
        napi_get_undefined(callbackData->env, &undefined);

        napi_value callbackResult = nullptr;
;
    }

    if (callbackData->callbackRef != nullptr) {
        napi_delete_reference(callbackData->env, callbackData->callbackRef);
    }

    napi_close_handle_scope(callbackData->env, scope);
    delete callbackData;
}

napi_value SendEventWithCallback(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 2, "Requires 2 arguments: callback and optional delay");

    napi_valuetype callbackType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &callbackType));
    NAPI_ASSERT(env, callbackType == napi_function, "First argument must be a function");

    uint32_t delayMs = DEFAULT_EVENT_DELAY_MS;
    if (argc > 1) {
        napi_valuetype delayType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &delayType));
        if (delayType == napi_number) {
            double delayValue = 0;
            NAPI_CALL(env, napi_get_value_double(env, argv[1], &delayValue));
            delayMs = static_cast<uint32_t>(delayValue);
        }
    }

    auto* callbackData = new EventCallbackData();
    callbackData->env = env;
    callbackData->handleId = GenerateEventHandle();
    callbackData->eventName = "SendEventWithCallback";
    callbackData->result = static_cast<int32_t>(delayMs);

    napi_ref callbackRef = nullptr;
    NAPI_CALL(env, napi_create_reference(env, argv[0], 1, &callbackRef));
    callbackData->callbackRef = callbackRef;

    g_eventHandleMap[callbackData->handleId] = false;

    napi_status status = napi_send_event(env, [callbackData]() {
        EventCallbackWrapper(callbackData);
    }, napi_eprio_normal, "SendEventWithCallback", napi_barrier_none);

    if (status != napi_ok) {
        LogError("Failed to send event");
        napi_delete_reference(env, callbackRef);
        delete callbackData;
        napi_throw_error(env, nullptr, "Failed to send event");
        return nullptr;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_uint64(env, callbackData->handleId, &result));

    LogInfo("Event sent successfully with handle: " + std::to_string(callbackData->handleId));
    return result;
}

napi_value SendHighPriorityEvent(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Requires 1 argument: callback");

    napi_valuetype callbackType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &callbackType));
    NAPI_ASSERT(env, callbackType == napi_function, "Argument must be a function");

    auto* callbackData = new EventCallbackData();
    callbackData->env = env;
    callbackData->handleId = GenerateEventHandle();
    callbackData->eventName = "SendHighPriorityEvent";
    callbackData->result = 0;

    napi_ref callbackRef = nullptr;
    NAPI_CALL(env, napi_create_reference(env, argv[0], 1, &callbackRef));
    callbackData->callbackRef = callbackRef;

    g_eventHandleMap[callbackData->handleId] = false;

    napi_status status = napi_send_event(env, [callbackData]() {
        EventCallbackWrapper(callbackData);
    }, napi_eprio_high, "SendHighPriorityEvent", napi_barrier_none);

    if (status != napi_ok) {
        LogError("Failed to send high priority event");
        napi_delete_reference(env, callbackRef);
        delete callbackData;
        napi_throw_error(env, nullptr, "Failed to send high priority event");
        return nullptr;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_uint64(env, callbackData->handleId, &result));

    LogInfo("High priority event sent successfully");
    return result;
}

napi_value SendIdlePriorityEvent(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Requires 1 argument: callback");

    napi_valuetype callbackType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &callbackType));
    NAPI_ASSERT(env, callbackType == napi_function, "Argument must be a function");

    auto* callbackData = new EventCallbackData();
    callbackData->env = env;
    callbackData->handleId = GenerateEventHandle();
    callbackData->eventName = "SendIdlePriorityEvent";
    callbackData->result = -1;

    napi_ref callbackRef = nullptr;
    NAPI_CALL(env, napi_create_reference(env, argv[0], 1, &callbackRef));
    callbackData->callbackRef = callbackRef;

    g_eventHandleMap[callbackData->handleId] = false;

    napi_status status = napi_send_event(env, [callbackData]() {
        EventCallbackWrapper(callbackData);
    }, napi_eprio_idle, "SendIdlePriorityEvent", napi_barrier_none);

    if (status != napi_ok) {
        LogError("Failed to send idle priority event");
        napi_delete_reference(env, callbackRef);
        delete callbackData;
        napi_throw_error(env, nullptr, "Failed to send idle priority event");
        return nullptr;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_uint64(env, callbackData->handleId, &result));

    LogInfo("Idle priority event sent successfully");
    return result;
}

napi_value SendVsyncBarrierEvent(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Requires 1 argument: callback");

    napi_valuetype callbackType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &callbackType));
    NAPI_ASSERT(env, callbackType == napi_function, "Argument must be a function");

    auto* callbackData = new EventCallbackData();
    callbackData->env = env;
    callbackData->handleId = GenerateEventHandle();
    callbackData->eventName = "SendVsyncBarrierEvent";
    callbackData->result = 1;

    napi_ref callbackRef = nullptr;
    NAPI_CALL(env, napi_create_reference(env, argv[0], 1, &callbackRef));
    callbackData->callbackRef = callbackRef;

    g_eventHandleMap[callbackData->handleId] = false;

    napi_status status = napi_send_event(env, [callbackData]() {
        EventCallbackWrapper(callbackData);
    }, napi_eprio_normal, "SendVsyncBarrierEvent", napi_barrier_need);

    if (status != napi_ok) {
        LogError("Failed to send vsync barrier event");
        napi_delete_reference(env, callbackRef);
        delete callbackData;
        napi_throw_error(env, nullptr, "Failed to send vsync barrier event");
        return nullptr;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_uint64(env, callbackData->handleId, &result));

    LogInfo("Vsync barrier event sent successfully");
    return result;
}

struct CancelableEventData {
    napi_env env;
    napi_ref callbackRef;
    uint64_t handleId;
    int32_t delayMs;
    bool cancelled;
};

void CancelableEventCallback(void* data)
{
    auto* eventData = static_cast<CancelableEventData*>(data);
    if (eventData == nullptr || eventData->cancelled) {
        return;
    }

    napi_handle_scope scope = nullptr;
    if (napi_open_handle_scope(eventData->env, &scope) != napi_ok) {
        delete eventData;
        return;
    }

    napi_value callback = nullptr;
    if (napi_get_reference_value(eventData->env, eventData->callbackRef, &callback) == napi_ok) {
        napi_value resultValue = nullptr;
        napi_create_int32(eventData->env, eventData->delayMs, &resultValue);

        napi_value undefined = nullptr;
        napi_get_undefined(eventData->env, &undefined);

        napi_value callbackResult = nullptr;
        napi_call_function(eventData->env, undefined, callback, 1, &resultValue, &callbackResult);
    }

    if (eventData->callbackRef != nullptr) {
        napi_delete_reference(eventData->env, eventData->callbackRef);
    }

    MarkEventCompleted(eventData->handleId);
    napi_close_handle_scope(eventData->env, scope);
    delete eventData;
}

napi_value SendCancelableEvent(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Requires at least 1 argument: callback");

    napi_valuetype callbackType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &callbackType));
    NAPI_ASSERT(env, callbackType == napi_function, "First argument must be a function");

    int32_t delayMs = DEFAULT_EVENT_DELAY_MS;
    if (argc > 1) {
        napi_valuetype delayType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &delayType));
        if (delayType == napi_number) {
            NAPI_CALL(env, napi_get_value_int32(env, argv[1], &delayMs));
        }
    }

    auto* eventData = new CancelableEventData();
    eventData->env = env;
    eventData->handleId = GenerateEventHandle();
    eventData->delayMs = delayMs;
    eventData->cancelled = false;

    napi_ref callbackRef = nullptr;
    NAPI_CALL(env, napi_create_reference(env, argv[0], 1, &callbackRef));
    eventData->callbackRef = callbackRef;

    g_eventHandleMap[eventData->handleId] = false;

    uint64_t handleId = 0;
    napi_status status = napi_send_cancelable_event(env, CancelableEventCallback, eventData,
        napi_eprio_normal, &handleId, "CancelableEvent", napi_barrier_none);

    if (status != napi_ok) {
        LogError("Failed to send cancelable event");
        napi_delete_reference(env, callbackRef);
        delete eventData;
        napi_throw_error(env, nullptr, "Failed to send cancelable event");
        return nullptr;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_uint64(env, handleId, &result));

    LogInfo("Cancelable event sent successfully");
    return result;
}

napi_value CancelEvent(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Requires 1 argument: handleId");

    napi_valuetype handleType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &handleType));
    NAPI_ASSERT(env, handleType == napi_number, "handleId must be a number");

    uint64_t handleId = 0;
    NAPI_CALL(env, napi_get_value_uint64(env, argv[0], &handleId));

    napi_status status = napi_cancel_event(env, handleId, "CancelableEvent");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, status == napi_ok, &result));

    if (status == napi_ok) {
        LogInfo("Event cancelled successfully: " + std::to_string(handleId));
    } else {
        LogError("Failed to cancel event: " + std::to_string(handleId));
    }

    return result;
}

napi_value IsEventCompleted(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Requires 1 argument: handleId");

    napi_valuetype handleType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &handleType));
    NAPI_ASSERT(env, handleType == napi_number, "handleId must be a number");

    uint64_t handleId = 0;
    NAPI_CALL(env, napi_get_value_uint64(env, argv[0], &handleId));

    bool completed = IsEventCompleted(handleId);

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, completed, &result));

    return result;
}

napi_value SendMultipleEvents(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 2, "Requires 2 arguments: count and callback");

    napi_valuetype countType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &countType));
    NAPI_ASSERT(env, countType == napi_number, "Count must be a number");

    napi_valuetype callbackType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[1], &callbackType));
    NAPI_ASSERT(env, callbackType == napi_function, "Callback must be a function");

    int32_t count = 0;
    NAPI_CALL(env, napi_get_value_int32(env, argv[0], &count));
    NAPI_ASSERT(env, count > 0 && count <= 100, "Count must be between 1 and 100");

    napi_value resultArray = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, count, &resultArray));

    for (int32_t i = 0; i < count; i++) {
        auto* callbackData = new EventCallbackData();
        callbackData->env = env;
        callbackData->handleId = GenerateEventHandle();
        callbackData->eventName = "MultipleEvent_" + std::to_string(i);
        callbackData->result = i;

        napi_ref callbackRef = nullptr;
        NAPI_CALL(env, napi_create_reference(env, argv[1], 1, &callbackRef));
        callbackData->callbackRef = callbackRef;

        g_eventHandleMap[callbackData->handleId] = false;

        napi_status status = napi_send_event(env, [callbackData]() {
            EventCallbackWrapper(callbackData);
        }, napi_eprio_normal, ("MultipleEvent_" + std::to_string(i)).c_str(), napi_barrier_none);

        if (status == napi_ok) {
            napi_value handleValue = nullptr;
            napi_create_uint64(env, callbackData->handleId, &handleValue);
            napi_set_element(env, resultArray, i, handleValue);
        } else {
            napi_delete_reference(env, callbackRef);
            delete callbackData;
        }
    }

    LogInfo("Sent " + std::to_string(count) + " events");
    return resultArray;
}

}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("sendEventWithCallback", SendEventWithCallback),
        DECLARE_NAPI_FUNCTION("sendHighPriorityEvent", SendHighPriorityEvent),
        DECLARE_NAPI_FUNCTION("sendIdlePriorityEvent", SendIdlePriorityEvent),
        DECLARE_NAPI_FUNCTION("sendVsyncBarrierEvent", SendVsyncBarrierEvent),
        DECLARE_NAPI_FUNCTION("sendCancelableEvent", SendCancelableEvent),
        DECLARE_NAPI_FUNCTION("cancelEvent", CancelEvent),
        DECLARE_NAPI_FUNCTION("isEventCompleted", IsEventCompleted),
        DECLARE_NAPI_FUNCTION("sendMultipleEvents", SendMultipleEvents),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

static napi_module g_eventNotificationModule = {
    .nm_version = MODULE_VERSION,
    .nm_flags = NO_MODULE_FLAGS,
    .nm_filename = nullptr,
EventNotificationModule(void)
{
    napi_module_register(&g_eventNotificationModule);
}
