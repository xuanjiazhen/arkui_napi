/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

static bool g_microtaskFlag = false;

static void ResetMicrotaskFlag()
{
    g_microtaskFlag = false;
}

static napi_value FlagCallback(napi_env env, napi_callback_info info)
{
    g_microtaskFlag = true;
    return nullptr;
}

static napi_value NoopCallback(napi_env env, napi_callback_info info)
{
    return nullptr;
}

// Attach a then/onRejected reaction to the promise through the JS `then` method.
static void AttachPromiseReaction(napi_env env, napi_value promise, bool useReject)
{
    napi_value thenFn = nullptr;
    napi_status status = napi_get_named_property(env, promise, "then", &thenFn);
    ASSERT_EQ(status, napi_ok);
    napi_value callback = nullptr;
    status = napi_create_function(env, "microtaskCallback", NAPI_AUTO_LENGTH, FlagCallback, nullptr, &callback);
    ASSERT_EQ(status, napi_ok);
    napi_value noop = nullptr;
    status = napi_create_function(env, "noopCallback", NAPI_AUTO_LENGTH, NoopCallback, nullptr, &noop);
    ASSERT_EQ(status, napi_ok);
    napi_value argv[2] = { useReject ? noop : callback, useReject ? callback : noop };
    napi_value result = nullptr;
    status = napi_call_function(env, promise, thenFn, 2, argv, &result);
    ASSERT_EQ(status, napi_ok);
}

class NapiMicrotaskScopeTest : public NativeEngineTest {};

/**
 * @tc.name: COpenScopeBasic001
 * @tc.desc: Open and close a microtask scope through the C API.
 * @tc.type: FUNC
 * @tc.require: 20260809952354 AC-1.1
 */
HWTEST_F(NapiMicrotaskScopeTest, COpenScopeBasic001, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    napi_microtask_scope scope = nullptr;
    ASSERT_EQ(napi_open_microtask_scope(env, &scope), napi_ok);
    ASSERT_NE(scope, nullptr);
    ASSERT_EQ(napi_close_microtask_scope(env, scope), napi_ok);
}

/**
 * @tc.name: CNullArgumentChecks001
 * @tc.desc: Null arguments are rejected with napi_invalid_arg without crashing.
 * @tc.type: FUNC
 * @tc.require: 20260809952354 AC-5.3
 */
HWTEST_F(NapiMicrotaskScopeTest, CNullArgumentChecks001, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    napi_microtask_scope scope = nullptr;
    ASSERT_EQ(napi_open_microtask_scope(nullptr, &scope), napi_invalid_arg);
    ASSERT_EQ(napi_open_microtask_scope(env, nullptr), napi_invalid_arg);
    ASSERT_EQ(napi_close_microtask_scope(nullptr, nullptr), napi_invalid_arg);
    ASSERT_EQ(napi_close_microtask_scope(env, nullptr), napi_invalid_arg);
}

/**
 * @tc.name: CNestedOpenRejected001
 * @tc.desc: Nesting is not supported: the second open returns napi_invalid_arg.
 * @tc.type: FUNC
 * @tc.require: 20260809952354 AC-5.1
 */
HWTEST_F(NapiMicrotaskScopeTest, CNestedOpenRejected001, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    napi_microtask_scope scope = nullptr;
    ASSERT_EQ(napi_open_microtask_scope(env, &scope), napi_ok);
    napi_microtask_scope scope2 = nullptr;
    ASSERT_EQ(napi_open_microtask_scope(env, &scope2), napi_invalid_arg);
    ASSERT_EQ(scope2, nullptr);
    // The original scope still closes normally.
    ASSERT_EQ(napi_close_microtask_scope(env, scope), napi_ok);
}

/**
 * @tc.name: CCloseMismatchedHandle001
 * @tc.desc: Closing with a null/stale handle returns napi_invalid_arg and does not break state.
 * @tc.type: FUNC
 * @tc.require: 20260809952354 AC-5.2
 */
HWTEST_F(NapiMicrotaskScopeTest, CCloseMismatchedHandle001, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    napi_microtask_scope scope = nullptr;
    ASSERT_EQ(napi_open_microtask_scope(env, &scope), napi_ok);
    ASSERT_EQ(napi_close_microtask_scope(env, nullptr), napi_invalid_arg);
    ASSERT_EQ(napi_close_microtask_scope(env, scope), napi_ok);
    // Stale handle: already closed.
    ASSERT_EQ(napi_close_microtask_scope(env, scope), napi_invalid_arg);
}

