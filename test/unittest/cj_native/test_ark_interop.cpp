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

#include "ark_interop_external.h"
#include "ark_interop_hitrace.h"
#include "ark_interop_internal.h"
#include "ark_interop_log.h"
#include "ark_interop_napi.h"
#include "gtest/gtest.h"
#include "uv_loop_handler.h"

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

namespace {

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

void MockContext::Init()
{
    InitModuleCallbacks();
    InitCycleFreeCallbacks();
    InitDebugModeCallbacks();
}

void MockContext::InitCycleFreeCallbacks()
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

void MockContext::InitDebugModeCallbacks()
{
}

MockContext* MockContext::instance_ = nullptr;

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

void MockContext::InitModuleCallbacks()
{
    static ARKTS_ModuleCallbacks callbacks {
        .exportModule = [](ARKTS_Env env, const char* dllName, ARKTS_Value exports)->ARKTS_Value {
            return instance_ ? instance_->ExportModule(env, dllName, exports) : exports;
        },
        .hasModuleHandle = [](const char* dllName)->bool {
            return instance_ ? instance_->HasModuleHandle(dllName) : false;
        },
        .throwJSError = [](ARKTS_Env env, ARKTS_Value error)->void {
            auto* inst = MockContext::GetInstance();
            if (!inst) {
                return;
            }
            if (inst->GetContextKind() == MockContextKind::ERROR_CAPTURE) {
                static_cast<ErrorCaptureContext*>(inst)->ReportCapturedJSError(env, error);
                return;
            }
            inst->ThrowJSError(env, error);
        },
        .throwNativeError = [](const char* error)->void {
            auto* inst = MockContext::GetInstance();
            if (!inst) {
                return;
            }
            if (inst->GetContextKind() == MockContextKind::ERROR_CAPTURE) {
                static_cast<ErrorCaptureContext*>(inst)->ReportCapturedNativeError(error);
                return;
            }
            inst->ThrowNativeError(error);
        },
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

Slab<std::function<ARKTS_Value(ARKTS_CallInfo)>> CycleFreeContext::callbacks_;
std::mutex CycleFreeContext::callbackMutex_;

void ArkInteropTest::TestPrime()
{
    auto env = MockContext::GetInstance()->GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto jsUDef = ARKTS_CreateUndefined();
    EXPECT_TRUE(ARKTS_IsUndefined(jsUDef));
    EXPECT_EQ(ARKTS_GetValueType(env, jsUDef), N_UNDEFINED);

    auto jsNull = ARKTS_CreateNull();
    EXPECT_TRUE(ARKTS_IsNull(jsNull));
    EXPECT_EQ(ARKTS_GetValueType(env, jsNull), N_NULL);

    ARKTS_Value jsBools[] {
        ARKTS_CreateBool(true),
        ARKTS_CreateBool(false)
    };
    EXPECT_TRUE(ARKTS_IsBool(jsBools[0]));
    EXPECT_EQ(ARKTS_GetValueType(env, jsBools[0]), N_BOOL);
    EXPECT_TRUE(ARKTS_GetValueBool(jsBools[0]));
    EXPECT_FALSE(ARKTS_GetValueBool(jsBools[1]));

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestNumber()
{
    auto env = MockContext::GetInstance()->GetEnv();
    auto scope = ARKTS_OpenScope(env);
    double origins[] {
        0.1,
        -12.1,
        12456.126546
    };
    constexpr auto totalCases = std::size(origins);
    ARKTS_Value jsValues[totalCases];
    double received[totalCases];

    for (size_t i = 0;i < totalCases; i++) {
        jsValues[i] = ARKTS_CreateF64(origins[i]);
        EXPECT_EQ(ARKTS_GetValueType(env, jsValues[i]), N_NUMBER);
        EXPECT_TRUE(ARKTS_IsNumber(jsValues[i]));
        received[i] = ARKTS_GetValueNumber(jsValues[i]);
        EXPECT_EQ(origins[i], received[i]);
    }

    auto jsNan = ARKTS_CreateF64(NAN);
    auto nNan = ARKTS_GetValueNumber(jsNan);
    EXPECT_TRUE(std::isnan(nNan));
    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestString()
{
    auto env = MockContext::GetInstance()->GetEnv();
    auto scope = ARKTS_OpenScope(env);
    const char* origins[] {
        "a plain text",
        "`~!@#$%^&*()_+[]\\",
        "中文字符",
        "😊😂🤣❤️😍😒👌😘",
    };
    ARKTS_Value jsValues[] {
        ARKTS_CreateUtf8(env, origins[0], strlen(origins[0])),
        ARKTS_CreateUtf8(env, origins[1], strlen(origins[1])),
        ARKTS_CreateUtf8(env, origins[2], strlen(origins[2])),
        ARKTS_CreateUtf8(env, origins[3], strlen(origins[3])),
    };
    EXPECT_TRUE(ARKTS_IsString(env, jsValues[0]));
    EXPECT_TRUE(ARKTS_IsHeapObject(jsValues[0]));
    EXPECT_EQ(ARKTS_GetValueType(env, jsValues[0]), N_STRING);
    for (auto i = 0;i < sizeof(jsValues)/sizeof(ARKTS_Value); i++) {
        auto size = ARKTS_GetValueUtf8Size(env, jsValues[i]);
        std::string result;
        result.resize(size - 1);
        ARKTS_GetValueUtf8(env, jsValues[i], size - 1, result.data());
        EXPECT_EQ(result, origins[i]);

        auto cStr = ARKTS_GetValueCString(env, jsValues[i]);
        result = cStr;
        EXPECT_EQ(result, origins[i]);
        ARKTS_FreeCString(cStr);
    }
    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestUtf16String()
{
    auto env = MockContext::GetInstance()->GetEnv();
    auto scope = ARKTS_OpenScope(env);

    std::string latin1Case[] {
        "a plain text",
        "hello, world!",
        "./[]124"
    };
    std::u16string utf16Cases[] {
        u"a plain text, 和",
        u"你好，世界！",
        u"🦐😀"
    };

    for (const auto& one : latin1Case) {
        auto value = ARKTS_CreateString(env, true, one.size(), one.data());
        EXPECT_TRUE(ARKTS_IsString(env, value));
        auto header = ARKTS_GetStringInfo(env, value);
        EXPECT_EQ(header.length, one.size());
        EXPECT_TRUE(header.isCompressed);
    }

    for (const auto& one : utf16Cases) {
        auto value = ARKTS_CreateString(env, false, one.size(), one.data());
        EXPECT_TRUE(ARKTS_IsString(env, value));
        auto header = ARKTS_GetStringInfo(env, value);
        EXPECT_EQ(header.length, one.size());
        EXPECT_FALSE(header.isCompressed);
    }

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestObject()
{
    auto env = MockContext::GetInstance()->GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto obj = ARKTS_CreateObject(env);
    EXPECT_TRUE(ARKTS_IsHeapObject(obj));
    EXPECT_TRUE(ARKTS_IsObject(env, obj));
    EXPECT_EQ(ARKTS_GetValueType(env, obj), N_OBJECT);
    auto keyA = ARKTS_CreateUtf8(env, "a", 1);
    EXPECT_FALSE(ARKTS_HasOwnProperty(env, obj, keyA));
    auto valueA = ARKTS_GetProperty(env, obj, keyA);
    EXPECT_TRUE(ARKTS_IsUndefined(valueA));
    valueA = ARKTS_CreateBool(true);
    ARKTS_SetProperty(env, obj, keyA, valueA);
    EXPECT_TRUE(ARKTS_HasOwnProperty(env, obj, keyA));
    auto receivedA = ARKTS_GetProperty(env, obj, keyA);
    EXPECT_TRUE(ARKTS_IsBool(receivedA));
    EXPECT_TRUE(ARKTS_GetValueBool(receivedA));
    auto keys = ARKTS_EnumOwnProperties(env, obj);
    EXPECT_TRUE(ARKTS_IsArray(env, keys));
    EXPECT_EQ(ARKTS_GetArrayLength(env, keys), 1);
    auto key = ARKTS_GetElement(env, keys, 0);
    EXPECT_TRUE(ARKTS_IsString(env, key));
    EXPECT_TRUE(ARKTS_StrictEqual(env, keyA, key));
    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestKeyable()
{
    auto env = MockContext::GetInstance()->GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto obj = ARKTS_CreateObject(env);
    ARKTS_Value allowedKeys[] {
        ARKTS_CreateUtf8(env, "a", 1),
        ARKTS_CreateI32(12),
        ARKTS_CreateSymbol(env, "a", 1)
    };
    auto value = ARKTS_CreateBool(true);
    for (auto key : allowedKeys) {
        ARKTS_SetProperty(env, obj, key, value);
        auto received = ARKTS_GetProperty(env, obj, key);
        EXPECT_TRUE(ARKTS_IsBool(received));
        EXPECT_TRUE(ARKTS_GetValueBool(received));
    }
    ARKTS_CloseScope(env, scope);
}

struct PropCase {
    char k;
    bool writable;
    bool enumerable;
    bool configurable;

    static PropCase cases[];
};

PropCase PropCase::cases[] {
    {'a', false, false, false},
    {'b', true, false, false},
    {'c', false, true, false},
    {'d', false, false, true},
    {'e', true, true, false},
    {'f', true, false, true},
    {'g', false, true, true},
    {'h', true, true, true}
};

void ArkInteropTest::TestDefineProperty()
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto obj = ARKTS_CreateObject(env);

    constexpr auto totalCases = std::size(PropCase::cases);
    auto valueF = ARKTS_CreateBool(false);
    ARKTS_Value keys[totalCases];
    for (auto i = 0; i < totalCases; i++) {
        keys[i] = ARKTS_CreateUtf8(env, &PropCase::cases[i].k, 1);
        ARKTS_DefineOwnProperty(env, obj, keys[i], valueF, static_cast<ARKTS_PropertyFlag>(
            (PropCase::cases[i].writable ? N_WRITABLE : 0) |
            (PropCase::cases[i].enumerable ? N_ENUMERABLE : 0) |
            (PropCase::cases[i].configurable ? N_CONFIGURABLE : 0)
        ));
    }
    constexpr int expectKeys = 4;
    auto jsKeys = ARKTS_EnumOwnProperties(env, obj);
    EXPECT_TRUE(ARKTS_IsArray(env, jsKeys));
    EXPECT_EQ(ARKTS_GetArrayLength(env, jsKeys), expectKeys);
    for (auto i = 0; i < expectKeys; i++) {
        auto jsKey = ARKTS_GetElement(env, jsKeys, i);
        EXPECT_TRUE(ARKTS_IsString(env, jsKey));
        char buffer = 0;
        ARKTS_GetValueUtf8(env, jsKey, 1, &buffer);
        EXPECT_TRUE(buffer == 'c' || buffer == 'e' || buffer == 'g' || buffer == 'h');
    }

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestDefinePropertyWritable()
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto obj = ARKTS_CreateObject(env);

    constexpr auto totalCases = std::size(PropCase::cases);
    auto valueT = ARKTS_CreateBool(true);
    auto valueF = ARKTS_CreateBool(false);
    ARKTS_Value keys[totalCases];
    for (auto i = 0; i < totalCases; i++) {
        keys[i] = ARKTS_CreateUtf8(env, &PropCase::cases[i].k, 1);
        ARKTS_DefineOwnProperty(env, obj, keys[i], valueF, static_cast<ARKTS_PropertyFlag>(
            (PropCase::cases[i].writable ? N_WRITABLE : 0) |
            (PropCase::cases[i].enumerable ? N_ENUMERABLE : 0) |
            (PropCase::cases[i].configurable ? N_CONFIGURABLE : 0)
        ));
    }

    for (auto i = 0; i < totalCases; i++) { // writable
        if (PropCase::cases[i].writable && PropCase::cases[i].configurable) {
            ARKTS_SetProperty(env, obj, keys[i], valueT);
            auto receivedJS = ARKTS_GetProperty(env, obj, keys[i]);
            auto recievedN = ARKTS_GetValueBool(receivedJS);
            EXPECT_TRUE(recievedN);
        }
    }

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestDefinePropertyConfigurable()
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto obj = ARKTS_CreateObject(env);

    constexpr auto totalCases = std::size(PropCase::cases);
    auto valueT = ARKTS_CreateBool(true);
    auto valueF = ARKTS_CreateBool(false);
    ARKTS_Value keys[totalCases];
    for (auto i = 0; i < totalCases; i++) {
        keys[i] = ARKTS_CreateUtf8(env, &PropCase::cases[i].k, 1);
        ARKTS_DefineOwnProperty(env, obj, keys[i], valueF, static_cast<ARKTS_PropertyFlag>(
            (PropCase::cases[i].writable ? N_WRITABLE : 0) |
            (PropCase::cases[i].enumerable ? N_ENUMERABLE : 0) |
            (PropCase::cases[i].configurable ? N_CONFIGURABLE : 0)
        ));
    }

    for (auto i = 0;i < totalCases; ++i) { // configurable
        if (PropCase::cases[i].configurable) {
            ARKTS_DefineOwnProperty(env, obj, keys[i], valueT,
                static_cast<ARKTS_PropertyFlag>(N_WRITABLE | N_ENUMERABLE | N_CONFIGURABLE));
            auto received = ARKTS_GetProperty(env, obj, keys[i]);
            EXPECT_TRUE(ARKTS_IsBool(received));
            EXPECT_TRUE(ARKTS_GetValueBool(received));
        }
    }
    auto jsKeys = ARKTS_EnumOwnProperties(env, obj);
    constexpr int expectLength = 6;
    EXPECT_EQ(ARKTS_GetArrayLength(env, jsKeys), expectLength);

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestDefinePropertyConfigurableV2()
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto obj = ARKTS_CreateObject(env);

    constexpr auto totalCases = std::size(PropCase::cases);
    auto valueT = ARKTS_CreateBool(true);
    auto valueF = ARKTS_CreateBool(false);
    ARKTS_Value keys[totalCases];
    for (auto i = 0; i < totalCases; i++) {
        keys[i] = ARKTS_CreateUtf8(env, &PropCase::cases[i].k, 1);
        ARKTS_DefineOwnProperty(env, obj, keys[i], valueF, static_cast<ARKTS_PropertyFlag>(
            (PropCase::cases[i].writable ? N_WRITABLE : 0) |
            (PropCase::cases[i].enumerable ? N_ENUMERABLE : 0) |
            (PropCase::cases[i].configurable ? N_CONFIGURABLE : 0)
        ));
    }

    for (auto i = 0;i < totalCases; ++i) { // configurable
        auto result = ARKTS_DefineOwnPropertyV2(env, obj, keys[i], valueT,
            static_cast<ARKTS_PropertyFlag>(N_WRITABLE | N_ENUMERABLE | N_CONFIGURABLE));
        if (PropCase::cases[i].configurable) {
            EXPECT_TRUE(result);
        } else {
            EXPECT_TRUE(local.HasAndClearJSError());
        }
    }
    auto jsKeys = ARKTS_EnumOwnProperties(env, obj);
    constexpr int expectLength = 6;
    EXPECT_EQ(ARKTS_GetArrayLength(env, jsKeys), expectLength);

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestInstanceOf()
{
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    auto global = ARKTS_GetGlobalConstant(env);
    char clError[] = "Error";
    auto jError = ARKTS_CreateUtf8(env, clError, sizeof(clError) - 1);
    auto errorCls = ARKTS_GetProperty(env, global, jError);
    auto errorObJ = ARKTS_New(env, errorCls, 0, nullptr);
    auto isError = ARKTS_InstanceOf(env, errorObJ, errorCls);
    EXPECT_TRUE(isError);
    auto jObj = ARKTS_CreateObject(env);
    EXPECT_FALSE(ARKTS_InstanceOf(env, jObj, errorCls));
}

void ArkInteropTest::TestArray()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);

    char c = 'c';
    ARKTS_Value origin[] {
        ARKTS_CreateBool(true),
        ARKTS_CreateF64(12.02),
        ARKTS_CreateUtf8(env, &c, 1),
        ARKTS_CreateObject(env),
    };
    constexpr auto arrSize = std::size(origin);
    auto arr = ARKTS_CreateArray(env, arrSize);
    EXPECT_TRUE(ARKTS_IsArray(env, arr));
    EXPECT_EQ(ARKTS_GetValueType(env, arr), N_OBJECT);
    EXPECT_EQ(ARKTS_GetArrayLength(env, arr), arrSize);
    for (auto i = 0; i < arrSize; i++) {
        auto initialValue = ARKTS_GetElement(env, arr, i);
        EXPECT_TRUE(ARKTS_IsUndefined(initialValue));
        ARKTS_SetElement(env, arr, i, origin[i]);
        auto receivedValue = ARKTS_GetElement(env, arr, i);
        EXPECT_TRUE(ARKTS_StrictEqual(env, origin[i], receivedValue));
    }
}

void ArkInteropTest::TestBigintInt64()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    {
        int64_t origins[] {
            0x123'4567'8945,
            -0x123'4567'8945,
            0x7FFF'FFFF'FFFF'FFFF,
            -0x7FFF'FFFF'FFFF'FFFF,
        };
        constexpr auto arrSize = std::size(origins);
        ARKTS_Value values[arrSize];
        for (auto i = 0; i < arrSize; i++) {
            values[i] = ARKTS_CreateBigInt(env, origins[i]);
            EXPECT_TRUE(ARKTS_IsBigInt(env, values[i]));
            EXPECT_EQ(ARKTS_GetValueType(env, values[i]), N_BIGINT);
            ARKTS_BigIntGetByteSize(env, values[i]);
        }
    }
}

void ArkInteropTest::TestBigint16Bytes()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    uint8_t origin[] {
        0, 10, 20, 30, 40, 50, 60, 70,
        80, 90, 70, 50, 20, 30, 40, 50
    };
    bool isNegative = false;
    auto value = ARKTS_CreateBigIntWithBytes(env, isNegative, std::size(origin), origin);
    EXPECT_TRUE(ARKTS_IsBigInt(env, value));
    EXPECT_EQ(ARKTS_GetValueType(env, value), N_BIGINT);
    EXPECT_EQ(ARKTS_BigIntGetByteSize(env, value), std::size(origin));
    uint8_t received[std::size(origin)];
    ARKTS_BigIntReadBytes(env, value, &isNegative, std::size(origin), received);
    EXPECT_FALSE(isNegative);
    for (auto i = 0; i < std::size(origin); i++) {
        EXPECT_EQ(origin[i], received[i]);
    }
}

void ArkInteropTest::TestBigint28Bytes()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    uint8_t origin[] {
        0, 10, 20, 30, 40, 50, 60, 70,
        80, 90, 70, 50, 20, 30, 40, 50,
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 20, 30,
    };
    bool isNegative = false;
    constexpr int expectSize = 32;
    // bigint words are 8 bytes aligned, received 32 bytes, and lower 4 bytes would be filled with 0.
    auto value = ARKTS_CreateBigIntWithBytes(env, isNegative, std::size(origin), origin);
    EXPECT_TRUE(ARKTS_IsBigInt(env, value));
    EXPECT_EQ(ARKTS_GetValueType(env, value), N_BIGINT);
    EXPECT_EQ(ARKTS_BigIntGetByteSize(env, value), expectSize);
    uint8_t received[expectSize];
    ARKTS_BigIntReadBytes(env, value, &isNegative, expectSize, received);
    EXPECT_FALSE(isNegative);
    for (auto i = 0; i < std::size(origin); i++) {
        EXPECT_EQ(origin[i], received[i + expectSize - std::size(origin)]);
    }
}

void ArkInteropTest::TestBigint32Bytes()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    uint8_t origin[] {
        0, 10, 20, 30, 40, 50, 60, 70,
        80, 90, 70, 50, 20, 30, 40, 50,
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 20, 30, 40, 50, 60, 70,
    };
    bool isNegative = false;
    auto value = ARKTS_CreateBigIntWithBytes(env, isNegative, std::size(origin), origin);
    EXPECT_TRUE(ARKTS_IsBigInt(env, value));
    EXPECT_EQ(ARKTS_GetValueType(env, value), N_BIGINT);
    EXPECT_EQ(ARKTS_BigIntGetByteSize(env, value), std::size(origin));
    uint8_t received[std::size(origin)];
    ARKTS_BigIntReadBytes(env, value, &isNegative, std::size(origin), received);
    EXPECT_FALSE(isNegative);
    for (auto i = 0; i < std::size(origin); i++) {
        EXPECT_EQ(origin[i], received[i]);
    }
}

void ArkInteropTest::TestSymbol()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);

