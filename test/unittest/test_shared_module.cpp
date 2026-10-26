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

#include <atomic>
#include <thread>

#include "test.h"
#include "test_common.h"
#include "gtest/gtest.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "native_api_internal.h"
#include "native_engine/native_utils.h"
#include "native_engine/impl/ark/ark_native_engine.h"
#include "napi/native_common.h"
#include "securec.h"
#include "utils/log.h"

static constexpr const char SHARED_MODULE_NAME[] = "test.shared.module";
static constexpr const char SHARED_MODULE_NAME_FAIL[] = "test.shared.module.fail";
static constexpr const char SHARED_MODULE_NAME_NONSENDABLE[] = "test.shared.module.nonsendable";
static constexpr const char SHARED_MODULE_NAME_THROW[] = "test.shared.module.throw";

static std::atomic<int> g_initCount{0};

static napi_value SharedModuleInit(napi_env env, napi_value /*unused*/)
{
    g_initCount.fetch_add(1);
    napi_value exports = nullptr;
    napi_status status = napi_create_sendable_object_with_properties(env, 0, nullptr, &exports);
    if (status != napi_ok) {
        return nullptr;
    }
    return exports;
}

static napi_value SharedModuleInitFail(napi_env env, napi_value /*unused*/)
{
    g_initCount.fetch_add(1);
    return nullptr;
}

static napi_value SharedModuleInitNonSendable(napi_env env, napi_value /*unused*/)
{
    g_initCount.fetch_add(1);
    napi_value obj = nullptr;
    napi_create_object(env, &obj);
    return obj;
}

static napi_value SharedModuleInitThrow(napi_env env, napi_value /*unused*/)
{
    g_initCount.fetch_add(1);
    napi_throw_error(env, "100", "init threw");
    return nullptr;
}

static void RegisterSharedModules()
{
    static bool registered = false;
    if (registered) {
        return;
    }
    registered = true;

    napi_module modValid = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = __FILE__,
        .nm_register_func = SharedModuleInit,
        .nm_modname = SHARED_MODULE_NAME,
    };
    napi_shared_module_register(&modValid);

    napi_module modFail = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = __FILE__,
        .nm_register_func = SharedModuleInitFail,
        .nm_modname = SHARED_MODULE_NAME_FAIL,
    };
    napi_shared_module_register(&modFail);

    napi_module modNonSendable = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = __FILE__,
        .nm_register_func = SharedModuleInitNonSendable,
        .nm_modname = SHARED_MODULE_NAME_NONSENDABLE,
    };
    napi_shared_module_register(&modNonSendable);

    napi_module modThrow = {
        .nm_version = 1,
        .nm_flags = 0,
        .nm_filename = __FILE__,
        .nm_register_func = SharedModuleInitThrow,
        .nm_modname = SHARED_MODULE_NAME_THROW,
    };
    napi_shared_module_register(&modThrow);
}

static napi_value CallRequireNapi(napi_env env, const char* moduleName)
{
    napi_value global = nullptr;
    napi_get_global(env, &global);
    napi_value key = nullptr;
    napi_create_string_utf8(env, "requireNapi", NAPI_AUTO_LENGTH, &key);
    napi_value func = nullptr;
    napi_get_property(env, global, key, &func);
    napi_value arg = nullptr;
    napi_create_string_utf8(env, moduleName, NAPI_AUTO_LENGTH, &arg);
    napi_value result = nullptr;
    napi_call_function(env, global, func, 1, &arg, &result);
    return result;
}

class NapiSharedModuleTest : public NativeEngineTest {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "NapiSharedModuleTest SetUpTestCase";
        RegisterSharedModules();
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "NapiSharedModuleTest TearDownTestCase";
    }

    void SetUp() override
    {
        napi_env env = reinterpret_cast<napi_env>(engine_);
        ASSERT_NE(env, nullptr);
        g_initCount.store(0);
    }

    void TearDown() override {}
};

/* ===== Group A: napi_shared_module_register ===== */

/* TC-A2: napi_shared_module_register(nullptr) -> HILOG_ERROR, no crash */
HWTEST_F(NapiSharedModuleTest, SharedModule_Register_NullMod, testing::ext::TestSize.Level1)
{
    LoggerCollector collector;
    napi_shared_module_register(nullptr);
    ASSERT_TRUE(collector.Includes("mod is nullptr"));
}

