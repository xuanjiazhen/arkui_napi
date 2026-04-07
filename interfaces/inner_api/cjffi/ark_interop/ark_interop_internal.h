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

#ifndef NAPI_ARK_INTEROP_INTERNAL_H
#define NAPI_ARK_INTEROP_INTERNAL_H

#include <string>
#include <native_engine/native_engine.h>

#include "ark_interop_macro.h"
#include "ark_interop_napi.h"
#include "ecmascript/napi/include/jsnapi.h"

using EcmaVM = panda::EcmaVM;

// below interface shares internal, never export
/** Callback invoked on the JS/uv loop thread with an opaque `data` id (async task / Cangjie queue slot). */
typedef void (*ARKTS_AsyncCallback)(ARKTS_Env env, int64_t);

/**
 * Internal implementation contracts for ArkTS/Cangjie interop.
 *
 * This header is *not* a public API. It bridges:
 * - ArkTS VM (Panda/JSNApi, EcmaVM, JSValueRef)
 * - N-API (NativeEngine / napi_env)
 * - Cangjie callbacks (ArrayBuffer/External/Lambda/Async destruction and invocation)
 *
 * Key constraints:
 * - Thread affinity: most VM operations must run on the runtime-bound thread; cross-thread access/free can crash GC.
 * - Value representation: `ARKTS_Value` is a bit-castable handle that can represent either a heap-object address or
 *   an inlined non-heap value. Conversions must branch on `IsHeapObject()` to avoid misinterpreting stack addresses.
 * - Callback contracts: ArkTS GC / event loop invokes callbacks at safe points; the Cangjie layer maintains its
 *   registries and reference counts based on those callbacks.
 */

/** Forward pending VM exception to Cangjie as a typed error (or log if configured). */
bool ARKTSInner_ReportJSErrors(ARKTS_Env env, bool abortIfUnhandled);
/** Log-format native fault then marshal into the Cangjie exception channel. */
void ARKTSInner_ReportNativeError(const char* format, ...) __attribute__((format(printf, 1, 2)));
/** Produce a single readable string for a JS error object/string (cross-language diagnostics). */
COV_EXPORT std::string ARKTSInner_FormatJSError(ARKTS_Env env, ARKTS_Value jsError);
/** Serialize `error` into fields expected by the Cangjie `MixedException` pipeline. */
bool ARKTSInner_ThrowJSErrorToCJ(ARKTS_Env env, ARKTS_Value error);
/** Throw a plain native message into Cangjie without a JS object. */
bool ARKTSInner_ThrowNativeErrorToCJ(const char* error);
/** Queue `callback(data)` on the runtime’s async loop (uv preferred; falls back to `EventHandler`). */
void ARKTSInner_CreateAsyncTask(ARKTS_Env env, ARKTS_AsyncCallback callback, int64_t data);
// Cangjie-registered GC / lifetime hooks
/** ArrayBuffer backing store finalizer: hint indexes a Cangjie finalizer slab entry. */
void ARKTSInner_CJArrayBufferDeleter(void*, void* buffer, void* hint);
/** External/native-pointer finalizer: payload is a Cangjie `SharedObject` id; `env` is `ARKTS_Env`. */
void ARKTSInner_CJExternalDeleter(void*, void* data, void* env);
/** Bridge from JS function invocation to Cangjie lambda `lambdaId`; returns raw `ARKTS_Result` bit pattern. */
ARKTS_Result ARKTSInner_CJLambdaInvoker(ARKTS_CallInfo callInfo, int64_t lambdaId);
/** Release lambda registration when the JS function proxy dies. */
void ARKTSInner_CJLambdaDeleter(ARKTS_Env env, int64_t lambdaId);
/** Entry point for async tasks created via `ARKTS_CreateAsyncTask` / `ARKTSInner_CreateAsyncTask`. */
COV_EXPORT void ARKTSInner_CJAsyncCallback(ARKTS_Env env, int64_t data);

/** Close scope then escape `ret` into a VM local for return-by-out-param patterns. */
panda::JSValueRef* ARKTSInner_Escape(ARKTS_Env env, ARKTS_Scope scope, ARKTS_Value ret);

/** 32-bit helper payload when a pointer-sized External cannot store the full int64 id inline. */
struct ARKTSInner_External {
    int64_t data;
};

#ifdef __OHOS__
#include <memory>
#include "event_handler.h"

using ARKTS_Loop = std::shared_ptr<OHOS::AppExecFwk::EventHandler>;
#else
using ARKTS_Loop = void*;
#endif

