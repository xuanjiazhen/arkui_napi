/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "native_utils.h"
#include "uv.h"
#define private public
#define protected public
#include "test.h"
#undef private
#include "test_common.h"
#include "utils/log.h"

// use macro instead of constexpr variable, due to need concat with other string
#define TEST_NAPI_UNCLOSED_CRITICAL_LOG " current interface cannot invoke under critical scope"
#define TEST_UNCLOSED_CRITICAL_CALLBACK_LOG "current interface cannot invoke under critical scope"

class NapiCriticalTest : public NativeEngineTest {
public:
    NapiCriticalTest() : NativeEngineTest(), runner_(engine_) {}
    ~NapiCriticalTest()
    {
        if (uv_loop_alive(runner_)) {
        }
        runner_.Run(UV_RUN_NOWAIT);
    }
    void SetUp() override
    {
        napi_open_critical_scope(reinterpret_cast<napi_env>(engine_), &scope_);
    }
    void TearDown() override
    {
        napi_close_critical_scope(reinterpret_cast<napi_env>(engine_), scope_);
    }

private:
public:
    void TriggerFailed()
    {
        GTEST_LOG_(ERROR) << "NapiCriticalTest failed: " << GetTestCaseName();
        ASSERT_TRUE(false);
    }
    UVLoopRunner runner_;
    napi_critical_scope scope_ = nullptr;
};

napi_value EmptyNapiCallback(napi_env env, napi_callback_info info)
{
    return nullptr;
}

template<typename T>
void FinalizeCallback([[maybe_unused]] napi_env env, void* data, [[maybe_unused]] void* hint)
{
    if (data == nullptr) {
        return;
    }
    delete reinterpret_cast<T*>(data);
}

