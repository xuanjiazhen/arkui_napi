/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "native_event.h"
#include <string>

#if defined(ENABLE_EVENT_HANDLER)
#include "event_handler.h"
#endif
#ifdef ENABLE_HITRACE
#include "hitrace_meter.h"
#endif
#include "native_engine.h"
#include "utils/log.h"
#if defined(ENABLE_EVENT_HANDLER)
using namespace OHOS::AppExecFwk;
#endif
class TraceLogClass {
#ifdef ENABLE_HITRACE
    uint64_t traceLable = HITRACE_TAG_ACE;
#endif
    std::string value;
    public:
        explicit TraceLogClass(std::string value) : value(value)
        {
#ifdef ENABLE_HITRACE
            StartTrace(traceLable, value);
#endif
            HILOG_INFO("log start{%{public}s}", value.c_str());
        }
        ~TraceLogClass()
        {
#ifdef ENABLE_HITRACE
            FinishTrace(traceLable);
#endif
            HILOG_INFO("log end{%{public}s}", value.c_str());
        }
};

NativeEvent::NativeEvent(NativeEngine* engine,
                         napi_value func,
                         napi_value asyncResource,
                         napi_value asyncResourceName,
                         size_t maxQueueSize,
                         size_t threadCount,
                         void* finalizeData,
                         NativeFinalize finalizeCallback,
                         void* context,
                         NativeThreadSafeFunctionCallJs callJsCallback)
                         :NativeSafeAsyncWork(engine, func, asyncResource, asyncResourceName,
                             maxQueueSize, threadCount, finalizeData, finalizeCallback,
                             context, callJsCallback)
{
}

bool NativeEvent::Init()
{
    sequence_.store(1, std::memory_order_relaxed);
    return NativeSafeAsyncWork::Init();
}

napi_status NativeEvent::SendCancelableEvent(const std::function<void(void*)> &callback,
                                             void *data,
                                             int32_t priority,
                                             const char* name,
                                             uint64_t* handleId)
{
    uint64_t eventId = GenerateUniqueID();
    auto task = [eng = engine_, callback, data, eventId]() {
        auto tcin = TraceLogClass("Cancelable Event callback:handleId:" + std::to_string(eventId));
        auto vm = eng->GetEcmaVm();
        panda::LocalScope scope(vm);
        callback(data);
    };
#ifdef ENABLE_EVENT_HANDLER
    if (eventHandler_) {
        auto evt = TraceLogClass("eventHandler Send task:" + std::string(name) + "|handleId:" + std::to_string(eventId));
        if (eventHandler_->PostTask(task,
                                    std::to_string(eventId),
                                    0,
                                    static_cast<EventQueue::Priority>(priority),
                                    {})) {
            *handleId = eventId;
            return napi_status::napi_ok;
        } else {
            *handleId = 0;
            return napi_status::napi_generic_failure;
        }
    }
#endif
    auto uvt = TraceLogClass("uv Send task:" + std::string(name) + "|handleId:" + std::to_string(eventId));
    std::function<void()> cb = task;
    CallbackWrapper *cbw = new (std::nothrow) CallbackWrapper();
    if (!cbw) {
        HILOG_ERROR("malloc failed!");
        return napi_status::napi_generic_failure;
    }
    cbw->cb = cb;
    cbw->handleId = eventId;
    napi_status status = SendConvertStatus2NapiStatus(reinterpret_cast<void *>(cbw), NATIVE_TSFUNC_NONBLOCKING);
    *handleId = eventId;
    if (status != napi_status::napi_ok) {
        auto uvtf = TraceLogClass("uv Send task failed " + std::to_string(status));
        HILOG_ERROR("send event failed(%{public}d)", status);
        delete cbw;
        cbw = nullptr;
        *handleId = 0;
    }
    return status;
}

napi_status NativeEvent::CancelEvent(const char* name, uint64_t handleId)
{
    auto tc = TraceLogClass("Cancel Event:" + std::string(name) + "|handleId:" + std::to_string(handleId));
    if (handleId == 0) {
        return napi_status::napi_invalid_arg;
    }
#ifdef ENABLE_EVENT_HANDLER
    if (eventHandler_) {
        auto evt = TraceLogClass("eventHandler remove task");
        eventHandler_->RemoveTask(std::to_string(handleId));
        return napi_status::napi_ok;
    }
#endif
    auto uvt = TraceLogClass("uv remove task");
    auto code = UvCancelEvent(handleId);
    napi_status status = napi_status::napi_ok;
    if (code == SafeAsyncCode::SAFE_ASYNC_FAILED) {
        status = napi_status::napi_generic_failure;
    }
    return status;
}

SafeAsyncCode NativeEvent::UvCancelEvent(uint64_t handleId)
{
    std::unique_lock<std::mutex> lock(mutex_);
    if (status_ == SafeAsyncStatus::SAFE_ASYNC_STATUS_CLOSED ||
        status_ == SafeAsyncStatus::SAFE_ASYNC_STATUS_CLOSING) {
        HILOG_WARN("Do not cancel, thread is closed!");
        return SafeAsyncCode::SAFE_ASYNC_CLOSED;
    }
    if (callJsCallback_ == nullptr) {
        return SafeAsyncCode::SAFE_ASYNC_FAILED;
    }

    for (auto& current : queue_) {
        if (current != nullptr) {
            CallbackWrapper *cbw = static_cast<CallbackWrapper *>(current);
            if (cbw->handleId == handleId) {
                current = nullptr;
                break;
            }
        }
    }
    return SafeAsyncCode::SAFE_ASYNC_OK;
}

napi_status NativeEvent::SendConvertStatus2NapiStatus(void *data, NativeThreadSafeFunctionCallMode mode)
{
    auto code = Send(data, mode);
    napi_status status = napi_status::napi_ok;
    switch (code) {
        case SafeAsyncCode::SAFE_ASYNC_OK:
            status = napi_status::napi_ok;
            break;
        case SafeAsyncCode::SAFE_ASYNC_QUEUE_FULL:
            status = napi_status::napi_queue_full;
            break;
        case SafeAsyncCode::SAFE_ASYNC_INVALID_ARGS:
            status = napi_status::napi_invalid_arg;
            break;
        case SafeAsyncCode::SAFE_ASYNC_CLOSED:
            status = napi_status::napi_closing;
            break;
        case SafeAsyncCode::SAFE_ASYNC_FAILED:
            status = napi_status::napi_generic_failure;
            break;
        default:
            HILOG_FATAL("this branch is unreachable, code is %{public}d", code);
            status = napi_status::napi_generic_failure;
            break;
    }
    return status;
}

uint64_t NativeEvent::GenerateUniqueID()
{
    uint64_t currentSequence = sequence_.fetch_add(1, std::memory_order_relaxed); 
    // if sequence_ max, store 1
    if (currentSequence == std::numeric_limits<uint64_t>::max()) {
        sequence_.store(1, std::memory_order_relaxed);
        return 1;
    } else {
        return currentSequence;
    }
}