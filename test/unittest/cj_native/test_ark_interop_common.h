/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef TEST_ARK_INTEROP_COMMON_H
#define TEST_ARK_INTEROP_COMMON_H

#include "ark_interop_external.h"
#include "ark_interop_hitrace.h"
#include "ark_interop_internal.h"
#include "ark_interop_log.h"
#include "ark_interop_napi.h"
#include "gtest/gtest.h"
#include "uv_loop_handler.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include "cj_environment.h"

#include "napi/native_api.h"

using namespace testing;
using namespace testing::ext;

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

enum StackInfoOpKind : uint32_t {
    SWITCH_TO_SUB_STACK_INFO = 0,
    SWITCH_TO_MAIN_STACK_INFO,
};

struct StackInfo {
    uint64_t stackLimit;
    uint64_t lastLeaveFrame;
};

enum class MockContextKind { NORMAL, ERROR_CAPTURE };

class ArkInteropTest : public testing::Test {
public:
    static void RunLocalTest()
    {
        TestPrime();
        TestNumber();
        TestString();
        TestUtf16String();
        TestObject();
        TestKeyable();
        TestDefineProperty();
        TestDefinePropertyWritable();
        TestDefinePropertyConfigurable();
        TestDefinePropertyConfigurableV2();
        TestArray();
        TestBigintInt64();
        TestBigint16Bytes();
        TestBigint28Bytes();
        TestBigint32Bytes();
        TestPromise();
        TestSymbol();
        TestFunction();
        TestClass();
        TestInstanceOf();
        TestArrayBuffer();
        TestUpdateStackInfo();
    }
private:
    static void TestPrime();
    static void TestNumber();
    static void TestString();
    static void TestUtf16String();
    static void TestObject();
    static void TestKeyable();
    static void TestDefineProperty();
    static void TestDefinePropertyWritable();
    static void TestDefinePropertyConfigurable();
    static void TestDefinePropertyConfigurableV2();
    static void TestArray();
    static void TestBigintInt64();
    static void TestBigint16Bytes();
    static void TestBigint28Bytes();
    static void TestBigint32Bytes();
    static void TestPromise();
    static void TestSymbol();
    static void TestFunction();
    static void TestClass();
    static void TestArrayBuffer();
    static void TestInstanceOf();
    static void TestUpdateStackInfo();
};

template <typename T>
struct Slab {
public:
    int64_t Add(T item)
    {
        int64_t result;
        if (last_ >= 0) {
            result = last_;
            auto& last = items_[result];
            last_ = last.prev;
            last.data = std::move(item);
        } else {
            result = items_.size();
            items_.push_back({std::move(item), -1});
        }
        ++length;
        return result;
    }

    void Del(int64_t id)
    {
        auto& removing = items_[id];
        if (!removing.data.has_value()) {
            return;
        }
        removing.data = std::nullopt;
        removing.prev = last_;
        last_ = id;
        --length;
    }

    size_t Size() const
    {
        return length;
    }

    bool Has(int64_t id) const
    {
        if (id < 0 || id >= length) {
            return false;
        }
        return items_[id].data.has_value();
    }

    T& Get(int64_t id)
    {
        return items_[id].data.value();
    }

    const T& Get(int64_t id) const
    {
        return items_[id].data.value();
    }
private:
    struct Item {
        std::optional<T> data;
        int64_t prev;
    };
    std::vector<Item> items_ {};
    int64_t last_ = -1;
    size_t length = 0;
};

class MockContext {
    static MockContext* instance_;
    static uint32_t NewId()
    {
        static uint32_t id = 0;
        return id++;
    }

public:
    static MockContext* GetInstance()
    {
        return instance_;
    }

    ARKTS_Engine GetEngine() const
    {
        return engine_;
    }

    ARKTS_Env GetEnv() const
    {
        return ARKTS_GetContext(engine_);
    }

    static void Init();

