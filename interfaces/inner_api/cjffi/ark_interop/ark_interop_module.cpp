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

#include <unordered_map>

#include <node_api.h>
#include <unistd.h>

#include "ark_interop_internal.h"
#include "ark_interop_external.h"
#include "ark_interop_log.h"
#include "cj_envsetup.h"

/**
 * @file ark_interop_module.cpp
 * @brief Loads Cangjie modules and registers callbacks: wires `OHOS::CJEnv`, `exportModule`, and bridges for ArrayBuffer/External/lambda/async/JSContext teardown.
 */

struct ARKTS_ModuleCallbacks {
    ARKTS_Value (*exportModule)(ARKTS_Env env, const char* dllName, ARKTS_Value exports) = nullptr;
    bool (*hasModuleHandle)(const char* dllName) = nullptr;
    void (*throwJSError)(ARKTS_Env env, ARKTS_Value) = nullptr;
    void (*throwNativeError)(const char*) = nullptr;
    void (*deleteArrayBufferRawData)(void* buffer, int64_t lambdaId) = nullptr;
    void (*deleteExternal)(int64_t id, ARKTS_Env env) = nullptr;
    ARKTS_Value (*invokerLambda)(ARKTS_CallInfo, int64_t lambdaId) = nullptr;
    void (*deleteLambda)(ARKTS_Env env, int64_t lambdaId) = nullptr;
    void (*invokeAsyncLambda)(ARKTS_Env env, int64_t lambdaId) = nullptr;
    void (*deleteJSContext)(ARKTS_Env env) = nullptr;
};

namespace {
// each native register only be available during module loading
ARKTS_ModuleCallbacks* g_cjModuleCallbacks = nullptr;
}

/**
 * @brief Forwards JS error `error` to the registered Cangjie `throwJSError`; logs and returns false if callbacks are missing.
 */
bool ARKTSInner_ThrowJSErrorToCJ(ARKTS_Env env, ARKTS_Value error)
{
    if (!g_cjModuleCallbacks) {
        LOGE("napi module depends on another napi module is forbidden");
        return false;
    }
    g_cjModuleCallbacks->throwJSError(env, error);
    return true;
}

/**
 * @brief Forwards a native C-string error to Cangjie `throwNativeError`; logs and returns false if callbacks are missing.
 */
bool ARKTSInner_ThrowNativeErrorToCJ(const char* error)
{
    if (!g_cjModuleCallbacks) {
        LOGE("napi module depends on another napi module is forbidden");
        return false;
    }
    g_cjModuleCallbacks->throwNativeError(error);
    return true;
}

/**
 * @brief Registers the Cangjie callback vtable once per process; duplicate registration logs an error and is ignored.
 * @note Copied to the heap for stable lifetime (caller stack must not be used).
 */
void ARKTS_SetCJModuleCallback(ARKTS_ModuleCallbacks* callback)
{
    if (!callback) {
        LOGE("register empty module callback is senseless");
        return;
    }
    if (g_cjModuleCallbacks) {
        LOGE("should never happen");
        return;
    }
    g_cjModuleCallbacks = new ARKTS_ModuleCallbacks(*callback);
}

/**
 * @brief On the main thread, registers stack-unwind info and the ArkVM pointer with the Cangjie runtime (stack-overflow diagnostics).
 * @note Uses `getpid()==gettid()` as a coarse main-thread check; `fixed` ensures a single registration.
 */
static void FixStackOverflow(OHOS::CJEnvMethods* methods, ARKTS_Env env)
{
    static bool fixed = false;
    if (fixed) {
        return;
    }
    // Stack/VM registration is only meaningful from the process main thread in this embedding.
    if (getpid() != gettid()) {
        return;
    }
    methods->registerStackInfoCallbacks(ARKTS_UpdateStackInfo);
    methods->registerArkVMInRuntime(reinterpret_cast<unsigned long long>(env));
    fixed = true;
}

/**
 * @brief Loads a Cangjie native module and fills `exports` under `JsiFastNativeScope` + `ARKTS_OpenScope`.
 * @return Escaped `JSValueRef*` via `ARKTSInner_Escape` (usually exports); on failure escapes `Undefined`.
 * @note Internal to interop; also used from the NAPI entry path.
 */
panda::JSValueRef* ARKTS_LoadModule(ARKTS_Env env, const char* dllName)
{
    LOGD("ARKTS_LoadCJModule start: %{public}s", dllName);
    static std::mutex globalMutex;
    std::lock_guard lock(globalMutex);
    panda::JsiFastNativeScope fastNativeScope(reinterpret_cast<panda::EcmaVM*>(env));
    auto scope = ARKTS_OpenScope(env);

    auto undefined = ARKTS_CreateUndefined();
    auto runtime = OHOS::CJEnv::LoadInstance();
    if (!runtime) {
        return ARKTSInner_Escape(env, scope, undefined);
    }
    FixStackOverflow(runtime, env);
    if (!runtime->startRuntime()) {
        LOGE("start cj runtime failed");
        return ARKTSInner_Escape(env, scope, undefined);
    }
    // Skip loadCJModule if interop already mapped this dll (e.g. preload); otherwise dlopen the Cangjie .so.
    if (!g_cjModuleCallbacks || !g_cjModuleCallbacks->hasModuleHandle(dllName)) {
        if (!runtime->loadCJModule(dllName)) {
            LOGE("load cj library failed, try load as native");
            return ARKTSInner_Escape(env, scope, undefined);
        }
    }

    // exportModule is registered by ohos_ark_interop; without it we cannot populate exports.
    if (!g_cjModuleCallbacks) {
        LOGE("cj module must dependent on ohos_ark_interop");
        return ARKTSInner_Escape(env, scope, undefined);
    }

    auto exports = ARKTS_CreateObject(env);

    exports = g_cjModuleCallbacks->exportModule(env, dllName, exports);

    // Escape closes `scope` and returns a JSValueRef* valid for the outer VM handle scope.
    return ARKTSInner_Escape(env, scope, exports);
}