/**
 * @tc.name: NapiUnclosedCriticalTest001
 * @tc.desc: Test check of unclosed critical scope after user callback.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiUnclosedCriticalTest001, testing::ext::TestSize.Level0)
{
    BasicDeathTest([]() {
        NativeEngineProxy env;
        static napi_critical_scope scope {};
        napi_value fn {};
        napi_create_function(
            env, "testFunc", NAPI_AUTO_LENGTH,
            [](napi_env env, [[maybe_unused]] napi_callback_info info) -> napi_value {
                napi_open_critical_scope(env, &scope);
                return nullptr;
            },
            nullptr, &fn);
        napi_call_function(env, nullptr, fn, 0, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError("[ArkNativeFunctionCallBack] critical scope still open after user callback 'testFunc'");
}

/**
 * @tc.name: NapiUnclosedCriticalTest002
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiUnclosedCriticalTest002, testing::ext::TestSize.Level0)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        UVLoopRunner runner(*env);
        napi_value name {};
        napi_create_string_utf8(env, "taskName", NAPI_AUTO_LENGTH, &name);
        static napi_critical_scope scope {};
        napi_async_work work {};
        napi_create_async_work(
            env, nullptr, name, []([[maybe_unused]] napi_env env, [[maybe_unused]] void* data) {},
            [](napi_env env, [[maybe_unused]] napi_status status, [[maybe_unused]] void* data) {
                napi_open_critical_scope(env, &scope);
            },
            nullptr, &work);
        napi_queue_async_work(env, work);
        runner.Run(UV_RUN_ONCE);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError("[AsyncAfterWorkCallback] critical scope still open after user callback");
}

/**
 * @tc.name: NapiUnclosedCriticalTest003
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiUnclosedCriticalTest003, testing::ext::TestSize.Level0)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        UVLoopRunner runner(*env);
        static napi_critical_scope scope {};
        napi_value undefined {};
        napi_get_undefined(env, &undefined);
        napi_threadsafe_function tsfn {};
        napi_create_threadsafe_function(
            env, nullptr, nullptr, undefined, 0, 1, nullptr, nullptr, nullptr,
            [](napi_env env, [[maybe_unused]] napi_value func, [[maybe_unused]] void* context,
               [[maybe_unused]] void* data) { napi_open_critical_scope(env, &scope); },
            &tsfn);
        napi_call_threadsafe_function(tsfn, nullptr, napi_tsfn_blocking);
        runner.Run(UV_RUN_ONCE);
        napi_release_threadsafe_function(tsfn, napi_tsfn_release);
        runner.Run(UV_RUN_ONCE);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError("critical scope still open after user callback");
}

/**
 * @tc.name: NapiUnclosedCriticalTest004
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiUnclosedCriticalTest004, testing::ext::TestSize.Level0)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        UVLoopRunner runner(*env);
        auto eventRunner = OHOS::AppExecFwk::EventRunner::Create(false);
        auto eventHandler = std::make_shared<OHOS::AppExecFwk::EventHandler>(eventRunner);

        static napi_critical_scope scope {};
        napi_threadsafe_function tsfn {};
        eventHandler->PostTask([&env, &tsfn] {
            napi_value undefined {};
            napi_get_undefined(env, &undefined);
            napi_create_threadsafe_function(
                env, nullptr, nullptr, undefined, 0, 1, nullptr, nullptr, nullptr,
                [](napi_env env, [[maybe_unused]] napi_value func, [[maybe_unused]] void* context,
                   [[maybe_unused]] void* data) {
                    OHOS::AppExecFwk::EventRunner::Current()->Stop();
                    napi_open_critical_scope(env, &scope);
                },
                &tsfn);
            napi_call_threadsafe_function_with_priority(tsfn, nullptr, napi_priority_immediate, false);
            napi_release_threadsafe_function(tsfn, napi_tsfn_release);
        });
        eventRunner->Run();
        runner.Run(UV_RUN_ONCE);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError("critical scope still open after user callback");
}

/**
 * @tc.name: NapiNonCriticalTest001
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest001, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_env result {};
        napi_open_critical_scope(env, &scope);
        napi_create_limit_runtime(env, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest002
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest002, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value error {};
        napi_create_error(env, "", "", &error);
        napi_open_critical_scope(env, &scope);
        napi_fatal_exception(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest003
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest003, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value name {};
        napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &name);
        napi_open_critical_scope(env, &scope);
        napi_async_work work {};
        napi_create_async_work(
            env, nullptr, name, []([[maybe_unused]] napi_env env, [[maybe_unused]] void* data) {},
            []([[maybe_unused]] napi_env env, [[maybe_unused]] napi_status status, [[maybe_unused]] void* data) {},
            nullptr, &work);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest004
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest004, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value name {};
        napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &name);
        napi_threadsafe_function tsfn {};
        napi_open_critical_scope(env, &scope);
        // use non-nullptr napi_value for func, skip check of call_js_cb
        napi_create_threadsafe_function(env, name, nullptr, name, 0, 1, nullptr, nullptr, nullptr, nullptr, &tsfn);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest005
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest005, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value name {};
        napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &name);
        napi_open_critical_scope(env, &scope);
        napi_async_context result {};
        napi_async_init(env, nullptr, name, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest006
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest006, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        // type check is after env check
        napi_value undefined {};
        napi_open_critical_scope(env, &scope);
        napi_make_callback(env, nullptr, undefined, undefined, 0, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest007
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest007, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_get_undefined(env, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest008
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest008, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_get_null(env, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest009
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest009, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_get_global(env, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest010
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest010, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_get_boolean(env, true, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest011
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest011, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_object(env, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest012
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest012, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_object_with_properties(env, nullptr, 0, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest013
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest013, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_object_with_named_properties(env, nullptr, 0, nullptr, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest014
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest014, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_array(env, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest015
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest015, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_array_with_length(env, 0, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest016
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest016, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_sendable_array(env, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest017
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest017, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_sendable_array_with_length(env, 0, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest018
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest018, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_double(env, 0, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest019
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest019, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_int32(env, 0, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest020
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest020, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_uint32(env, 0, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest021
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest021, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_int64(env, 0, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest022
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest022, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_string_latin1(env, "", 0, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest023
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest023, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_string_utf8(env, "", 0, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest024
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest024, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_string_utf16(env, u"", 0, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest025
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest025, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_symbol(env, nullptr, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest026
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest026, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_function(env, "", 0, EmptyNapiCallback, nullptr, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest027
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest027, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value name {};
        napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &name);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_error(env, name, name, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest028
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest028, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value name {};
        napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &name);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_type_error(env, name, name, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest029
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest029, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value name {};
        napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &name);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_range_error(env, name, name, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest030
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest030, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value undefined {};
        napi_get_undefined(env, &undefined);
        napi_open_critical_scope(env, &scope);
        napi_valuetype type {};
        napi_typeof(env, undefined, &type);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest031
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest031, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value name {};
        napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &name);
        napi_open_critical_scope(env, &scope);
        napi_get_value_string_latin1(env, name, nullptr, 0, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest032
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest032, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value name {};
        napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &name);
        napi_open_critical_scope(env, &scope);
        napi_get_value_string_utf8(env, name, nullptr, 0, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest033
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest033, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value name {};
        napi_create_string_utf8(env, "", NAPI_AUTO_LENGTH, &name);
        napi_open_critical_scope(env, &scope);
        napi_get_value_string_utf16(env, name, nullptr, 0, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest034
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest034, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_critival_scope result {};
        napi_open_critical_scope(env, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest035
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest035, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value undefined {};
        napi_get_undefined(env, &undefined);
        napi_value result {};
        napi_open_critical_scope(env, &scope);
        napi_coerce_to_bool(env, &undefined, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest036
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest036, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value undefined {};
        napi_get_undefined(env, &undefined);
        napi_value result {};
        napi_open_critical_scope(env, &scope);
        napi_coerce_to_number(env, &undefined, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest037
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest037, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value undefined {};
        napi_get_undefined(env, &undefined);
        napi_value result {};
        napi_open_critical_scope(env, &scope);
        napi_coerce_to_object(env, &undefined, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest038
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest038, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value undefined {};
        napi_get_undefined(env, &undefined);
        napi_value result {};
        napi_open_critical_scope(env, &scope);
        napi_coerce_to_string(env, &undefined, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest039
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest039, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_get_prototype(env, object, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest040
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest040, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_get_property_names(env, object, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest041
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest041, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        napi_open_critical_scope(env, &scope);
        napi_set_property(env, object, object, object);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest042
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest042, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        napi_open_critical_scope(env, &scope);
        bool result {};
        napi_has_property(env, object, object, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest043
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest043, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_get_property(env, object, object, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest044
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest044, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        napi_open_critical_scope(env, &scope);
        bool result {};
        napi_delete_property(env, object, object, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest045
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest045, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        napi_open_critical_scope(env, &scope);
        bool result {};
        napi_has_own_property(env, object, object, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest046
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest046, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        napi_open_critical_scope(env, &scope);
        napi_set_named_property(env, object, "", object);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest047
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest047, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        napi_open_critical_scope(env, &scope);
        bool result {};
        napi_has_named_property(env, object, "", &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest048
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest048, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_get_named_property(env, object, "", &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest049
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest049, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_get_own_property_descriptor(env, object, "", &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest050
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest050, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value array {};
        napi_create_array(env, &array);
        napi_open_critical_scope(env, &scope);
        napi_set_element(env, array, 0, array);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest051
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest051, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value array {};
        napi_create_array(env, &array);
        napi_open_critical_scope(env, &scope);
        napi_has_element(env, nullptr, 0, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest052
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest052, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value array {};
        napi_create_array(env, &array);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_get_element(env, array, 0, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest053
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest053, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value array {};
        napi_create_array(env, &array);
        napi_open_critical_scope(env, &scope);
        bool result {};
        napi_delete_element(env, array, 0, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest054
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest054, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        napi_open_critical_scope(env, &scope);
        std::vector<napi_property_descriptor> properties { { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr,
                                                             napi_default, nullptr } };
        napi_define_properties(env, object, properties.size(), properties.data());
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest055
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest055, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value array {};
        napi_create_array(env, &array);
        napi_open_critical_scope(env, &scope);
        bool result {};
        napi_is_array(env, array, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest056
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest056, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value undefined {};
        napi_get_undefined(env, &undefined);
        napi_open_critical_scope(env, &scope);
        bool result {};
        napi_is_sendable(env, undefined, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest057
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest057, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value undefined {};
        napi_get_undefined(env, &undefined);
        napi_open_critical_scope(env, &scope);
        bool result {};
        napi_strict_equals(env, undefined, undefined, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest058
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest058, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value undefined {};
        napi_get_undefined(env, &undefined);
        napi_open_critical_scope(env, &scope);
        napi_call_function(env, undefined, undefined, 0, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest059
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest059, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value undefined {};
        napi_get_undefined(env, &undefined);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_new_instance(env, undefined, 0, nullptr, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest060
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest060, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value undefined {};
        napi_get_undefined(env, &undefined);
        napi_open_critical_scope(env, &scope);
        bool result {};
        napi_instanceof(env, undefined, undefined, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest061
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest061, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value fn {};
        napi_create_function(
            env, "", 0,
            [](napi_env env, napi_callback_info info) {
                napi_open_critical_scope(env, &scope);
                napi_value newTarget;
                napi_get_new_target(env, info, &newTarget);
                napi_close_critical_scope(env, scope);
                return nullptr;
            },
            nullptr, &fn);
        napi_call_function(env, nullptr, fn, 0, nullptr, nullptr);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest062
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest062, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_define_class(env, "", 0, EmptyNapiCallback, nullptr, 0, nullptr, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest063
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest063, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_define_sendable_class(env, "", 0, EmptyNapiCallback, nullptr, 0, nullptr, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest064
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest064, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_sendable_object_with_properties(env, 0, nullptr, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest065
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest065, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_map(env, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest066
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest066, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_create_sendable_map(env, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest067
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest067, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value map {};
        napi_create_map(env, &map);
        napi_open_critical_scope(env, &scope);
        napi_map_set_property(env, map, map, map);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest068
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest068, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value map {};
        napi_create_map(env, &map);
        napi_open_critical_scope(env, &scope);
        napi_map_set_named_property(env, map, "", map);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest069
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest069, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value map {};
        napi_create_map(env, &map);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_map_get_property(env, map, map, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest070
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest070, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value map {};
        napi_create_map(env, &map);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_map_get_named_property(env, map, "", &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest071
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest071, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value map {};
        napi_create_map(env, &map);
        napi_open_critical_scope(env, &scope);
        bool result {};
        napi_map_has_property(env, map, map, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest072
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest072, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value map {};
        napi_create_map(env, &map);
        napi_open_critical_scope(env, &scope);
        bool result {};
        napi_map_has_named_property(env, map, "", &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest073
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest073, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value map {};
        napi_create_map(env, &map);
        napi_open_critical_scope(env, &scope);
        napi_map_delete_property(env, map, map);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest074
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest074, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value map {};
        napi_create_map(env, &map);
        napi_open_critical_scope(env, &scope);
        napi_map_clear(env, map);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest075
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest075, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value map {};
        napi_create_map(env, &map);
        napi_open_critical_scope(env, &scope);
        size_t size {};
        napi_map_get_size(env, map, &size);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest076
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest076, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value map {};
        napi_create_map(env, &map);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_map_get_entries(env, map, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest077
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest077, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value map {};
        napi_create_map(env, &map);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_map_get_keys(env, map, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest078
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest078, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value map {};
        napi_create_map(env, &map);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_map_get_values(env, map, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest079
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest079, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value map {};
        napi_create_map(env, &map);
        napi_value it {};
        napi_map_get_entries(env, map, &result);
        napi_open_critical_scope(env, &scope);
        napi_value result {};
        napi_map_iterator_get_next(env, it, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest080
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest080, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        // test case would crash, no need release after test
        auto data = new int { 1 };
        napi_open_critical_scope(env, &scope);
        napi_wrap(env, object, data, FinalizeCallback<int>, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest081
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest081, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        // test case would crash, no need release after test
        auto data = new int { 1 };
        napi_open_critical_scope(env, &scope);
        napi_wrap_enhance(env, object, data, FinalizeCallback<int>, true, nullptr, 0, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest082
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest082, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        // test case would crash, no need release after test
        auto data = new int { 1 };
        napi_open_critical_scope(env, &scope);
        napi_wrap_async_finalizer(env, object, data, FinalizeCallback<int>, nullptr, nullptr, 0);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest083
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest083, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        // test case would crash, no need release after test
        auto data = new int { 1 };
        napi_open_critical_scope(env, &scope);
        napi_wrap_with_size(env, object, data, FinalizeCallback<int>, nullptr, nullptr, 0);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest084
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest084, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        napi_open_critical_scope(env, &scope);
        void* result {};
        napi_unwrap(env, nullptr, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest085
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest085, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_value object {};
        napi_create_object(env, &object);
        napi_open_critical_scope(env, &scope);
        void* result {};
        napi_remove_wrap(env, object, &result);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest086
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest086, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_wrap_sendable(env, nullptr, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest087
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest087, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_wrap_sendable_with_size(env, nullptr, nullptr, nullptr, nullptr, 0);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest088
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest088, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_unwrap_sendable(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest089
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest089, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_remove_wrap_sendable(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest090
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest090, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_external(env, nullptr, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest091
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest091, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_external_with_size(env, nullptr, nullptr, nullptr, nullptr, 0);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest092
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest092, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_get_value_external(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest093
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest093, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_reference(env, nullptr, 0, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest094
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest094, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_delete_reference(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest095
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest095, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_reference_ref(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest096
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest096, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_reference_unref(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest097
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest097, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_get_reference_value(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest098
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest098, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_open_handle_scope(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest099
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest099, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_close_handle_scope(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest100
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest100, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_open_escapable_handle_scope(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest101
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest101, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_close_escapable_handle_scope(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest102
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest102, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_escape_handle(env, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest103
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest103, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_throw(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest104
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest104, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_throw_error(env, "", "");
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest105
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest105, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_throw_type_error(env, "", "");
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest106
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest106, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_throw_range_error(env, "", "");
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest107
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest107, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_error(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest108
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest108, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_exception_pending(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest109
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest109, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_get_and_clear_last_exception(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest110
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest110, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_arraybuffer(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest111
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest111, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_arraybuffer(env, 0, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest112
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest112, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_sendable_arraybuffer(env, 0, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest113
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest113, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_external_arraybuffer(env, nullptr, 0, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest114
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest114, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_get_arraybuffer_info(env, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest115
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest115, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_typedarray(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest116
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest116, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_buffer(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest117
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest117, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_buffer(env, 0, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest118
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest118, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_buffer_copy(env, 0, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest119
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest119, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_external_buffer(env, 0, nullptr, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest120
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest120, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_get_buffer_info(env, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest121
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest121, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_object_freeze(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest122
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest122, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_object_seal(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest123
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest123, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_typedarray(env, napi_int8_array, 0, nullptr, 0, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest124
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest124, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_sendable_typedarray(env, napi_int8_array, 0, nullptr, 0, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest125
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest125, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_get_typedarray_info(env, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest126
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest126, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_dataview(env, 0, nullptr, 0, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest127
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest127, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_dataview(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest128
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest128, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_get_dataview_info(env, nullptr, nullptr, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest129
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest129, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_get_version(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest130
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest130, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_promise(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest131
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest131, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_resolve_deferred(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest132
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest132, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_reject_deferred(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest133
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest133, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_promise(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest134
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest134, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_set_promise_rejection_callback(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest135
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest135, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_run_script(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest136
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest136, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_run_actor(env, "", nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest137
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest137, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_load_module(env, "", nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest138
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest138, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_load_module_with_info(env, "", "", nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest139
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest139, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_callable(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest140
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest140, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_arguments_object(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest141
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest141, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_async_function(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest142
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest142, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_boolean_object(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest143
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest143, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_generator_function(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest144
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest144, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_map_iterator(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest145
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest145, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_set_iterator(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest146
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest146, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_generator_object(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest147
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest147, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_module_namespace_object(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest148
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest148, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_proxy(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest149
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest149, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_reg_exp(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest150
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest150, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_number_object(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest151
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest151, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_map(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest152
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest152, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_set(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest153
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest153, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_string_object(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest154
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest154, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_symbol_object(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest155
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest155, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_weak_map(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest156
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest156, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_weak_set(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest157
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest157, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_runtime(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest158
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest158, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_serialize(env, nullptr, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest159
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest159, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_serialize_inner(env, nullptr, nullptr, nullptr, true, true, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest160
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest160, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_deserialize(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest161
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest161, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_delete_serialization_data(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest162
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest162, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_bigint_int64(env, 0, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest163
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest163, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_bigint_uint64(env, 0, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest164
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest164, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_get_value_bigint_int64(env, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest165
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest165, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_get_value_bigint_uint64(env, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest166
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest166, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_date(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest167
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest167, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_detached_arraybuffer(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest168
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest168, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_get_all_property_names(env, nullptr, napi_key_include_prototypes, napi_key_all_properties,
                                    napi_key_keep_numbers, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest169
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest169, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_detach_arraybuffer(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest170
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest170, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_type_tag_object(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest171
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest171, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_check_object_type_tag(env, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest172
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest172, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_date(env, 0, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest173
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest173, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_get_date_value(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest174
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest174, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_add_finalizer(env, nullptr, nullptr, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest175
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest175, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_bigint_words(env, 0, 0, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest176
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest176, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_get_value_bigint_words(env, nullptr, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest177
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest177, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_run_script_path(env, "", nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest178
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest178, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_big_int64_array(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest179
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest179, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_big_uint64_array(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest180
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest180, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_shared_array_buffer(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest181
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest181, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_object_get_keys(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest182
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest182, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_queue_async_work_with_qos(env, nullptr, napi_qos_default);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest183
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest183, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_queue_async_work_with_queue(env, nullptr, napi_qos_default, 0);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest184
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest184, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_coerce_to_native_binding_object(env, nullptr, nullptr, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest185
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest185, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_add_detached_finalizer(env, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest186
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest186, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_run_event_loop(env, napi_event_mode_default);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest187
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest187, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_stop_event_loop(env);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest188
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest188, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_concurrent_function(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest189
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest189, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_open_fast_native_scope(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest190
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest190, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_get_shared_array_buffer_info(env, nullptr, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest191
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest191, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_encode(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest192
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest192, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_is_bitvector(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest193
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest193, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_throw_jsvalue(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest194
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest194, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_load_module_with_path(env, "", nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest195
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest195, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_ark_context(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest196
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest196, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_switch_ark_context(env);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest197
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest197, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_destroy_ark_context(env);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest198
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest198, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_create_strong_reference(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest199
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest199, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_delete_strong_reference(env, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}

/**
 * @tc.name: NapiNonCriticalTest200
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest200, testing::ext::TestSize.Level1)
{
    BasicDeathTest([] {
        NativeEngineProxy env;
        napi_critical_scope scope {};
        napi_open_critical_scope(env, &scope);
        napi_get_strong_reference_value(env, nullptr, nullptr);
        napi_close_critical_scope(env, scope);
    })
        .AssertSignal(SIGABRT)
        .AssertError(TEST_NAPI_UNCLOSED_CRITICAL_LOG);
}
