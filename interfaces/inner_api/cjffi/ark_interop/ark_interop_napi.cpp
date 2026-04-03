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

#include "ark_interop_napi.h"

#include <thread>
#include <ecmascript/js_tagged_value.h>

#include "ark_interop_external.h"
#include "ark_interop_internal.h"
#include "ark_interop_log.h"
#include "ark_interop_scope.h"
#include "securec.h"

#ifdef ERROR
#undef ERROR
#endif

using namespace panda;
using namespace panda::ecmascript;

/**
 * @file ark_interop_napi.cpp
 * @brief C-side implementation of the ArkTS interop API (`ARKTS_*` in `ark_interop_napi.h`) consumed by Cangjie FFI.
 *
 * Covers value projection, JS calls, collections, ArrayBuffer/External, Promises, scopes, exception propagation, and
 * cycle-safe lambda/extern bridges.
 */

/**
 * @brief Propagate a pending VM exception into the Cangjie error path.
 *
 * If the EcmaVM currently has a pending uncaught exception, it is fetched and cleared, converted to
 * `ARKTS_Value`, and forwarded to `ARKTSInner_ThrowJSErrorToCJ` (which ultimately throws on the Cangjie side).
 *
 * @param env EcmaVM handle (`ARKTS_Env` / `EcmaVM*`).
 * @param abortIfUnhandled Reserved for callers that need strict propagation policy (handled inside error pipeline).
 * @return `true` if an exception was present and was reported, otherwise `false`.
 */

bool ARKTSInner_ReportJSErrors(ARKTS_Env env, bool abortIfUnhandled)
{
    auto vm = P_CAST(env, EcmaVM*);
    if (!JSNApi::HasPendingException(vm)) {
        return false;
    }

    // Consumes VM pending state so further native API calls are not stuck in "exception pending".
    auto exception = JSNApi::GetAndClearUncaughtException(vm);
    auto jError = ARKTS_FromHandle(exception);

    // Marshals the JS error into Cangjie (`MixedException` pipeline); abortIfUnhandled reserved for stricter policies.
    ARKTSInner_ThrowJSErrorToCJ(env, jError);

    return true;
}

/**
 * @brief Format a printf-style diagnostic and raise it as a native error toward Cangjie.
 *
 * Uses a fixed stack buffer and `vsnprintf_s`; on formatting failure falls back to the raw format string.
 */

void ARKTSInner_ReportNativeError(const char* format, ...)
{
    constexpr size_t MAX_MESSAGE_SIZE = 0x100;
    char buffer[MAX_MESSAGE_SIZE];

    va_list args;
    va_start(args, format);
#ifdef __clang__
    /* code specific to clang compiler */
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#elif __GNUC__
    /* code for GNU C compiler */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
    int ret = vsnprintf_s(buffer, sizeof(buffer) / sizeof(char), MAX_MESSAGE_SIZE - 1, format, args);
#ifdef __clang__
#pragma clang diagnostic pop
#elif __GNUC__
#pragma GCC diagnostic pop
#endif
    va_end(args);
    const char* formatMsg;

    if (ret == -1) {
        formatMsg = format;
    } else {
        buffer[ret] = '\0';
        formatMsg = buffer;
    }
    ARKTSInner_ThrowNativeErrorToCJ(formatMsg);
}

/**
 * @brief Human-readable serialization of a JS error value for logging/diagnostics.
 *
 * Strings are returned verbatim. Error objects concatenate `name`, `message`, and `stack` when present.
 * Operates under `JsiFastNativeScope` because property reads may touch the heap.
 */

std::string ARKTSInner_FormatJSError(ARKTS_Env env, ARKTS_Value jsError)
{
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    auto exception = BIT_CAST(jsError, Local<JSValueRef>);
    if (exception->IsString(vm)) {
        auto errorInsJs = *BIT_CAST(jsError, StringRef*);
        return errorInsJs.ToString(vm);
    } else if (exception->IsError(vm)) {
        auto errorObJ = *BIT_CAST(jsError, ObjectRef*);
        auto nameValue = errorObJ.Get(vm, StringRef::NewFromUtf8(vm, "name"));
        auto messageValue = errorObJ.Get(vm, StringRef::NewFromUtf8(vm, "message"));
        auto stackValue = errorObJ.Get(vm, StringRef::NewFromUtf8(vm, "stack"));
        std::string name = nameValue->IsString(vm) ? nameValue->ToString(vm)->ToString(vm) : "";
        auto message = messageValue->IsString(vm) ? messageValue->ToString(vm)->ToString(vm) : "";
        auto stack = stackValue->IsString(vm) ? stackValue->ToString(vm)->ToString(vm) : "";
        return name + "\n" + message + "\n" + stack;
    } else {
        return "[unknown js Error type]";
    }
}

/**
 * @brief Return the ArkTS global object (`globalThis` / VM global binding).
 *
 * Used by the Cangjie interop layer to resolve built-in constructors (`Map`, `Error`, etc.) without hard-coded addresses.
 */