    const char originDesc[] = "TestSymbol";

    auto symbol = ARKTS_CreateSymbol(env, originDesc, sizeof(originDesc) - 1);
    EXPECT_TRUE(ARKTS_IsSymbol(env, symbol));
    EXPECT_EQ(ARKTS_GetValueType(env, symbol), N_SYMBOL);

    auto retDesc = ARKTS_GetSymbolDesc(env, symbol);
    EXPECT_EQ(std::string(retDesc), originDesc);
    ARKTS_FreeCString(retDesc);

    auto obj = ARKTS_CreateObject(env);
    ARKTS_SetProperty(env, obj, symbol, ARKTS_CreateBool(true));
    auto value = ARKTS_GetProperty(env, obj, symbol);
    EXPECT_TRUE(ARKTS_IsBool(value));
    EXPECT_TRUE(ARKTS_GetValueBool(value));

    auto symbol1 = ARKTS_CreateSymbol(env, originDesc, sizeof(originDesc) - 1);
    ARKTS_SetProperty(env, obj, symbol1, ARKTS_CreateBool(false));
    auto value1 = ARKTS_GetProperty(env, obj, symbol1);
    EXPECT_TRUE(ARKTS_IsBool(value1));
    EXPECT_FALSE(ARKTS_GetValueBool(value1));

    value = ARKTS_GetProperty(env, obj, symbol);
    EXPECT_TRUE(ARKTS_IsBool(value));
    EXPECT_TRUE(ARKTS_GetValueBool(value));
}

void ArkInteropTest::TestFunction()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    
    bool called = false;
    auto id = MockContext::GetInstance()->StoreFunc([&called, env](ARKTS_CallInfo info)->ARKTS_Value {
        called = true;
        EXPECT_EQ(ARKTS_GetArgCount(info), 0);
        EXPECT_TRUE(ARKTS_IsUndefined(ARKTS_GetThisArg(info)));
        return ARKTS_CreateUndefined();
    });
    auto func = ARKTS_CreateFunc(env, id);
    EXPECT_TRUE(ARKTS_IsCallable(env, func));
    EXPECT_EQ(ARKTS_GetValueType(env, func), N_FUNCTION);
    ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);
    EXPECT_TRUE(called);
    called = false;
    ARKTS_Value args[] {
        ARKTS_CreateNull(),
        ARKTS_CreateBool(true),
        ARKTS_CreateF64(45.1),
        ARKTS_CreateObject(env)
    };
    static constexpr auto totalArgs = std::size(args);
    id = MockContext::GetInstance()->StoreFunc([env, &called, origin = args](ARKTS_CallInfo info)->ARKTS_Value {
        called = true;
        auto self = ARKTS_GetThisArg(info);
        EXPECT_TRUE(ARKTS_IsObject(env, self));
        EXPECT_EQ(totalArgs, ARKTS_GetArgCount(info));
        for (auto i = 0; i < totalArgs; i++) {
            EXPECT_TRUE(ARKTS_StrictEqual(env, ARKTS_GetArg(info, i), origin[i]));
        }
        return ARKTS_CreateBool(true);
    });
    func = ARKTS_CreateFunc(env, id);
    ARKTS_Call(env, func, ARKTS_CreateObject(env), std::size(args), args);
    EXPECT_TRUE(called);
}

void ArkInteropTest::TestClass()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    auto id = MockContext::GetInstance()->StoreFunc([](ARKTS_CallInfo info)->ARKTS_Value {
        return ARKTS_GetThisArg(info);
    });
    auto clazz = ARKTS_CreateClass(env, id, ARKTS_CreateUndefined());
    EXPECT_TRUE(ARKTS_IsClass(env, clazz));
    EXPECT_EQ(ARKTS_GetValueType(env, clazz), N_FUNCTION);
    auto obj = ARKTS_New(env, clazz, 0, nullptr);
    EXPECT_TRUE(ARKTS_IsObject(env, obj));
    EXPECT_TRUE(ARKTS_InstanceOf(env, obj, clazz));
}

void ArkInteropTest::TestPromise()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    auto scope = ARKTS_OpenScope(env);

    auto promiseCap = ARKTS_CreatePromiseCapability(env);
    auto promise = ARKTS_GetPromiseFromCapability(env, promiseCap);
    EXPECT_TRUE(ARKTS_IsPromise(env, promise));
    EXPECT_EQ(ARKTS_GetValueType(env, promise), N_OBJECT);
    
    bool resolved = false;
    auto id = MockContext::GetInstance()->StoreFunc([&resolved, env](ARKTS_CallInfo info)->ARKTS_Value {
        resolved = true;
        EXPECT_EQ(ARKTS_GetArgCount(info), 1);
        auto arg = ARKTS_GetArg(info, 0);
        EXPECT_TRUE(ARKTS_IsUndefined(arg));
        return ARKTS_CreateBool(true);
    });
    auto onResolved = ARKTS_CreateFunc(env, id);
    ARKTS_PromiseThen(env, promise, onResolved, ARKTS_CreateUndefined());
    ARKTS_PromiseCapabilityResolve(env, promiseCap, ARKTS_CreateUndefined());
    EXPECT_TRUE(resolved);

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestArrayBuffer()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    auto scope = ARKTS_OpenScope(env);

    uint8_t origin[] {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        'a', 'b', 'c', 'd', 'e', 'f', 'g'
    };
    auto arrSize = std::size(origin);
    auto buffer = ARKTS_CreateArrayBuffer(env, arrSize);
    EXPECT_TRUE(ARKTS_IsArrayBuffer(env, buffer));
    EXPECT_EQ(ARKTS_GetValueType(env, buffer), N_OBJECT);
    EXPECT_EQ(ARKTS_GetArrayBufferLength(env, buffer), arrSize);
    auto dst = reinterpret_cast<uint8_t*>(ARKTS_GetArrayBufferRawPtr(env, buffer));
    for (auto i = 0; i < arrSize; i++) {
        dst[i] = origin[i];
    }
    uint8_t received[std::size(origin)];
    auto endpoint = ARKTS_ArrayBufferReadBytes(env, buffer, received, arrSize);
    EXPECT_EQ(endpoint, arrSize);

    for (auto i = 0; i < arrSize; i++) {
        EXPECT_EQ(origin[i], received[i]);
    }

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestUpdateStackInfo()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    auto scope = ARKTS_OpenScope(env);
    unsigned long long vmAddress = reinterpret_cast<unsigned long long>(env);
    StackInfo stackInfo;
    stackInfo.stackLimit = 0x5be506e000;
    stackInfo.lastLeaveFrame = 0;
    ARKTS_UpdateStackInfo(vmAddress, &stackInfo, SWITCH_TO_SUB_STACK_INFO);
    ARKTS_UpdateStackInfo(vmAddress, &stackInfo, SWITCH_TO_MAIN_STACK_INFO);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, Types)
{
    RunLocalTest();
}

TEST_F(ArkInteropTest, PromiseThen)
{
    MockContext local(ARKTS_CreateEngineWithNewThread());
    auto env = local.GetEnv();
    EXPECT_TRUE(env);

    std::condition_variable cv;
    bool isComplete = false;
    auto funcId = local.StoreAsyncFunc([&local, &cv, &isComplete, env] {
        auto scope = ARKTS_OpenScope(env);
        auto promiseCap = ARKTS_CreatePromiseCapability(env);
        auto promise = ARKTS_GetPromiseFromCapability(env, promiseCap);
        auto funcId = local.StoreFunc([&cv, &isComplete](ARKTS_CallInfo info) {
            isComplete = true;
            cv.notify_one();
            return ARKTS_CreateUndefined();
        });
        auto func = ARKTS_CreateFunc(env, funcId);
        ARKTS_PromiseThen(env, promise, func, ARKTS_CreateUndefined());
        ARKTS_PromiseCapabilityResolve(env, promiseCap, ARKTS_CreateUndefined());
        ARKTS_CloseScope(env, scope);
    });
    ARKTS_CreateAsyncTask(env, funcId);
    std::mutex mutex;
    std::unique_lock lock(mutex);

    constexpr int checkDuration = 100;
    int waitTimes = 50; // set 100ms timeout
    while (!isComplete && waitTimes--) {
        cv.wait_for(lock, std::chrono::milliseconds(checkDuration));
    }
    // EXPECT no core dump, no timeout
    EXPECT_TRUE(waitTimes > 0);
}

TEST_F(ArkInteropTest, CycleFreeFunc)
{
    CycleFreeContext local;
    auto env = local.GetEnv();
    auto funcCount = local.GetCycleFreeFuncCount();
    auto scope = ARKTS_OpenScope(env);
    auto isCalled = false;
    auto id = local.StoreCycleFreeFunc([&isCalled](ARKTS_CallInfo callInfo) {
        isCalled = true;
        return ARKTS_CreateUndefined();
    });
    EXPECT_EQ(local.GetCycleFreeFuncCount(), funcCount + 1);
    auto func = ARKTS_CreateCycleFreeFunc(local.GetEnv(), id);
    EXPECT_TRUE(ARKTS_IsCallable(env, func));
    ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);
    EXPECT_TRUE(isCalled);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CycleFreeExtern)
{
    CycleFreeContext local;
    auto env = local.GetEnv();
    auto funcCount = local.GetCycleFreeFuncCount();
    {
        auto scope = ARKTS_OpenScope(env);
        auto id = local.StoreCycleFreeFunc(nullptr);
        EXPECT_EQ(local.GetCycleFreeFuncCount(), funcCount + 1);
        auto object = ARKTS_CreateCycleFreeExtern(local.GetEnv(), id);
        EXPECT_TRUE(ARKTS_IsExternal(env, object));
        auto resid = ARKTS_GetExternalData(env, object);
        EXPECT_EQ(resid, id);

        ARKTS_CloseScope(env, scope);
    }
}

