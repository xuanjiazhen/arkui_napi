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

#include <thread>

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

namespace {
ARKTS_Engine engine_ = nullptr;

} // namespace

class ArkInteropTest : public testing::Test {};

HWTEST_F(ArkInteropTest, ArkTSInteropNapiAsync001, TestSize.Level1)
{
    ARKTS_Env env = ARKTS_GetContext(engine_);
    ARKTS_CreateAsyncTask(env, 0);
    ARKTS_CreateAsyncTask(nullptr, 0);
    ARKTS_CreateAsyncTask(env, 0);
}

HWTEST_F(ArkInteropTest, ArkTSInteropNapi001, TestSize.Level1)
{
    ARKTS_Env env = ARKTS_GetContext(engine_);
    ARKTS_GetGlobalConstant(env);
    ARKTS_InitEventHandle(env);
}

HWTEST_F(ArkInteropTest, ArkTSInteropNapi005, TestSize.Level1)
{
    ARKTS_Env env = ARKTS_GetContext(engine_);
    EXPECT_NE(ARKTS_GetNAPIEnv(engine_), nullptr);

    auto scope = ARKTS_OpenScope(env);
    auto subscope = ARKTS_OpenScope(env);
    ARKTS_CloseScope(env, subscope);
    auto val = ARKTS_CreateF64(12.34);
    EXPECT_NE(ARKTS_Return(env, scope, val), nullptr);
}

HWTEST_F(ArkInteropTest, ArkTSInteropNapi006, TestSize.Level1)
{
    ARKTS_Env env = ARKTS_GetContext(engine_);
    ARKTS_Value objv = ARKTS_CreateObject(env);
    char origeStr[] = "key01";
    auto key = ARKTS_CreateUtf8(env, origeStr, strlen(origeStr));
    ARKTS_Value get = ARKTS_CreateFunc(env, 0);
    ARKTS_Value set = ARKTS_CreateFunc(env, 0);
    ARKTS_DefineAccessors(env, objv, key, {get, set, ARKTS_PropertyFlag::N_ENUMERABLE});
}

HWTEST_F(ArkInteropTest, ArkTSInteropNapi007, TestSize.Level1)
{
    ARKTS_Env env = ARKTS_GetContext(engine_);
    ARKTS_GetValueType(env, ARKTS_CreateNull());
    ARKTS_GetValueType(env, ARKTS_CreateUndefined());

    char des[] = "symbol test";
    auto symv = ARKTS_CreateSymbol(env, des, strlen(des));
    ARKTS_GetValueType(env, symv);
    ARKTS_StrictEqual(env, symv, symv);

    auto boolv = ARKTS_CreateBool(false);
    ARKTS_GetValueType(env, boolv);
    ARKTS_StrictEqual(env, boolv, boolv);
}

HWTEST_F(ArkInteropTest, ArkTSInteropNapiExternal001, TestSize.Level1)
{
    ARKTS_Env env = ARKTS_GetContext(engine_);
    char dllName[] = "123";
    ARKTS_LoadModule(env, dllName);
}

HWTEST_F(ArkInteropTest, ArkTSInteropNapiInstanceOf, TestSize.Level1)
{
    ARKTS_Env env = ARKTS_GetContext(engine_);
    auto global = ARKTS_GetGlobalConstant(env);
    char clError[] = "Error";
    auto jError = ARKTS_CreateUtf8(env, clError, sizeof(clError));
    auto errorCls = ARKTS_GetProperty(env, global, jError);
    auto errorObJ = ARKTS_New(env, errorCls, 0, nullptr);
    auto isError = ARKTS_InstanceOf(env, errorObJ, errorCls);
    EXPECT_TRUE(isError);
    EXPECT_FALSE(ARKTS_InstanceOf(env, jError, errorCls));
}

HWTEST_F(ArkInteropTest, ArkTSInteropNapiHitrace001, TestSize.Level1)
{
    char name[] = "";
    int32_t taskId = 0;
    ARKTS_HiTraceStartTrace(name, taskId);
    ARKTS_HiTraceFinishTrace(name, taskId);
    ARKTS_HiTraceCountTrace(name, taskId);
}

HWTEST_F(ArkInteropTest, ArkTSInteropNapiLog001, TestSize.Level1)
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);

    bool called = false;
    auto id = MockContext::GetInstance()->StoreFunc([&called](ARKTS_CallInfo info)->ARKTS_Value {
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
    auto id = MockContext::GetInstance()->StoreFunc([&resolved](ARKTS_CallInfo info)->ARKTS_Value {
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

    constexpr int checkDuration = 10;
    int waitTimes = 10; // set 100ms timeout
    while (!isComplete && waitTimes--) {
        cv.wait_for(lock, std::chrono::milliseconds(checkDuration));
    }
    // EXPECT no core dump, no timeout
    EXPECT_TRUE(waitTimes > 0);
}

HWTEST_F(ArkInteropTest, ArkTSInteropNapiCreateEngineNew, TestSize.Level1)
{
    auto engine = ARKTS_CreateEngineWithNewThread();

    auto curTid = ARKTS_GetPosixThreadId();
    auto engineTid = ARKTS_GetThreadIdOfEngine(engine);

    EXPECT_NE(curTid, engineTid);

    ARKTS_DestroyEngine(engine);
}

int main(int argc, char** argv)
{
    LOGI("main in");
    testing::GTEST_FLAG(output) = "xml:./";
    testing::InitGoogleTest(&argc, argv);

    auto runner = OHOS::AppExecFwk::EventRunner::Create(true);
    EXPECT_TRUE(runner);
    auto handler = std::make_shared<OHOS::AppExecFwk::EventHandler>(runner);
    EXPECT_TRUE(handler);

    int ret = -1;
    std::condition_variable cv;

    auto success = handler->PostTask([&ret, &cv] {
        engine_ = ARKTS_CreateEngine();

        ret = testing::UnitTest::GetInstance()->Run();
        cv.notify_all();
    });

    EXPECT_TRUE(success);

    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    auto status = cv.wait_for(lock, std::chrono::seconds(10));

    EXPECT_EQ(status, std::cv_status::no_timeout);

    ARKTS_DestroyEngine(engine_);
    engine_ = nullptr;
    runner->Stop();

    LOGI("main out");
    return ret;
}