    explicit MockContext(ARKTS_Engine engine, bool needDestroyEngine = true): idPrefix_(NewId()),
        lastContext_(instance_), engine_(engine), needDestroyEngine_(needDestroyEngine)
    {
        instance_ = this;
    }

    MockContext(): idPrefix_(NewId()), lastContext_(instance_), engine_(instance_->GetEngine()),
        needDestroyEngine_(false)
    {
        instance_ = this;
    }

    ~MockContext()
    {
        if (needDestroyEngine_) {
            ARKTS_DestroyEngine(engine_);
        }
        engine_ = nullptr;
        instance_ = lastContext_;
    }

    int64_t StoreFunc(std::function<ARKTS_Value(ARKTS_CallInfo)> call)
    {
        return ToId(callbacks_.Add(std::move(call)));
    }

    int64_t StoreAsyncFunc(std::function<void ()> call)
    {
        return ToId(asyncCallbacks_.Add(std::move(call)));
    }

    int64_t StoreArrayBufferDeleter(std::function<void (void*)> call)
    {
        return ToId(nativeDeleters_.Add(std::move(call)));
    }

    int64_t StoreNativeData(void* data, std::function<void (void*)>deleter)
    {
        return ToId(data_.Add({data, std::move(deleter)}));
    }

    void* GetFuncAddr(int64_t id)
    {
        auto index = ToIndex(id);
        if (data_.Has(index)) {
            return &data_.Get(index);
        }
        return nullptr;
    }

    void SetNativeCallingCallback(std::function<void(const void*)> callback)
    {
        nativeCallingCallback_ = std::move(callback);
    }

    void SetNativeReturnCallback(std::function<void (const void*)> callback)
    {
        nativeReturnCallback_ = std::move(callback);
    }

    void NotifyNativeCalling(const void* addr)
    {
        if (nativeCallingCallback_) {
            nativeCallingCallback_(addr);
        }
    }

    void NotifyNativeReturn(const void* addr)
    {
        if (nativeReturnCallback_) {
            nativeReturnCallback_(addr);
        }
    }

    void* GetNativeData(int64_t id)
    {
        auto index = ToIndex(id);
        if (index < 0) {
            return nullptr;
        }
        return data_.Get(index).data;
    }

    void SetFinalizerCallback(std::function<void (ARKTS_Env)> callback)
    {
        finalizerCallback_ = std::move(callback);
    }

protected:
    virtual MockContextKind GetContextKind() const
    {
        return MockContextKind::NORMAL;
    }

    virtual ARKTS_Value ExportModule(ARKTS_Env env, const char* dllName, ARKTS_Value exports)
    {
        return exports;
    }

    virtual bool HasModuleHandle(const char* dllName)
    {
        return false;
    }

    virtual void ThrowJSError(ARKTS_Env env, ARKTS_Value value)
    {
        (void)env;
        (void)value;
        EXPECT_TRUE(false);
    }

    virtual void ThrowNativeError(const char* msg)
    {
        printf("[Native Error]: %s\n", msg);
        EXPECT_TRUE(false);
    }

    virtual void DeleteArrayBufferRawData(void* buffer, int64_t id)
    {
        auto index = ToIndex(id);
        if (index < 0) {
            return;
        }
        nativeDeleters_.Get(index)(buffer);
        nativeDeleters_.Del(index);
    }

    virtual void DeleteExternal(int64_t id, ARKTS_Env env)
    {
        auto index = ToIndex(id);
        if (index < 0) {
            return;
        }
        auto& data = data_.Get(index);
        data.deleter(data.data);
        data_.Del(index);
    }

    virtual ARKTS_Value InvokeLambda(ARKTS_CallInfo info, int64_t id)
    {
        auto index = ToIndex(id);
        if (index < 0) {
            return ARKTS_CreateUndefined();
        }
        return callbacks_.Get(index)(info);
    }

    virtual void DeleteLambda(ARKTS_Env env, int64_t id)
    {
        auto index = ToIndex(id);
        if (index < 0) {
            return;
        }
        callbacks_.Del(index);
    }

