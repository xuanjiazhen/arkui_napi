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

#include <string>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <memory>

#include "ark_interop_internal.h"
#include "ark_interop_log.h"
#include "ark_interop_napi.h"
#include "ark_interop_external.h"
#include "jsnapi.h"
#include "native_engine/impl/ark/ark_native_engine.h"
#ifdef __OHOS__
#include "uv_loop_handler.h"
#endif

/**
 * @file ark_interop_engine.cpp
 * @brief ArkTS engine lifecycle: create/destroy `EcmaVM` and `ArkNativeEngine`, integrate uv with OHOS `EventHandler`, load ABC entry points, and bridge `require`.
 */

struct ARKTS_Engine_ {
    panda::EcmaVM* vm;
    ArkNativeEngine* engine;
    std::unordered_map<std::string, std::string> loadedAbcs;
#ifdef __OHOS__
    std::shared_ptr<OHOS::AppExecFwk::EventHandler> eventHandler;
    std::shared_ptr<OHOS::AppExecFwk::EventRunner> newRunner;
#endif
    uint64_t threadId;
};

#ifdef __OHOS__
static constexpr char TIMER_TASK[] = "uv_timer_task";

/** @brief `FileDescriptorListener`: backend fd became readable; forwards to `OnTriggered`. */
void UVLoopHandler::OnReadable(int32_t)
{
    OnTriggered();
}

/** @brief `FileDescriptorListener`: backend fd became writable; forwards to `OnTriggered`. */
void UVLoopHandler::OnWritable(int32_t)
{
    OnTriggered();
}

/**
 * @brief Core driver: runs `uv_run(NOWAIT)`, then schedules the next wake-up on `EventHandler` using `uv_backend_timeout` (libuv timer semantics).
 * @note Under `RUNTIME_EMULATOR`, skips duplicate-timestamp short-circuit because the host clock may be inaccurate.
 */
void UVLoopHandler::OnTriggered()
{
    // Process work/timer callbacks already due; do not block for I/O.
    uv_run(uvLoop_, UV_RUN_NOWAIT);
    auto eventHandler = GetOwner();
    if (!eventHandler) {
        return;
    }

    int32_t timeout = uv_backend_timeout(uvLoop_);
    if (timeout < 0) {
        // No pending libuv timers; cancel our echo timer on the EventHandler if any.
        if (haveTimerTask_) {
            eventHandler->RemoveTask(TIMER_TASK);
        }
        return;
    }

    // Next wakeup = now + backend timeout; used to dedupe identical scheduled instants (except emulator).
    int64_t timeStamp = static_cast<int64_t>(uv_now(uvLoop_)) + timeout;
    // we don't check timestamp in emulator for computer clock is inaccurate
#ifndef RUNTIME_EMULATOR
    if (timeStamp == lastTimeStamp_) {
        return;
    }
#endif

    if (haveTimerTask_) {
        eventHandler->RemoveTask(TIMER_TASK);
    }

    auto callback = [wp = weak_from_this()] {
        auto sp = wp.lock();
        if (sp) {
            // Timer task is triggered, so there is no timer task now.
            sp->haveTimerTask_ = false;
            sp->OnTriggered();
        }
    };
    eventHandler->PostTask(callback, TIMER_TASK, timeout);
    lastTimeStamp_ = timeStamp;
    haveTimerTask_ = true;
}
#endif

/**
 * @brief Synchronously tears down the engine: removes fd listener, deletes `ArkNativeEngine`, destroys the VM, runs `ARKTS_DisposeEventHandler`, etc.
 * @note On OHOS the engine thread destroys via `PostTask` into this function; otherwise it is called directly.
 */