class GlobalWeakTest {
public:
    GlobalWeakTest(): local(ARKTS_CreateEngineWithNewThread()), status(CREATING)
    {
    }
    ~GlobalWeakTest()
    {
        ReleaseGlobals();
    }

    void Start()
    {
        ScheduleNext();
    }

    void WaitForComplete()
    {
        std::mutex mutex;
        std::unique_lock lock(mutex);
        while (status != COMPLETE) {
            cv.wait(lock);
        }
    }

    bool IsComplete() const
    {
        return status == COMPLETE;
    }

    bool IsAllDisposed() const
    {
        for (auto one : globals) {
            if (ARKTS_GlobalIsAlive(local.GetEnv(), one)) {
                return false;
            }
        }
        return true;
    }

private:
    static constexpr int objectCnt = 1000;

    void CreateWeakObjects()
    {
        auto env = local.GetEnv();
        auto scope = ARKTS_OpenScope(env);
        for (auto i = 0; i < objectCnt; i++) {
            auto object = ARKTS_CreateObject(env);
            auto global = ARKTS_CreateGlobal(env, object);
            ARKTS_GlobalSetWeak(env, global);
            globals.push_back(global);
        }
        ARKTS_CloseScope(env, scope);
        panda::JSNApi::TriggerGC(P_CAST(env, EcmaVM*), panda::JSNApi::TRIGGER_GC_TYPE::FULL_GC);
    }

    void DoAssertion()
    {
        for (auto one : globals) {
            EXPECT_TRUE(!ARKTS_GlobalIsAlive(local.GetEnv(), one));
        }
    }

    void ReleaseGlobals()
    {
        for (auto one : globals) {
            ARKTS_DisposeGlobalSync(local.GetEnv(), one);
        }
    }

    enum Status {
        CREATING,
        ASSERTION,
        DISPOSE,
        COMPLETE
    };

    void ScheduleNext()
    {
        auto id = local.StoreAsyncFunc([this] {
            DoNext();
        });
        ARKTS_CreateAsyncTask(local.GetEnv(), id);
    }

    void DoNext()
    {
        switch (status) {
            case CREATING:
                CreateWeakObjects();
                status = ASSERTION;
                break;
            case ASSERTION:
                DoAssertion();
                status = DISPOSE;
                break;
            case DISPOSE:
                status = COMPLETE;
                cv.notify_all();
                return;
            default: ;
        }
        ScheduleNext();
    }

    CycleFreeContext local;
    Status status;
    std::condition_variable cv;
    std::vector<ARKTS_Global> globals;
};

TEST_F(ArkInteropTest, GlobalWeak)
{
    GlobalWeakTest weakTest;
    weakTest.Start();
    weakTest.WaitForComplete();
    EXPECT_TRUE(weakTest.IsComplete());
}

TEST_F(ArkInteropTest, GlobalToValue)
{
    MockContext local;
    auto env = local.GetEnv();
    {
        auto scope = ARKTS_OpenScope(env);
        auto object = ARKTS_CreateObject(env);
        auto global = ARKTS_CreateGlobal(env, object);
        auto value = ARKTS_GlobalToValue(env, global);
        auto received = ARKTS_GlobalFromValue(env, value);
        EXPECT_EQ(received, global);
        ARKTS_DisposeGlobalSync(env, global);
        ARKTS_CloseScope(env, scope);
    }
}

HWTEST_F(ArkInteropTest, ArkTSInteropNapiCreateEngineNew, TestSize.Level1)
{
    MockContext local(ARKTS_CreateEngineWithNewThread());

    auto curTid = ARKTS_GetPosixThreadId();
    auto engineTid = ARKTS_GetThreadIdOfEngine(local.GetEngine());

    EXPECT_NE(curTid, engineTid);

    auto env = local.GetEnv();
    EXPECT_TRUE(env);
    bool isComplete = false;
    std::condition_variable cv;
    auto funcId = local.StoreAsyncFunc([&isComplete, &cv] {
        ArkInteropTest::RunLocalTest();
        isComplete = true;
        cv.notify_one();
    });
    ARKTS_CreateAsyncTask(env, funcId);
    std::mutex mutex;
    std::unique_lock lock(mutex);
    constexpr int checkDuration = 10;
    int waitTimes = 100; // set 1000ms timeout
    while (!isComplete && waitTimes--) {
        cv.wait_for(lock, std::chrono::milliseconds(checkDuration));
    }
    EXPECT_TRUE(waitTimes > 0);
}

TEST_F(ArkInteropTest, ScopeMT)
{
    constexpr int threadCount = 1000;
    std::thread threads[threadCount];
    for (int i = 0; i < threadCount; i++) {
        threads[i] = std::thread([] {
            panda::RuntimeOption options;
            auto vm = panda::JSNApi::CreateJSVM(options);
            EXPECT_TRUE(vm);
            auto env = P_CAST(vm, ARKTS_Env);
            auto scope = ARKTS_OpenScope(env);
            EXPECT_TRUE(scope);
            ARKTS_CloseScope(env, scope);
            panda::JSNApi::DestroyJSVM(vm);
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
}

TEST_F(ArkInteropTest, GlobalRelease)
{
    MockContext local(ARKTS_CreateEngineWithNewThread());
    auto env = local.GetEnv();

    bool isComplete = false;
    int loops = 10;
    std::condition_variable cv;
    std::function <void()> callback;
    callback = [&isComplete, &cv, &loops, env, &callback] {
        if (loops == 0) {
            isComplete = true;
            cv.notify_one();
            return;
        }
        auto scope = ARKTS_OpenScope(env);
        auto totalRepeat = 200000;
        for (int i = 0;i < totalRepeat; ++i) {
            auto object = ARKTS_CreateObject(env);
            auto global = ARKTS_CreateGlobal(env, object);
            ARKTS_DisposeGlobal(env, global);
        }
        ARKTS_CloseScope(env, scope);
        --loops;
        auto funcId = MockContext::GetInstance()->StoreAsyncFunc(callback);
        ARKTS_CreateAsyncTask(env, funcId);
    };
    auto funcId = local.StoreAsyncFunc(callback);
    ARKTS_CreateAsyncTask(env, funcId);
    std::mutex mutex;
    std::unique_lock lock(mutex);
    int waitTimes = 200;
    int msEachTime = 100;
    while (!isComplete && waitTimes--) {
        cv.wait_for(lock, std::chrono::milliseconds(msEachTime));
    }
    EXPECT_TRUE(isComplete);
}

TEST_F(ArkInteropTest, GlobalReleaseSync)
{
    MockContext local;
    auto env = local.GetEnv();
    int loops = 10;
    auto totalRepeat = 200000;
    for (int i = 0;i < loops; ++i) {
        auto scope = ARKTS_OpenScope(env);
        for (int j = 0;j < totalRepeat; ++j) {
            auto object = ARKTS_CreateObject(env);
            auto global = ARKTS_CreateGlobal(env, object);
            ARKTS_DisposeGlobalSync(env, global);
        }
        EXPECT_FALSE(panda::JSNApi::HasPendingException(P_CAST(env, EcmaVM*)));
        ARKTS_CloseScope(env, scope);
    }
}

TEST_F(ArkInteropTest, PromiseRelease)
{
    MockContext local;
    auto env = local.GetEnv();
    int loops = 20;
    auto totalRepeat = 20000;
    auto undefined = ARKTS_CreateUndefined();
    for (int i = 0;i < loops; ++i) {
        auto scope = ARKTS_OpenScope(env);
        for (int j = 0;j < totalRepeat; ++j) {
            auto promiseCap = ARKTS_CreatePromiseCapability(env);
            ARKTS_PromiseCapabilityResolve(env, promiseCap, undefined);
        }
        EXPECT_FALSE(panda::JSNApi::HasPendingException(P_CAST(env, EcmaVM*)));
        ARKTS_CloseScope(env, scope);
    }
}

TEST_F(ArkInteropTest, CheckFreeContext)
{
    bool isDisposed = false;
    {
        MockContext local(ARKTS_CreateEngineWithNewThread());
        local.SetFinalizerCallback([&isDisposed](ARKTS_Env) {
            isDisposed = true;
        });
    }
    EXPECT_TRUE(isDisposed);
}

TEST_F(ArkInteropTest, DebugMode)
{
    MockContext local;
    auto env = local.GetEnv();
    auto id = local.StoreFunc([](ARKTS_CallInfo) {
        return ARKTS_CreateUndefined();
    });
    auto func = ARKTS_CreateFunc(env, id);
    local.SetNativeCallingCallback([&local, id](const void* addr) {
        EXPECT_EQ(addr, local.GetFuncAddr(id));
    });
    ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);
}

TEST_F(ArkInteropTest, SetCJModuleCallbackWithNull)
{
    ARKTS_SetCJModuleCallback(nullptr);
}

TEST_F(ArkInteropTest, CJLambdaInvoker)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    bool invoked = false;
    auto lambdaId = local.StoreFunc([&invoked](ARKTS_CallInfo info) -> ARKTS_Value {
        invoked = true;
        return ARKTS_CreateBool(true);
    });
    
    auto func = ARKTS_CreateFunc(env, lambdaId);
    ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);
    EXPECT_TRUE(invoked);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CJLambdaInvokerWithoutCallback)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    auto func = ARKTS_CreateFunc(env, 12345);
    ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CJLambdaDeleter)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    auto lambdaId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_CreateUndefined();
    });
    
    auto func = ARKTS_CreateFunc(env, lambdaId);
    ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CJAsyncCallback)
{
    auto engine = ARKTS_CreateEngineWithNewThread();
    ASSERT_TRUE(engine);
    {
        MockContext local(engine, false);
        auto env = local.GetEnv();
        auto scope = ARKTS_OpenScope(env);

        bool invoked = false;
        std::condition_variable cv;
        auto asyncId = local.StoreAsyncFunc([&invoked, &cv] {
            invoked = true;
            cv.notify_one();
        });

        ARKTS_CreateAsyncTask(env, asyncId);

        std::mutex mutex;
        std::unique_lock lock(mutex);
        constexpr int checkDuration = 100;
        int waitTimes = 50;
        while (!invoked && waitTimes--) {
            cv.wait_for(lock, std::chrono::milliseconds(checkDuration));
        }
        EXPECT_TRUE(invoked);

        ARKTS_CloseScope(env, scope);
    }
    std::thread destroyer([engine] { ARKTS_DestroyEngine(engine); });
    destroyer.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(ArkInteropTest, GetNAPIEnv)
{
    MockContext local;
    auto engine = local.GetEngine();
    auto napiEnv = ARKTS_GetNAPIEnv(engine);
    EXPECT_TRUE(napiEnv);
}

TEST_F(ArkInteropTest, GetNAPIEnvWithNullEngine)
{
    ErrorCaptureContext local;
    auto napiEnv = ARKTS_GetNAPIEnv(nullptr);
    EXPECT_FALSE(napiEnv);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetContext)
{
    MockContext local;
    auto engine = local.GetEngine();
    auto context = ARKTS_GetContext(engine);
    EXPECT_TRUE(context);
}

TEST_F(ArkInteropTest, GetContextWithNullEngine)
{
    ErrorCaptureContext local;
    auto context = ARKTS_GetContext(nullptr);
    EXPECT_FALSE(context);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetThreadIdOfEngine)
{
    MockContext local;
    auto engine = local.GetEngine();
    auto threadId = ARKTS_GetThreadIdOfEngine(engine);
    EXPECT_GT(threadId, 0);
}

TEST_F(ArkInteropTest, GetThreadIdOfEngineWithNullEngine)
{
    ErrorCaptureContext local;
    auto threadId = ARKTS_GetThreadIdOfEngine(nullptr);
    EXPECT_EQ(threadId, 0);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, CreateEngine)
{
    auto engine = ARKTS_CreateEngineWithNewThread();
    ASSERT_TRUE(engine);
    std::thread destroyer([engine] { ARKTS_DestroyEngine(engine); });
    destroyer.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(ArkInteropTest, DestroyEngineWithNullEngine)
{
    ErrorCaptureContext local;
    ARKTS_DestroyEngine(nullptr);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, LoadEntryFromAbc)
{
    MockContext local;
    auto engine = local.GetEngine();
    
    const char* testFile = "/data/test_module.abc";
    const char* entryPoint = "test_entry";
    
    bool result = ARKTS_LoadEntryFromAbc(engine, testFile, entryPoint, false);
    EXPECT_FALSE(result);
}

TEST_F(ArkInteropTest, ImportFromEntry)
{
    MockContext local;
    auto engine = local.GetEngine();

    auto value = ARKTS_ImportFromEntry(engine, "test_entry", "test_import");
    EXPECT_EQ(value.value, 0ULL);
}

TEST_F(ArkInteropTest, RequireNativeModule)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    const char* target = "test_module";
    
    (void)ARKTS_Require(env, target, true, false, nullptr);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, RequireNativeModuleWithNullEnv)
{
    ErrorCaptureContext local;
    const char* target = "test_module";
    
    (void)ARKTS_Require(nullptr, target, true, false, nullptr);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, RequireNativeModuleWithRelativePath)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    const char* target = "test_module";
    const char* relativePath = "./test_module";
    
    (void)ARKTS_Require(env, target, true, false, relativePath);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, RequireAppModule)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    const char* target = "test_module";
    
    (void)ARKTS_Require(env, target, true, true, nullptr);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, RequireJsModule)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    const char* target = "test_module";
    
    (void)ARKTS_Require(env, target, false, false, nullptr);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, LoadModuleByNapiEnv)
{
    MockContext local;
    auto engine = local.GetEngine();
    void* napiEnv = ARKTS_GetNAPIEnv(engine);
    ASSERT_NE(napiEnv, nullptr);
    // nullptr dllName: early return in ARKTS_LoadModuleByNapiEnv (avoids CJ loader crash in host UT).
    EXPECT_EQ(ARKTS_LoadModuleByNapiEnv(napiEnv, nullptr), nullptr);
}

TEST_F(ArkInteropTest, LoadModuleByNapiEnvWithNullEnv)
{
    const char* dllName = "test_module";
    
    auto result = ARKTS_LoadModuleByNapiEnv(nullptr, dllName);
    EXPECT_FALSE(result);
}

TEST_F(ArkInteropTest, LoadModuleByNapiEnvWithValidParams)
{
    ModuleRegisteredContext local;
    auto engine = local.GetEngine();
    void* napiEnv = ARKTS_GetNAPIEnv(engine);
    ASSERT_NE(napiEnv, nullptr);

    auto result = ARKTS_LoadModuleByNapiEnv(napiEnv, "test_module");
    (void)result;
    local.HasAndClearNativeError();
    local.HasAndClearJSError();
}