    virtual void InvokeAsyncLambda(ARKTS_Env env, int64_t id)
    {
        auto index = ToIndex(id);
        if (index < 0) {
            return;
        }
        asyncCallbacks_.Get(index)();
        asyncCallbacks_.Del(index);
    }

    virtual void DeleteJSContext(ARKTS_Env env)
    {
        if (finalizerCallback_) {
            finalizerCallback_(env);
        }
    }

    virtual ARKTS_Value InvokeCycleFreeFunc(ARKTS_CallInfo callInfo, uint32_t id)
    {
        return ARKTS_CreateUndefined();
    }

    virtual void ReleaseCycleFreeExt(uint32_t id)
    {
    }

    int64_t ToId(int64_t index) const
    {
        return GetPrefixMask() | index;
    }

    int64_t ToIndex(int64_t id) const
    {
        auto myMask = GetPrefixMask();
        if ((id & myMask) == myMask) {
            return id & 0xFFFF'FFFF;
        }
        return -1;
    }

private:
    static void InitModuleCallbacks();
    static void InitCycleFreeCallbacks();
    static void InitDebugModeCallbacks();

    int64_t GetPrefixMask() const
    {
        constexpr auto idBits = 32;
        return static_cast<int64_t>(idPrefix_) << idBits;
    }

    uint32_t idPrefix_;
    MockContext* lastContext_;
    ARKTS_Engine engine_;
    bool needDestroyEngine_;
    Slab<std::function<ARKTS_Value (ARKTS_CallInfo)>> callbacks_;
    Slab<std::function<void ()>> asyncCallbacks_;
    Slab<std::function<void (void*)>> nativeDeleters_;
    struct AnyData {
        void* data;
        std::function<void (void*)> deleter;
    };
    Slab<AnyData> data_;
    std::function<void (ARKTS_Env)> finalizerCallback_;
    std::function<void (const void*)> nativeCallingCallback_;
    std::function<void (const void*)> nativeReturnCallback_;
};

inline void MockContext::Init()
{
    InitModuleCallbacks();
    InitCycleFreeCallbacks();
    InitDebugModeCallbacks();
}

inline ARKTS_Value ExportModuleCallback(ARKTS_Env env, const char* dllName, ARKTS_Value exports)
{
    return MockContext::GetInstance() ? MockContext::GetInstance()->ExportModule(env, dllName, exports) : exports;
}

inline bool HasModuleHandleCallback(const char* dllName)
{
    return MockContext::GetInstance() ? MockContext::GetInstance()->HasModuleHandle(dllName) : false;
}

inline void ThrowJSErrorCallback(ARKTS_Env env, ARKTS_Value error)
{
    auto* inst = MockContext::GetInstance();
    if (!inst) {
        return;
    }
    if (inst->GetContextKind() == MockContextKind::ERROR_CAPTURE) {
        static_cast<ErrorCaptureContext*>(inst)->ReportCapturedJSError(env, error);
        return;
    }
    inst->ThrowJSError(env, error);
}

inline void ThrowNativeErrorCallback(const char* error)
{
    auto* inst = MockContext::GetInstance();
    if (!inst) {
        return;
    }
    if (inst->GetContextKind() == MockContextKind::ERROR_CAPTURE) {
        static_cast<ErrorCaptureContext*>(inst)->ReportCapturedNativeError(error);
        return;
    }
    inst->ThrowNativeError(error);
}

