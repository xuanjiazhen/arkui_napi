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


/**
 * @tc.name: NapiUnclosedCriticalTest001
 * @tc.desc: Test check of unclosed critical scope after user callback.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiUnclosedCriticalTest001, testing::ext::TestSize.Level0)
{
    BasicDeathTest(
        []() {
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
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[ArkNativeFunctionCallback] critical scope still open after user callback 'testFunc'"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiUnclosedCriticalTest002
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiUnclosedCriticalTest002, testing::ext::TestSize.Level0)
{
    BasicDeathTest(
        []() {
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
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[AsyncAfterWorkCallback] critical scope still open after user callback"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiUnclosedCriticalTest003
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiUnclosedCriticalTest003, testing::ext::TestSize.Level0)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            UVLoopRunner runner(*env);
            static napi_critical_scope scope {};
            napi_threadsafe_function tsfn {};
            napi_create_threadsafe_function(
                env, nullptr, nullptr, nullptr, 0, 1, nullptr, nullptr, nullptr,
                [](napi_env env, [[maybe_unused]] napi_value func, [[maybe_unused]] void* context,
                   [[maybe_unused]] void* data) { napi_open_critical_scope(env, &scope); },
                &tsfn);
            runner.Run(UV_RUN_ONCE);
            napi_release_threadsafe_function(tsfn, napi_tsfn_release);
            runner.Run(UV_RUN_ONCE);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[ProcessAsyncHandle] critical scope still open after user callback "),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest001
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest001, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_limit_runtime(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_limit_runtime] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest002
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest002, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_fatal_exception(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_fatal_exception] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest003
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest003, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_async_work(env, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_async_work] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest004
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest004, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_delete_async_work(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_delete_async_work] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest005
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest005, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_queue_async_work(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_queue_async_work] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest006
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest006, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_cancel_async_work(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_cancel_async_work] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest007
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest007, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_uv_event_loop(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_uv_event_loop] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest008
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest008, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_add_async_cleanup_hook(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_add_async_cleanup_hook] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest009
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest009, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_threadsafe_function(env, nullptr, nullptr, nullptr, 0, 0, nullptr, nullptr, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_threadsafe_function] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest010
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest010, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_ref_threadsafe_function(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_ref_threadsafe_function] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest011
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest011, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_unref_threadsafe_function(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_unref_threadsafe_function] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest012
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest012, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_async_init(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_async_init] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest013
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest013, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_async_destroy(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_async_destroy] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest014
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest014, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_open_callback_scope(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_open_callback_scope] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest015
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest015, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_close_callback_scope(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_close_callback_scope] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest016
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest016, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_set_instance_data(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_set_instance_data] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest017
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest017, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_instance_data(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_instance_data] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest018
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest018, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            node_api_get_module_file_name(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[node_api_get_module_file_name] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest019
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest019, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_make_callback(env, nullptr, nullptr, nullptr, 0, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_make_callback] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest020
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest020, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_undefined(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_undefined] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest021
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest021, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_null(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_null] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest022
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest022, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_global(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_global] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest023
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest023, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_boolean(env, true, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_boolean] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest024
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest024, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_object(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_object] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest025
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest025, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_object_with_properties(env, nullptr, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_object_with_properties] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest026
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest026, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_object_with_named_properties(env, nullptr, 0, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_object_with_named_properties] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest027
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest027, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_array(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_array] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest028
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest028, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_array_with_length(env, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_array_with_length] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest029
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest029, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_sendable_array(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_sendable_array] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest030
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest030, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_sendable_array_with_length(env, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_sendable_array_with_length] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest031
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest031, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_double(env, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_double] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest032
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest032, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_int32(env, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_int32] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest033
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest033, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_uint32(env, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_uint32] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest034
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest034, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_int64(env, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_int64] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest035
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest035, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_string_latin1(env, "", 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_string_latin1] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest036
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest036, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_string_utf8(env, "", 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_string_utf8] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest037
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest037, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_string_utf16(env, u"", 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_string_utf16] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest038
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest038, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_symbol(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_symbol] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest039
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest039, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_function(env, "", 0, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_function] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest040
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest040, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_error(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_error] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest041
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest041, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_type_error(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_type_error] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest042
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest042, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_range_error(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_range_error] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest043
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest043, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_typeof(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_typeof] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest044
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest044, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_value_double(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_value_double] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest045
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest045, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_value_int32(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_value_int32] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest046
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest046, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_value_uint32(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_value_uint32] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest047
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest047, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_value_int64(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_value_int64] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest048
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest048, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_value_bool(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_value_bool] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest049
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest049, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_value_string_latin1(env, nullptr, nullptr, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_value_string_latin1] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest050
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest050, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_value_string_utf8(env, nullptr, nullptr, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_value_string_utf8] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest051
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest051, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_value_string_utf16(env, nullptr, nullptr, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_value_string_utf16] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest052
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest052, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_open_critical_scope(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_open_critical_scope] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest053
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest053, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_coerce_to_bool(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_coerce_to_bool] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest054
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest054, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_coerce_to_number(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_coerce_to_number] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest055
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest055, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_coerce_to_object(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_coerce_to_object] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest056
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest056, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_coerce_to_string(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_coerce_to_string] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest057
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest057, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_prototype(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_prototype] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest058
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest058, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_property_names(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_property_names] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest059
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest059, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_set_property(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_set_property] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest060
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest060, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_has_property(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_has_property] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest061
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest061, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_property(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_property] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest062
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest062, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_delete_property(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_delete_property] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest063
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest063, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_has_own_property(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_has_own_property] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest064
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest064, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_set_named_property(env, nullptr, "", nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_set_named_property] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest065
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest065, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_has_named_property(env, nullptr, "", nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_has_named_property] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest066
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest066, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_named_property(env, nullptr, "", nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_named_property] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest067
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest067, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_own_property_descriptor(env, nullptr, "", nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_own_property_descriptor] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest068
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest068, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_set_element(env, nullptr, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_set_element] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest069
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest069, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_has_element(env, nullptr, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_has_element] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest070
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest070, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_element(env, nullptr, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_element] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest071
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest071, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_delete_element(env, nullptr, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_delete_element] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest072
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest072, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_define_properties(env, nullptr, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_define_properties] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest073
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest073, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_array(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_array] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest074
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest074, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_sendable(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_sendable] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest075
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest075, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_strict_equals(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_strict_equals] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest076
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest076, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_call_function(env, nullptr, nullptr, 0, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_call_function] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest077
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest077, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_new_instance(env, nullptr, 0, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_new_instance] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest078
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest078, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_instanceof(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_instanceof] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest079
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest079, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_new_target(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_new_target] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest080
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest080, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_define_class(env, "", 0, nullptr, nullptr, 0, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_define_class] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest081
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest081, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_define_sendable_class(env, "", 0, nullptr, nullptr, 0, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_define_sendable_class] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest082
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest082, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_sendable_object_with_properties(env, 0, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_sendable_object_with_properties] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest083
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest083, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_map(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_map] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest084
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest084, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_sendable_map(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_sendable_map] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest085
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest085, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_map_set_property(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_map_set_property] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest086
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest086, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_map_set_named_property(env, nullptr, "", nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_map_set_named_property] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest087
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest087, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_map_get_property(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_map_get_property] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest088
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest088, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_map_get_named_property(env, nullptr, "", nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_map_get_named_property] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest089
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest089, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_map_has_property(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_map_has_property] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest090
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest090, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_map_has_named_property(env, nullptr, "", nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_map_has_named_property] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest091
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest091, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_map_delete_property(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_map_delete_property] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest092
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest092, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_map_clear(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_map_clear] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest093
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest093, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_map_get_size(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_map_get_size] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest094
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest094, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_map_get_entries(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_map_get_entries] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest095
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest095, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_map_get_keys(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_map_get_keys] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest096
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest096, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_map_get_values(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_map_get_values] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest097
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest097, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_map_iterator_get_next(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_map_iterator_get_next] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest098
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest098, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_wrap(env, nullptr, nullptr, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_wrap] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest099
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest099, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_wrap_enhance(env, nullptr, nullptr, nullptr, true, nullptr, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_wrap_enhance] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest100
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest100, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_wrap_async_finalizer(env, nullptr, nullptr, nullptr, nullptr, nullptr, 0);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_wrap_async_finalizer] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest101
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest101, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_wrap_with_size(env, nullptr, nullptr, nullptr, nullptr, nullptr, 0);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_wrap_with_size] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest102
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest102, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_unwrap(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_unwrap] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest103
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest103, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_remove_wrap(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_remove_wrap] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest104
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest104, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_wrap_sendable(env, nullptr, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_wrap_sendable] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest105
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest105, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_wrap_sendable_with_size(env, nullptr, nullptr, nullptr, nullptr, 0);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_wrap_sendable_with_size] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest106
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest106, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_unwrap_sendable(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_unwrap_sendable] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest107
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest107, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_remove_wrap_sendable(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_remove_wrap_sendable] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest108
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest108, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_external(env, nullptr, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_external] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest109
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest109, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_external_with_size(env, nullptr, nullptr, nullptr, nullptr, 0);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_external_with_size] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest110
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest110, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_value_external(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_value_external] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest111
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest111, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_reference(env, nullptr, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_reference] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest112
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest112, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_delete_reference(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_delete_reference] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest113
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest113, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_reference_ref(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_reference_ref] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest114
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest114, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_reference_unref(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_reference_unref] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest115
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest115, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_reference_value(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_reference_value] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest116
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest116, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_open_handle_scope(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_open_handle_scope] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest117
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest117, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_close_handle_scope(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_close_handle_scope] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest118
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest118, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_open_escapable_handle_scope(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_open_escapable_handle_scope] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest119
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest119, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_close_escapable_handle_scope(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_close_escapable_handle_scope] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest120
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest120, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_escape_handle(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_escape_handle] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest121
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest121, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_throw(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_throw] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest122
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest122, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_throw_error(env, "", "");
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_throw_error] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest123
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest123, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_throw_type_error(env, "", "");
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_throw_type_error] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest124
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest124, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_throw_range_error(env, "", "");
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_throw_range_error] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest125
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest125, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_error(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_error] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest126
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest126, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_exception_pending(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_exception_pending] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest127
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest127, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_and_clear_last_exception(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_and_clear_last_exception] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest128
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest128, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_arraybuffer(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_arraybuffer] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest129
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest129, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_arraybuffer(env, 0, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_arraybuffer] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest130
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest130, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_sendable_arraybuffer(env, 0, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_sendable_arraybuffer] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest131
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest131, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_external_arraybuffer(env, nullptr, 0, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_external_arraybuffer] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest132
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest132, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_arraybuffer_info(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_arraybuffer_info] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest133
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest133, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_typedarray(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_typedarray] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest134
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest134, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_buffer(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_buffer] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest135
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest135, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_buffer(env, 0, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_buffer] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest136
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest136, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_buffer_copy(env, 0, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_buffer_copy] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest137
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest137, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_external_buffer(env, 0, nullptr, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_external_buffer] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest138
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest138, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_buffer_info(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_buffer_info] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest139
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest139, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_object_freeze(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_object_freeze] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest140
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest140, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_object_seal(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_object_seal] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest141
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest141, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_typedarray(env, napi_int8_array, 0, nullptr, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_typedarray] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest142
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest142, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_sendable_typedarray(env, napi_int8_array, 0, nullptr, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_sendable_typedarray] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest143
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest143, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_typedarray_info(env, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_typedarray_info] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest144
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest144, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_dataview(env, 0, nullptr, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_dataview] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest145
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest145, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_dataview(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_dataview] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest146
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest146, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_dataview_info(env, nullptr, nullptr, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_dataview_info] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest147
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest147, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_version(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_version] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest148
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest148, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_promise(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_promise] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest149
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest149, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_resolve_deferred(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_resolve_deferred] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest150
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest150, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_reject_deferred(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_reject_deferred] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest151
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest151, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_promise(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_promise] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest152
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest152, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_set_promise_rejection_callback(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_set_promise_rejection_callback] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest153
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest153, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_run_script(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_run_script] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest154
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest154, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_run_actor(env, "", nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_run_actor] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest155
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest155, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_load_module(env, "", nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_load_module] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest156
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest156, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_load_module_with_info(env, "", "", nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_load_module_with_info] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest157
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest157, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_callable(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_callable] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest158
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest158, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_arguments_object(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_arguments_object] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest159
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest159, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_async_function(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_async_function] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest160
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest160, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_boolean_object(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_boolean_object] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest161
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest161, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_generator_function(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_generator_function] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest162
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest162, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_map_iterator(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_map_iterator] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest163
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest163, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_set_iterator(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_set_iterator] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest164
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest164, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_generator_object(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_generator_object] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest165
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest165, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_module_namespace_object(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_module_namespace_object] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest166
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest166, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_proxy(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_proxy] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest167
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest167, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_reg_exp(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_reg_exp] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest168
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest168, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_number_object(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_number_object] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest169
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest169, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_map(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_map] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest170
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest170, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_set(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_set] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest171
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest171, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_string_object(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_string_object] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest172
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest172, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_symbol_object(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_symbol_object] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest173
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest173, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_weak_map(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_weak_map] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest174
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest174, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_weak_set(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_weak_set] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest175
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest175, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_runtime(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_runtime] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest176
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest176, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_serialize(env, nullptr, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_serialize] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest177
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest177, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_serialize_inner(env, nullptr, nullptr, nullptr, true, true, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_serialize_inner] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest178
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest178, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_deserialize(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_deserialize] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest179
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest179, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_delete_serialization_data(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_delete_serialization_data] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest180
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest180, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_bigint_int64(env, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_bigint_int64] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest181
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest181, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_bigint_uint64(env, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_bigint_uint64] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest182
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest182, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_value_bigint_int64(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_value_bigint_int64] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest183
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest183, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_value_bigint_uint64(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_value_bigint_uint64] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest184
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest184, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_date(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_date] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest185
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest185, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_detached_arraybuffer(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_detached_arraybuffer] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest186
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest186, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_all_property_names(env, nullptr, napi_key_include_prototypes, napi_key_all_properties, napi_key_keep_numbers, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_all_property_names] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest187
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest187, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_detach_arraybuffer(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_detach_arraybuffer] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest188
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest188, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_type_tag_object(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_type_tag_object] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest189
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest189, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_check_object_type_tag(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_check_object_type_tag] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest190
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest190, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_date(env, 0, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_date] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest191
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest191, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_date_value(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_date_value] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest192
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest192, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_add_finalizer(env, nullptr, nullptr, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_add_finalizer] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest193
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest193, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_bigint_words(env, 0, 0, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_bigint_words] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest194
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest194, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_value_bigint_words(env, nullptr, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_value_bigint_words] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest195
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest195, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_run_script_path(env, "", nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_run_script_path] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest196
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest196, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_big_int64_array(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_big_int64_array] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest197
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest197, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_big_uint64_array(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_big_uint64_array] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest198
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest198, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_shared_array_buffer(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_shared_array_buffer] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest199
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest199, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_object_get_keys(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_object_get_keys] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest200
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest200, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_queue_async_work_with_qos(env, nullptr, napi_qos_default);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_queue_async_work_with_qos] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest201
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest201, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_queue_async_work_with_queue(env, nullptr, napi_qos_default, 0);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_queue_async_work_with_queue] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest202
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest202, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_coerce_to_native_binding_object(env, nullptr, nullptr, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_coerce_to_native_binding_object] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest203
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest203, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_add_detached_finalizer(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_add_detached_finalizer] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest204
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest204, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_run_event_loop(env, napi_event_mode_default);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_run_event_loop] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest205
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest205, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_stop_event_loop(env);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_stop_event_loop] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest206
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest206, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_concurrent_function(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_concurrent_function] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest207
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest207, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_open_fast_native_scope(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_open_fast_native_scope] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest208
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest208, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_get_shared_array_buffer_info(env, nullptr, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_get_shared_array_buffer_info] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest209
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest209, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_encode(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_encode] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest210
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest210, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_is_bitvector(env, nullptr, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_is_bitvector] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest211
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest211, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_throw_jsvalue(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_throw_jsvalue] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest212
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest212, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_load_module_with_path(env, "", nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_load_module_with_path] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest213
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest213, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_create_ark_context(env, nullptr);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_create_ark_context] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest214
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest214, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_switch_ark_context(env);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_switch_ark_context] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

/**
 * @tc.name: NapiNonCriticalTest215
 * @tc.desc: Test interface cannot invoke while critical scope is opening.
 * @tc.type: FUNC
 */
HWTEST_F(NapiCriticalTest, NapiNonCriticalTest215, testing::ext::TestSize.Level1)
{
    BasicDeathTest(
        []() {
            NativeEngineProxy env;
            napi_critical_scope scope {};
            napi_open_critical_scope(env, &scope);
            napi_destroy_ark_context(env);
            napi_close_critical_scope(env, scope);
        },
        []([[maybe_unused]] const std::string& out, const std::string& err) {
            ASSERT_NE(err.find("[napi_destroy_ark_context] current interface cannot invoke under critical scope"),
                      std::string::npos);
        });
}