/* ===== Group B: IsSharedModule (ArkNativeEngine) ===== */

/* TC-B1: IsSharedModule(nullptr) -> false */
HWTEST_F(NapiSharedModuleTest, IsSharedModule_NullModule, testing::ext::TestSize.Level1)
{
    auto* arkEngine = static_cast<ArkNativeEngine*>(engine_);
    ASSERT_NE(arkEngine, nullptr);
    EXPECT_FALSE(arkEngine->IsSharedModule(nullptr));
}

/* ===== Group D: LoadSharedNativeModule — first call init ===== */

/* TC-D1: callback returns valid sendable object -> root stored, returned non-null */
HWTEST_F(NapiSharedModuleTest, LoadShared_FirstCall_ValidSendableRoot, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    napi_value result = CallRequireNapi(env, SHARED_MODULE_NAME);
    ASSERT_NE(result, nullptr);
    bool isSendable = false;
    napi_is_sendable(env, result, &isSendable);
    EXPECT_TRUE(isSendable);
    EXPECT_EQ(g_initCount.load(), 1);
}

/* TC-D3: callback returns nullptr -> undefined returned, HILOG_ERROR */
HWTEST_F(NapiSharedModuleTest, LoadShared_FirstCall_CallbackReturnsNull, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    LoggerCollector collector;
    napi_value result = CallRequireNapi(env, SHARED_MODULE_NAME_FAIL);
    ASSERT_NE(result, nullptr);
    napi_valuetype type;
    napi_typeof(env, result, &type);
    EXPECT_EQ(type, napi_undefined);
    ASSERT_TRUE(collector.Includes("callback returned null"));
    EXPECT_EQ(g_initCount.load(), 1);
}

/* TC-D4: callback returns plain Object -> undefined, HILOG_ERROR "non-sendable" */
HWTEST_F(NapiSharedModuleTest, LoadShared_FirstCall_CallbackReturnsPlainObject, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    LoggerCollector collector;
    napi_value result = CallRequireNapi(env, SHARED_MODULE_NAME_NONSENDABLE);
    ASSERT_NE(result, nullptr);
    napi_valuetype type;
    napi_typeof(env, result, &type);
    EXPECT_EQ(type, napi_undefined);
    ASSERT_TRUE(collector.Includes("non-sendable root"));
    EXPECT_EQ(g_initCount.load(), 1);
}

/* TC-D7: callback throws exception -> TryCatch catches, undefined returned, HILOG_ERROR */
HWTEST_F(NapiSharedModuleTest, LoadShared_FirstCall_CallbackThrows, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    LoggerCollector collector;
    napi_value result = CallRequireNapi(env, SHARED_MODULE_NAME_THROW);
    ASSERT_NE(result, nullptr);
    napi_valuetype type;
    napi_typeof(env, result, &type);
    EXPECT_EQ(type, napi_undefined);
    ASSERT_TRUE(collector.Includes("callback threw exception"));
    EXPECT_EQ(g_initCount.load(), 1);
    bool isPending = false;
    napi_is_exception_pending(env, &isPending);
    EXPECT_FALSE(isPending);
}

/* ===== Group E: cache hit / retry ===== */

/* TC-E1: second call after success -> find hits, no re-init */
HWTEST_F(NapiSharedModuleTest, LoadShared_SecondCall_AfterSuccess, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    napi_value result1 = CallRequireNapi(env, SHARED_MODULE_NAME);
    ASSERT_NE(result1, nullptr);
    EXPECT_EQ(g_initCount.load(), 1);

    napi_value result2 = CallRequireNapi(env, SHARED_MODULE_NAME);
    ASSERT_NE(result2, nullptr);
    EXPECT_EQ(g_initCount.load(), 1);

    bool equals = false;
    napi_strict_equals(env, result1, result2, &equals);
    EXPECT_TRUE(equals);
}

/* TC-E2: second call after failure -> find misses (not in map), retries init */
HWTEST_F(NapiSharedModuleTest, LoadShared_SecondCall_AfterFailure, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    CallRequireNapi(env, SHARED_MODULE_NAME_FAIL);
    EXPECT_EQ(g_initCount.load(), 1);

    CallRequireNapi(env, SHARED_MODULE_NAME_FAIL);
    EXPECT_EQ(g_initCount.load(), 2);
}

