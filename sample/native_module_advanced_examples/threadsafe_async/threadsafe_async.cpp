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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "hilog/log.h"

namespace {
constexpr int32_t HILOG_MODULE_ID = 0x0001;
constexpr uint32_t MODULE_VERSION = 1;
constexpr uint32_t NO_MODULE_FLAGS = 0;
constexpr uint32_t DEFAULT_ASYNC_DELAY_MS = 1000;
constexpr uint32_t MAX_THREAD_COUNT = 10;
constexpr uint32_t DEFAULT_THREAD_COUNT = 3;

void LogInfo(const std::string& message)
{
    HiLogPrint(LOG_CORE, LOG_INFO, HILOG_MODULE_ID, "ThreadSafeAsync", "%{public}s", message.c_str());
}

void LogError(const std::string& message)
{
    HiLogPrint(LOG_CORE, LOG_ERROR, HILOG_MODULE_ID, "ThreadSafeAsync", "%{public}s", message.c_str());
}

struct AsyncWorkContext {
    napi_async_work work = nullptr;
    napi_deferred deferred = nullptr;
    napi_threadsafe_function threadsafeFunc = nullptr;
    napi_ref callbackRef = nullptr;
    int32_t input = 0;
    int32_t output = 0;
    std::string operationName;
    bool usePromise = false;
    bool useThreadsafeFunction = false;
    std::atomic<bool> completed{false};
};

struct ThreadsafeFunctionContext {
    napi_env env;
    napi_threadsafe_function func;
    std::atomic<int32_t> callCount{0};
    std::atomic<bool> active{true};
};

std::vector<std::unique_ptr<ThreadsafeFunctionContext>> g_threadsafeFunctions;
std::mutex g_threadsafeFunctionsMutex;

void AsyncWorkExecute(napi_env env, void* data)
{
    (void)env;
    auto* context = static_cast<AsyncWorkContext*>(data);
    if (context == nullptr) {
        return;
    }

    LogInfo("Async work executing: " + context->operationName);

    std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_ASYNC_DELAY_MS));

    context->output = context->input * 2;

    LogInfo("Async work completed: " + context->operationName + ", result: " + std::to_string(context->output));
}

void AsyncWorkComplete(napi_env env, napi_status status, void* data)
{
    (void)status;
    auto* context = static_cast<AsyncWorkContext*>(data);
    if (context == nullptr) {
        return;
    }

    napi_handle_scope scope = nullptr;
    if (napi_open_handle_scope(env, &scope) != napi_ok) {
        return;
    }

    napi_value result = nullptr;
    napi_create_int32(env, context->output, &result);

    if (context->usePromise && context->deferred != nullptr) {
        napi_resolve_deferred(env, context->deferred, result);
    } else if (context->callbackRef != nullptr) {
        napi_value callback = nullptr;
        napi_get_reference_value(env, context->callbackRef, &callback);

        napi_value undefined = nullptr;
        napi_get_undefined(env, &undefined);

        napi_value callbackResult = nullptr;
        napi_call_function(env, undefined, callback, 1, &result, &callbackResult);
    }

    if (context->callbackRef != nullptr) {
        napi_delete_reference(env, context->callbackRef);
    }
    if (context->work != nullptr) {
        napi_delete_async_work(env, context->work);
    }

    context->completed.store(true);
    napi_close_handle_scope(env, scope);
    delete context;
}

