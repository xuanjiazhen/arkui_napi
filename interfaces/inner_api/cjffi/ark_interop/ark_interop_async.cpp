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

#include "ark_interop_internal.h"
#include "ark_interop_napi.h"
#include "ark_interop_log.h"

#include <unordered_map>
#include <mutex>

using namespace panda::ecmascript;

/**
 * @file ark_interop_async.cpp
 * @brief Marshals async callbacks from worker threads onto the ArkTS thread: prefers `uv_async` tied to `napi_env`, else falls back to `EventHandler::PostTask` on OHOS.
 *
 * On first bind of `vm` to `uv_loop_t`, `ARKTSInner_InitLoop` registers `uv_async` and N-API env cleanup hooks so the queue is freed on engine shutdown and Cangjie global state is released.
 */

#ifdef __OHOS__
using namespace OHOS::AppExecFwk;

namespace {
std::unordered_map<ARKTS_Env, ARKTS_Loop> g_eventHandlers_;
std::mutex g_eventHandlerMutex_;

struct ARKTS_Loop_ {
    enum Status {
        IDLE,
        REQUESTING,
        STOPPING,
        STOPPED,
    };
    uv_async_t asyncReq;
    Status status;
    std::vector<std::function<void ()>> callbacks;
    std::mutex mutex;
    std::condition_variable cv;
    bool hasCleanupHook;

    explicit ARKTS_Loop_(napi_env env, uv_loop_t* loop);
    ~ARKTS_Loop_() = default;

    void PostTask(std::function<void ()> task);
    // only to be call by uv_queue_task
    void DrainTasks();
};

std::unordered_map<ARKTS_Env, ARKTS_Loop_*> g_uvLoops_;
std::mutex g_uvLoopMutex_;
}
#endif

/**
 * @brief (Deprecated path) Lazily creates and caches an `EventHandler` backed by `EventRunner::Current()` per `ARKTS_Env`.
 * @param env EcmaVM pointer cast to `ARKTS_Env`.
 * @return Existing or new handler; always `nullptr` when not building for OHOS.
 */
ARKTS_Loop ARKTS_GetOrCreateEventHandler(ARKTS_Env env)
{
    if (!env) {
        return nullptr;
    }
#ifdef __OHOS__
    std::lock_guard lock(g_eventHandlerMutex_);
    auto searchRet = g_eventHandlers_.find(env);
    if (searchRet != g_eventHandlers_.end()) {
        return searchRet->second;
    }
    // New handler uses this thread's EventRunner so PostTask runs on the JS/runtime thread.
    auto handler = std::make_shared<EventHandler>(EventRunner::Current());
    g_eventHandlers_[env] = handler;
    return handler;
#else
    return nullptr;
#endif
}

/**
 * @brief Looks up `ARKTS_Loop_` for `env` (vm) in `g_uvLoops_` (populated only after successful `ARKTSInner_InitLoop`).
 * @note Table access is guarded by `g_uvLoopMutex_`.
 */
static ARKTS_Loop_* ARKTSInner_GetUvLoop(ARKTS_Env env)
{
    std::lock_guard lock(g_uvLoopMutex_);
    auto searchRet = g_uvLoops_.find(env);
    if (searchRet != g_uvLoops_.end()) {
        return searchRet->second;
    } else {
        return nullptr;
    }
}

#ifdef __OHOS__
/**
 * @brief Returns a registered `EventHandler` if any, without creating one (unlike `GetOrCreate`); used when no uv queue exists in the map.
 */
static ARKTS_Loop ARKTS_GetEventHandler(ARKTS_Env env)
{
    if (!env) {
        return nullptr;
    }
    std::lock_guard lock(g_eventHandlerMutex_);
    auto searchRet = g_eventHandlers_.find(env);
    if (searchRet != g_eventHandlers_.end()) {
        return searchRet->second;
    }
    return nullptr;
}
#endif