inline void MockContext::InitModuleCallbacks()
{
    static ARKTS_ModuleCallbacks callbacks {
        .exportModule = ExportModuleCallback,
        .hasModuleHandle = HasModuleHandleCallback,
        .throwJSError = ThrowJSErrorCallback,
        .throwNativeError = ThrowNativeErrorCallback,
        .deleteArrayBufferRawData = [](void* buffer, int64_t lambdaId)->void {
            if (instance_) instance_->DeleteArrayBufferRawData(buffer, lambdaId);
        },
        .deleteExternal = [](int64_t id, ARKTS_Env env)->void {
            if (instance_) instance_->DeleteExternal(id, env);
        },
        .invokerLambda = [](ARKTS_CallInfo callInfo, int64_t lambdaId)->ARKTS_Value {
            return instance_ ? instance_->InvokeLambda(callInfo, lambdaId) : ARKTS_CreateUndefined();
        },
        .deleteLambda = [](ARKTS_Env env, int64_t id)->void {
            if (instance_) instance_->DeleteLambda(env, id);
        },
        .invokeAsyncLambda = [](ARKTS_Env env, int64_t id)->void {
            if (instance_) instance_->InvokeAsyncLambda(env, id);
        },
        .deleteJSContext = [](ARKTS_Env env)->void {
            if (instance_) instance_->DeleteJSContext(env);
        }
    };
    ARKTS_SetCJModuleCallback(&callbacks);
}

inline void MockContext::InitCycleFreeCallbacks()
{
    static ARKTS_CycleFreeCallback cycleFreeCallback {
        .funcInvoker = [](ARKTS_CallInfo callInfo, int64_t id) {
            if (instance_) {
                return instance_->InvokeCycleFreeFunc(callInfo, id);
            }
            return ARKTS_CreateUndefined();
        },
        .refRelease = [](int64_t id) {
            if (instance_) {
                instance_->ReleaseCycleFreeExt(id);
            }
        }
    };
    ARKTS_RegisterCycleFreeCallback(cycleFreeCallback);
}

inline void MockContext::InitDebugModeCallbacks()
{
}

class ErrorCaptureContext : public MockContext {
public:
    ErrorCaptureContext() = default;
    explicit ErrorCaptureContext(ARKTS_Engine engine, bool needDestroyEngine = true)
        : MockContext(engine, needDestroyEngine) {}
    ~ErrorCaptureContext()
    {
        if (hasJSError_ || hasNativeError_) {
            printf("has unhandled js error or native error\n");
            std::abort();
        }
    }

    bool HasAndClearJSError()
    {
        if (hasJSError_) {
            hasJSError_ = false;
            return true;
        }
        return false;
    }

    bool HasAndClearNativeError()
    {
        if (hasNativeError_) {
            hasNativeError_ = false;
            return true;
        }
        return false;
    }

    void ReportCapturedNativeError(const char* error)
    {
        if (hasNativeError_) {
            printf("has unhandled native error: %s\n", error);
            std::abort();
        }
        hasNativeError_ = true;
    }

    void ReportCapturedJSError(ARKTS_Env /* env */, ARKTS_Value /* value */)
    {
        if (hasJSError_) {
            printf("has unhandled js error\n");
            std::abort();
        }
        hasJSError_ = true;
    }
protected:
    MockContextKind GetContextKind() const override
    {
        return MockContextKind::ERROR_CAPTURE;
    }

    void ThrowJSError(ARKTS_Env env, ARKTS_Value value) override
    {
        ReportCapturedJSError(env, value);
    }

    void ThrowNativeError(const char* msg) override
    {
        printf("[Native Error]: %s\n", msg);
        ReportCapturedNativeError(msg);
    }

private:
    bool hasJSError_ = false;
    bool hasNativeError_ = false;
};

class ModuleRegisteredContext : public ErrorCaptureContext {
protected:
    bool HasModuleHandle(const char* /* dllName */) override
    {
        return true;
    }
};

class CycleFreeContext : public ErrorCaptureContext {
public:
    CycleFreeContext() = default;
    explicit CycleFreeContext(ARKTS_Engine engine, bool needDestroyEngine = true)
        : ErrorCaptureContext(engine, needDestroyEngine) {}
    uint32_t StoreCycleFreeFunc(std::function<ARKTS_Value(ARKTS_CallInfo)> callback)
    {
        std::lock_guard lock(callbackMutex_);
        return callbacks_.Add(std::move(callback));
    }