napi_value RunAsyncWork(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Requires at least 1 argument: input");

    napi_valuetype inputType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &inputType));
    NAPI_ASSERT(env, inputType == napi_number, "Input must be a number");

    int32_t input = 0;
    NAPI_CALL(env, napi_get_value_int32(env, argv[0], &input));

    bool usePromise = (argc == 1);

    auto* context = new AsyncWorkContext();
    context->input = input;
    context->operationName = "AsyncWork";
    context->usePromise = usePromise;

    napi_value resourceName = nullptr;
    napi_create_string_utf8(context->operationName.c_str(), NAPI_AUTO_LENGTH, &resourceName);

    if (usePromise) {
        napi_value promise = nullptr;
        NAPI_CALL(env, napi_create_promise(env, &context->deferred, &promise));

        NAPI_CALL(env, napi_create_async_work(env, nullptr, resourceName,
            AsyncWorkExecute, AsyncWorkComplete, context, &context->work));

        NAPI_CALL(env, napi_queue_async_work(env, context->work));

        LogInfo("Async work queued (promise mode)");
        return promise;
    } else {
        napi_valuetype callbackType = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &callbackType));
        NAPI_ASSERT(env, callbackType == napi_function, "Second argument must be a function");

        NAPI_CALL(env, napi_create_reference(env, argv[1], 1, &context->callbackRef));

        NAPI_CALL(env, napi_create_async_work(env, nullptr, resourceName,
            AsyncWorkExecute, AsyncWorkComplete, context, &context->work));

        NAPI_CALL(env, napi_queue_async_work(env, context->work));

        napi_value result = nullptr;
        NAPI_CALL(env, napi_get_undefined(env, &result));

        LogInfo("Async work queued (callback mode)");
        return result;
    }
}

void ThreadsafeFunctionCallJs(napi_env env, napi_value jsCallback, void* context, void* data)
{
    (void)context;
    if (env == nullptr || jsCallback == nullptr) {
        return;
    }

    napi_handle_scope scope = nullptr;
    if (napi_open_handle_scope(env, &scope) != napi_ok) {
        return;
    }

    int32_t value = static_cast<int32_t>(reinterpret_cast<intptr_t>(data));

    napi_value resultValue = nullptr;
    napi_create_int32(env, value, &resultValue);

    napi_value undefined = nullptr;
    napi_get_undefined(env, &undefined);

    napi_value callbackResult = nullptr;
    napi_call_function(env, undefined, jsCallback, 1, &resultValue, &callbackResult);

    napi_close_handle_scope(env, scope);
}

void ThreadsafeFunctionFinalizer(napi_env env, void* data, void* hint)
{
    (void)env;
    (void)hint;
    auto* tsfnContext = static_cast<ThreadsafeFunctionContext*>(data);
    if (tsfnContext != nullptr) {
        tsfnContext->active.store(false);
        if (tsfnContext->func != nullptr) {
            napi_release_threadsafe_function(tsfnContext->func, napi_tsfn_release);
        }
        delete tsfnContext;
        LogInfo("Threadsafe function finalized");
    }
}

napi_value CreateThreadsafeFunction(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Requires 1 argument: callback");

    napi_valuetype callbackType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &callbackType));
    NAPI_ASSERT(env, callbackType == napi_function, "Callback must be a function");

    napi_value resourceName = nullptr;
    napi_create_string_utf8("ThreadsafeFunction", NAPI_AUTO_LENGTH, &resourceName);

    auto* tsfnContext = new ThreadsafeFunctionContext();
    tsfnContext->env = env;
    tsfnContext->active.store(true);

    napi_value asyncResource = nullptr;
    napi_create_object(env, &asyncResource);

    NAPI_CALL(env, napi_create_threadsafe_function(env, argv[0], asyncResource, resourceName,
        0, 1, nullptr, ThreadsafeFunctionFinalizer, tsfnContext, ThreadsafeFunctionCallJs, &tsfnContext->func));

    {
        std::lock_guard<std::mutex> lock(g_threadsafeFunctionsMutex);
        g_threadsafeFunctions.emplace_back(tsfnContext);
    }

    napi_value result = nullptr;
    napi_create_int64(env, reinterpret_cast<int64_t>(tsfnContext), &result);

    LogInfo("Threadsafe function created");
    return result;
}