ARKTS_Value ARKTS_GetGlobalConstant(ARKTS_Env env)
{
    ARKTS_ASSERT_U(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    // Same object as globalThis / global in scripts; not a new allocation.
    auto result = JSNApi::GetGlobalObject(vm);
    return ARKTS_FromHandle(result);
}

/**
 * @brief Best-effort numeric fingerprint of `std::this_thread::get_id()` for thread-affinity checks.
 *
 * Note: the raw bit pattern is not guaranteed to equal a kernel TID on all platforms; it only distinguishes threads
 * from the same process in practice for the runtime’s self-checks.
 */

uint64_t ARKTS_GetPosixThreadId()
{
    auto id = std::this_thread::get_id();
    // Opaque hash of std::thread::id; used only to compare with ARKTS_GetThreadIdOfEngine in asserts.
    return *reinterpret_cast<uint64_t*>(&id);
}

/**
 * @brief Map an `ARKTS_Value` to a coarse `typeof` bucket (`N_UNDEFINED` … `N_OBJECT`).
 *
 * Important: primitive tags are read from the immediate representation; heap-backed values dereference through the VM
 * to distinguish string, symbol, function, bigint, native pointer (`N_EXTERNAL`), or generic object.
 */

ARKTS_ValueType ARKTS_GetValueType(ARKTS_Env env, ARKTS_Value src)
{
    ARKTS_ASSERT(env, "env is null", N_UNDEFINED);
    auto value = BIT_CAST(src, JSValueRef);
    if (value.IsHole()) {
        return N_UNDEFINED;
    } else if (value.IsNull()) {
        return N_NULL;
    } else if (value.IsUndefined()) {
        return N_UNDEFINED;
    } else if (value.IsBoolean()) {
        return N_BOOL;
    } else if (value.IsNumber()) {
        return N_NUMBER;
    }
    // Heap path: first tag only said IsHeapObject; load real header for string/symbol/function/etc.
    auto vm = P_CAST(env, EcmaVM*);
    JsiFastNativeScope fastNativeScope(vm);
    value = *BIT_CAST(src, JSValueRef*);
    if (value.IsNull()) {
        return N_NULL;
    } else if (value.IsString(vm)) {
        return N_STRING;
    } else if (value.IsSymbol(vm)) {
        return N_SYMBOL;
    } else if (value.IsFunction(vm)) {
        return N_FUNCTION;
    } else if (value.IsBigInt(vm)) {
        return N_BIGINT;
    } else if (value.IsNativePointer(vm)) {
        return N_EXTERNAL;
    } else {
        return N_OBJECT;
    }
}

/**
 * @brief JavaScript `===` semantics for supported types (primitive fast paths + VM `IsStrictEquals` for references).
 *
 * Requires identical `ARKTS_ValueType` for `true`; `undefined` and `null` compare only to themselves.
 */

bool ARKTS_StrictEqual(ARKTS_Env env, ARKTS_Value a, ARKTS_Value b)
{
    ARKTS_ASSERT_F(env, "env is NULL");
    auto aType = ARKTS_GetValueType(env, a);
    auto bType = ARKTS_GetValueType(env, b);
    if (aType != bType) {
        return false;
    }
    if (aType == N_UNDEFINED || aType == N_NULL) {
        return true;
    }
    switch (aType) {
        case N_BOOL: {
            auto aRef = BIT_CAST(a, BooleanRef);
            auto bRef = BIT_CAST(b, BooleanRef);

            return aRef.Value() == bRef.Value();
        }
        case N_NUMBER: {
            auto aRef = BIT_CAST(a, JSValueRef);
            auto bRef = BIT_CAST(b, JSValueRef);

            double aValue = aRef.IsInt() ? BIT_CAST(a, IntegerRef).Value() : BIT_CAST(a, NumberRef).Value();
            double bValue = bRef.IsInt() ? BIT_CAST(b, IntegerRef).Value() : BIT_CAST(b, NumberRef).Value();

            return aValue == bValue;
        }
        default:
            // Objects, strings, symbols, etc.: defer to VM strict equality (reference and type rules).
            auto vm = P_CAST(env, EcmaVM*);
            panda::JsiFastNativeScope fastNativeScope(vm);
            auto aTag = *BIT_CAST(a, JSValueRef*);
            auto bTag = BIT_CAST(b, Local<JSValueRef>);
            return aTag.IsStrictEquals(vm, bTag);
    }
}

/**
 * @brief Tagged `null` value (no VM allocation).
 */

ARKTS_Value ARKTS_CreateNull()
{
    return  { .value = JSTaggedValue::VALUE_NULL };
}

/**
 * @brief True if the tagged value equals the VM’s canonical `null` bit pattern.
 */

bool ARKTS_IsNull(ARKTS_Value value)
{
    return value.value == JSTaggedValue::VALUE_NULL;
}

/**
 * @brief Tagged `undefined` value (no VM allocation).
 */

ARKTS_Value ARKTS_CreateUndefined()
{
    return { .value = JSTaggedValue::VALUE_UNDEFINED };
}

/**
 * @brief True if the tagged value equals the canonical `undefined` pattern.
 */

bool ARKTS_IsUndefined(ARKTS_Value value)
{
    return value.value == JSTaggedValue::VALUE_UNDEFINED;
}

/**
 * @brief Tagged boolean `true`/`false` (no VM allocation).
 */

ARKTS_Value ARKTS_CreateBool(bool value)
{
    return { .value = value ? JSTaggedValue::VALUE_TRUE : JSTaggedValue::VALUE_FALSE };
}

/**
 * @brief True if the immediate `JSValueRef` tag encodes a boolean.
 */

bool ARKTS_IsBool(ARKTS_Value value)
{
    auto v = BIT_CAST(value, JSValueRef);
    return v.IsBoolean();
}

/**
 * @brief Read the boolean payload; asserts when the tag is not boolean (debug builds).
 */

bool ARKTS_GetValueBool(ARKTS_Value value)
{
    auto tag = BIT_CAST(value, JSValueRef);
    ARKTS_ASSERT_F(tag.IsBoolean(), "not a boolean");
    auto boolRef = BIT_CAST(value, BooleanRef);
    return boolRef.Value();
}

/**
 * @brief Box a IEEE-754 double as a JS number (`JSTaggedValue`).
 */

ARKTS_Value ARKTS_CreateF64(double value)
{
    JSTaggedValue result(value);
    return BIT_CAST(result, ARKTS_Value);
}

/**
 * @brief Box a 32-bit integer as a JS number (preferred path for int payload).
 */

EXPORT ARKTS_Value ARKTS_CreateI32(int32_t value)
{
    JSTaggedValue result(value);
    return BIT_CAST(result, ARKTS_Value);
}

/**
 * @brief Read numeric payload, branching on int-vs-double representation inside the tag.
 */

double ARKTS_GetValueNumber(ARKTS_Value value)
{
    auto ref = BIT_CAST(value, JSValueRef);
    ARKTS_ASSERT_I(ref.IsNumber(), "not a number");
    if (ref.IsInt()) {
        return BIT_CAST(value, IntegerRef).Value();
    } else {
        return BIT_CAST(value, NumberRef).Value();
    }
}

/**
 * @brief True if the value carries a JS number tag (int or double).
 */

bool ARKTS_IsNumber(ARKTS_Value value)
{
    auto jValue = BIT_CAST(value, JSValueRef);
    return jValue.IsNumber();
}

struct LambdaData {
    ARKTS_Env env;
    int64_t lambdaId;
};

/**
 * @brief JS→native trampoline for Cangjie lambdas: forwards to `ARKTSInner_CJLambdaInvoker`.
 *
 * `LambdaData` carries `env` + `lambdaId` registered on the Cangjie side. Returns `Undefined` if user data is missing.
 */

static Local<JSValueRef> CJLambdaInvoker(JsiRuntimeCallInfo* callInfo)
{
    auto data = reinterpret_cast<LambdaData*>(callInfo->GetData());
    if (!data) {
        return JSValueRef::Undefined(callInfo->GetVM());
    }
    // Bridges JS invocation to Cangjie by stable lambdaId (see ARKTS_CreateFunc).
    auto result = ARKTSInner_CJLambdaInvoker(P_CAST(callInfo, ARKTS_CallInfo), data->lambdaId);
    return BIT_CAST(result, Local<JSValueRef>);
}

/**
 * @brief Finalizer hook when the JS function proxy is collected: notifies Cangjie to drop the slab slot (`lambdaId`).
 */

static void CJLambdaDeleter(void* /* env: napi_env */, void* /*invoker*/, void* raw)
{
    auto data = reinterpret_cast<LambdaData*>(raw);
    if (!data) {
        return;
    }
    ARKTSInner_CJLambdaDeleter(data->env, data->lambdaId);
    delete data;
}

/**
 * @brief Create a callable JS function whose body invokes Cangjie lambda `lambdaId` through `CJLambdaInvoker`.
 *
 * Panda attaches `CJLambdaDeleter` so IDs are released when the JS function is GC’d.
 */

ARKTS_Value ARKTS_CreateFunc(ARKTS_Env env, int64_t lambdaId)
{
    ARKTS_ASSERT_U(env, "env is null");

    auto vm = P_CAST(env, EcmaVM*);
    // `needSendable=true`: host embedding may restrict callable creation; GC uses CJLambdaDeleter on LambdaData*.
    auto result = FunctionRef::New(vm, CJLambdaInvoker, CJLambdaDeleter, new LambdaData {env, lambdaId}, true);

    return ARKTS_FromHandle(result);
}

/**
 * @brief True if the value is a heap constructor (`IsConstructor`).
 *
 * Non-heap values short-circuit to `false` before touching the VM.
 */

bool ARKTS_IsClass(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_F(env, "env is null");
    ARKTS_ASSERT_F(value.value, "value is null");
    auto tag = BIT_CAST(value, JSValueRef);
    if (!tag.IsHeapObject()) {
        return false;
    }
    auto vm = P_CAST(env, EcmaVM*);
    JsiFastNativeScope fastNativeScope(vm);
    tag = *P_CAST(value.pointer, JSValueRef*);
    return tag.IsConstructor(vm);
}

/**
 * @brief Define a JS constructor whose body uses the same lambda bridge as `ARKTS_CreateFunc`.
 *
 * When `base` is itself a class (`ARKTS_IsClass`), the new function inherits from `base` (`Inherit`).
 */

ARKTS_Value ARKTS_CreateClass(ARKTS_Env env, int64_t lambdaId, ARKTS_Value base)
{
    ARKTS_ASSERT_U(env, "env is null");

    auto vm = P_CAST(env, EcmaVM*);

    // Class body still dispatches through CJLambdaInvoker; `true` matches ARKTS_CreateFunc (sendable ctor).
    auto result = FunctionRef::NewClassFunction(vm, CJLambdaInvoker, CJLambdaDeleter,
                                                new LambdaData {env, lambdaId}, true);

    if (ARKTS_IsClass(env, base)) {
        // Wire `Constructor.__proto__` / prototype chain to `base` when the caller passed a constructor.
        panda::JsiFastNativeScope fastNativeScope(vm);
        auto baseClass = BIT_CAST(base, Local<FunctionRef>);
        result->Inherit(vm, baseClass);
    }

    return ARKTS_FromHandle(result);
}

/**
 * @brief `%Constructor.prototype%` for a valid class/constructor value.
 */

ARKTS_Value ARKTS_GetPrototype(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_U(env, "env is null");
    ARKTS_ASSERT_U(ARKTS_IsClass(env, value), "value is not constructor");
    auto vm = P_CAST(env, EcmaVM*);
    auto clazz = BIT_CAST(value, Local<FunctionRef>);
    auto result = clazz->GetFunctionPrototype(vm);
    return ARKTS_FromHandle(result);
}

/**
 * @brief ECMA `instanceof` using VM `InstanceOf` after guarding heap object + constructor invariants.
 */

bool ARKTS_InstanceOf(ARKTS_Env env, ARKTS_Value object, ARKTS_Value clazz)
{
    ARKTS_ASSERT_F(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    ARKTS_ASSERT_F(ARKTS_IsHeapObject(object), "object is not heap object");
    ARKTS_ASSERT_F(ARKTS_IsClass(env, clazz), "clazz is not a class");

    auto targetObject = BIT_CAST(object, Local<JSValueRef>);
    auto targetClass = BIT_CAST(clazz, Local<FunctionRef>);

    return targetObject->InstanceOf(vm, targetClass);
}

/**
 * @brief Equivalent to `typeof(value) === "function"` via `ARKTS_GetValueType`.
 */

bool ARKTS_IsCallable(ARKTS_Env env, ARKTS_Value value)
{
    return ARKTS_GetValueType(env, value) == N_FUNCTION;
}

static constexpr auto MAX_CALL_ARGS = 255;

/**
 * @brief Convert each `ARKTS_Value` argument into `Local<JSValueRef>` for a varargs JS call/ctor.
 */

ARKTS_INLINE void FormatArguments(int32_t numArgs, ARKTS_Value args[], Local<JSValueRef>* result)
{
    for (int32_t i = 0;i < numArgs; ++i) {
        result[i] = ARKTS_ToHandle<JSValueRef>(args[i]);
    }
}

/**
 * @brief Invoke a JS function with explicit `this` and argument array (`FunctionRef::Call`).
 *
 * After the call, pending exceptions are surfaced through `ARKTSInner_ReportJSErrors` (aborting path on failure).
 */

ARKTS_Value ARKTS_Call(ARKTS_Env env, ARKTS_Value func, ARKTS_Value thisArg, int32_t numArgs, ARKTS_Value args[])
{
    ARKTS_ASSERT_U(env, "env is null");
    ARKTS_ASSERT_U(ARKTS_IsCallable(env, func), "func is not callable");
    ARKTS_ASSERT_U(numArgs >= 0, "numArgs must be non-negative");
    ARKTS_ASSERT_U(numArgs <= MAX_CALL_ARGS, "too many arguments, 255 most");

    auto vm = P_CAST(env, EcmaVM*);
    JsiFastNativeScope fastNativeScope(vm);

    // `func` is a heap Function; follow pointer to the FunctionRef vtable used by Call.
    auto funcHandle = *P_CAST(func.pointer, FunctionRef*);
    auto thisHandle = ARKTS_ToHandle<JSValueRef>(thisArg);

    Local<JSValueRef> formattedArgs[MAX_CALL_ARGS];
    FormatArguments(numArgs, args, formattedArgs);

    auto result = funcHandle.Call(vm, thisHandle, formattedArgs, numArgs);

    // abortIfUnhandled=true: propagate any pending exception into Cangjie immediately after the call.
    ARKTSInner_ReportJSErrors(env, true);

    return ARKTS_FromHandle(result);
}

/**
 * @brief `new Constructor(...args)` (`FunctionRef::Constructor`).
 *
 * Surfaces VM exceptions like `ARKTS_Call`.
 */

ARKTS_Value ARKTS_New(ARKTS_Env env, ARKTS_Value clazz, int32_t numArgs, ARKTS_Value args[])
{
    ARKTS_ASSERT_U(env, "env is null");
    ARKTS_ASSERT_U(ARKTS_IsClass(env, clazz), "clazz is not class");
    ARKTS_ASSERT_U(numArgs >= 0, "numArgs must be non-negative");
    ARKTS_ASSERT_U(numArgs <= MAX_CALL_ARGS, "too many arguments, 255 most");

    auto vm = P_CAST(env, EcmaVM*);
    JsiFastNativeScope fastNativeScope(vm);
    // Same FunctionRef indirection as ARKTS_Call; Constructor allocates `this` and runs `[[Construct]]`.
    auto funcHandle = *P_CAST(clazz.pointer, FunctionRef*);

    Local<JSValueRef> formattedArgs[MAX_CALL_ARGS];
    FormatArguments(numArgs, args, formattedArgs);

    auto result = funcHandle.Constructor(vm, formattedArgs, numArgs);

    ARKTSInner_ReportJSErrors(env, true);

    return ARKTS_FromHandle(result);
}

// JSArray elements stored in vector mode and dict mode.
// default in vector mode,
// if capacity grows gap bigger than 1024, will transform to dict mode.
/**
 * @brief Allocate a JSArray with `size` holes (`ArrayRef::New`).
 */
ARKTS_Value ARKTS_CreateArray(ARKTS_Env env, uint32_t size)
{
    ARKTS_ASSERT_U(env, "env is null");

    auto vm = P_CAST(env, EcmaVM*);
    auto result = ArrayRef::New(vm, size);
    return ARKTS_FromHandle(result);
}

/**
 * @brief Allocate a JSArray and populate indices `0..size-1` from `data`.
 */

ARKTS_Value ARKTS_CreateArrayWithInit(ARKTS_Env env, uint32_t size, ARKTS_Value* data)
{
    ARKTS_ASSERT_U(env, "env is null");
    ARKTS_ASSERT_U(!size || data, "data is null");
    auto vm = P_CAST(env, EcmaVM*);
    JsiFastNativeScope fastNativeScope(vm);

    auto result = ArrayRef::New(vm, size);
    for (uint32_t i = 0;i < size; ++i) {
        ArrayRef::SetValueAt(vm, result, i, ARKTS_ToHandle<JSValueRef>(data[i]));
    }

    return ARKTS_FromHandle(result);
}

/**
 * @brief Prefer fast `ArrayRef::Length` for real arrays; otherwise read the `"length"` property (typed array fallback).
 */

uint32_t ARKTS_GetArrayLength(ARKTS_Env env, ARKTS_Value array)
{
    ARKTS_ASSERT_I(env, "env is null");
    ARKTS_ASSERT_I(ARKTS_IsArray(env, array), "array is not array");

    auto vm = P_CAST(env, EcmaVM*);
    JsiFastNativeScope fastNativeScope(vm);
    auto ref = P_CAST(array.pointer, JSValueRef*);
    if (ref->IsJSArray(vm)) {
        return P_CAST(array.pointer, ArrayRef*)->Length(vm);
    }
    // Typed arrays / exotic objects: fall back to reading the `length` property as a number.
    auto l = ARKTS_CreateUtf8(env, "length", 6);
    auto r = ARKTS_GetProperty(env, array, l);
    if (ARKTS_IsNumber(r)) {
        auto n = ARKTS_GetValueNumber(r);
        return static_cast<uint32_t>(n);
    }
    return 0;
}

/**
 * @brief `array[index] = value` using `ArrayRef::SetValueAt`; clears VM exceptions if JS sets trap throws.
 */

void ARKTS_SetElement(ARKTS_Env env, ARKTS_Value array, uint32_t index, ARKTS_Value value)
{
    ARKTS_ASSERT_V(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    ARKTS_ASSERT_V(ARKTS_IsArray(env, array), "array is not array");

    auto jArr = BIT_CAST(array, Local<ArrayRef>);

    ArrayRef::SetValueAt(vm, jArr, index, ARKTS_ToHandle<JSValueRef>(value));
    ARKTSInner_ReportJSErrors(env, false);
}

/**
 * @brief Read an element with bounds check; reports JS exceptions from accessors if any.
 */

ARKTS_Value ARKTS_GetElement(ARKTS_Env env, ARKTS_Value array, uint32_t index)
{
    ARKTS_ASSERT_U(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    ARKTS_ASSERT_U(ARKTS_IsArray(env, array), "array is not array");
    auto arr = BIT_CAST(array, Local<ArrayRef>);
    ARKTS_ASSERT_U(arr->Length(vm) > index, "out of index");

    auto result = ArrayRef::GetValueAt(vm, arr, index);
    ARKTSInner_ReportJSErrors(env, false);
    return ARKTS_FromHandle(result);
}

/**
 * @brief True for VM arrays (non-heap immediates return `false` before deref).
 */

bool ARKTS_IsArray(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_F(env, "env is NULL");
    ARKTS_ASSERT_F(value.value, "value is null");
    auto v = BIT_CAST(value, JSValueRef);
    if (!v.IsHeapObject()) {
        return false;
    }
    auto vm = P_CAST(env, EcmaVM*);
    JsiFastNativeScope fastNativeScope(vm);
    v = *P_CAST(value.pointer, JSValueRef*);
    return v.IsArray(vm);
}

/**
 * @brief Allocate zero-initialized `ArrayBuffer` of `length` bytes.
 */

ARKTS_Value ARKTS_CreateArrayBuffer(ARKTS_Env env, int32_t length)
{
    ARKTS_ASSERT_U(env, "env is null");
    ARKTS_ASSERT_U(length >= 0, "length must be non-negative");
    auto vm = P_CAST(env, EcmaVM*);
    auto result = ArrayBufferRef::New(vm, length);
    return ARKTS_FromHandle(result);
}

/**
 * @brief Wrap external bytes in an ArrayBuffer and register `ARKTSInner_CJArrayBufferDeleter` for GC teardown.
 *
 * 64-bit: `finalizerHint` is passed directly as the deleter hint. 32-bit: hint is boxed in `ARKTSInner_External`
 * because pointer-width hints cannot always carry the full id.
 */

ARKTS_Value ARKTS_CreateArrayBufferWithData(ARKTS_Env env, void* buffer, int32_t length,
    int64_t finalizerHint)
{
    ARKTS_ASSERT_U(env, "env is null");
    ARKTS_ASSERT_U(length >= 0, "length must be non-negative");
    ARKTS_ASSERT_U(buffer, "buffer is null");

    auto vm = P_CAST(env, EcmaVM*);
    // Encoding matches @brief above: 64-bit hint is the id in void* metadata; 32-bit allocates ARKTSInner_External.
    if constexpr (sizeof(void*) == 8) {
        auto result = ArrayBufferRef::New(vm, buffer, length, ARKTSInner_CJArrayBufferDeleter,
            reinterpret_cast<void*>(finalizerHint));

        return ARKTS_FromHandle(result);
    } else {
        auto pointer = new (std::nothrow) ARKTSInner_External {finalizerHint};
        if (!pointer) {
            return ARKTS_CreateUndefined();
        }
        auto result = ArrayBufferRef::New(vm, buffer, length, ARKTSInner_CJArrayBufferDeleter,
            pointer);

        return ARKTS_FromHandle(result);
    }
}

/**
 * @brief True for ArrayBuffer, any TypedArray view, or DataView.
 */

bool ARKTS_IsArrayBuffer(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_F(env, "env is null");
    ARKTS_ASSERT_F(value.value, "value is null");
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    auto tag = BIT_CAST(value, JSValueRef);
    if (!tag.IsHeapObject()) {
        return false;
    }
    auto handle = BIT_CAST(value, Local<JSValueRef>);
    return handle->IsArrayBuffer(vm) || handle->IsTypedArray(vm) || handle->IsDataView(vm);
}

/**
 * @brief Byte length of the logical view (TypedArray/DataView subtract offset from backing store length).
 */

int32_t ARKTS_GetArrayBufferLength(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_I(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    ARKTS_ASSERT_I(ARKTS_IsArrayBuffer(env, value), "value is not arrayBuffer");

    auto handle = BIT_CAST(value, Local<JSValueRef>);
    if (handle->IsArrayBuffer(vm)) {
        return BIT_CAST(value, Local<ArrayBufferRef>)->ByteLength(vm);
    } else if (handle->IsTypedArray(vm)) {
        auto arr = BIT_CAST(value, Local<TypedArrayRef>);
        return arr->ByteLength(vm) - arr->ByteOffset(vm);
    } else {
        auto arr = BIT_CAST(value, Local<DataViewRef>);
        return arr->ByteLength() - arr->ByteOffset();
    }
}

/**
 * @brief Mutable pointer into backing store (applies byte offset for typed arrays/data views).
 */

void* ARKTS_GetArrayBufferRawPtr(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_P(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    ARKTS_ASSERT_P(ARKTS_IsArrayBuffer(env, value), "value is not arrayBuffer");

    auto handle = BIT_CAST(value, Local<JSValueRef>);
    if (handle->IsArrayBuffer(vm)) {
        return BIT_CAST(value, Local<ArrayBufferRef>)->GetBuffer(vm);
    } else if (handle->IsTypedArray(vm)) {
        auto arr = BIT_CAST(value, Local<TypedArrayRef>);
        auto rawStart = arr->GetArrayBuffer(vm)->GetBuffer(vm);
        auto rawOffset = arr->ByteOffset(vm);
        return P_CAST(rawStart, uint8_t*) + rawOffset;
    } else {
        auto arr = BIT_CAST(value, Local<DataViewRef>);
        auto rawStart = arr->GetArrayBuffer(vm)->GetBuffer(vm);
        auto rawOffset = arr->ByteOffset();
        return P_CAST(rawStart, uint8_t*) + rawOffset;
    }
}

/**
 * @brief Safe `memcpy_s` from buffer view into `dest`, capped by both source length and `count`.
 */

int32_t ARKTS_ArrayBufferReadBytes(ARKTS_Env env, ARKTS_Value buffer, void* dest, int32_t count)
{
    ARKTS_ASSERT_I(env, "env is null");
    ARKTS_ASSERT_I(buffer.value, "buffer is null");
    ARKTS_ASSERT_I(dest, "dest is null");

    auto src = ARKTS_GetArrayBufferRawPtr(env, buffer);
    auto srcSize = ARKTS_GetArrayBufferLength(env, buffer);
    auto targetSize = std::min(srcSize, count);
    auto status = memcpy_s(dest, count, src, targetSize);
    if (status != 0) {
        LOGE("ARKTS_ArrayBufferReadBytes error, memcpy_s ret %{public}d", status);
        return 0;
    }
    return targetSize;
}

/**
 * @brief Create a `NativePointer` JS value whose payload is a Cangjie-managed id; GC invokes `ARKTSInner_CJExternalDeleter`.
 *
 * 64-bit stores the id as the raw pointer; 32-bit boxes `ARKTSInner_External`.
 */

ARKTS_Value ARKTS_CreateExternal(ARKTS_Env env, int64_t data)
{
    ARKTS_ASSERT_U(env, "env is null");

    auto vm = P_CAST(env, EcmaVM*);
    // Same pointer-width encoding as ARKTS_CreateArrayBufferWithData / GetExternalData.
    if constexpr (sizeof(void*) == 8) {
        auto result = NativePointerRef::New(vm, reinterpret_cast<void*>(static_cast<intptr_t>(data)),
            ARKTSInner_CJExternalDeleter, env);
        return ARKTS_FromHandle(result);
    } else {
        auto pointer = new (std::nothrow) ARKTSInner_External {data};
        if (!pointer) {
            return ARKTS_CreateUndefined();
        }
        auto result = NativePointerRef::New(vm, pointer, ARKTSInner_CJExternalDeleter, env);
        return ARKTS_FromHandle(result);
    }
}

/**
 * @brief True if the heap value is a native pointer (`IsNativePointer`).
 */

bool ARKTS_IsExternal(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_F(env, "env is null");
    ARKTS_ASSERT_F(value.value, "value is null");
    auto prime = BIT_CAST(value, JSValueRef);
    if (!prime.IsHeapObject()) {
        return false;
    }
    auto vm = P_CAST(env, EcmaVM*);
    JsiFastNativeScope fastNativeScope(vm);
    auto handle = BIT_CAST(value, JSValueRef*);
    return handle->IsNativePointer(vm);
}

/**
 * @brief Recover the int64 id from an External; 64-bit reads raw pointer bits, 32-bit unwraps `ARKTSInner_External`.
 */

int64_t ARKTS_GetExternalData(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_I(ARKTS_IsExternal(env, value), "value is not external data");

    auto vm = P_CAST(env, EcmaVM*);
    JsiFastNativeScope fastNativeScope(vm);

    auto external = *P_CAST(value.pointer, NativePointerRef*);
    // Inverse of ARKTS_CreateExternal: raw bits vs ARKTSInner_External* payload.

    if constexpr (sizeof(void*) == 8) {
        return reinterpret_cast<int64_t>(external.Value());
    } else {
        auto data = external.Value();
        if (!data) {
            LOGE("invalid external data");
            return -1;
        }
        auto pointer = reinterpret_cast<ARKTSInner_External*>(data);
        return pointer->data;
    }
}

/**
 * @brief Allocate `{ promise, resolve, reject }` and root it in a `Global<PromiseCapabilityRef>` handle (`ARKTS_Promise`).
 */

ARKTS_Promise ARKTS_CreatePromiseCapability(ARKTS_Env env)
{
    ARKTS_ASSERT_P(env, "env is null");

    auto vm = P_CAST(env, EcmaVM*);

    auto promise = PromiseCapabilityRef::New(vm);
    // Global<> keeps the capability rooted across native code until resolve/reject frees the handle.
    auto result = new Global<PromiseCapabilityRef>(vm, promise);

    return BIT_CAST(result, ARKTS_Promise);
}

/**
 * @brief Extract the JS Promise object referenced by the capability.
 */

ARKTS_Value ARKTS_GetPromiseFromCapability(ARKTS_Env env, ARKTS_Promise prom)
{
    ARKTS_ASSERT_U(env, "env is null");
    ARKTS_ASSERT_U(prom, "prom is null");

    auto vm = P_CAST(env, EcmaVM*);
    JsiFastNativeScope fastNativeScope(vm);
    auto promise = *P_CAST(prom, Global<PromiseCapabilityRef>*);
    auto result = (*promise)->GetPromise(vm);

    return ARKTS_FromHandle(result);
}

/**
 * @brief Resolve the deferred promise then tear down the global handle (`FreeGlobalHandleAddr`) and delete wrapper.
 */

void ARKTS_PromiseCapabilityResolve(ARKTS_Env env, ARKTS_Promise prom, ARKTS_Value result)
{
    ARKTS_ASSERT_V(env, "env is null");
    ARKTS_ASSERT_V(prom, "prom is null");

    auto vm = P_CAST(env, EcmaVM*);
    JsiFastNativeScope fastNativeScope(vm);
    auto promise = P_CAST(prom, Global<PromiseCapabilityRef>*);
    (*promise)->Resolve(vm, ARKTS_ToHandle<JSValueRef>(result));
    promise->FreeGlobalHandleAddr();
    delete promise;
}

/**
 * @brief Reject the deferred promise and release native roots (same teardown as resolve).
 */

void ARKTS_PromiseCapabilityReject(ARKTS_Env env, ARKTS_Promise prom, ARKTS_Value result)
{
    ARKTS_ASSERT_V(env, "env is null");
    ARKTS_ASSERT_V(prom, "prom is null");

    auto vm = P_CAST(env, EcmaVM*);
    JsiFastNativeScope fastNativeScope(vm);
    auto promise = P_CAST(prom, Global<PromiseCapabilityRef>*);
    (*promise)->Reject(vm, ARKTS_ToHandle<JSValueRef>(result));
    promise->FreeGlobalHandleAddr();
    delete promise;
}

/**
 * @brief True for heap objects tagged as Promise.
 */

bool ARKTS_IsPromise(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_F(env, "env is null");
    ARKTS_ASSERT_F(value.value, "value is null");
    auto v = BIT_CAST(value, JSValueRef);
    if (!v.IsHeapObject()) {
        return false;
    }
    auto vm = P_CAST(env, EcmaVM*);
    JsiFastNativeScope fastNativeScope(vm);
    v = *P_CAST(value.pointer, JSValueRef*);
    return v.IsPromise(vm);
}

/**
 * @brief Attach fulfillment/rejection handlers (`PromiseRef::Then`); optional rejection callback may be omitted.
 */

ARKTS_Value ARKTS_PromiseThen(ARKTS_Env env, ARKTS_Value prom, ARKTS_Value onFulfilled, ARKTS_Value onRejected)
{
    ARKTS_ASSERT_U(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    ARKTS_ASSERT_U(ARKTS_IsPromise(env, prom), "arg is not a JSPromise");
    ARKTS_ASSERT_U(ARKTS_IsCallable(env, onFulfilled), "onFulfilled is not callable");

    // Promise is stored as inline PromiseRef bits in ARKTS_Value; unwrap to call Then/Catch overloads.
    auto promise = *BIT_CAST(prom, PromiseRef*);
    auto onFulfilledFunc = BIT_CAST(onFulfilled, Local<FunctionRef>);
    Local<PromiseRef> result;
    if (!ARKTS_IsUndefined(onRejected)) {
        ARKTS_ASSERT_U(ARKTS_IsCallable(env, onRejected), "optional arg 'onRejected' is set but not callable");
        auto onRejectedFunc = BIT_CAST(onRejected, Local<FunctionRef>);
        result = promise.Then(vm, onFulfilledFunc, onRejectedFunc);
    } else {
        result = promise.Then(vm, onFulfilledFunc);
    }
    return ARKTS_FromHandle(result);
}

/**
 * @brief Attach rejection-only handler (`PromiseRef::Catch`).
 */

ARKTS_Value ARKTS_PromiseCatch(ARKTS_Env env, ARKTS_Value prom, ARKTS_Value callback)
{
    ARKTS_ASSERT_U(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    ARKTS_ASSERT_U(ARKTS_IsPromise(env, prom), "arg is not a JSPromise");
    ARKTS_ASSERT_U(ARKTS_IsCallable(env, callback), "callback is not callable");

    // Catch uses pointer-to-embedded PromiseRef (API shape differs from Then’s by-value PromiseRef).
    auto promise = BIT_CAST(prom, PromiseRef*);
    auto callbackFunc = BIT_CAST(callback, Local<FunctionRef>);
    auto result = promise->Catch(vm, callbackFunc);
    return ARKTS_FromHandle(result);
}

/**
 * @brief Open a `panda::LocalScope` to pin native-created locals across nested calls until explicitly closed.
 */

ARKTS_Scope ARKTS_OpenScope(ARKTS_Env env)
{
    ARKTS_ASSERT_P(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    return CJ::ARKTS_ScopeManager::OpenScope(vm);
}

/**
 * @brief Close the handle scope and lift `value` into an `ARKTS_Result` suitable for returning across the native boundary.
 *
 * On failure to close, reports native error ("exit scope of unknown").
 */

ARKTS_Result ARKTS_Return(ARKTS_Env env, ARKTS_Scope scope, ARKTS_Value value)
{
    ARKTS_ASSERT(env, "env is null", {});
    ARKTS_ASSERT(value.value, "value is invalid", {});

    auto vm = P_CAST(env, EcmaVM*);
    if (!CJ::ARKTS_ScopeManager::CloseScope(scope)) {
        ARKTSInner_ReportNativeError("exit scope of unknown");
    }

    // ARKTS_Result packages the handle for return across the FFI boundary (see also ARKTSInner_Escape).
    return ARKTS_ToResult(vm, value);
}

/**
 * @brief Close a previously opened scope without producing a result value.
 */

void ARKTS_CloseScope(ARKTS_Env env, ARKTS_Scope scope)
{
    ARKTS_ASSERT_V(env, "env is null");
    ARKTS_ASSERT_V(scope, "scope is invalid");

    if (!CJ::ARKTS_ScopeManager::CloseScope(scope)) {
        ARKTSInner_ReportNativeError("exit scope of unknown");
    }
}

/**
 * @brief Close scope then materialize `ret` into a VM local (`JSNApi::CreateLocal`) for escape semantics used by bindings.
 */

panda::JSValueRef* ARKTSInner_Escape(ARKTS_Env env, ARKTS_Scope scope, ARKTS_Value ret)
{
    if (!CJ::ARKTS_ScopeManager::CloseScope(scope)) {
        ARKTSInner_ReportNativeError("exit scope of unknown");
    }

    auto vm = P_CAST(env, EcmaVM*);
    auto value = ARKTS_ToValue(ret);

    auto result = JSNApi::CreateLocal(vm, value);

    return BIT_CAST(result, JSValueRef*);
}

/**
 * @brief Throw a JS exception: strings become `Exception::Error`, objects are thrown as-is; no-op if VM already pending.
 */

void ARKTS_Throw(ARKTS_Env env, ARKTS_Value error)
{
    ARKTS_ASSERT_V(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);

    if (JSNApi::HasPendingException(vm)) {
        LOGE("ARKTS_Throw failed, vm has uncaught exception");
        return;
    }

    auto exception = ARKTS_ToHandle<JSValueRef>(error);
    if (exception->IsString(vm)) {
        JSNApi::ThrowException(vm, Exception::Error(vm, BIT_CAST(exception, Local<StringRef>)));
    } else {
        JSNApi::ThrowException(vm, exception);
    }
}

/**
 * @brief Recover `ARKTS_Env` (`EcmaVM*`) from a `JsiRuntimeCallInfo` frame (callback trampoline context).
 */

ARKTS_Env ARKTS_GetCallEnv(ARKTS_CallInfo info)
{
    ARKTS_ASSERT_I(info, "info is null");
    auto runInfo = P_CAST(info, JsiRuntimeCallInfo*);
    auto vm = runInfo->GetVM();
    return P_CAST(vm, ARKTS_Env);
}

/**
 * @brief Number of actual arguments supplied by the JS caller (`GetArgsNumber`).
 */

uint32_t ARKTS_GetArgCount(ARKTS_CallInfo info)
{
    ARKTS_ASSERT_I(info, "info is null");
    auto runInfo = P_CAST(info, JsiRuntimeCallInfo*);
    return runInfo->GetArgsNumber();
}

/**
 * @brief Fetch argument by index as `ARKTS_Value` with bounds assertion.
 */

ARKTS_Value ARKTS_GetArg(ARKTS_CallInfo info, uint32_t index)
{
    ARKTS_ASSERT_U(info, "info is null");
    auto runInfo = P_CAST(info, JsiRuntimeCallInfo*);
    ARKTS_ASSERT_U(runInfo->GetArgsNumber() > index, "out of index");
    // Each arg is already a VM local bound to this frame.
    auto result = runInfo->GetCallArgRef(index);
    return ARKTS_FromHandle(result);
}

/**
 * @brief Value of JS `this` for the active call frame.
 */

ARKTS_Value ARKTS_GetThisArg(ARKTS_CallInfo info)
{
    ARKTS_ASSERT_U(info, "info is null");
    auto runInfo = P_CAST(info, JsiRuntimeCallInfo*);
    auto result = runInfo->GetThisRef();
    return ARKTS_FromHandle(result);
}

/**
 * @brief Retrieve the `napi_env` associated with this VM (`JSNApi::GetEnv`) for N-API interop.
 */

void* ARKTS_GetGlobalNapiEnv(ARKTS_Env env)
{
    ARKTS_ASSERT_P(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    return JSNApi::GetEnv(vm);
}

/**
 * @brief Forward stack metadata into the engine (`JSNApi::UpdateStackInfo`) for diagnostics/JS stack attribution.
 */

void ARKTS_UpdateStackInfo(unsigned long long vmAddress, void *subStackInfo, unsigned int opKind)
{
    ARKTS_ASSERT_V(vmAddress, "vm is null");
    ARKTS_ASSERT_V(subStackInfo, "stack info is null");
    auto vm = reinterpret_cast<panda::EcmaVM*>(vmAddress);
    panda::JSNApi::UpdateStackInfo(vm, subStackInfo, opKind);
}

namespace {
ARKTS_CycleFreeCallback* g_cycleFreeCallback = nullptr;
}

/**
 * @brief Install the singleton weak-cycle-free bridge callbacks (function invoke + refcount release) exactly once.
 */

void ARKTS_RegisterCycleFreeCallback(ARKTS_CycleFreeCallback callback)
{
    if (g_cycleFreeCallback == nullptr) {
        g_cycleFreeCallback = new ARKTS_CycleFreeCallback(callback);
    } else {
        LOGE("register cycle free callback failed, already registered.");
    }
}

/**
 * @brief Weak-friendly function path: dispatches through `g_cycleFreeCallback->funcInvoker` with lambda id.
 */

static Local<JSValueRef> CycleFreeFuncInvoker(JsiRuntimeCallInfo *callInfo)
{
    Local<JSValueRef> result;
    auto vm = callInfo->GetVM();
    if (!vm) {
        LOGE("failed to invoke cycleFree func, vm is null");
        return result;
    }
    result = JSValueRef::Undefined(vm);
    auto data = reinterpret_cast<LambdaData*>(callInfo->GetData());
    if (!data) {
        LOGE("failed to invoke cycleFree func, LambdaData is null");
        return result;
    }
    if (!g_cycleFreeCallback) {
        LOGE("failed to invoke cycleFree func, cycleFreeCallback is null");
        return result;
    }
    auto value = g_cycleFreeCallback->funcInvoker(P_CAST(callInfo, ARKTS_CallInfo), data->lambdaId);
    auto res = ARKTS_ToResult(vm, value);
    result = BIT_CAST(res, Local<JSValueRef>);
    return result;
}

/**
 * @brief Release bookkeeping for weak functions: calls `refRelease(lambdaId)` then frees `LambdaData`.
 */

static void CycleFreeFuncReleaser(void* /*env*/, void* /*data*/, void* hint)
{
    auto data = reinterpret_cast<LambdaData*>(hint);
    if (!data) {
        return;
    }
    if (!g_cycleFreeCallback) {
        delete data;
        return;
    }
    g_cycleFreeCallback->refRelease(data->lambdaId);
    delete data;
}

/**
 * @brief Like `ARKTS_CreateFunc` but wired to `CycleFreeFuncInvoker` / `CycleFreeFuncReleaser` for cycle-safe graphs.
 */

ARKTS_Value ARKTS_CreateCycleFreeFunc(ARKTS_Env env, int64_t id)
{
    ARKTS_ASSERT_U(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    auto data = new (std::nothrow) LambdaData {env, id};
    if (!data) {
        return ARKTS_CreateUndefined();
    }
    // No fifth `needSendable` flag here: weak/cycle-free path uses the default ctor overload.
    auto result = FunctionRef::New(vm, CycleFreeFuncInvoker, CycleFreeFuncReleaser, data);
    return ARKTS_FromHandle(result);
}

/**
 * @brief GC callback for weak externals: decode pointer payload back to `lambdaId` and `refRelease` it.
 */

static void CycleFreeObjectReleaser(void* /*env*/, void* nativePointer, void* /*hint*/)
{
    if (!g_cycleFreeCallback) {
        return;
    }
    // nativePointer is the deleter payload: id bits (64-bit) or ARKTSInner_External* (32-bit).
    if constexpr (sizeof(void*) == 8) {
        auto id = static_cast<int64_t>(reinterpret_cast<uintptr_t>(nativePointer));
        g_cycleFreeCallback->refRelease(id);
    } else {
        auto pointer = reinterpret_cast<ARKTSInner_External*>(nativePointer);
        g_cycleFreeCallback->refRelease(pointer->data);
    }
}

/**
 * @brief Create a NativePointer external that releases `id` through `CycleFreeObjectReleaser`.
 *
 * Mirrors 32/64-bit payload encoding used by `ARKTS_CreateExternal`.
 */

ARKTS_Value ARKTS_CreateCycleFreeExtern(ARKTS_Env env, int64_t id)
{
    ARKTS_ASSERT_U(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    // Payload layout must stay in sync with CycleFreeObjectReleaser and ARKTS_CreateExternal.

    if constexpr (sizeof(void*) == 8) {
        auto data = reinterpret_cast<void*>(static_cast<uintptr_t>(id));
        auto result = NativePointerRef::New(vm, data, CycleFreeObjectReleaser, nullptr);
        return ARKTS_FromHandle(result);
    } else {
        auto pointer = new ARKTSInner_External {id};
        auto result = NativePointerRef::New(vm, pointer, CycleFreeObjectReleaser, nullptr);
        return ARKTS_FromHandle(result);
    }
}

/**
 * @brief Atomically fetch and clear VM uncaught exception; returns `Undefined` handle if none was pending.
 */

ARKTS_Value ARKTS_GetExceptionAndClear(ARKTS_Env env)
{
    ARKTS_ASSERT_U(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    // Clears VM pending state so the next JS entry does not rethrow the same exception automatically.
    auto exception = JSNApi::GetAndClearUncaughtException(vm);
    return ARKTS_FromHandle(exception);
}

/**
 * @brief Synchronously load either a native ACE module (`ExecuteNativeModule`) or normal module namespace (`GetModuleNameSpaceFromFile`).
 *
 * Exceptions during load are left to `ARKTSInner_ReportJSErrors` with non-aborting policy here.
 */

ARKTS_Value ARKTS_RequireArkModule(ARKTS_Env env, const char* path, size_t pathLen, ARKTS_ModuleKind kind)
{
    ARKTS_ASSERT_U(env, "env is null");
    ARKTS_ASSERT_U(path, "path is null");
    ARKTS_ASSERT_U(kind >= ARKTS_NativeModule && kind <= ARKTS_NormalModule, "module kind is invalid");

    auto vm = P_CAST(env, EcmaVM*);
    std::string srcPath(path, pathLen);
    Local<JSValueRef> result;
    // special native api provided by ace(like: @native:ohos.curves).
    if (kind == ARKTS_NativeModule) {
        result = JSNApi::ExecuteNativeModule(vm, srcPath);
    } else {
        result = JSNApi::GetModuleNameSpaceFromFile(vm, srcPath);
    }
    ARKTSInner_ReportJSErrors(env, false);
    return ARKTS_FromHandle(result);
}