TEST_F(ArkInteropTest, UpdateStackInfoWithNullAddress)
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    StackInfo stackInfo;
    stackInfo.stackLimit = 0x5be506e000;
    stackInfo.lastLeaveFrame = 0;
    ARKTS_UpdateStackInfo(0, &stackInfo, SWITCH_TO_SUB_STACK_INFO);
    EXPECT_TRUE(local.HasAndClearNativeError());
    ARKTS_UpdateStackInfo(0, &stackInfo, SWITCH_TO_MAIN_STACK_INFO);
    EXPECT_TRUE(local.HasAndClearNativeError());
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, UpdateStackInfoWithNullStackInfo)
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    unsigned long long vmAddress = reinterpret_cast<unsigned long long>(env);
    ARKTS_UpdateStackInfo(vmAddress, nullptr, SWITCH_TO_SUB_STACK_INFO);
    EXPECT_TRUE(local.HasAndClearNativeError());
    ARKTS_UpdateStackInfo(vmAddress, nullptr, SWITCH_TO_MAIN_STACK_INFO);
    EXPECT_TRUE(local.HasAndClearNativeError());
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateCycleFreeFunc)
{
    CycleFreeContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    bool called = false;
    auto id = local.StoreCycleFreeFunc([&called](ARKTS_CallInfo callInfo) {
        called = true;
        return ARKTS_CreateUndefined();
    });
    
    auto func = ARKTS_CreateCycleFreeFunc(env, id);
    EXPECT_TRUE(ARKTS_IsCallable(env, func));
    
    ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);
    EXPECT_TRUE(called);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateCycleFreeExtern)
{
    CycleFreeContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    auto id = local.StoreCycleFreeFunc([](ARKTS_CallInfo callInfo) {
        return ARKTS_CreateUndefined();
    });
    
    auto external = ARKTS_CreateCycleFreeExtern(env, id);
    EXPECT_TRUE(ARKTS_IsExternal(env, external));
    
    auto data = ARKTS_GetExternalData(env, external);
    EXPECT_EQ(data, id);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateGlobal)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    auto object = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, object);
    
    auto value = ARKTS_GlobalToValue(env, global);
    auto global2 = ARKTS_GlobalFromValue(env, value);
    
    EXPECT_EQ(global, global2);
    
    ARKTS_DisposeGlobal(env, global);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateGlobalWithWeak)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    auto object = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, object);
    
    ARKTS_GlobalSetWeak(env, global);
    
    auto isAlive = ARKTS_GlobalIsAlive(env, global);
    EXPECT_TRUE(isAlive);
    
    ARKTS_DisposeGlobal(env, global);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, DisposeGlobalSync)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    auto object = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, object);
    
    ARKTS_DisposeGlobalSync(env, global);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateEngineWithNewThread)
{
    auto engine = ARKTS_CreateEngineWithNewThread();
    ASSERT_TRUE(engine);
    auto env = ARKTS_GetContext(engine);
    EXPECT_TRUE(env);

    std::thread destroyer([engine] { ARKTS_DestroyEngine(engine); });
    destroyer.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(ArkInteropTest, CreateEngineWithNewThreadNull)
{
    auto engine = ARKTS_CreateEngineWithNewThread();
    if (!engine) {
        return;
    }
    std::thread destroyer([engine] { ARKTS_DestroyEngine(engine); });
    destroyer.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

TEST_F(ArkInteropTest, PromiseCapabilityReject)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    auto promiseCap = ARKTS_CreatePromiseCapability(env);
    auto promise = ARKTS_GetPromiseFromCapability(env, promiseCap);
    
    bool rejected = false;
    auto noopId = MockContext::GetInstance()->StoreFunc([](ARKTS_CallInfo info)->ARKTS_Value {
        (void)info;
        return ARKTS_CreateUndefined();
    });
    auto rejectId = MockContext::GetInstance()->StoreFunc([&rejected, env](ARKTS_CallInfo info)->ARKTS_Value {
        rejected = true;
        EXPECT_EQ(ARKTS_GetArgCount(info), 1);
        auto arg = ARKTS_GetArg(info, 0);
        EXPECT_TRUE(ARKTS_IsUndefined(arg));
        return ARKTS_CreateUndefined();
    });
    auto onFulfilled = ARKTS_CreateFunc(env, noopId);
    auto onRejected = ARKTS_CreateFunc(env, rejectId);
    ARKTS_PromiseThen(env, promise, onFulfilled, onRejected);
    ARKTS_PromiseCapabilityReject(env, promiseCap, ARKTS_CreateUndefined());
    EXPECT_TRUE(rejected);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ArrayBufferWithDeleter)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    uint8_t data[] = {1, 2, 3, 4, 5};
    auto buffer = ARKTS_CreateArrayBuffer(env, sizeof(data));
    auto ptr = reinterpret_cast<uint8_t*>(ARKTS_GetArrayBufferRawPtr(env, buffer));
    memcpy(ptr, data, sizeof(data));
    
    bool deleted = false;
    (void)local.StoreArrayBufferDeleter([&deleted](void* buffer) {
        deleted = true;
    });
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ExternalWithDeleter)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    int* data = new int(42);
    auto deleterId = local.StoreNativeData(data, [](void* ptr) {
        delete static_cast<int*>(ptr);
    });
    
    auto external = ARKTS_CreateExternal(env, deleterId);
    EXPECT_TRUE(ARKTS_IsExternal(env, external));
    
    auto retrievedData = ARKTS_GetExternalData(env, external);
    EXPECT_EQ(retrievedData, deleterId);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetPropertyNames)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    auto obj = ARKTS_CreateObject(env);
    auto key1 = ARKTS_CreateUtf8(env, "key1", 4);
    auto key2 = ARKTS_CreateUtf8(env, "key2", 4);
    
    ARKTS_SetProperty(env, obj, key1, ARKTS_CreateI32(1));
    ARKTS_SetProperty(env, obj, key2, ARKTS_CreateI32(2));
    
    auto keys = ARKTS_EnumOwnProperties(env, obj);
    EXPECT_TRUE(ARKTS_IsArray(env, keys));
    EXPECT_EQ(ARKTS_GetArrayLength(env, keys), 2);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, StrictEqual)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    auto num1 = ARKTS_CreateI32(42);
    auto num2 = ARKTS_CreateI32(42);
    auto num3 = ARKTS_CreateI32(43);
    
    EXPECT_TRUE(ARKTS_StrictEqual(env, num1, num2));
    EXPECT_FALSE(ARKTS_StrictEqual(env, num1, num3));
    
    auto str1 = ARKTS_CreateUtf8(env, "test", 4);
    auto str2 = ARKTS_CreateUtf8(env, "test", 4);
    auto str3 = ARKTS_CreateUtf8(env, "test2", 5);
    
    EXPECT_TRUE(ARKTS_StrictEqual(env, str1, str2));
    EXPECT_FALSE(ARKTS_StrictEqual(env, str1, str3));
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateInt32)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    int32_t values[] = {0, 1, -1, 2147483647, -2147483648};
    
    for (auto value : values) {
        auto jsValue = ARKTS_CreateI32(value);
        auto numValue = ARKTS_GetValueNumber(jsValue);
        EXPECT_EQ(static_cast<int32_t>(numValue), value);
    }
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetValueType)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    auto undefined = ARKTS_CreateUndefined();
    EXPECT_EQ(ARKTS_GetValueType(env, undefined), N_UNDEFINED);
    
    auto null = ARKTS_CreateNull();
    EXPECT_EQ(ARKTS_GetValueType(env, null), N_NULL);
    
    auto boolVal = ARKTS_CreateBool(true);
    EXPECT_EQ(ARKTS_GetValueType(env, boolVal), N_BOOL);
    
    auto numVal = ARKTS_CreateI32(42);
    EXPECT_EQ(ARKTS_GetValueType(env, numVal), N_NUMBER);
    
    auto strVal = ARKTS_CreateUtf8(env, "test", 4);
    EXPECT_EQ(ARKTS_GetValueType(env, strVal), N_STRING);
    
    auto objVal = ARKTS_CreateObject(env);
    EXPECT_EQ(ARKTS_GetValueType(env, objVal), N_OBJECT);
    
    auto arrVal = ARKTS_CreateArray(env, 0);
    EXPECT_EQ(ARKTS_GetValueType(env, arrVal), N_OBJECT);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CallWithArguments)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    bool called = false;
    auto id = local.StoreFunc([&called, env](ARKTS_CallInfo info) -> ARKTS_Value {
        called = true;
        EXPECT_EQ(ARKTS_GetArgCount(info), 3);
        
        auto arg0 = ARKTS_GetArg(info, 0);
        auto arg1 = ARKTS_GetArg(info, 1);
        auto arg2 = ARKTS_GetArg(info, 2);
        
        EXPECT_TRUE(ARKTS_IsNumber(arg0));
        EXPECT_TRUE(ARKTS_IsString(env, arg1));
        EXPECT_TRUE(ARKTS_IsBool(arg2));
        
        return ARKTS_CreateUndefined();
    });
    
    auto func = ARKTS_CreateFunc(env, id);
    
    ARKTS_Value args[] = {
        ARKTS_CreateI32(42),
        ARKTS_CreateUtf8(env, "test", 4),
        ARKTS_CreateBool(true)
    };
    
    ARKTS_Call(env, func, ARKTS_CreateUndefined(), 3, args);
    EXPECT_TRUE(called);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, NewWithArguments)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    bool constructed = false;
    auto id = local.StoreFunc([&constructed](ARKTS_CallInfo info) -> ARKTS_Value {
        constructed = true;
        return ARKTS_GetThisArg(info);
    });
    
    auto clazz = ARKTS_CreateClass(env, id, ARKTS_CreateUndefined());
    
    ARKTS_Value args[] = {
        ARKTS_CreateI32(42),
        ARKTS_CreateUtf8(env, "test", 4)
    };
    
    auto instance = ARKTS_New(env, clazz, 2, args);
    EXPECT_TRUE(constructed);
    EXPECT_TRUE(ARKTS_IsObject(env, instance));
    EXPECT_TRUE(ARKTS_InstanceOf(env, instance, clazz));
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ArrayOperations)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    auto arr = ARKTS_CreateArray(env, 5);
    EXPECT_EQ(ARKTS_GetArrayLength(env, arr), 5);
    
    for (int i = 0; i < 5; i++) {
        ARKTS_SetElement(env, arr, i, ARKTS_CreateI32(i));
    }
    
    for (int i = 0; i < 5; i++) {
        auto elem = ARKTS_GetElement(env, arr, i);
        auto numValue = ARKTS_GetValueNumber(elem);
        EXPECT_EQ(static_cast<int>(numValue), i);
    }
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ObjectGetPrototypeOf)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    auto classId = MockContext::GetInstance()->StoreFunc([](ARKTS_CallInfo info)->ARKTS_Value {
        return ARKTS_GetThisArg(info);
    });
    auto clazz = ARKTS_CreateClass(env, classId, ARKTS_CreateUndefined());
    EXPECT_TRUE(ARKTS_IsClass(env, clazz));
    auto proto = ARKTS_GetPrototype(env, clazz);
    EXPECT_TRUE(ARKTS_IsHeapObject(proto));
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ArrayBufferCreateWithData)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};
    auto buffer = ARKTS_CreateArrayBuffer(env, sizeof(data));
    
    auto ptr = reinterpret_cast<uint8_t*>(ARKTS_GetArrayBufferRawPtr(env, buffer));
    memcpy(ptr, data, sizeof(data));
    
    EXPECT_EQ(ARKTS_GetArrayBufferLength(env, buffer), sizeof(data));
    
    uint8_t received[sizeof(data)];
    auto bytesRead = ARKTS_ArrayBufferReadBytes(env, buffer, received, sizeof(data));
    EXPECT_EQ(bytesRead, sizeof(data));
    
    for (size_t i = 0; i < sizeof(data); i++) {
        EXPECT_EQ(received[i], data[i]);
    }
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, BigIntOperations)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    int64_t bigIntValue = 1234567890123456789LL;
    auto bigInt = ARKTS_CreateBigInt(env, bigIntValue);
    EXPECT_TRUE(ARKTS_IsBigInt(env, bigInt));
    
    auto byteSize = ARKTS_BigIntGetByteSize(env, bigInt);
    EXPECT_GT(byteSize, 0);
    
    bool isNegative = false;
    uint8_t bytes[16];
    ARKTS_BigIntReadBytes(env, bigInt, &isNegative, sizeof(bytes), bytes);
    EXPECT_FALSE(isNegative);
    
    auto bigInt2 = ARKTS_CreateBigIntWithBytes(env, false, byteSize, bytes);
    EXPECT_TRUE(ARKTS_IsBigInt(env, bigInt2));
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, SymbolOperations)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    const char* desc = "test_symbol";
    auto symbol = ARKTS_CreateSymbol(env, desc, strlen(desc));
    EXPECT_TRUE(ARKTS_IsSymbol(env, symbol));
    
    auto symbolDesc = ARKTS_GetSymbolDesc(env, symbol);
    EXPECT_STREQ(symbolDesc, desc);
    ARKTS_FreeCString(symbolDesc);
    
    auto symbol2 = ARKTS_CreateSymbol(env, desc, strlen(desc));
    EXPECT_FALSE(ARKTS_StrictEqual(env, symbol, symbol2));
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, PromiseChaining)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    
    auto promiseCap = ARKTS_CreatePromiseCapability(env);
    auto promise = ARKTS_GetPromiseFromCapability(env, promiseCap);
    
    bool resolved = false;
    bool chained = false;
    
    auto resolveId = local.StoreFunc([&resolved, env](ARKTS_CallInfo info) -> ARKTS_Value {
        resolved = true;
        return ARKTS_GetArg(info, 0);
    });
    
    auto chainId = local.StoreFunc([&chained, env](ARKTS_CallInfo info) -> ARKTS_Value {
        chained = true;
        return ARKTS_GetArg(info, 0);
    });
    
    auto onResolved = ARKTS_CreateFunc(env, resolveId);
    auto onChain = ARKTS_CreateFunc(env, chainId);
    
    auto chainedPromise = ARKTS_PromiseThen(env, promise, onResolved, ARKTS_CreateUndefined());
    (void)ARKTS_PromiseThen(env, chainedPromise, onChain, ARKTS_CreateUndefined());
    
    ARKTS_PromiseCapabilityResolve(env, promiseCap, ARKTS_CreateI32(42));
    
    EXPECT_TRUE(resolved);
    EXPECT_TRUE(chained);
    
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, HiTraceStartTrace)
{
    ARKTS_HiTraceStartTrace("test_trace", 1);
}

TEST_F(ArkInteropTest, HiTraceFinishTrace)
{
    ARKTS_HiTraceFinishTrace("test_trace", 1);
}