napi_value CallThreadsafeFunction(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 2, "Requires 2 arguments: function handle and value");

    napi_valuetype handleType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &handleType));
    NAPI_ASSERT(env, handleType == napi_number, "Function handle must be a number");

    napi_valuetype valueType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[1], &valueType));
    NAPI_ASSERT(env, valueType == napi_number, "Value must be a number");

    int64_t handleValue = 0;
    NAPI_CALL(env, napi_get_value_int64(env, argv[0], &handleValue));

    int32_t value = 0;
    NAPI_CALL(env, napi_get_value_int32(env, argv[1], &value));

    auto* tsfnContext = reinterpret_cast<ThreadsafeFunctionContext*>(handleValue);
    if (tsfnContext == nullptr || !tsfnContext->active.load()) {
        napi_throw_error(env, nullptr, "Invalid threadsafe function handle");
        return nullptr;
    }

    void* data = reinterpret_cast<void*>(static_cast<intptr_t>(value));
    napi_status status = napi_call_threadsafe_function(tsfnContext->func, data, napi_tsfn_nonblocking);

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, status == (napi_ok), &result));

    tsfnContext->callCount.fetch_add(1);

    LogInfo("Threadsafe function called, count: " + std::to_string(tsfnContext->callCount.load()));
    return result;
}

struct ThreadWorker {
    std::thread workerThread;
    std::atomic<bool> running{false};
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<int32_t> taskQueue;
    napi_threadsafe_function callbackFunc = nullptr;
    std::atomic<int32_t> processedCount{0};
};

std::vector<std::unique_ptr<ThreadWorker>> g_threadWorkers;
std::mutex g_threadWorkersMutex;

void WorkerThreadFunc(ThreadWorker* worker)
{
    LogInfo("Worker thread started");

    while (worker->running.load()) {
        int32_t task = 0;
        bool hasTask = false;

        {
            std::unique_lock<std::mutex> lock(worker->mutex);
            worker->cv.wait(lock, [worker]() {
                return !worker->taskQueue.empty() || !worker->running.load();
            });

            if (!worker->running.load()) {
                break;
            }

            if (!worker->taskQueue.empty()) {
                task = worker->taskQueue.front();
                worker->taskQueue.erase(worker->taskQueue.begin());
                hasTask = true;
            }
        }

        if (hasTask && worker->callbackFunc != nullptr) {
            int32_t result = task * 3;
            worker->processedCount.fetch_add(1);

            void* data = reinterpret_cast<void*>(static_cast<intptr_t>(result));
            napi_call_threadsafe_function(worker->callbackFunc, data, napi_tsfn_blocking);
        }
    }

    LogInfo("Worker thread stopped");
}

napi_value CreateWorkerThread(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Requires 1 argument: callback");

    napi_valuetype callbackType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &callbackType));
    NAPI_ASSERT(env, callbackType == napi_function, "Callback must be a function");

    napi_value resourceName = nullptr;
    napi_create_string_utf8("WorkerThreadCallback", NAPI_AUTO_LENGTH, &resourceName);

    auto* worker = new ThreadWorker();
    worker->running.store(true);

    napi_value asyncResource = nullptr;
    napi_create_object(env, &asyncResource);

    NAPI_CALL(env, napi_create_threadsafe_function(env, argv[0], asyncResource, resourceName,
        0, 1, nullptr, [](napi_env env, void* data, void* hint) {
            (void)env;
            (void)hint;
            auto* callbackData = static_cast<ThreadWorker*>(data);
            if (callbackData != nullptr && callbackData->callbackFunc != nullptr) {
                napi_release_threadsafe_function(callbackData->callbackFunc, napi_tsfn_release);
            }
        }, worker, ThreadsafeFunctionCallJs, &worker->callbackFunc));

    worker->workerThread = std::thread(WorkerThreadFunc, worker);

    {
        std::lock_guard<std::mutex> lock(g_threadWorkersMutex);
        g_threadWorkers.emplace_back(worker);
    }

    napi_value result = nullptr;
    napi_create_int64(env, reinterpret_cast<int64_t>(worker), &result);

    LogInfo("Worker thread created");
    return result;
}