// @deprecated
/**
 * @brief Warms up a thread-local `EventHandler` by calling `ARKTS_GetOrCreateEventHandler` and discarding the result.
 * @deprecated Prefer letting `ARKTSInner_InitLoop` / engine init own async dispatch.
 */
void ARKTS_InitEventHandle(ARKTS_Env env)
{
    ARKTS_ASSERT_V(env, "env is null");
    void (ARKTS_GetOrCreateEventHandler(env));
}

/**
 * @brief Tears down async dispatch for `env`: drops cached `EventHandler` on OHOS and erases `g_uvLoops_` entries (`ARKTS_Loop_` deletion is done by N-API cleanup finalizer).
 */
void ARKTS_DisposeEventHandler(ARKTS_Env env)
{
    if (!env) {
        return;
    }
#ifdef __OHOS__
    {
        std::lock_guard lock(g_eventHandlerMutex_);
        g_eventHandlers_.erase(env);
    }
#endif
    {
        std::lock_guard lock(g_uvLoopMutex_);
        // Only removes the map entry; the ARKTS_Loop_* itself is deleted by napi finalizer when env dies.
        g_uvLoops_.erase(env);
    }
}

/**
 * @brief Posts `callback(env, data)` to the JS thread: if InitLoop ran, enqueues onto `ARKTS_Loop_`; otherwise on OHOS uses `EventHandler::PostTask`.
 * @param env Current EcmaVM as `ARKTS_Env`.
 * @param callback C function invoked on the target thread.
 * @param data Opaque id (often a Cangjie async task / handle slot).
 */
void ARKTSInner_CreateAsyncTask(ARKTS_Env env, ARKTS_AsyncCallback callback, int64_t data)
{
    // Prefer uv_async (N-API / Node style) and match InitLoop cleanup semantics.
    if (auto loop = ARKTSInner_GetUvLoop(env)) {
        loop->PostTask([env, callback, data] {
            callback(env, data);
        });
        return;
    }
#ifdef __OHOS__
    // Fallback when uv loop was never registered (e.g. legacy path without ARKTSInner_InitLoop).
    auto handler = ARKTS_GetEventHandler(env);
    if (!handler) {
        LOGE("event handler not initialized");
        return;
    }
    handler->PostTask([env, callback, data] {
        callback(env, data);
    });
#endif
}

/**
 * @brief Public Cangjie ABI: posts `callbackId` using `ARKTSInner_CJAsyncCallback`.
 * @param callbackId Cangjie-registered async lambda / task slot id.
 */
void ARKTS_CreateAsyncTask(ARKTS_Env env, int64_t callbackId)
{
    ARKTS_ASSERT_V(env, "env is null");
    ARKTSInner_CreateAsyncTask(env, ARKTSInner_CJAsyncCallback, callbackId);
}

/**
 * @brief Creates `uv_async` on `uv_loop_t` and drains the queue in the callback; registers a `napi_env` finalizer to `delete` this object when the env is destroyed.
 * @note If `hasCleanupHook` is false, construction failed and `InitLoop` tears down and returns false.
 */
ARKTS_Loop_::ARKTS_Loop_(napi_env env, uv_loop_t* loop) : asyncReq(), status(IDLE)
{
    // Wake thread: swap out pending tasks without holding the mutex during execution.
    auto error = uv_async_init(loop, &asyncReq, [](uv_async_t* work) {
        auto loop = static_cast<ARKTS_Loop_*>(work->data);
        if (loop) {
            loop->DrainTasks();
        }
    });
    if (error != 0) {
        LOGE("uv_async_init failed, status: %{public}d", error);
        hasCleanupHook = false;
        return;
    }
    asyncReq.data = this;
    // Finalizer runs when napi_env is shutting down, after native work should have stopped posting.
    hasCleanupHook = napi_add_cleanup_finalizer(env, [](void* data) {
        if (!data) {
            return;
        }
        auto loop = reinterpret_cast<ARKTS_Loop_*>(data);
        delete loop;
    }, this) == napi_ok;
}