TEST_F(ArkInteropTest, HiTraceCountTrace)
{
    ARKTS_HiTraceCountTrace("test_trace", 100);
}

TEST_F(ArkInteropTest, InitEventHandle)
{
    MockContext local;
    auto env = local.GetEnv();
    ARKTS_InitEventHandle(env);
}

TEST_F(ArkInteropTest, InitEventHandleWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_InitEventHandle(nullptr);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, CreateAsyncTask)
{
    GTEST_SKIP() << "async task on global engine may prevent clean shutdown";
}

TEST_F(ArkInteropTest, CreateAsyncTaskWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_CreateAsyncTask(nullptr, 12345);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

// --- Coverage improvement tests ---

// === Module Loading Tests ===

TEST_F(ArkInteropTest, RequireArkModuleWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_RequireArkModule(nullptr, "test", 4, ARKTS_NativeModule);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, RequireArkModuleWithNullPath)
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto val = ARKTS_RequireArkModule(env, nullptr, 0, ARKTS_NativeModule);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, RequireArkNativeModule)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto result = ARKTS_RequireArkModule(env, "@native:ohos.curves", 20, ARKTS_NativeModule);
    (void)result;

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, RequireArkNormalModule)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto result = ARKTS_RequireArkModule(env, "nonexistent_module", 18, ARKTS_NormalModule);
    (void)result;

    ARKTS_CloseScope(env, scope);
}

// === Scope Tests ===

TEST_F(ArkInteropTest, CloseScopeWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_CloseScope(nullptr, nullptr);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, OpenScopeWithNullEnv)
{
    ErrorCaptureContext local;
    auto scope = ARKTS_OpenScope(nullptr);
    (void)scope;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, ReturnWithScope)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto value = ARKTS_CreateI32(42);
    auto result = ARKTS_Return(env, scope, value);
    EXPECT_TRUE(result.value != 0);
}

// === Global Tests ===

TEST_F(ArkInteropTest, CreateGlobalWithNullEnv)
{
    ErrorCaptureContext local;
    auto result = ARKTS_CreateGlobal(nullptr, ARKTS_CreateI32(1));
    (void)result;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, DisposeGlobalAsync)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, obj);
    ARKTS_DisposeGlobal(env, global);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, DisposeGlobalSyncWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_DisposeGlobalSync(nullptr, nullptr);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, DisposeGlobalWithNullEnv)
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    ARKTS_DisposeGlobal(nullptr, nullptr);
    EXPECT_TRUE(local.HasAndClearNativeError());

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetGlobalNapiEnv)
{
    MockContext local;
    auto env = local.GetEnv();
    auto napiEnv = ARKTS_GetGlobalNapiEnv(env);
    EXPECT_TRUE(napiEnv != nullptr);
}

TEST_F(ArkInteropTest, GetGlobalNapiEnvWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetGlobalNapiEnv(nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetGlobalValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, obj);
    EXPECT_TRUE(ARKTS_GlobalIsAlive(env, global));

    auto val = ARKTS_GetGlobalValue(global);
    EXPECT_TRUE(ARKTS_IsHeapObject(val));

    ARKTS_DisposeGlobalSync(env, global);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetGlobalValueWithNull)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetGlobalValue(nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GlobalClearWeakWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_GlobalClearWeak(nullptr, nullptr);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GlobalConstant)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto globalObj = ARKTS_GetGlobalConstant(env);
    EXPECT_TRUE(ARKTS_IsHeapObject(globalObj));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GlobalFromValueWithNullEnv)
{
    ErrorCaptureContext local;
    auto result = ARKTS_GlobalFromValue(nullptr, ARKTS_CreateI32(0));
    (void)result;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GlobalIsAliveWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_GlobalIsAlive(nullptr, nullptr);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GlobalSetWeakAndClearWeak)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, obj);

    ARKTS_GlobalSetWeak(env, global);
    EXPECT_TRUE(ARKTS_GlobalIsAlive(env, global));

    ARKTS_GlobalClearWeak(env, global);
    EXPECT_TRUE(ARKTS_GlobalIsAlive(env, global));

    ARKTS_DisposeGlobalSync(env, global);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GlobalSetWeakTwice)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, obj);
    ARKTS_GlobalSetWeak(env, global);
    ARKTS_GlobalSetWeak(env, global);
    EXPECT_TRUE(ARKTS_GlobalIsAlive(env, global));

    ARKTS_DisposeGlobalSync(env, global);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GlobalSetWeakWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_GlobalSetWeak(nullptr, nullptr);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GlobalToValueWithNullEnv)
{
    ErrorCaptureContext local;
    auto result = ARKTS_GlobalToValue(nullptr, nullptr);
    (void)result;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

// === Object Tests ===

TEST_F(ArkInteropTest, CreateObjectWithNullEnv)
{
    ErrorCaptureContext local;
    auto obj = ARKTS_CreateObject(nullptr);
    (void)obj;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, DefineAccessors)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto key = ARKTS_CreateUtf8(env, "prop", 4);

    auto getterId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_CreateI32(42);
    });
    auto getterFunc = ARKTS_CreateFunc(env, getterId);

    ARKTS_Accessor accessor;
    accessor.getter = getterFunc;
    accessor.setter = ARKTS_CreateUndefined();
    accessor.attrs = static_cast<ARKTS_PropertyFlag>(N_CONFIGURABLE | N_ENUMERABLE);

    ARKTS_DefineAccessors(env, obj, key, accessor);

    auto val = ARKTS_GetProperty(env, obj, key);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(val)), 42);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, DefineAccessorsV2)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto key = ARKTS_CreateUtf8(env, "val", 3);

    auto getterId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_CreateI32(100);
    });
    auto getterFunc = ARKTS_CreateFunc(env, getterId);

    ARKTS_Accessor accessor;
    accessor.getter = getterFunc;
    accessor.setter = ARKTS_CreateUndefined();
    accessor.attrs = static_cast<ARKTS_PropertyFlag>(N_CONFIGURABLE | N_ENUMERABLE);

    auto result = ARKTS_DefineAccessorsV2(env, obj, key, accessor);
    EXPECT_TRUE(result);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, DefineAccessorsV2WithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_Accessor accessor = {};
    ARKTS_DefineAccessorsV2(nullptr, ARKTS_CreateI32(1), ARKTS_CreateI32(1), accessor);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, DefineAccessorsWithBothGetterSetter)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto key = ARKTS_CreateUtf8(env, "gs", 2);

    auto getterId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_CreateI32(77);
    });
    auto setterId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_CreateUndefined();
    });

    ARKTS_Accessor accessor;
    accessor.getter = ARKTS_CreateFunc(env, getterId);
    accessor.setter = ARKTS_CreateFunc(env, setterId);
    accessor.attrs = static_cast<ARKTS_PropertyFlag>(N_CONFIGURABLE | N_ENUMERABLE);

    ARKTS_DefineAccessors(env, obj, key, accessor);
    auto val = ARKTS_GetProperty(env, obj, key);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(val)), 77);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, DefineAccessorsWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_Accessor accessor = {};
    ARKTS_DefineAccessors(nullptr, ARKTS_CreateI32(1), ARKTS_CreateI32(1), accessor);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, DefineOwnProperty)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto key = ARKTS_CreateUtf8(env, "defined", 7);
    auto val = ARKTS_CreateI32(999);

    ARKTS_DefineOwnProperty(env, obj, key, val,
        static_cast<ARKTS_PropertyFlag>(N_WRITABLE | N_ENUMERABLE | N_CONFIGURABLE));

    EXPECT_TRUE(ARKTS_HasOwnProperty(env, obj, key));
    auto readBack = ARKTS_GetProperty(env, obj, key);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(readBack)), 999);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, DefineOwnPropertyV2)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto key = ARKTS_CreateUtf8(env, "myProp", 6);
    auto val = ARKTS_CreateI32(123);

    auto result = ARKTS_DefineOwnPropertyV2(env, obj, key, val,
        static_cast<ARKTS_PropertyFlag>(N_WRITABLE | N_ENUMERABLE | N_CONFIGURABLE));
    EXPECT_TRUE(result);

    auto readBack = ARKTS_GetProperty(env, obj, key);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(readBack)), 123);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, DefineOwnPropertyWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_DefineOwnProperty(nullptr, ARKTS_CreateI32(1), ARKTS_CreateI32(1), ARKTS_CreateI32(1),
        static_cast<ARKTS_PropertyFlag>(N_WRITABLE));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, DefineOwnPropertyWithNumberKey)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto numKey = ARKTS_CreateI32(42);
    auto val = ARKTS_CreateI32(999);
    ARKTS_DefineOwnProperty(env, obj, numKey, val,
        static_cast<ARKTS_PropertyFlag>(N_WRITABLE | N_ENUMERABLE | N_CONFIGURABLE));

    EXPECT_TRUE(ARKTS_HasOwnProperty(env, obj, numKey));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, EnumOwnProperties)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto key1 = ARKTS_CreateUtf8(env, "a", 1);
    auto key2 = ARKTS_CreateUtf8(env, "b", 1);
    ARKTS_SetProperty(env, obj, key1, ARKTS_CreateI32(1));
    ARKTS_SetProperty(env, obj, key2, ARKTS_CreateI32(2));

    auto props = ARKTS_EnumOwnProperties(env, obj);
    EXPECT_TRUE(ARKTS_IsArray(env, props));
    EXPECT_EQ(ARKTS_GetArrayLength(env, props), 2);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, EnumOwnPropertiesWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_EnumOwnProperties(nullptr, ARKTS_CreateI32(1));
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetPropertyWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetProperty(nullptr, ARKTS_CreateI32(1), ARKTS_CreateI32(1));
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, HasOwnPropertyWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_HasOwnProperty(nullptr, ARKTS_CreateI32(1), ARKTS_CreateI32(1));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, HasOwnPropertyWithNumberKey)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto arr = ARKTS_CreateArray(env, 3);
    ARKTS_SetElement(env, arr, 0, ARKTS_CreateI32(100));

    auto obj = ARKTS_CreateObject(env);
    auto key = ARKTS_CreateUtf8(env, "x", 1);
    ARKTS_SetProperty(env, obj, key, ARKTS_CreateI32(1));
    EXPECT_TRUE(ARKTS_HasOwnProperty(env, obj, key));

    auto missingKey = ARKTS_CreateUtf8(env, "missing", 7);
    EXPECT_FALSE(ARKTS_HasOwnProperty(env, obj, missingKey));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, HasOwnPropertyWithSymbolKey)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto sym = ARKTS_CreateSymbol(env, "propSym", 7);
    ARKTS_SetProperty(env, obj, sym, ARKTS_CreateI32(42));
    EXPECT_TRUE(ARKTS_HasOwnProperty(env, obj, sym));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsObjectWithNonHeapValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsObject(env, ARKTS_CreateI32(42)));
    EXPECT_FALSE(ARKTS_IsObject(env, ARKTS_CreateBool(false)));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsObjectWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_IsObject(nullptr, ARKTS_CreateI32(1));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, SetPropertyWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_SetProperty(nullptr, ARKTS_CreateI32(1), ARKTS_CreateI32(1), ARKTS_CreateI32(1));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, SetPropertyWithSymbolKey)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto sym = ARKTS_CreateSymbol(env, "mySymbol", 8);

    ARKTS_SetProperty(env, obj, sym, ARKTS_CreateI32(777));
    auto val = ARKTS_GetProperty(env, obj, sym);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(val)), 777);

    ARKTS_CloseScope(env, scope);
}

// === Array Tests ===

TEST_F(ArkInteropTest, CreateArrayWithInit)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    ARKTS_Value initData[] = {
        ARKTS_CreateI32(10), ARKTS_CreateI32(20), ARKTS_CreateI32(30)
    };
    auto arr = ARKTS_CreateArrayWithInit(env, 3, initData);
    EXPECT_TRUE(ARKTS_IsArray(env, arr));
    EXPECT_EQ(ARKTS_GetArrayLength(env, arr), 3);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(ARKTS_GetElement(env, arr, 0))), 10);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(ARKTS_GetElement(env, arr, 2))), 30);

    auto emptyArr = ARKTS_CreateArrayWithInit(env, 0, nullptr);
    EXPECT_TRUE(ARKTS_IsArray(env, emptyArr));
    EXPECT_EQ(ARKTS_GetArrayLength(env, emptyArr), 0);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateArrayWithNullEnv)
{
    ErrorCaptureContext local;
    auto arr = ARKTS_CreateArray(nullptr, 0);
    (void)arr;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetArrayLengthBasic)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto arr = ARKTS_CreateArray(env, 5);
    auto len = ARKTS_GetArrayLength(env, arr);
    EXPECT_EQ(len, 5u);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsArrayWithNonHeapValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsArray(env, ARKTS_CreateI32(1)));
    EXPECT_FALSE(ARKTS_IsArray(env, ARKTS_CreateNull()));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsArrayWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_IsArray(nullptr, ARKTS_CreateI32(1));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

// === ArrayBuffer Tests ===

TEST_F(ArkInteropTest, ArrayBufferReadBytes)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto ab = ARKTS_CreateArrayBuffer(env, 4);
    auto ptr = reinterpret_cast<uint8_t*>(ARKTS_GetArrayBufferRawPtr(env, ab));
    ptr[0] = 0x11;
    ptr[1] = 0x22;
    ptr[2] = 0x33;
    ptr[3] = 0x44;

    uint8_t dest[4] = {};
    auto read = ARKTS_ArrayBufferReadBytes(env, ab, dest, 4);
    EXPECT_EQ(read, 4);
    EXPECT_EQ(dest[0], 0x11);
    EXPECT_EQ(dest[3], 0x44);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateArrayBufferWithData)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    uint8_t rawData[] = {0xAA, 0xBB, 0xCC, 0xDD};
    auto buffer = ARKTS_CreateArrayBufferWithData(env, rawData, sizeof(rawData), 0);
    EXPECT_TRUE(ARKTS_IsArrayBuffer(env, buffer));
    EXPECT_EQ(ARKTS_GetArrayBufferLength(env, buffer), static_cast<int32_t>(sizeof(rawData)));

    auto ptr = reinterpret_cast<uint8_t*>(ARKTS_GetArrayBufferRawPtr(env, buffer));
    EXPECT_EQ(ptr[0], 0xAA);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateArrayBufferWithDataNullBuffer)
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto val = ARKTS_CreateArrayBufferWithData(env, nullptr, 1, 0);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, CreateArrayBufferWithDataNullEnv)
{
    ErrorCaptureContext local;
    uint8_t data[] = {1};
    auto val = ARKTS_CreateArrayBufferWithData(nullptr, data, 1, 0);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, CreateArrayBufferWithNullEnv)
{
    ErrorCaptureContext local;
    auto buf = ARKTS_CreateArrayBuffer(nullptr, 0);
    (void)buf;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, DataViewBufferLengthAndPtr)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto ab = ARKTS_CreateArrayBuffer(env, 8);
    auto global = ARKTS_GetGlobalConstant(env);
    auto dvName = ARKTS_CreateUtf8(env, "DataView", -1);
    auto dvCtor = ARKTS_GetProperty(env, global, dvName);
    ASSERT_TRUE(ARKTS_IsCallable(env, dvCtor));

    auto dataView = ARKTS_New(env, dvCtor, 1, &ab);
    EXPECT_TRUE(ARKTS_IsArrayBuffer(env, dataView));

    auto len = ARKTS_GetArrayBufferLength(env, dataView);
    EXPECT_EQ(len, 8);

    auto ptr = ARKTS_GetArrayBufferRawPtr(env, dataView);
    EXPECT_NE(ptr, nullptr);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsArrayBufferWithNonHeapValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsArrayBuffer(env, ARKTS_CreateI32(1)));
    EXPECT_FALSE(ARKTS_IsArrayBuffer(env, ARKTS_CreateBool(true)));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsArrayBufferWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_IsArrayBuffer(nullptr, ARKTS_CreateI32(1));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, TypedArrayBufferLengthAndPtr)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto global = ARKTS_GetGlobalConstant(env);
    auto ctorName = ARKTS_CreateUtf8(env, "Int8Array", -1);
    auto int8Ctor = ARKTS_GetProperty(env, global, ctorName);
    ASSERT_TRUE(ARKTS_IsCallable(env, int8Ctor));

    ARKTS_Value sizeArg = ARKTS_CreateI32(4);
    auto typedArr = ARKTS_New(env, int8Ctor, 1, &sizeArg);
    EXPECT_TRUE(ARKTS_IsArrayBuffer(env, typedArr));

    auto len = ARKTS_GetArrayBufferLength(env, typedArr);
    EXPECT_EQ(len, 4);

    auto ptr = ARKTS_GetArrayBufferRawPtr(env, typedArr);
    EXPECT_NE(ptr, nullptr);

    ARKTS_CloseScope(env, scope);
}