napi_value SubmitTaskToWorker(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 2, "Requires 2 arguments: worker handle and task value");

    napi_valuetype handleType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &handleType));
    NAPI_ASSERT(env, handleType == napi_number, "Worker handle must be a number");

    napi_valuetype taskType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[1], &taskType));
    NAPI_ASSERT(env, taskType == napi_number, "Task value must be a number");

    int64_t handleValue = 0;
    NAPI_CALL(env, napi_get_value_int64(env, argv[0], &handleValue));

    int32_t taskValue = 0;
    NAPI_CALL(env, napi_get_value_int32(env, argv[1], &taskValue));

    auto* worker = reinterpret_cast<ThreadWorker*>(handleValue);
    if (worker == nullptr || !worker->running.load()) {
        napi_throw_error(env, nullptr, "Invalid worker handle");
        return nullptr;
    }

    {
        std::lock_guard<std::mutex> lock(worker->mutex);
        worker->taskQueue.push_back(taskValue);
    }
    worker->cv.notify_one();

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));

    LogInfo("Task submitted to worker: " + std::to_string(taskValue));
    return result;
}

napi_value StopWorkerThread(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Requires 1 argument: worker handle");

    napi_valuetype handleType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &handleType));
    NAPI_ASSERT(env, handleType == napi_number, "Worker handle must be a number");

    int64_t handleValue = 0;
    NAPI_CALL(env, napi_get_value_int64(env, argv[0], &handleValue));

    auto* worker = reinterpret_cast<ThreadWorker*>(handleValue);
    if (worker == nullptr) {
        napi_throw_error(env, nullptr, "Invalid worker handle");
        return nullptr;
    }

    worker->running.store(false);
    worker->cv.notify_all();

    if (worker->workerThread.joinable()) {
        worker->workerThread.join();
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));

    LogInfo("Worker thread stopped");
    return result;
}

napi_value GetWorkerStats(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Requires 1 argument: worker handle");

    napi_valuetype handleType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &handleType));
    NAPI_ASSERT(env, handleType == napi_number, "Worker handle must be a number");

    int64_t handleValue = 0;
    NAPI_CALL(env, napi_get_value_int64(env, argv[0], &handleValue));

    auto* worker = reinterpret_cast<ThreadWorker*>(handleValue);
    if (worker == nullptr) {
        napi_throw_error(env, nullptr, "Invalid worker handle");
        return nullptr;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value runningValue = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, worker->running.load(), &runningValue));
    NAPI_CALL(env, napi_set_named_property(env, result, "running", runningValue));

    napi_value processedCountValue = nullptr;
    NAPI_CALL(env, napi_create_int32(env, worker->processedCount.load(), &processedCountValue));
    NAPI_CALL(env, napi_set_named_property(env, result, "processedCount", processedCountValue));

    size_t queueSize = 0;
    {
        std::lock_guard<std::mutex> lock(worker->mutex);
        queueSize = worker->taskQueue.size();
    }

    napi_value queueSizeValue = nullptr;
    NAPI_CALL(env, napi_create_uint32(env, queueSize, &queueSizeValue));
    NAPI_CALL(env, napi_set_named_property(env, result, "queueSize", queueSizeValue));

    return result;
}

}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("runAsyncWork", RunAsyncWork),
        DECLARE_NAPI_FUNCTION("createThreadsafeFunction", CreateThreadsafeFunction),
        DECLARE_NAPI_FUNCTION("callThreadsafeFunction", CallThreadsafeFunction),
        DECLARE_NAPI_FUNCTION("createWorkerThread", CreateWorkerThread),
        DECLARE_NAPI_FUNCTION("submitTaskToWorker", SubmitTaskToWorker),
        DECLARE_NAPI_FUNCTION("stopWorkerThread", StopWorkerThread),
        DECLARE_NAPI_FUNCTION("getWorkerStats", GetWorkerStats),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

static napi_module g_threadsafeAsyncModule = {
    .nm_version = MODULE_VERSION,
    .nm_flags = NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "threadsafe_async",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterThreadsafeAsyncModule(void)
{
    napi_module_register(&g_threadsafeAsyncModule);
}