/**
 * @brief Resolves `EcmaVM` and `uv_loop` from `NativeEngine*`, ensures `ARKTSInner_InitLoop`, then calls `ARKTS_LoadModule`.
 */
EXPORT panda::JSValueRef* ARKTS_LoadModuleByNapiEnv(void* env, const char* dllName)
{
    if (!env) {
        LOGE("env is null");
        return nullptr;
    }
    if (!dllName) {
        LOGE("dllName is null");
        return nullptr;
    }
    auto engine = reinterpret_cast<NativeEngine*>(env);
    auto vm = const_cast<EcmaVM*>(engine->GetEcmaVm());
    if (!vm) {
        LOGE("vm is null");
        return nullptr;
    }
    auto loop = engine->GetUVLoop();
    if (!loop) {
        LOGE("uvloop is null");
        return nullptr;
    }
    // Required so ARKTSInner_CreateAsyncTask and global disposal have a uv-backed queue tied to this napi_env.
    if (!ARKTSInner_InitLoop(reinterpret_cast<napi_env>(env), reinterpret_cast<ARKTS_Env>(vm), loop)) {
        LOGE("init loop failed");
        return nullptr;
    }

    return ARKTS_LoadModule(reinterpret_cast<ARKTS_Env>(vm), dllName);
}

/**
 * @brief ArrayBuffer backing-store finalizer: `hint` carries the Cangjie `lambdaId` (encoding). 
 * @note On 32-bit, `hint` points at heap `ARKTSInner_External` because pointers cannot hold a full int64 id.
 */
void ARKTSInner_CJArrayBufferDeleter(void*, void* buffer, void* hint)
{
    if (!g_cjModuleCallbacks) {
        LOGE("native ark-interop library has no registered module");
        return;
    }
    // 64-bit: hint is the id; 32-bit: hint is heap ARKTSInner_External*.
    if constexpr (sizeof(void*) == 8) {
        g_cjModuleCallbacks->deleteArrayBufferRawData(buffer,
            reinterpret_cast<int64_t>(hint));
    } else {
        auto pointer = reinterpret_cast<ARKTSInner_External*>(hint);
        g_cjModuleCallbacks->deleteArrayBufferRawData(buffer, pointer->data);
        delete pointer;
    }
}

/**
 * @brief External object finalizer: `data` is the Cangjie `SharedObject` id; third argument is `ARKTS_Env`.
 */
void ARKTSInner_CJExternalDeleter(void*, void* data, void* env)
{
    if (!g_cjModuleCallbacks) {
        LOGE("native ark-interop library has no registered module");
        return;
    }
    // 64-bit: data is the id; 32-bit: data is ARKTSInner_External*.
    if constexpr (sizeof(void*) == 8) {
        g_cjModuleCallbacks->deleteExternal(
            reinterpret_cast<int64_t>(data),
            reinterpret_cast<ARKTS_Env>(env)
        );
    } else {
        auto pointer = reinterpret_cast<ARKTSInner_External*>(data);
        g_cjModuleCallbacks->deleteExternal(
            pointer->data,
            reinterpret_cast<ARKTS_Env>(env)
        );
        delete pointer;
    }
}

/**
 * @brief JS-to-Cangjie lambda bridge: opens a scope, calls `invokerLambda`, then returns via `ARKTS_Result` / `ARKTS_Return`.
 */
ARKTS_Result ARKTSInner_CJLambdaInvoker(ARKTS_CallInfo callInfo, int64_t lambdaId)
{
    auto env = ARKTS_GetCallEnv(callInfo);
    auto scope = ARKTS_OpenScope(env);

    if (!g_cjModuleCallbacks) {
        LOGE("native ark-interop library has no registered module");
        return ARKTS_Return(env, scope, ARKTS_CreateUndefined());
    }

    // invokerLambda returns ARKTS_Value; ARKTS_Return re-roots it for the caller's result slot.
    auto result = g_cjModuleCallbacks->invokerLambda(callInfo, lambdaId);
    return ARKTS_Return(env, scope, result);
}

/** @brief Releases the Cangjie lambda slot when the JS function proxy is destroyed. */
void ARKTSInner_CJLambdaDeleter(ARKTS_Env env, int64_t lambdaId)
{
    if (!g_cjModuleCallbacks) {
        LOGE("native ark-interop library has no registered module");
        return;
    }

    g_cjModuleCallbacks->deleteLambda(env, lambdaId);
}

/**
 * @brief Fixed entry for `ARKTS_CreateAsyncTask` on the JS thread: treats `data` as a Cangjie async task id for `invokeAsyncLambda`.
 */
void ARKTSInner_CJAsyncCallback(ARKTS_Env env, int64_t data)
{
    if (!g_cjModuleCallbacks) {
        LOGE("native ark-interop library has no registered module");
        return;
    }

    g_cjModuleCallbacks->invokeAsyncLambda(env, data);
}

/**
 * @brief N-API / engine cleanup hook: notifies Cangjie to tear down the `JSContext` tied to `env`.
 */
void ARKTS_DisposeJSContext(ARKTS_Env env)
{
    if (!g_cjModuleCallbacks) {
        LOGE("native ark-interop library has no registered module");
        return;
    }
    g_cjModuleCallbacks->deleteJSContext(env);
}