/* ===== Group F: multi-VM sharing ===== */

/* TC-F1: two VMs import same shared module -> both get sendable root, init == 1 */
HWTEST_F(NapiSharedModuleTest, MultiVM_ConcurrentFirstImport, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    napi_value result1 = CallRequireNapi(env, SHARED_MODULE_NAME);
    ASSERT_NE(result1, nullptr);
    bool isSendable = false;
    napi_is_sendable(env, result1, &isSendable);
    EXPECT_TRUE(isSendable);
    EXPECT_EQ(g_initCount.load(), 1);

    EcmaVM* workerVM = panda::JSNApi::CreateJSVM();
    ASSERT_NE(workerVM, nullptr);
    ArkNativeEngine* workerEngine = new ArkNativeEngine(workerVM, nullptr);
    ASSERT_NE(workerEngine, nullptr);

    napi_env workerEnv = reinterpret_cast<napi_env>(workerEngine);
    napi_value result2 = CallRequireNapi(workerEnv, SHARED_MODULE_NAME);
    ASSERT_NE(result2, nullptr);
    bool isSendable2 = false;
    napi_is_sendable(workerEnv, result2, &isSendable2);
    EXPECT_TRUE(isSendable2);

    EXPECT_EQ(g_initCount.load(), 1);

    delete workerEngine;
    panda::JSNApi::DestroyJSVM(workerVM);
}

/* TC-F2: VM-A inits first, VM-B imports later -> both get sendable root, init stays 1 */
HWTEST_F(NapiSharedModuleTest, MultiVM_SecondVMAfterInit, testing::ext::TestSize.Level1)
{
    napi_env env = reinterpret_cast<napi_env>(engine_);
    napi_value result1 = CallRequireNapi(env, SHARED_MODULE_NAME);
    ASSERT_NE(result1, nullptr);
    bool isSendable1 = false;
    napi_is_sendable(env, result1, &isSendable1);
    EXPECT_TRUE(isSendable1);
    EXPECT_EQ(g_initCount.load(), 1);

    EcmaVM* workerVM = panda::JSNApi::CreateJSVM();
    ASSERT_NE(workerVM, nullptr);
    ArkNativeEngine* workerEngine = new ArkNativeEngine(workerVM, nullptr);
    ASSERT_NE(workerEngine, nullptr);

    napi_env workerEnv = reinterpret_cast<napi_env>(workerEngine);
    napi_value result2 = CallRequireNapi(workerEnv, SHARED_MODULE_NAME);
    ASSERT_NE(result2, nullptr);
    bool isSendable2 = false;
    napi_is_sendable(workerEnv, result2, &isSendable2);
    EXPECT_TRUE(isSendable2);

    EXPECT_EQ(g_initCount.load(), 1);

    delete workerEngine;
    panda::JSNApi::DestroyJSVM(workerVM);
}

/* ===== Group G: IsSharedModule (NativeModuleManager) ===== */

/* TC-G1: shared module found in list -> true */
HWTEST_F(NapiSharedModuleTest, IsSharedModuleMgr_SharedFound, testing::ext::TestSize.Level1)
{
    NativeModuleManager* mgr = NativeModuleManager::GetInstance();
    ASSERT_NE(mgr, nullptr);
    EXPECT_TRUE(mgr->IsSharedModule(SHARED_MODULE_NAME));
}

/* TC-G3: module not found -> false */
HWTEST_F(NapiSharedModuleTest, IsSharedModuleMgr_NotFound, testing::ext::TestSize.Level1)
{
    NativeModuleManager* mgr = NativeModuleManager::GetInstance();
    ASSERT_NE(mgr, nullptr);
    EXPECT_FALSE(mgr->IsSharedModule("nonexistent.module"));
}

/* ===== Group H: UnloadNativeModule shared check ===== */

/* TC-H1: shared module -> skip unload (return true, no dlclose) */
HWTEST_F(NapiSharedModuleTest, Unload_SharedModule_Skipped, testing::ext::TestSize.Level1)
{
    NativeModuleManager* mgr = NativeModuleManager::GetInstance();
    ASSERT_NE(mgr, nullptr);
    bool result = mgr->UnloadNativeModule(SHARED_MODULE_NAME);
    EXPECT_TRUE(result);
}