/**
 * @brief Appends a task to `callbacks` under the mutex; if IDLE, calls `uv_async_send` (up to 3 retries).
 * @note Tasks are dropped when status is STOPPING/STOPPED to avoid Cangjie callbacks during shutdown.
 */
void ARKTS_Loop_::PostTask(std::function<void()> task)
{
    std::lock_guard lock(mutex);
    if (status >= STOPPING) {
        return;
    }
    callbacks.push_back(std::move(task));
    if (status == IDLE) {
        // uv_async_send is non-blocking but can transiently fail under load; retry a few times.
        int tryTimes = 3;
        while (tryTimes--) {
            auto ret = uv_async_send(&asyncReq);
            if (ret == 0) {
                status = REQUESTING;
                break;
            }
        }
    }
}

/**
 * @brief Called on the uv thread: swaps `callbacks` under lock, resets to IDLE, then runs tasks without the lock.
 */
void ARKTS_Loop_::DrainTasks()
{
    std::vector<std::function<void ()>> pendingTasks;
    {
        std::lock_guard lock(mutex);
        // Shutdown: drop pending callbacks and notify waiters if any.
        if (status >= STOPPING) {
            if (status == STOPPING) {
                status = STOPPED;
                cv.notify_all();
            }
            return;
        }
        // Normal path: usually REQUESTING with a non-empty queue; IDLE again allows PostTask to send.
        std::swap(callbacks, pendingTasks);
        status = IDLE;
    }
    for (const auto& task : pendingTasks) {
        task();
    }
}

// napi_add_env_cleanup_hook dedupes by hint; using env as hint can collide, so use a dedicated CleanupTask.
struct CleanupTask {
    ARKTS_Env env;
};

/**
 * @brief Binds `vm` to the host `uv_loop_t`: allocates `ARKTS_Loop_`, stores it in `g_uvLoops_`, and registers `napi_add_env_cleanup_hook` to call
 *        `ARKTS_DisposeJSContext`, `ARKTS_DisposeEventHandler`, and `ARKTS_RemoveGlobalManager` when the env is torn down.
 * @return True if mapping exists or setup succeeds; false on allocation failure or missing cleanup hook.
 */
bool ARKTSInner_InitLoop(napi_env env, ARKTS_Env vm, uv_loop_t* loop)
{
    std::lock_guard lock(g_uvLoopMutex_);
    if (g_uvLoops_.find(vm) != g_uvLoops_.end()) {
        return true;
    }
    auto arktsLoop = new (std::nothrow) ARKTS_Loop_(env, loop);
    if (!arktsLoop) {
        LOGE("new ARKTS_Loop_ failed.");
        return false;
    }
    if (!arktsLoop->hasCleanupHook) {
        LOGE("ARKTSLoop has no cleanup hook.");
        delete arktsLoop;
        return false;
    }

    auto cleanupTask = new (std::nothrow) CleanupTask {vm};
    if (!cleanupTask) {
        LOGE("new CleanupTask failed.");
        delete arktsLoop;
        return false;
    }
    // Order: notify Cangjie JSContext teardown, drop async maps, then global-manager tables (see each callee).
    auto status = napi_add_env_cleanup_hook(env, [](void* data) {
        if (!data) {
            return;
        }
        auto task = reinterpret_cast<CleanupTask*>(data);
        ARKTS_DisposeJSContext(task->env);
        ARKTS_DisposeEventHandler(task->env);
        ARKTS_RemoveGlobalManager(task->env);
        delete task;
    }, cleanupTask);
    if (status != napi_ok) {
        LOGE("add env cleanup hook failed: %{public}d", status);
        delete arktsLoop;
        delete cleanupTask;
        return false;
    }
    // GlobalManager must exist before any ARKTS_Global disposal that schedules AsyncDisposer.
    ARKTS_InitGlobalManager(vm);
    g_uvLoops_[vm] = arktsLoop;
    return true;
}