// @deprecated
/** @deprecated Prefer uv-based dispatch from `ARKTSInner_InitLoop`; legacy `EventHandler` accessor per `ARKTS_Env`. */
ARKTS_Loop ARKTS_GetOrCreateEventHandler(ARKTS_Env env);
/** Wire `vm` to `loop` via `uv_async` + cleanup hooks so async work marshals to the JS thread safely. */
bool ARKTSInner_InitLoop(napi_env env, ARKTS_Env vm, uv_loop_t* loop);
/** Tear down event-handler + uv loop maps for a disposed runtime. */
void ARKTS_DisposeEventHandler(ARKTS_Env env);
/** Notify Cangjie that the `JSContext` for this `env` must be torn down (native hook from engine cleanup). */
void ARKTS_DisposeJSContext(ARKTS_Env env);
/** Initialize per-VM global-handle bookkeeping (weak/strong tables, see `ark_interop_global.cpp`). */
void ARKTS_InitGlobalManager(ARKTS_Env env);
/** Release per-VM global manager state during shutdown. */
void ARKTS_RemoveGlobalManager(ARKTS_Env env);

/**
 * @brief Convert `ARKTS_Value` into `Local<T>` for a VM API call.
 *
 * Heap objects use their stable address; non-heap immediates point at the stack slot holding `value` so the VM can
 * materialize a handle without allocating — **callers must keep `value` alive** until the handle is consumed.
 */
template<typename T>
ARKTS_INLINE panda::Local<T> ARKTS_ToHandle(ARKTS_Value& value)
{
    auto v = BIT_CAST(value, panda::JSValueRef);
    ARKTS_Value addr;
    if (v.IsHeapObject()) {
        addr = value;
    } else {
        // Non-heap values are passed via an address wrapper: store the value in a temporary stack slot and pass
        // its address as the handle. Callers must ensure the referenced storage remains valid for the duration
        // of the conversion/use.
        addr.pointer = &value;
    }
    return BIT_CAST(addr, panda::Local<T>);
}

/**
 * @brief Read the immediate `JSValueRef` bits for inlined primitives, or load from the heap slot when tagged as object.
 */
ARKTS_INLINE panda::JSValueRef ARKTS_ToValue(ARKTS_Value value)
{
    auto v = BIT_CAST(value, panda::JSValueRef);
    if (v.IsHeapObject()) {
        return *BIT_CAST(value, panda::JSValueRef*);
    } else {
        return v;
    }
}

ARKTS_INLINE bool operator==(ARKTS_Value lhs, ARKTS_Value rhs)
{
    return lhs.value == rhs.value;
}

ARKTS_INLINE bool operator!=(ARKTS_Value lhs, ARKTS_Value rhs)
{
    return lhs.value != rhs.value;
}

/**
 * @brief Encode a `Local` back into the compact `ARKTS_Value` ABI (address masked to 48 bits on heap paths).
 *
 * `Null` becomes `Undefined` per historical interop convention; heap objects mask the raw pointer into the handle word.
 */
template<typename T>
ARKTS_INLINE ARKTS_Value ARKTS_FromHandle(panda::Local<T>& value)
{
    if (value.IsNull()) {
        return ARKTS_CreateUndefined();
    } else if (value->IsHeapObject()) {
        constexpr uint64_t addrMask = 0xFFFF'FFFF'FFFF;
        auto addr = BIT_CAST(value, uint64_t) & addrMask;
        return BIT_CAST(addr, ARKTS_Value);
    } else {
        return *BIT_CAST(value, ARKTS_Value*);
    }
}

/**
 * @brief Normalize `value` into the tagged layout used for `ARKTS_Result` out-parameters after closing a native scope.
 *
 * Non-heap values are re-localized in the VM so downstream consumers always see a coherent tagged reference.
 */
ARKTS_INLINE ARKTS_Result ARKTS_ToResult(panda::EcmaVM* vm, ARKTS_Value value)
{
    auto tag = BIT_CAST(value, panda::JSValueRef);
    if (!tag.IsHeapObject()) {
        auto local = panda::JSNApi::CreateLocal(vm, tag);
        return BIT_CAST(local, ARKTS_Result);
    } else {
        return BIT_CAST(value, ARKTS_Result);
    }
}

#define NATIVE_ERROR(msg) ARKTSInner_ReportNativeError("[%s]: %s", __FUNCTION__, msg)
#define ARKTS_ASSERT(condition, error, ret)                                            \
    if (!(condition)) {                                                   \
        NATIVE_ERROR(error);                                                        \
        return ret;                                                                 \
    }
#define ARKTS_ASSERT_U(condition, error) ARKTS_ASSERT(condition, error, ARKTS_CreateUndefined())
#define ARKTS_ASSERT_P(condition, error) ARKTS_ASSERT(condition, error, nullptr)
#define ARKTS_ASSERT_F(condition, error) ARKTS_ASSERT(condition, error, false)
#define ARKTS_ASSERT_I(condition, error) ARKTS_ASSERT(condition, error, 0)
#define ARKTS_ASSERT_V(condition, error) ARKTS_ASSERT(condition, error,)

#ifdef DEBUG_JS
#define ARKTS_CHECK(condition, ret)                                                    \
    if (UNLIKELY(!(condition))) {                                                   \
        return ret;                                                                 \
    }
#else
#define ARKTS_CHECK(condition, ret) condition
#endif

#define ARKTS_CHECK_F(condition) ARKTS_CHECK(condition, false)

#endif // NAPI_ARK_INTEROP_INTERNAL_H