// === String Tests ===

TEST_F(ArkInteropTest, CreateLongUtf8String)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    std::string longStr(200, 'A');
    auto str = ARKTS_CreateUtf8(env, longStr.c_str(), longStr.size());
    EXPECT_TRUE(ARKTS_IsString(env, str));

    auto size = ARKTS_GetValueUtf8Size(env, str);
    EXPECT_EQ(size, static_cast<int32_t>(longStr.size() + 1));

    auto cstr = ARKTS_GetValueCString(env, str);
    EXPECT_STREQ(cstr, longStr.c_str());
    ARKTS_FreeCString(cstr);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateStringCompressedLong)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    std::string longStr(200, 'B');
    auto str = ARKTS_CreateString(env, true, longStr.size(), longStr.c_str());
    EXPECT_TRUE(ARKTS_IsString(env, str));
    auto info = ARKTS_GetStringInfo(env, str);
    EXPECT_EQ(info.length, 200);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateStringShortCompressed)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    const char* text = "short";
    auto str = ARKTS_CreateString(env, true, strlen(text), text);
    EXPECT_TRUE(ARKTS_IsString(env, str));
    auto info = ARKTS_GetStringInfo(env, str);
    EXPECT_TRUE(info.isCompressed);
    EXPECT_EQ(info.length, strlen(text));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateStringShortUncompressed)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    uint16_t utf16Data[] = {0x4F60, 0x597D};
    auto str = ARKTS_CreateString(env, false, 2, utf16Data);
    EXPECT_TRUE(ARKTS_IsString(env, str));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateStringUncompressed)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    uint16_t utf16Data[] = {0x4F60, 0x597D, 0x0041};
    auto str = ARKTS_CreateString(env, false, 3, utf16Data);
    EXPECT_TRUE(ARKTS_IsString(env, str));
    auto info = ARKTS_GetStringInfo(env, str);
    EXPECT_FALSE(info.isCompressed);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateStringUncompressedLong)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    std::vector<uint16_t> longUtf16(200, 0x4E2D);
    auto str = ARKTS_CreateString(env, false, longUtf16.size(), longUtf16.data());
    EXPECT_TRUE(ARKTS_IsString(env, str));
    auto info = ARKTS_GetStringInfo(env, str);
    EXPECT_FALSE(info.isCompressed);
    EXPECT_EQ(info.length, 200);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateStringWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateString(nullptr, true, 1, "a");
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, CreateUtf8WithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateUtf8(nullptr, "t", 1);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetStringInfoWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetStringInfo(nullptr, ARKTS_CreateI32(1));
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetValueCStringAndFree)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto str = ARKTS_CreateUtf8(env, "hello_world", -1);
    const char* cstr = ARKTS_GetValueCString(env, str);
    ASSERT_NE(cstr, nullptr);
    EXPECT_STREQ(cstr, "hello_world");
    ARKTS_FreeCString(cstr);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetValueCStringWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetValueCString(nullptr, ARKTS_CreateI32(1));
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, StringCopy)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    const char* text = "hello world";
    auto str = ARKTS_CreateUtf8(env, text, strlen(text));
    auto info = ARKTS_GetStringInfo(env, str);
    EXPECT_EQ(info.length, strlen(text));

    char buf[64] = {};
    ARKTS_StringCopy(env, str, buf, info.length);
    EXPECT_EQ(std::string(buf, info.length), text);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, StringCopyUncompressed)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    uint16_t utf16Data[] = {0x4F60, 0x597D, 0x0041};
    auto str = ARKTS_CreateString(env, false, 3, utf16Data);
    auto info = ARKTS_GetStringInfo(env, str);
    EXPECT_FALSE(info.isCompressed);

    uint16_t buf[4] = {};
    ARKTS_StringCopy(env, str, buf, info.length);
    EXPECT_EQ(buf[0], 0x4F60);
    EXPECT_EQ(buf[1], 0x597D);
    EXPECT_EQ(buf[2], 0x0041);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, StringCopyWithNullEnv)
{
    ErrorCaptureContext local;
    char buf[4];
    ARKTS_StringCopy(nullptr, ARKTS_CreateI32(1), buf, 1);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, StringGetValueUtf8)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    const char* text = "test string";
    auto str = ARKTS_CreateUtf8(env, text, strlen(text));

    auto size = ARKTS_GetValueUtf8Size(env, str);
    EXPECT_EQ(size, static_cast<int32_t>(strlen(text) + 1));

    char buf[64] = {};
    auto copied = ARKTS_GetValueUtf8(env, str, sizeof(buf), buf);
    EXPECT_GT(copied, 0);
    EXPECT_STREQ(buf, text);

    ARKTS_CloseScope(env, scope);
}

// === Symbol Tests ===

TEST_F(ArkInteropTest, CreateSymbolWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateSymbol(nullptr, "test", 4);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetValueTypeSymbol)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto sym = ARKTS_CreateSymbol(env, "test_sym", -1);
    auto t = ARKTS_GetValueType(env, sym);
    EXPECT_EQ(t, N_SYMBOL);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsSymbolWithNonHeapValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsSymbol(env, ARKTS_CreateI32(42)));
    EXPECT_FALSE(ARKTS_IsSymbol(env, ARKTS_CreateBool(true)));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, SymbolWithEmptyDescription)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto symbol = ARKTS_CreateSymbol(env, "", 0);
    EXPECT_TRUE(ARKTS_IsSymbol(env, symbol));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, SymbolWithNullDescription)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto symbol = ARKTS_CreateSymbol(env, nullptr, 0);
    EXPECT_TRUE(ARKTS_IsSymbol(env, symbol));

    ARKTS_CloseScope(env, scope);
}

// === Promise Tests ===

TEST_F(ArkInteropTest, CreatePromiseCapabilityWithNullEnv)
{
    ErrorCaptureContext local;
    auto p = ARKTS_CreatePromiseCapability(nullptr);
    (void)p;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, IsPromiseWithNonHeapValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsPromise(env, ARKTS_CreateI32(0)));
    EXPECT_FALSE(ARKTS_IsPromise(env, ARKTS_CreateBool(true)));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsPromiseWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_IsPromise(nullptr, ARKTS_CreateI32(1));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, PromiseCapabilityResolveWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_PromiseCapabilityResolve(nullptr, nullptr, ARKTS_CreateI32(1));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, PromiseCatch)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto promiseCap = ARKTS_CreatePromiseCapability(env);
    auto promise = ARKTS_GetPromiseFromCapability(env, promiseCap);

    bool caught = false;
    auto catchId = local.StoreFunc([&caught](ARKTS_CallInfo info) -> ARKTS_Value {
        caught = true;
        return ARKTS_CreateUndefined();
    });
    auto catchFunc = ARKTS_CreateFunc(env, catchId);
    auto catchedPromise = ARKTS_PromiseCatch(env, promise, catchFunc);
    EXPECT_TRUE(ARKTS_IsPromise(env, catchedPromise));

    ARKTS_PromiseCapabilityReject(env, promiseCap, ARKTS_CreateI32(99));
    EXPECT_TRUE(caught);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, PromiseCatchWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_PromiseCatch(nullptr, ARKTS_CreateI32(1), ARKTS_CreateI32(1));
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, PromiseThenWithRejectHandler)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto promiseCap = ARKTS_CreatePromiseCapability(env);
    auto promise = ARKTS_GetPromiseFromCapability(env, promiseCap);

    bool fulfilled = false;
    bool rejected = false;

    auto fulfilledId = local.StoreFunc([&fulfilled](ARKTS_CallInfo) -> ARKTS_Value {
        fulfilled = true;
        return ARKTS_CreateUndefined();
    });
    auto rejectedId = local.StoreFunc([&rejected](ARKTS_CallInfo) -> ARKTS_Value {
        rejected = true;
        return ARKTS_CreateUndefined();
    });

    auto onFulfilled = ARKTS_CreateFunc(env, fulfilledId);
    auto onRejected = ARKTS_CreateFunc(env, rejectedId);
    auto thenPromise = ARKTS_PromiseThen(env, promise, onFulfilled, onRejected);
    EXPECT_TRUE(ARKTS_IsPromise(env, thenPromise));

    ARKTS_PromiseCapabilityReject(env, promiseCap, ARKTS_CreateI32(42));
    EXPECT_TRUE(rejected);

    ARKTS_CloseScope(env, scope);
}

// === BigInt Tests ===

TEST_F(ArkInteropTest, BigIntWithBytes)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    uint8_t bytes[] = {0xFF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                       0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
    auto bigint = ARKTS_CreateBigIntWithBytes(env, true, sizeof(bytes), bytes);
    EXPECT_TRUE(ARKTS_IsBigInt(env, bigint));

    auto byteSize = ARKTS_BigIntGetByteSize(env, bigint);
    EXPECT_GT(byteSize, 0);

    bool isNeg = false;
    uint8_t readBack[32] = {};
    ARKTS_BigIntReadBytes(env, bigint, &isNeg, sizeof(readBack), readBack);
    EXPECT_TRUE(isNeg);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateBigIntWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateBigInt(nullptr, 1);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetValueTypeBigInt)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto bi = ARKTS_CreateBigInt(env, 42);
    auto t = ARKTS_GetValueType(env, bi);
    EXPECT_EQ(t, N_BIGINT);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsBigIntWithNonBigInt)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsBigInt(env, ARKTS_CreateI32(42)));
    EXPECT_FALSE(ARKTS_IsBigInt(env, ARKTS_CreateNull()));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, NegativeBigInt)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    int64_t value = -9876543210LL;
    auto bigint = ARKTS_CreateBigInt(env, value);
    EXPECT_TRUE(ARKTS_IsBigInt(env, bigint));

    bool isNeg = false;
    auto byteSize = ARKTS_BigIntGetByteSize(env, bigint);
    EXPECT_GT(byteSize, 0);

    uint8_t bytes[16] = {};
    ARKTS_BigIntReadBytes(env, bigint, &isNeg, sizeof(bytes), bytes);
    EXPECT_TRUE(isNeg);

    ARKTS_CloseScope(env, scope);
}

// === External Tests ===

TEST_F(ArkInteropTest, CreateExternalWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateExternal(nullptr, 1);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, ExternalCreateAndGet)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    int64_t data = 0x12345678ABCDEF00LL;
    auto ext = ARKTS_CreateExternal(env, data);
    EXPECT_TRUE(ARKTS_IsExternal(env, ext));
    EXPECT_EQ(ARKTS_GetExternalData(env, ext), data);
    EXPECT_EQ(ARKTS_GetValueType(env, ext), N_EXTERNAL);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetExternalDataWithNonExternal)
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto val = ARKTS_GetExternalData(env, ARKTS_CreateI32(1));
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetValueTypeExternal)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto ext = ARKTS_CreateExternal(env, 123);
    auto t = ARKTS_GetValueType(env, ext);
    EXPECT_EQ(t, N_EXTERNAL);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsExternalWithNonHeapValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsExternal(env, ARKTS_CreateI32(1)));
    EXPECT_FALSE(ARKTS_IsExternal(env, ARKTS_CreateNull()));

    ARKTS_CloseScope(env, scope);
}

// === Function Tests ===

TEST_F(ArkInteropTest, CallFuncWithArgs)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto addId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        auto a = ARKTS_GetValueNumber(ARKTS_GetArg(info, 0));
        auto b = ARKTS_GetValueNumber(ARKTS_GetArg(info, 1));
        return ARKTS_CreateF64(a + b);
    });
    auto addFunc = ARKTS_CreateFunc(env, addId);

    ARKTS_Value args[] = {ARKTS_CreateF64(10.0), ARKTS_CreateF64(20.0)};
    auto result = ARKTS_Call(env, addFunc, ARKTS_CreateUndefined(), 2, args);
    EXPECT_NEAR(ARKTS_GetValueNumber(result), 30.0, 0.001);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CallWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_Call(nullptr, ARKTS_CreateI32(1), ARKTS_CreateUndefined(), 0, nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, CreateFuncWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateFunc(nullptr, 1);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, IsCallableWithNonFunction)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsCallable(env, ARKTS_CreateI32(1)));
    EXPECT_FALSE(ARKTS_IsCallable(env, ARKTS_CreateObject(env)));

    ARKTS_CloseScope(env, scope);
}

// === Class Tests ===