static void ARKTSInner_DisposeEngine(ARKTS_Engine engine)
{
#ifdef __OHOS__
    if (auto loop = engine->engine->GetUVLoop()) {
        auto fd = uv_backend_fd(loop);
        engine->eventHandler->RemoveFileDescriptorListener(fd);
    }
#endif
    // Drop Cangjie runtime hooks before tearing down native engine and VM.
    ARKTS_DisposeJSContext(reinterpret_cast<ARKTS_Env>(engine->vm));
    delete engine->engine;
    if (engine->vm) {
        auto env = P_CAST(engine->vm, ARKTS_Env);
        // Clears g_uvLoops_/handlers so async work cannot target a dead VM.
        ARKTS_DisposeEventHandler(env);
        panda::JSNApi::DestroyJSVM(engine->vm);
    }
    engine->engine = nullptr;
    engine->vm = nullptr;
}

/**
 * @brief Binds the NativeEngine uv loop to the VM (`JSNApi::SetLoop`) and `ARKTSInner_InitLoop` for async bridging; on OHOS registers backend fd listener to drive uv.
 */
static bool ARKTSInner_InitEngineLoop(ARKTS_Engine result, ArkNativeEngine* engine, EcmaVM* vm)
{
#ifdef __OHOS__
    result->eventHandler = ARKTS_GetOrCreateEventHandler(P_CAST(vm, ARKTS_Env));
    if (!result->eventHandler) {
        return false;
    }
#endif
    auto loop = engine->GetUVLoop();
    if (!loop) {
        if (!engine->ReinitUVLoop()) {
            LOGE("init uv loop failed");
            return false;
        }
        loop = engine->GetUVLoop();
    }
    panda::JSNApi::SetLoop(vm, loop);
    if (!ARKTSInner_InitLoop(reinterpret_cast<napi_env>(engine), reinterpret_cast<ARKTS_Env>(vm), loop)) {
        LOGE("init async loop failed");
        return false;
    }

#ifdef __OHOS__
    // Prime the loop so backend fd and timers exist before we subscribe the FileDescriptorListener.
    uv_run(loop, UV_RUN_NOWAIT);
    auto fd = uv_backend_fd(loop);
    uint32_t events = OHOS::AppExecFwk::FILE_DESCRIPTOR_INPUT_EVENT | OHOS::AppExecFwk::FILE_DESCRIPTOR_OUTPUT_EVENT;
    result->eventHandler->AddFileDescriptorListener(fd, events, std::make_shared<UVLoopHandler>(loop), "uvLoopTask");
#endif
    return true;
}

/**
 * @brief Creates an `EcmaVM` with default `RuntimeOption`, wraps `ArkNativeEngine`, and completes uv/async setup.
 * @return `ARKTS_Engine` on success; on failure frees partial allocations and returns `nullptr`.
 */
ARKTS_Engine ARKTS_CreateEngine()
{
    panda::RuntimeOption options;
    options.SetLogLevel(panda::RuntimeOption::LOG_LEVEL::INFO);
    auto vm = panda::JSNApi::CreateJSVM(options);
    if (!vm) {
        LOGE("create EcmaVM failed");
        return nullptr;
    }
    auto engine = new ArkNativeEngine(vm, nullptr);
    if (!engine) {
        LOGE("alloc ArkEngine failed");
        panda::JSNApi::DestroyJSVM(vm);
        return nullptr;
    }

    auto result = new ARKTS_Engine_;
    if (!result) {
        LOGE("alloc ARKTS_Engine_ failed");
        panda::JSNApi::DestroyJSVM(vm);
        delete engine;
        return nullptr;
    }
    result->vm = vm;
    result->engine = engine;
    result->threadId = ARKTS_GetPosixThreadId();

    if (!ARKTSInner_InitEngineLoop(result, engine, vm)) {
        ARKTSInner_DisposeEngine(result);
        delete result;
        return nullptr;
    }

    return result;
}

/**
 * @brief Returns the underlying `ArkNativeEngine*` (usable as `napi_env` for Native API calls).
 */
void* ARKTS_GetNAPIEnv(ARKTS_Engine engine)
{
    ARKTS_ASSERT_P(engine, "engine is null");
    return engine->engine;
}