    size_t GetCycleFreeFuncCount() const
    {
        return callbacks_.Size();
    }

protected:
    ARKTS_Value InvokeCycleFreeFunc(ARKTS_CallInfo callInfo, uint32_t id) override
    {
        std::lock_guard lock(callbackMutex_);
        auto callback = callbacks_.Get(id);
        if (!callback) {
            return ARKTS_CreateUndefined();
        }
        return callback(callInfo);
    }

    void ReleaseCycleFreeExt(uint32_t id) override
    {
        callbacks_.Del(id);
    }
private:
    static Slab<std::function<ARKTS_Value(ARKTS_CallInfo)>> callbacks_;
    static std::mutex callbackMutex_;
};

struct AssertTestContext {
    ARKTS_Env nullEnv = nullptr;
    ARKTS_CallInfo nullInfo = nullptr;
    ARKTS_Engine nullEngine = nullptr;
    ARKTS_Scope nullScope = nullptr;
    ARKTS_Global nullGlobal = nullptr;
    ARKTS_Promise nullPromise = nullptr;
    ARKTS_Value zeroValue { 0 };
    char smallBuf[8] = {};
    uint8_t smallBytes[8] = {};
    bool isNegative = false;
    ARKTS_Value oneArg[1] = { zeroValue };
    ARKTS_Accessor accessor { zeroValue, zeroValue, N_WRITABLE };
    ARKTS_Engine tempEngine = nullptr;
    ARKTS_Env normalEnv = nullptr;
};