TEST_F(ArkInteropTest, CreateClassNoBase)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto classId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_GetThisArg(info);
    });
    auto clazz = ARKTS_CreateClass(env, classId, ARKTS_CreateI32(0));
    EXPECT_TRUE(ARKTS_IsClass(env, clazz));

    auto proto = ARKTS_GetPrototype(env, clazz);
    EXPECT_TRUE(ARKTS_IsHeapObject(proto));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateClassWithInheritance)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto baseId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_GetThisArg(info);
    });
    auto baseClass = ARKTS_CreateClass(env, baseId, ARKTS_CreateUndefined());
    EXPECT_TRUE(ARKTS_IsClass(env, baseClass));

    auto childId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_GetThisArg(info);
    });
    auto childClass = ARKTS_CreateClass(env, childId, baseClass);
    EXPECT_TRUE(ARKTS_IsClass(env, childClass));

    auto instance = ARKTS_New(env, childClass, 0, nullptr);
    EXPECT_TRUE(ARKTS_InstanceOf(env, instance, childClass));
    EXPECT_TRUE(ARKTS_InstanceOf(env, instance, baseClass));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetPrototypeOfClass)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto classId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_GetThisArg(info);
    });
    auto clazz = ARKTS_CreateClass(env, classId, ARKTS_CreateUndefined());
    EXPECT_TRUE(ARKTS_IsClass(env, clazz));

    auto proto = ARKTS_GetPrototype(env, clazz);
    EXPECT_TRUE(ARKTS_IsHeapObject(proto));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsClassWithNonClass)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsClass(env, ARKTS_CreateI32(1)));
    EXPECT_FALSE(ARKTS_IsClass(env, ARKTS_CreateObject(env)));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, NewInstanceAndInstanceOf)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto classId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_GetThisArg(info);
    });
    auto clazz = ARKTS_CreateClass(env, classId, ARKTS_CreateUndefined());
    EXPECT_TRUE(ARKTS_IsClass(env, clazz));

    auto instance = ARKTS_New(env, clazz, 0, nullptr);
    EXPECT_TRUE(ARKTS_IsObject(env, instance));
    EXPECT_TRUE(ARKTS_InstanceOf(env, instance, clazz));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, NewWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_New(nullptr, ARKTS_CreateI32(1), 0, nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

// === CycleFree Tests ===

TEST_F(ArkInteropTest, CreateCycleFreeExternAndVerify)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto ext = ARKTS_CreateCycleFreeExtern(env, 42);
    EXPECT_TRUE(ARKTS_IsExternal(env, ext));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateCycleFreeExternWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateCycleFreeExtern(nullptr, 1);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, CreateCycleFreeFuncAndCall)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto func = ARKTS_CreateCycleFreeFunc(env, 1);
    EXPECT_TRUE(ARKTS_IsCallable(env, func));
    auto result = ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);
    (void)result;

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateCycleFreeFuncWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateCycleFreeFunc(nullptr, 1);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, RegisterCycleFreeCallbackDuplicate)
{
    ARKTS_CycleFreeCallback cb;
    cb.funcInvoker = [](ARKTS_CallInfo, int64_t) -> ARKTS_Value { return ARKTS_CreateUndefined(); };
    cb.refRelease = [](int64_t) {};
    ARKTS_RegisterCycleFreeCallback(cb);
}

// === Throw/Exception Tests ===

TEST_F(ArkInteropTest, GetExceptionAndClearNoException)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto result = ARKTS_GetExceptionAndClear(env);
    (void)result;

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetExceptionAndClearWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetExceptionAndClear(nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, ThrowDuplicateErrorConflict)
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto err1 = ARKTS_CreateUtf8(env, "first", -1);
    ARKTS_Throw(env, err1);
    auto err2 = ARKTS_CreateUtf8(env, "second", -1);
    ARKTS_Throw(env, err2);

    auto caught = ARKTS_GetExceptionAndClear(env);
    EXPECT_TRUE(ARKTS_IsHeapObject(caught));
    local.HasAndClearNativeError();
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ThrowError)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto strErr = ARKTS_CreateUtf8(env, "test error", -1);
    ARKTS_Throw(env, strErr);

    auto exception = ARKTS_GetExceptionAndClear(env);
    EXPECT_FALSE(ARKTS_IsUndefined(exception));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ThrowObjectError)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    ARKTS_Throw(env, obj);

    auto exception = ARKTS_GetExceptionAndClear(env);
    EXPECT_FALSE(ARKTS_IsUndefined(exception));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ThrowObjectErrorAndCatch)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto global = ARKTS_GetGlobalConstant(env);
    auto errorCtorName = ARKTS_CreateUtf8(env, "Error", -1);
    auto errorCtor = ARKTS_GetProperty(env, global, errorCtorName);
    auto msg = ARKTS_CreateUtf8(env, "test throw", -1);
    auto errorObj = ARKTS_New(env, errorCtor, 1, &msg);
    ARKTS_Throw(env, errorObj);
    auto caught = ARKTS_GetExceptionAndClear(env);
    EXPECT_TRUE(ARKTS_IsHeapObject(caught));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ThrowWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_Throw(nullptr, ARKTS_CreateI32(1));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, ThrowWithPendingException)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto strErr = ARKTS_CreateUtf8(env, "first error", -1);
    ARKTS_Throw(env, strErr);

    auto strErr2 = ARKTS_CreateUtf8(env, "second error", -1);
    ARKTS_Throw(env, strErr2);

    auto exception = ARKTS_GetExceptionAndClear(env);
    (void)exception;

    ARKTS_CloseScope(env, scope);
}

// === CallInfo Tests ===

TEST_F(ArkInteropTest, GetArgCountWithNull)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetArgCount(nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetArgWithNull)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetArg(nullptr, 0);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetCallEnvWithNull)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetCallEnv(nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetThisArgWithNull)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetThisArg(nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

// === Value/Type Tests ===

TEST_F(ArkInteropTest, GetValueNumberF64)
{
    auto val = ARKTS_CreateF64(2.718);
    EXPECT_NEAR(ARKTS_GetValueNumber(val), 2.718, 0.001);

    auto intVal = ARKTS_CreateI32(100);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(intVal)), 100);
}

TEST_F(ArkInteropTest, GetValueTypeFunction)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto funcId = local.StoreFunc([](ARKTS_CallInfo) { return ARKTS_CreateUndefined(); });
    auto func = ARKTS_CreateFunc(env, funcId);
    auto t = ARKTS_GetValueType(env, func);
    EXPECT_EQ(t, N_FUNCTION);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetValueTypeNull)
{
    MockContext local;
    auto env = local.GetEnv();
    auto t = ARKTS_GetValueType(env, ARKTS_CreateNull());
    EXPECT_EQ(t, N_NULL);
}

TEST_F(ArkInteropTest, GetValueTypeObject)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto obj = ARKTS_CreateObject(env);
    auto t = ARKTS_GetValueType(env, obj);
    EXPECT_EQ(t, N_OBJECT);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetValueTypeVariants)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_EQ(ARKTS_GetValueType(env, ARKTS_CreateUndefined()), N_UNDEFINED);
    EXPECT_EQ(ARKTS_GetValueType(env, ARKTS_CreateNull()), N_NULL);
    EXPECT_EQ(ARKTS_GetValueType(env, ARKTS_CreateBool(true)), N_BOOL);
    EXPECT_EQ(ARKTS_GetValueType(env, ARKTS_CreateI32(42)), N_NUMBER);
    EXPECT_EQ(ARKTS_GetValueType(env, ARKTS_CreateF64(3.14)), N_NUMBER);

    auto str = ARKTS_CreateUtf8(env, "test", 4);
    EXPECT_EQ(ARKTS_GetValueType(env, str), N_STRING);

    auto sym = ARKTS_CreateSymbol(env, "sym", 3);
    EXPECT_EQ(ARKTS_GetValueType(env, sym), N_SYMBOL);

    auto obj = ARKTS_CreateObject(env);
    EXPECT_EQ(ARKTS_GetValueType(env, obj), N_OBJECT);

    auto funcId = local.StoreFunc([](ARKTS_CallInfo) -> ARKTS_Value {
        return ARKTS_CreateUndefined();
    });
    auto func = ARKTS_CreateFunc(env, funcId);
    EXPECT_EQ(ARKTS_GetValueType(env, func), N_FUNCTION);

    auto ext = ARKTS_CreateExternal(env, 42);
    EXPECT_EQ(ARKTS_GetValueType(env, ext), N_EXTERNAL);

    auto bigint = ARKTS_CreateBigInt(env, 99);
    EXPECT_EQ(ARKTS_GetValueType(env, bigint), N_BIGINT);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsHeapObjectWithPrimitive)
{
    EXPECT_FALSE(ARKTS_IsHeapObject(ARKTS_CreateI32(42)));
    EXPECT_FALSE(ARKTS_IsHeapObject(ARKTS_CreateBool(true)));
}

TEST_F(ArkInteropTest, IsNumberVariants)
{
    EXPECT_TRUE(ARKTS_IsNumber(ARKTS_CreateI32(0)));
    EXPECT_TRUE(ARKTS_IsNumber(ARKTS_CreateF64(0.0)));
    EXPECT_FALSE(ARKTS_IsNumber(ARKTS_CreateBool(true)));
    EXPECT_FALSE(ARKTS_IsNumber(ARKTS_CreateNull()));
    EXPECT_FALSE(ARKTS_IsNumber(ARKTS_CreateUndefined()));
}

TEST_F(ArkInteropTest, StrictEqualBooleans)
{
    MockContext local;
    auto env = local.GetEnv();
    EXPECT_TRUE(ARKTS_StrictEqual(env, ARKTS_CreateBool(true), ARKTS_CreateBool(true)));
    EXPECT_FALSE(ARKTS_StrictEqual(env, ARKTS_CreateBool(true), ARKTS_CreateBool(false)));
}

TEST_F(ArkInteropTest, StrictEqualDifferentTypes)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_StrictEqual(env, ARKTS_CreateI32(1), ARKTS_CreateUtf8(env, "1", 1)));
    EXPECT_FALSE(ARKTS_StrictEqual(env, ARKTS_CreateBool(true), ARKTS_CreateI32(1)));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, StrictEqualHeapObjects)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto str1 = ARKTS_CreateUtf8(env, "hello", 5);
    auto str2 = ARKTS_CreateUtf8(env, "hello", 5);
    EXPECT_TRUE(ARKTS_StrictEqual(env, str1, str2));

    auto str3 = ARKTS_CreateUtf8(env, "world", 5);
    EXPECT_FALSE(ARKTS_StrictEqual(env, str1, str3));

    auto obj1 = ARKTS_CreateObject(env);
    auto obj2 = ARKTS_CreateObject(env);
    EXPECT_TRUE(ARKTS_StrictEqual(env, obj1, obj1));
    EXPECT_FALSE(ARKTS_StrictEqual(env, obj1, obj2));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, StrictEqualNumbers)
{
    MockContext local;
    auto env = local.GetEnv();
    EXPECT_TRUE(ARKTS_StrictEqual(env, ARKTS_CreateI32(42), ARKTS_CreateI32(42)));
    EXPECT_FALSE(ARKTS_StrictEqual(env, ARKTS_CreateI32(1), ARKTS_CreateI32(2)));
    EXPECT_TRUE(ARKTS_StrictEqual(env, ARKTS_CreateF64(3.14), ARKTS_CreateF64(3.14)));
}

TEST_F(ArkInteropTest, StrictEqualSameUndefined)
{
    MockContext local;
    auto env = local.GetEnv();
    EXPECT_TRUE(ARKTS_StrictEqual(env, ARKTS_CreateUndefined(), ARKTS_CreateUndefined()));
    EXPECT_TRUE(ARKTS_StrictEqual(env, ARKTS_CreateNull(), ARKTS_CreateNull()));
}

// === Primitive Types Tests ===

TEST_F(ArkInteropTest, BoolOperations)
{
    auto t = ARKTS_CreateBool(true);
    auto f = ARKTS_CreateBool(false);
    EXPECT_TRUE(ARKTS_IsBool(t));
    EXPECT_TRUE(ARKTS_IsBool(f));
    EXPECT_TRUE(ARKTS_GetValueBool(t));
    EXPECT_FALSE(ARKTS_GetValueBool(f));
    EXPECT_FALSE(ARKTS_IsBool(ARKTS_CreateI32(0)));
}

TEST_F(ArkInteropTest, CreateF64)
{
    auto val = ARKTS_CreateF64(3.14);
    EXPECT_TRUE(ARKTS_IsNumber(val));
    EXPECT_NEAR(ARKTS_GetValueNumber(val), 3.14, 0.001);
}

TEST_F(ArkInteropTest, NullAndUndefined)
{
    auto n = ARKTS_CreateNull();
    auto u = ARKTS_CreateUndefined();
    EXPECT_TRUE(ARKTS_IsNull(n));
    EXPECT_TRUE(ARKTS_IsUndefined(u));
    EXPECT_FALSE(ARKTS_IsNull(u));
    EXPECT_FALSE(ARKTS_IsUndefined(n));
    EXPECT_FALSE(ARKTS_IsNull(ARKTS_CreateI32(0)));
    EXPECT_FALSE(ARKTS_IsUndefined(ARKTS_CreateI32(0)));
}

// === CJ Callbacks Tests ===

TEST_F(ArkInteropTest, SetCJModuleCallbackDuplicate)
{
    ARKTS_ModuleCallbacks dummyCallbacks {};
    ARKTS_SetCJModuleCallback(&dummyCallbacks);
}

// === Event/Async Tests ===

TEST_F(ArkInteropTest, CreateAsyncTaskViaEventHandler)
{
    auto dummyEnv = reinterpret_cast<ARKTS_Env>(static_cast<uintptr_t>(0xBEEF));
    ARKTS_InitEventHandle(dummyEnv);
    ARKTS_CreateAsyncTask(dummyEnv, 0);
}

TEST_F(ArkInteropTest, OnTriggeredNoTimer)
{
    MockContext local;
    auto engine = local.GetEngine();
    void* napiEnv = ARKTS_GetNAPIEnv(engine);
    ASSERT_NE(napiEnv, nullptr);

    auto nativeEngine = reinterpret_cast<NativeEngine*>(napiEnv);
    auto loop = nativeEngine->GetUVLoop();
    ASSERT_NE(loop, nullptr);

    auto runner = OHOS::AppExecFwk::EventRunner::Create(false);
    auto eventHandler = std::make_shared<OHOS::AppExecFwk::EventHandler>(runner);
    ASSERT_NE(eventHandler, nullptr);

    auto uvHandler = std::make_shared<UVLoopHandler>(loop);
    int pipefd[2];
    if (pipe(pipefd) == 0) {
        uint32_t events = OHOS::AppExecFwk::FILE_DESCRIPTOR_INPUT_EVENT;
        eventHandler->AddFileDescriptorListener(pipefd[0], events, uvHandler, "testNoTimer");

        uvHandler->OnReadable(pipefd[0]);
        uvHandler->OnWritable(pipefd[0]);

        eventHandler->RemoveFileDescriptorListener(pipefd[0]);
        close(pipefd[0]);
        close(pipefd[1]);
    }
}

// === Lifecycle Tests ===

TEST_F(ArkInteropTest, DisposeEventHandlerViaEngineDestroy)
{
    MockContext local(ARKTS_CreateEngineWithNewThread(), true);
}

TEST_F(ArkInteropTest, DisposeJSContextViaEngineDestroy)
{
    MockContext local(ARKTS_CreateEngineWithNewThread(), true);
}

// === Misc Tests ===

TEST_F(ArkInteropTest, FreeCStringNull)
{
    ARKTS_FreeCString(nullptr);
}