/**
 * @brief Destroys the engine: **must not** be called on the engine's own thread (asserts `GetPosixThreadId` != `GetThreadIdOfEngine`).
 *        On OHOS posts disposal onto the engine thread with `EventHandler::PostTask`, waits for completion, removes fd listeners, and stops the runner.
 */
void ARKTS_DestroyEngine(ARKTS_Engine engine)
{
    ARKTS_ASSERT_V(engine, "engine is null");
    ARKTS_ASSERT_V(ARKTS_GetPosixThreadId() != ARKTS_GetThreadIdOfEngine(engine),
        "can not destroy engine at it's running thread");
#ifdef __OHOS__
    std::condition_variable cv;
    std::atomic<bool> isComplete = false;
    engine->eventHandler->PostTask([engine, &cv, &isComplete] {
        ARKTSInner_DisposeEngine(engine);
        isComplete = true;
        cv.notify_one();
    });
    std::mutex mutex;
    std::unique_lock lock(mutex);
    // Poll wait: destruction must run on engine thread; spurious wakeups are harmless.
    while (!isComplete) {
        cv.wait_for(lock, std::chrono::seconds(1));
    }
    engine->eventHandler->RemoveAllFileDescriptorListeners();
    if (engine->newRunner) {
        engine->newRunner->Stop();
    }
#else
    ARKTSInner_DisposeEngine(engine);
#endif
    delete engine;
}

/**
 * @brief Casts the engine's `EcmaVM*` to `ARKTS_Env` (Cangjie VM handle).
 */
ARKTS_Env ARKTS_GetContext(ARKTS_Engine engine)
{
    ARKTS_ASSERT_P(engine, "engine is null");
    return P_CAST(engine->vm, ARKTS_Env);
}

/**
 * @brief Executes an entry in a `.abc` (`JSNApi::Execute`) and records `entryPoint -> filePath` so the same entry cannot be shadowed by another file.
 * @param forceReload If true, skips cache lookup but still requires a readable file and successful Execute.
 */
bool ARKTS_LoadEntryFromAbc(ARKTS_Engine engine, const char* filePath, const char* entryPoint, bool forceReload)
{
    ARKTS_ASSERT_F(engine, "engine is null");
    ARKTS_ASSERT_F(filePath, "filePath is null");
    ARKTS_ASSERT_F(entryPoint, "entryPoint is null");

    if (!forceReload) {
        auto existed = engine->loadedAbcs.find(entryPoint);
        if (existed != engine->loadedAbcs.end()) {
            if (existed->second == filePath) {
                // Same entry already executed from this abc; avoid duplicate Execute.
                return true;
            } else {
                LOGE("can't shadow loaded entryPoint from another .abc, entryPoint: %{public}s", entryPoint);
                return false;
            }
        }
    }
    if (access(filePath, R_OK) != 0) {
        LOGE("no such file: %{public}s", filePath);
        return false;
    }
    auto vm = engine->vm;
    panda::JSNApi::NotifyLoadModule(vm);
    auto success = panda::JSNApi::Execute(vm, filePath, entryPoint);
    if (success) {
        engine->loadedAbcs[entryPoint] = filePath;
    }
    return success;
}

/**
 * @brief Calls `JSNApi::GetExportObject` on a loaded `entryPoint`; returns `Undefined` if not loaded or export is hole.
 */
ARKTS_Value ARKTS_ImportFromEntry(ARKTS_Engine engine, const char* entryPoint, const char* importName)
{
    ARKTS_ASSERT_U(engine, "engine is null");
    ARKTS_ASSERT_U(entryPoint, "entryPoint is null");
    ARKTS_ASSERT_U(importName, "importName is null");

    if (engine->loadedAbcs.find(entryPoint) == engine->loadedAbcs.end()) {
        return ARKTS_CreateUndefined();
    }

    auto vm = engine->vm;
    auto value = panda::JSNApi::GetExportObject(vm, entryPoint, importName);
    if (value->IsHole()) {
        return ARKTS_CreateUndefined();
    }

    return ARKTS_FromHandle(value);
}