static void TriggerBasicAssertBranches(AssertTestContext& ctx)
{
    (void)ARKTS_GetGlobalConstant(ctx.nullEnv);
    (void)ARKTS_GetGlobalNapiEnv(ctx.nullEnv);
    (void)ARKTS_GetValueType(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_StrictEqual(ctx.nullEnv, ctx.zeroValue, ctx.zeroValue);
}

static void TriggerStringAssertBranches(AssertTestContext& ctx)
{
    (void)ARKTS_GetValueUtf8(ctx.nullEnv, ctx.zeroValue, 0, ctx.smallBuf);
    (void)ARKTS_GetValueUtf8Size(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_CreateUtf8(ctx.nullEnv, nullptr, 0);
    (void)ARKTS_GetValueCString(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_IsString(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_GetStringInfo(ctx.nullEnv, ctx.zeroValue);
    ARKTS_StringCopy(ctx.nullEnv, ctx.zeroValue, ctx.smallBuf, 0);
    (void)ARKTS_CreateString(ctx.nullEnv, false, 0, nullptr);
    if (ctx.normalEnv) {
        (void)ARKTS_CreateUtf8(ctx.normalEnv, nullptr, 0);
        (void)ARKTS_IsString(ctx.normalEnv, ctx.zeroValue);
        (void)ARKTS_GetValueUtf8Size(ctx.normalEnv, ctx.zeroValue);
        (void)ARKTS_GetValueUtf8(ctx.normalEnv, ctx.zeroValue, 1, ctx.smallBuf);
        (void)ARKTS_GetValueCString(ctx.normalEnv, ctx.zeroValue);
        (void)ARKTS_CreateString(ctx.normalEnv, false, 0, nullptr);
        auto oneChar = ARKTS_CreateUtf8(ctx.normalEnv, "a", 1);
        (void)ARKTS_GetValueUtf8(ctx.normalEnv, oneChar, 1, nullptr);
    }
}

static void TriggerFunctionClassAssertBranches(AssertTestContext& ctx)
{
    (void)ARKTS_CreateFunc(ctx.nullEnv, 0);
    (void)ARKTS_CreateExternal(ctx.nullEnv, 0);
    (void)ARKTS_IsCallable(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_IsClass(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_CreateClass(ctx.nullEnv, 0, ctx.zeroValue);
    (void)ARKTS_GetPrototype(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_InstanceOf(ctx.nullEnv, ctx.zeroValue, ctx.zeroValue);
}

static void TriggerCallInfoAssertBranches(AssertTestContext& ctx)
{
    (void)ARKTS_GetCallEnv(ctx.nullInfo);
    (void)ARKTS_GetArgCount(ctx.nullInfo);
    (void)ARKTS_GetArg(ctx.nullInfo, 0);
    (void)ARKTS_GetThisArg(ctx.nullInfo);
}

static void TriggerCallNewAssertBranches(AssertTestContext& ctx)
{
    (void)ARKTS_Call(ctx.nullEnv, ctx.zeroValue, ctx.zeroValue, 0, nullptr);
    (void)ARKTS_New(ctx.nullEnv, ctx.zeroValue, 0, nullptr);
}

static void TriggerObjectAssertBranches(AssertTestContext& ctx)
{
    (void)ARKTS_CreateObject(ctx.nullEnv);
    (void)ARKTS_IsObject(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_HasOwnProperty(ctx.nullEnv, ctx.zeroValue, ctx.zeroValue);
    (void)ARKTS_EnumOwnProperties(ctx.nullEnv, ctx.zeroValue);
    ARKTS_DefineOwnProperty(ctx.nullEnv, ctx.zeroValue, ctx.zeroValue, ctx.zeroValue, N_WRITABLE);
    (void)ARKTS_DefineOwnPropertyV2(ctx.nullEnv, ctx.zeroValue, ctx.zeroValue, ctx.zeroValue, N_WRITABLE);
    ARKTS_DefineAccessors(ctx.nullEnv, ctx.zeroValue, ctx.zeroValue, ctx.accessor);
    (void)ARKTS_DefineAccessorsV2(ctx.nullEnv, ctx.zeroValue, ctx.zeroValue, ctx.accessor);
    ARKTS_SetProperty(ctx.nullEnv, ctx.zeroValue, ctx.zeroValue, ctx.zeroValue);
    (void)ARKTS_GetProperty(ctx.nullEnv, ctx.zeroValue, ctx.zeroValue);
    if (ctx.normalEnv) {
        auto invalidObj = ARKTS_CreateUndefined();
        auto invalidKey = ARKTS_CreateUndefined();
        (void)ARKTS_IsObject(ctx.normalEnv, ctx.zeroValue);
        (void)ARKTS_HasOwnProperty(ctx.normalEnv, invalidObj, invalidKey);
        (void)ARKTS_EnumOwnProperties(ctx.normalEnv, invalidObj);
        ARKTS_DefineOwnProperty(ctx.normalEnv, invalidObj, invalidKey, ctx.zeroValue, N_WRITABLE);
        (void)ARKTS_DefineOwnPropertyV2(ctx.normalEnv, invalidObj, invalidKey, ctx.zeroValue, N_WRITABLE);
        ARKTS_DefineAccessors(ctx.normalEnv, invalidObj, invalidKey, ctx.accessor);
        (void)ARKTS_DefineAccessorsV2(ctx.normalEnv, invalidObj, invalidKey, ctx.accessor);
        ARKTS_SetProperty(ctx.normalEnv, invalidObj, invalidKey, ctx.zeroValue);
        (void)ARKTS_GetProperty(ctx.normalEnv, invalidObj, invalidKey);
        auto validObj = ARKTS_CreateObject(ctx.normalEnv);
        (void)ARKTS_HasOwnProperty(ctx.normalEnv, validObj, invalidKey);
        ARKTS_DefineOwnProperty(ctx.normalEnv, validObj, invalidKey, ctx.zeroValue, N_WRITABLE);
        (void)ARKTS_DefineOwnPropertyV2(ctx.normalEnv, validObj, invalidKey, ctx.zeroValue, N_WRITABLE);
        ARKTS_DefineAccessors(ctx.normalEnv, validObj, invalidKey, ctx.accessor);
        (void)ARKTS_DefineAccessorsV2(ctx.normalEnv, validObj, invalidKey, ctx.accessor);
        ARKTS_SetProperty(ctx.normalEnv, validObj, invalidKey, ctx.zeroValue);
        (void)ARKTS_GetProperty(ctx.normalEnv, validObj, invalidKey);
    }
}

static void TriggerArrayAssertBranches(AssertTestContext& ctx)
{
    (void)ARKTS_CreateArray(ctx.nullEnv, 0);
    (void)ARKTS_CreateArrayWithInit(ctx.nullEnv, 0, ctx.oneArg);
    (void)ARKTS_GetArrayLength(ctx.nullEnv, ctx.zeroValue);
    ARKTS_SetElement(ctx.nullEnv, ctx.zeroValue, 0, ctx.zeroValue);
    (void)ARKTS_GetElement(ctx.nullEnv, ctx.zeroValue, 0);
    (void)ARKTS_IsArray(ctx.nullEnv, ctx.zeroValue);
}

static void TriggerGlobalAssertBranches(AssertTestContext& ctx)
{
    (void)ARKTS_CreateGlobal(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_GlobalIsAlive(ctx.nullEnv, ctx.nullGlobal);
    if (ctx.normalEnv) {
        (void)ARKTS_GlobalIsAlive(ctx.normalEnv, ctx.nullGlobal);
    }
    (void)ARKTS_GetGlobalValue(ctx.nullGlobal);
    ARKTS_DisposeGlobal(ctx.nullEnv, ctx.nullGlobal);
    if (ctx.normalEnv) {
        ARKTS_DisposeGlobal(ctx.normalEnv, ctx.nullGlobal);
    }
    ARKTS_DisposeGlobalSync(ctx.nullEnv, ctx.nullGlobal);
    if (ctx.normalEnv) {
        ARKTS_DisposeGlobalSync(ctx.normalEnv, ctx.nullGlobal);
    }
    ARKTS_GlobalSetWeak(ctx.nullEnv, ctx.nullGlobal);
    if (ctx.normalEnv) {
        ARKTS_GlobalSetWeak(ctx.normalEnv, ctx.nullGlobal);
    }
    ARKTS_GlobalClearWeak(ctx.nullEnv, ctx.nullGlobal);
    if (ctx.normalEnv) {
        ARKTS_GlobalClearWeak(ctx.normalEnv, ctx.nullGlobal);
    }
    (void)ARKTS_GlobalToValue(ctx.nullEnv, ctx.nullGlobal);
    if (ctx.normalEnv) {
        (void)ARKTS_GlobalToValue(ctx.normalEnv, ctx.nullGlobal);
    }
    (void)ARKTS_GlobalFromValue(ctx.nullEnv, ctx.zeroValue);
}

static void TriggerSymbolScopeAssertBranches(AssertTestContext& ctx)
{
    (void)ARKTS_IsExternal(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_GetExternalData(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_CreateSymbol(ctx.nullEnv, nullptr, 0);
    (void)ARKTS_IsSymbol(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_GetSymbolDesc(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_OpenScope(ctx.nullEnv);
    ARKTS_CloseScope(ctx.nullEnv, ctx.nullScope);
    if (ctx.normalEnv) {
        ARKTS_CloseScope(ctx.normalEnv, ctx.nullScope);
    }
}

static void TriggerExceptionEngineAssertBranches(AssertTestContext& ctx)
{
    (void)ARKTS_GetExceptionAndClear(ctx.nullEnv);
    (void)ARKTS_RequireArkModule(ctx.nullEnv, nullptr, 0, ARKTS_NormalModule);
    (void)ARKTS_GetNAPIEnv(ctx.nullEngine);
    ARKTS_DestroyEngine(ctx.nullEngine);
    (void)ARKTS_GetContext(ctx.nullEngine);
    (void)ARKTS_GetThreadIdOfEngine(ctx.nullEngine);
    (void)ARKTS_LoadEntryFromAbc(ctx.nullEngine, nullptr, nullptr, false);
    (void)ARKTS_ImportFromEntry(ctx.nullEngine, nullptr, nullptr);
    (void)ARKTS_Require(ctx.nullEnv, nullptr, false, false, nullptr);
}

static void TriggerPromiseAssertBranches(AssertTestContext& ctx)
{
    (void)ARKTS_CreatePromiseCapability(ctx.nullEnv);
    (void)ARKTS_GetPromiseFromCapability(ctx.nullEnv, ctx.nullPromise);
    if (ctx.normalEnv) {
        (void)ARKTS_GetPromiseFromCapability(ctx.normalEnv, ctx.nullPromise);
    }
    ARKTS_PromiseCapabilityResolve(ctx.nullEnv, ctx.nullPromise, ctx.zeroValue);
    if (ctx.normalEnv) {
        ARKTS_PromiseCapabilityResolve(ctx.normalEnv, ctx.nullPromise, ctx.zeroValue);
    }
    ARKTS_PromiseCapabilityReject(ctx.nullEnv, ctx.nullPromise, ctx.zeroValue);
    if (ctx.normalEnv) {
        ARKTS_PromiseCapabilityReject(ctx.normalEnv, ctx.nullPromise, ctx.zeroValue);
    }
    (void)ARKTS_IsPromise(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_PromiseThen(ctx.nullEnv, ctx.zeroValue, ctx.zeroValue, ctx.zeroValue);
    (void)ARKTS_PromiseCatch(ctx.nullEnv, ctx.zeroValue, ctx.zeroValue);
    ARKTS_CreateAsyncTask(ctx.nullEnv, 0);
    ARKTS_InitEventHandle(ctx.nullEnv);
}

static void TriggerArrayBufferBigIntAssertBranches(AssertTestContext& ctx)
{
    (void)ARKTS_CreateArrayBuffer(ctx.nullEnv, 0);
    (void)ARKTS_CreateArrayBufferWithData(ctx.nullEnv, nullptr, 0, 0);
    (void)ARKTS_IsArrayBuffer(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_GetArrayBufferLength(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_GetArrayBufferRawPtr(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_ArrayBufferReadBytes(ctx.nullEnv, ctx.zeroValue, ctx.smallBuf, 0);
    (void)ARKTS_CreateBigInt(ctx.nullEnv, 0);
    (void)ARKTS_CreateBigIntWithBytes(ctx.nullEnv, false, 0, ctx.smallBytes);
    (void)ARKTS_IsBigInt(ctx.nullEnv, ctx.zeroValue);
    (void)ARKTS_BigIntGetByteSize(ctx.nullEnv, ctx.zeroValue);
    ARKTS_BigIntReadBytes(ctx.nullEnv, ctx.zeroValue, &ctx.isNegative, 0, ctx.smallBytes);
}

static void TriggerCycleFreeMiscAssertBranches(AssertTestContext& ctx)
{
    (void)ARKTS_CreateCycleFreeFunc(ctx.nullEnv, 0);
    (void)ARKTS_CreateCycleFreeExtern(ctx.nullEnv, 0);
    ARKTS_UpdateStackInfo(0, nullptr, 0);
}

static void TriggerArktsAssertEarlyReturnBranches()
{
    AssertTestContext ctx;
    ctx.tempEngine = ARKTS_CreateEngine();
    ctx.normalEnv = ctx.tempEngine ? ARKTS_GetContext(ctx.tempEngine) : nullptr;

    TriggerBasicAssertBranches(ctx);
    TriggerStringAssertBranches(ctx);
    TriggerFunctionClassAssertBranches(ctx);
    TriggerCallInfoAssertBranches(ctx);
    TriggerCallNewAssertBranches(ctx);
    TriggerObjectAssertBranches(ctx);
    TriggerArrayAssertBranches(ctx);
    TriggerGlobalAssertBranches(ctx);
    TriggerSymbolScopeAssertBranches(ctx);
    TriggerExceptionEngineAssertBranches(ctx);
    TriggerPromiseAssertBranches(ctx);
    TriggerArrayBufferBigIntAssertBranches(ctx);
    TriggerCycleFreeMiscAssertBranches(ctx);

    if (ctx.tempEngine) {
        ARKTS_DestroyEngine(ctx.tempEngine);
    }
}

#endif // TEST_ARK_INTEROP_COMMON_H