TEST_F(ArkInteropTest, GetPosixThreadId)
{
    auto id = ARKTS_GetPosixThreadId();
    EXPECT_GT(id, 0ULL);
}

// === Other Tests ===

TEST_F(ArkInteropTest, ClearWeakOnNonWeak)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, obj);
    ARKTS_GlobalClearWeak(env, global);
    EXPECT_TRUE(ARKTS_GlobalIsAlive(env, global));

    ARKTS_DisposeGlobalSync(env, global);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsStringWithNonHeapValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsString(env, ARKTS_CreateI32(42)));
    EXPECT_FALSE(ARKTS_IsString(env, ARKTS_CreateBool(true)));
    EXPECT_FALSE(ARKTS_IsString(env, ARKTS_CreateNull()));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, SetAndGetElement)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto arr = ARKTS_CreateArray(env, 5);
    ARKTS_SetElement(env, arr, 0, ARKTS_CreateI32(100));
    ARKTS_SetElement(env, arr, 4, ARKTS_CreateI32(500));

    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(ARKTS_GetElement(env, arr, 0))), 100);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(ARKTS_GetElement(env, arr, 4))), 500);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, SetAndGetElementValues)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto arr = ARKTS_CreateArray(env, 3);
    ARKTS_SetElement(env, arr, 0, ARKTS_CreateI32(10));
    ARKTS_SetElement(env, arr, 1, ARKTS_CreateI32(20));
    ARKTS_SetElement(env, arr, 2, ARKTS_CreateI32(30));

    auto v0 = ARKTS_GetElement(env, arr, 0);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(v0)), 10);
    auto v2 = ARKTS_GetElement(env, arr, 2);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(v2)), 30);

    ARKTS_CloseScope(env, scope);
}


} // namespace// mocks
namespace panda {
bool JSNApi::IsMixedDebugEnabled(const EcmaVM* vm)
{
    return true;
}

void JSNApi::NotifyNativeCalling(const EcmaVM* vm, const void* nativeAddress)
{
    MockContext::GetInstance()->NotifyNativeCalling(nativeAddress);
}

void JSNApi::NotifyNativeReturn(const EcmaVM* vm, const void* nativeAddress)
{
    MockContext::GetInstance()->NotifyNativeReturn(nativeAddress);
}
}

// Call exported APIs (that use ARKTS_ASSERT_* macros) before MockContext::Init(),
// to cover the early-return branches when parameters are invalid (e.g. env is null).
static void TriggerArktsAssertEarlyReturnBranches()
{
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

    ARKTS_Engine tempEngine = ARKTS_CreateEngine();
    ARKTS_Env normalEnv = tempEngine ? ARKTS_GetContext(tempEngine) : nullptr;

    // Basic
    (void)ARKTS_GetGlobalConstant(nullEnv);
    (void)ARKTS_GetGlobalNapiEnv(nullEnv);
    (void)ARKTS_GetValueType(nullEnv, zeroValue);
    (void)ARKTS_StrictEqual(nullEnv, zeroValue, zeroValue);

    // String
    (void)ARKTS_GetValueUtf8(nullEnv, zeroValue, 0, smallBuf);
    (void)ARKTS_GetValueUtf8Size(nullEnv, zeroValue);
    (void)ARKTS_CreateUtf8(nullEnv, nullptr, 0);
    (void)ARKTS_GetValueCString(nullEnv, zeroValue);
    (void)ARKTS_IsString(nullEnv, zeroValue);
    (void)ARKTS_GetStringInfo(nullEnv, zeroValue);
    ARKTS_StringCopy(nullEnv, zeroValue, smallBuf, 0);
    (void)ARKTS_CreateString(nullEnv, false, 0, nullptr);
    if (normalEnv) {
        (void)ARKTS_CreateUtf8(normalEnv, nullptr, 0);
        (void)ARKTS_IsString(normalEnv, zeroValue);
        (void)ARKTS_GetValueUtf8Size(normalEnv, zeroValue);
        (void)ARKTS_GetValueUtf8(normalEnv, zeroValue, 1, smallBuf);
        (void)ARKTS_GetValueCString(normalEnv, zeroValue);
        (void)ARKTS_CreateString(normalEnv, false, 0, nullptr);
        auto oneChar = ARKTS_CreateUtf8(normalEnv, "a", 1);
        (void)ARKTS_GetValueUtf8(normalEnv, oneChar, 1, nullptr);
    }

    // Function/Class
    (void)ARKTS_CreateFunc(nullEnv, 0);
    (void)ARKTS_CreateExternal(nullEnv, 0);
    (void)ARKTS_IsCallable(nullEnv, zeroValue);
    (void)ARKTS_IsClass(nullEnv, zeroValue);
    (void)ARKTS_CreateClass(nullEnv, 0, zeroValue);
    (void)ARKTS_GetPrototype(nullEnv, zeroValue);
    (void)ARKTS_InstanceOf(nullEnv, zeroValue, zeroValue);

    // CallInfo
    (void)ARKTS_GetCallEnv(nullInfo);
    (void)ARKTS_GetArgCount(nullInfo);
    (void)ARKTS_GetArg(nullInfo, 0);
    (void)ARKTS_GetThisArg(nullInfo);

    // Call/New
    (void)ARKTS_Call(nullEnv, zeroValue, zeroValue, 0, nullptr);
    (void)ARKTS_New(nullEnv, zeroValue, 0, nullptr);

    // Object
    (void)ARKTS_CreateObject(nullEnv);
    (void)ARKTS_IsObject(nullEnv, zeroValue);
    (void)ARKTS_HasOwnProperty(nullEnv, zeroValue, zeroValue);
    (void)ARKTS_EnumOwnProperties(nullEnv, zeroValue);
    ARKTS_DefineOwnProperty(nullEnv, zeroValue, zeroValue, zeroValue, N_WRITABLE);
    (void)ARKTS_DefineOwnPropertyV2(nullEnv, zeroValue, zeroValue, zeroValue, N_WRITABLE);
    ARKTS_DefineAccessors(nullEnv, zeroValue, zeroValue, accessor);
    (void)ARKTS_DefineAccessorsV2(nullEnv, zeroValue, zeroValue, accessor);
    ARKTS_SetProperty(nullEnv, zeroValue, zeroValue, zeroValue);
    (void)ARKTS_GetProperty(nullEnv, zeroValue, zeroValue);
    if (normalEnv) {
        auto invalidObj = ARKTS_CreateUndefined();
        auto invalidKey = ARKTS_CreateUndefined();
        (void)ARKTS_IsObject(normalEnv, zeroValue);
        (void)ARKTS_HasOwnProperty(normalEnv, invalidObj, invalidKey);
        (void)ARKTS_EnumOwnProperties(normalEnv, invalidObj);
        ARKTS_DefineOwnProperty(normalEnv, invalidObj, invalidKey, zeroValue, N_WRITABLE);
        (void)ARKTS_DefineOwnPropertyV2(normalEnv, invalidObj, invalidKey, zeroValue, N_WRITABLE);
        ARKTS_DefineAccessors(normalEnv, invalidObj, invalidKey, accessor);
        (void)ARKTS_DefineAccessorsV2(normalEnv, invalidObj, invalidKey, accessor);
        ARKTS_SetProperty(normalEnv, invalidObj, invalidKey, zeroValue);
        (void)ARKTS_GetProperty(normalEnv, invalidObj, invalidKey);
        auto validObj = ARKTS_CreateObject(normalEnv);
        (void)ARKTS_HasOwnProperty(normalEnv, validObj, invalidKey);
        ARKTS_DefineOwnProperty(normalEnv, validObj, invalidKey, zeroValue, N_WRITABLE);
        (void)ARKTS_DefineOwnPropertyV2(normalEnv, validObj, invalidKey, zeroValue, N_WRITABLE);
        ARKTS_DefineAccessors(normalEnv, validObj, invalidKey, accessor);
        (void)ARKTS_DefineAccessorsV2(normalEnv, validObj, invalidKey, accessor);
        ARKTS_SetProperty(normalEnv, validObj, invalidKey, zeroValue);
        (void)ARKTS_GetProperty(normalEnv, validObj, invalidKey);
    }

    // Array
    (void)ARKTS_CreateArray(nullEnv, 0);
    (void)ARKTS_CreateArrayWithInit(nullEnv, 0, oneArg);
    (void)ARKTS_GetArrayLength(nullEnv, zeroValue);
    ARKTS_SetElement(nullEnv, zeroValue, 0, zeroValue);
    (void)ARKTS_GetElement(nullEnv, zeroValue, 0);
    (void)ARKTS_IsArray(nullEnv, zeroValue);

    // Global
    (void)ARKTS_CreateGlobal(nullEnv, zeroValue);
    (void)ARKTS_GlobalIsAlive(nullEnv, nullGlobal);
    if (normalEnv) {
        (void)ARKTS_GlobalIsAlive(normalEnv, nullGlobal);
    }
    (void)ARKTS_GetGlobalValue(nullGlobal);
    ARKTS_DisposeGlobal(nullEnv, nullGlobal);
    if (normalEnv) {
        ARKTS_DisposeGlobal(normalEnv, nullGlobal);
    }
    ARKTS_DisposeGlobalSync(nullEnv, nullGlobal);
    if (normalEnv) {
        ARKTS_DisposeGlobalSync(normalEnv, nullGlobal);
    }
    ARKTS_GlobalSetWeak(nullEnv, nullGlobal);
    if (normalEnv) {
        ARKTS_GlobalSetWeak(normalEnv, nullGlobal);
    }
    ARKTS_GlobalClearWeak(nullEnv, nullGlobal);
    if (normalEnv) {
        ARKTS_GlobalClearWeak(normalEnv, nullGlobal);
    }
    (void)ARKTS_GlobalToValue(nullEnv, nullGlobal);
    if (normalEnv) {
        (void)ARKTS_GlobalToValue(normalEnv, nullGlobal);
    }
    (void)ARKTS_GlobalFromValue(nullEnv, zeroValue);

    // External
    (void)ARKTS_IsExternal(nullEnv, zeroValue);
    (void)ARKTS_GetExternalData(nullEnv, zeroValue);

    // Symbol
    (void)ARKTS_CreateSymbol(nullEnv, nullptr, 0);
    (void)ARKTS_IsSymbol(nullEnv, zeroValue);
    (void)ARKTS_GetSymbolDesc(nullEnv, zeroValue);

    // Scope
    (void)ARKTS_OpenScope(nullEnv);
    ARKTS_CloseScope(nullEnv, nullScope);
    if (normalEnv) {
        ARKTS_CloseScope(normalEnv, nullScope);
    }

    // Exception
    (void)ARKTS_GetExceptionAndClear(nullEnv);
    (void)ARKTS_RequireArkModule(nullEnv, nullptr, 0, ARKTS_NormalModule);

    // Engine/Module
    (void)ARKTS_GetNAPIEnv(nullEngine);
    ARKTS_DestroyEngine(nullEngine);
    (void)ARKTS_GetContext(nullEngine);
    (void)ARKTS_GetThreadIdOfEngine(nullEngine);
    (void)ARKTS_LoadEntryFromAbc(nullEngine, nullptr, nullptr, false);
    (void)ARKTS_ImportFromEntry(nullEngine, nullptr, nullptr);
    (void)ARKTS_Require(nullEnv, nullptr, false, false, nullptr);

    // Promise
    (void)ARKTS_CreatePromiseCapability(nullEnv);
    (void)ARKTS_GetPromiseFromCapability(nullEnv, nullPromise);
    if (normalEnv) {
        (void)ARKTS_GetPromiseFromCapability(normalEnv, nullPromise);
    }
    ARKTS_PromiseCapabilityResolve(nullEnv, nullPromise, zeroValue);
    if (normalEnv) {
        ARKTS_PromiseCapabilityResolve(normalEnv, nullPromise, zeroValue);
    }
    ARKTS_PromiseCapabilityReject(nullEnv, nullPromise, zeroValue);
    if (normalEnv) {
        ARKTS_PromiseCapabilityReject(normalEnv, nullPromise, zeroValue);
    }
    (void)ARKTS_IsPromise(nullEnv, zeroValue);
    (void)ARKTS_PromiseThen(nullEnv, zeroValue, zeroValue, zeroValue);
    (void)ARKTS_PromiseCatch(nullEnv, zeroValue, zeroValue);
    ARKTS_CreateAsyncTask(nullEnv, 0);
    ARKTS_InitEventHandle(nullEnv);

    // ArrayBuffer/BigInt
    (void)ARKTS_CreateArrayBuffer(nullEnv, 0);
    (void)ARKTS_CreateArrayBufferWithData(nullEnv, nullptr, 0, 0);
    (void)ARKTS_IsArrayBuffer(nullEnv, zeroValue);
    (void)ARKTS_GetArrayBufferLength(nullEnv, zeroValue);
    (void)ARKTS_GetArrayBufferRawPtr(nullEnv, zeroValue);
    (void)ARKTS_ArrayBufferReadBytes(nullEnv, zeroValue, smallBuf, 0);
    (void)ARKTS_CreateBigInt(nullEnv, 0);
    (void)ARKTS_CreateBigIntWithBytes(nullEnv, false, 0, smallBytes);
    (void)ARKTS_IsBigInt(nullEnv, zeroValue);
    (void)ARKTS_BigIntGetByteSize(nullEnv, zeroValue);
    ARKTS_BigIntReadBytes(nullEnv, zeroValue, &isNegative, 0, smallBytes);

    // CycleFree
    (void)ARKTS_CreateCycleFreeFunc(nullEnv, 0);
    (void)ARKTS_CreateCycleFreeExtern(nullEnv, 0);

    // Misc
    ARKTS_UpdateStackInfo(0, nullptr, 0);

    if (tempEngine) {
        ARKTS_DestroyEngine(tempEngine);
    }
}

int main(int argc, char** argv)
{
    LOGI("main in");
    testing::GTEST_FLAG(output) = "xml:./";
    testing::InitGoogleTest(&argc, argv);

    OHOS::CJEnvironment::SetAppPath("/data/test");

    auto runner = OHOS::AppExecFwk::EventRunner::Create(true);
    EXPECT_TRUE(runner);
    auto handler = std::make_shared<OHOS::AppExecFwk::EventHandler>(runner);
    EXPECT_TRUE(handler);

    TriggerArktsAssertEarlyReturnBranches();

    MockContext::Init();

    int ret = -1;
    std::condition_variable cv;

    ARKTS_Engine globalEngine;

    auto success = handler->PostTask([&ret, &cv, &globalEngine] {
        MockContext global(ARKTS_CreateEngine(), false);
        globalEngine = global.GetEngine();
        ret = testing::UnitTest::GetInstance()->Run();
        cv.notify_all();
    });

    EXPECT_TRUE(success);

    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    auto status = cv.wait_for(lock, std::chrono::seconds(30));

    EXPECT_EQ(status, std::cv_status::no_timeout);
    ARKTS_DestroyEngine(globalEngine);

    runner->Stop();

    LOGI("main out");
    return ret;
}