/**
 * @tc.name: CEndToEndDeferral001
 * @tc.desc: Microtasks accumulated inside the scope (resolve + JS call) are drained on close.
 * @tc.type: FUNC
 * @tc.require: 20260809952354 AC-1.2 / AC-1.3
 */
HWTEST_F(NapiMicrotaskScopeTest, CEndToEndDeferral001, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    ResetMicrotaskFlag();
    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    ASSERT_EQ(napi_create_promise(env, &deferred, &promise), napi_ok);
    ASSERT_NO_FATAL_FAILURE(AttachPromiseReaction(env, promise, false));
    napi_value noop = nullptr;
    ASSERT_EQ(napi_create_function(env, "noop", NAPI_AUTO_LENGTH, NoopCallback, nullptr, &noop), napi_ok);

    napi_microtask_scope scope = nullptr;
    ASSERT_EQ(napi_open_microtask_scope(env, &scope), napi_ok);
    ASSERT_EQ(napi_resolve_deferred(env, deferred, promise), napi_ok);
    ASSERT_FALSE(g_microtaskFlag);
    // A JS call inside the scope must not drain the pending microtask either.
    napi_value callResult = nullptr;
    ASSERT_EQ(napi_call_function(env, promise, noop, 0, nullptr, &callResult), napi_ok);
    ASSERT_FALSE(g_microtaskFlag);
    ASSERT_EQ(napi_close_microtask_scope(env, scope), napi_ok);
    ASSERT_TRUE(g_microtaskFlag);
}

/**
 * @tc.name: CNewInstanceDeferred001
 * @tc.desc: napi_new_instance inside the scope does not drain the pending microtask.
 * @tc.type: FUNC
 * @tc.require: 20260809952354 AC-2.1
 */
HWTEST_F(NapiMicrotaskScopeTest, CNewInstanceDeferred001, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    ResetMicrotaskFlag();
    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    ASSERT_EQ(napi_create_promise(env, &deferred, &promise), napi_ok);
    ASSERT_NO_FATAL_FAILURE(AttachPromiseReaction(env, promise, false));
    napi_value ctor = nullptr;
    ASSERT_EQ(napi_create_function(env, "Ctor", NAPI_AUTO_LENGTH, NoopCallback, nullptr, &ctor), napi_ok);

    napi_microtask_scope scope = nullptr;
    ASSERT_EQ(napi_open_microtask_scope(env, &scope), napi_ok);
    ASSERT_EQ(napi_resolve_deferred(env, deferred, promise), napi_ok);
    ASSERT_FALSE(g_microtaskFlag);
    napi_value instance = nullptr;
    ASSERT_EQ(napi_new_instance(env, ctor, 0, nullptr, &instance), napi_ok);
    ASSERT_NE(instance, nullptr);
    ASSERT_FALSE(g_microtaskFlag);
    ASSERT_EQ(napi_close_microtask_scope(env, scope), napi_ok);
    ASSERT_TRUE(g_microtaskFlag);
}

/**
 * @tc.name: CRejectDeferred001
 * @tc.desc: napi_reject_deferred inside the scope does not drain the pending microtask.
 * @tc.type: FUNC
 * @tc.require: 20260809952354 AC-4.1
 */
HWTEST_F(NapiMicrotaskScopeTest, CRejectDeferred001, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    ResetMicrotaskFlag();
    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    ASSERT_EQ(napi_create_promise(env, &deferred, &promise), napi_ok);
    ASSERT_NO_FATAL_FAILURE(AttachPromiseReaction(env, promise, true));

    napi_microtask_scope scope = nullptr;
    ASSERT_EQ(napi_open_microtask_scope(env, &scope), napi_ok);
    ASSERT_EQ(napi_reject_deferred(env, deferred, promise), napi_ok);
    ASSERT_FALSE(g_microtaskFlag);
    ASSERT_EQ(napi_close_microtask_scope(env, scope), napi_ok);
    ASSERT_TRUE(g_microtaskFlag);
}

/**
 * @tc.name: CDefaultAutoDrain001
 * @tc.desc: Without a scope, resolve/reject drain the microtask queue immediately (default behavior).
 * @tc.type: FUNC
 * @tc.require: 20260809952354 AC-6.1
 */
HWTEST_F(NapiMicrotaskScopeTest, CDefaultAutoDrain001, testing::ext::TestSize.Level0)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    ResetMicrotaskFlag();
    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    ASSERT_EQ(napi_create_promise(env, &deferred, &promise), napi_ok);
    ASSERT_NO_FATAL_FAILURE(AttachPromiseReaction(env, promise, false));
    ASSERT_EQ(napi_resolve_deferred(env, deferred, promise), napi_ok);
    ASSERT_TRUE(g_microtaskFlag);
}