/**
 * @brief Loads via global `requireInternal` (ArkTS modules) or `requireNapi` (native NAPI).
 * @param relativePath If non-null, calls `requireNapi` with the four-argument form (app flag and relative path).
 */
ARKTS_Value ARKTS_Require(
    ARKTS_Env env, const char* target, bool isNativeModule, bool isAppModule, const char* relativePath)
{
    ARKTS_ASSERT_U(env, "env is null");

    auto global = ARKTS_GetGlobalConstant(env);
    auto targetValue = ARKTS_CreateUtf8(env, target, -1);

    ARKTS_ASSERT_U(ARKTS_IsHeapObject(global), "js global is null");
    if (!isNativeModule) {
        // ArkTS module graph: runtime installs `requireInternal` on globalThis.
        auto funcName = ARKTS_CreateUtf8(env, "requireInternal", -1);
        auto funcValue = ARKTS_GetProperty(env, global, funcName);
        ARKTS_ASSERT_U(ARKTS_IsCallable(env, funcValue), "global func requireInternal is undefined");
        return ARKTS_Call(env, funcValue, ARKTS_CreateUndefined(), 1, &targetValue);
    }
    // Native .so path: third slot is reserved (undefined) when relativePath is used (compat with host signature).
    auto funcName = ARKTS_CreateUtf8(env, "requireNapi", -1);
    auto funcValue = ARKTS_GetProperty(env, global, funcName);
    ARKTS_ASSERT_U(ARKTS_IsCallable(env, funcValue), "global func requireNapi is undefined");

    if (relativePath) {
        ARKTS_Value args[] = { targetValue, ARKTS_CreateBool(isAppModule), ARKTS_CreateUndefined(),
            ARKTS_CreateUtf8(env, relativePath, -1) };
        return ARKTS_Call(env, funcValue, ARKTS_CreateUndefined(), sizeof(args) / sizeof(ARKTS_Value), args);
    } else {
        ARKTS_Value args[] = { targetValue, ARKTS_CreateBool(isAppModule) };
        return ARKTS_Call(env, funcValue, ARKTS_CreateUndefined(), sizeof(args) / sizeof(ARKTS_Value), args);
    }
}

#ifdef __OHOS__
/**
 * @brief Runs `ARKTS_CreateEngine()` on a dedicated `EventRunner` thread and waits on a condition variable; stops the runner on failure.
 * @return On success, stores `newRunner` in `ARKTS_Engine_` for later shutdown in `DestroyEngine`.
 */
ARKTS_Engine ARKTS_CreateEngineWithNewThread()
{
    ARKTS_Engine result = nullptr;
    auto newRunner = OHOS::AppExecFwk::EventRunner::Create(true);
    if (!newRunner) {
        return nullptr;
    }
    auto handler = std::make_shared<OHOS::AppExecFwk::EventHandler>(newRunner);
    if (!handler) {
        newRunner->Stop();
        return nullptr;
    }
    std::mutex mutex;
    std::unique_lock lock(mutex);
    std::condition_variable cv;
    std::atomic<bool> createComplete = false;
    auto postSuccess = handler->PostTask([&result, &cv, &createComplete] {
        result = ARKTS_CreateEngine();
        createComplete = true;
        cv.notify_one();
    });
    if (!postSuccess) {
        newRunner->Stop();
        return nullptr;
    }
    // Engine and EventHandler must be created on the runner thread; spin until PostTask finishes.
    while (!createComplete) {
        cv.wait_for(lock, std::chrono::milliseconds(1));
    }
    if (!result) {
        newRunner->Stop();
        return nullptr;
    }
    result->newRunner = newRunner;
    return result;
}
#endif

/** @brief POSIX thread id captured at engine creation (used to reject same-thread destroy). */
uint64_t ARKTS_GetThreadIdOfEngine(ARKTS_Engine engine)
{
    ARKTS_ASSERT_I(engine, "engine is null");
    return engine->threadId;
}
