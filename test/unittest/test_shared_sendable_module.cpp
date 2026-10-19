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
#include <string>
#include <thread>

#include "gtest/gtest.h"
#include "napi/native_api.h"
#include "napi/native_common.h"
#include "napi/native_node_api.h"
#include "test.h"
#include "test_common.h"

namespace {
constexpr int32_t SHARED_ROOT_MARKER = 4396; // distinctive value carried by the shared root

std::atomic<int> g_sharedRootInitCount = 0;
std::atomic<int> g_badRootInitCount = 0;

// Shared-sendable module contract: the register callback receives undefined as exports
// and RETURNS the sendable root object.
napi_value SharedSendableRootInit(napi_env env, napi_value exports)
{
    g_sharedRootInitCount.fetch_add(1);
    napi_value marker = nullptr;
    napi_create_int32(env, SHARED_ROOT_MARKER, &marker);
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_DEFAULT_PROPERTY("marker", marker),
    };
    napi_value root = nullptr;
    napi_status status = napi_create_sendable_object_with_properties(env, 1, desc, &root);
    if (status != napi_ok) {
        return nullptr;
    }
    return root;
}

// Invalid shared module: returns a plain (non-sendable) object.
napi_value BadSharedRootInit(napi_env env, napi_value exports)
{
    g_badRootInitCount.fetch_add(1);
    napi_value root = nullptr;
    napi_create_object(env, &root);
    return root;
}

napi_module g_sharedSendableModule = {
    1,                                          // nm_version
    0,                                          // nm_flags
    nullptr,                                    // nm_filename
    SharedSendableRootInit,                     // nm_register_func
    "testshare.sendable",                       // nm_modname
    nullptr,                                    // nm_priv
    { nullptr, nullptr, nullptr, nullptr },     // reserved
};

napi_module g_badSharedModule = {
    1,                                          // nm_version
    0,                                          // nm_flags
    nullptr,                                    // nm_filename
    BadSharedRootInit,                          // nm_register_func
    "testshare.badroot",                        // nm_modname
    nullptr,                                    // nm_priv
    { nullptr, nullptr, nullptr, nullptr },     // reserved
};

// Data extracted from a module load, computed inside the engine's handle scope (raw
// napi_value must not cross the scope where it was created).
struct LoadResult {
    bool loaded = false;      // a non-undefined exports object was produced
    bool markerReadOk = false;
    int32_t marker = 0;
};

// Loads `moduleName` on `engine` through the production requireNapi entry (the same
// global function JS code calls). Device builds register it as "requireNapi"; host
// PREVIEW builds as "requireNapiPreview", so both names are probed.
LoadResult LoadModuleOnEngine(ArkNativeEngine* engine, const char* moduleName)
{
    LoadResult result;
    napi_env env = reinterpret_cast<napi_env>(engine);
    napi_handle_scope scope = nullptr;
    if (napi_open_handle_scope(env, &scope) != napi_ok) {
        return result;
    }
    napi_value global = nullptr;
    napi_value requireNapi = nullptr;
    napi_valuetype reqType = napi_undefined;
    const char* requireNames[] = { "requireNapi", "requireNapiPreview" };
    if (napi_get_global(env, &global) == napi_ok) {
        for (const char* requireName : requireNames) {
            if (napi_get_named_property(env, global, requireName, &requireNapi) == napi_ok &&
                napi_typeof(env, requireNapi, &reqType) == napi_ok && reqType == napi_function) {
                break;
            }
        }
    }
    if (reqType == napi_function) {
        napi_value name = nullptr;
        napi_value value = nullptr;
        if (napi_create_string_utf8(env, moduleName, NAPI_AUTO_LENGTH, &name) == napi_ok &&
            napi_call_function(env, global, requireNapi, 1, &name, &value) == napi_ok) {
            napi_valuetype type = napi_undefined;
            if (napi_typeof(env, value, &type) == napi_ok && type != napi_undefined) {
                result.loaded = true;
                napi_value markerValue = nullptr;
                result.markerReadOk = napi_get_named_property(env, value, "marker", &markerValue) == napi_ok;
                if (result.markerReadOk) {
                    napi_get_value_int32(env, markerValue, &result.marker);
                }
            }
        }
    }
    napi_value exception = nullptr;
    napi_get_and_clear_last_exception(env, &exception);
    napi_close_handle_scope(env, scope);
    return result;
}

// ets_runtime allows one JSThread per OS thread (thread_local currentThread), so every
// additional VM is created on its own thread, like CheckSendableReferenceValueInThread
// in test_sendable_napi.cpp.
LoadResult LoadModuleInNewThread(const char* moduleName)
{
    LoadResult result;
    std::thread worker([&result, moduleName] {
        panda::RuntimeOption options;
        options.SetGcType(panda::RuntimeOption::GC_TYPE::GEN_GC);
        options.SetLogLevel(panda::RuntimeOption::LOG_LEVEL::ERROR);
        options.SetDebuggerLibraryPath("");
        panda::EcmaVM* vm = panda::JSNApi::CreateJSVM(options);
        if (vm == nullptr) {
            return;
        }
        ArkNativeEngine* engine = new ArkNativeEngine(vm, nullptr);
        result = LoadModuleOnEngine(engine, moduleName);
        delete engine;
        panda::JSNApi::DestroyJSVM(vm);
    });
    worker.join();
    return result;
}
} // namespace

class SharedSendableModuleTest : public NativeEngineTest {
public:
    static void SetUpTestCase()
    {
        // Registered once for the whole binary: the manager, the shared root cache and
        // the init counters are process-level state, which is exactly what is tested.
        napi_shared_module_register(&g_sharedSendableModule);
        napi_shared_module_register(&g_badSharedModule);
    }

    static void TearDownTestCase() {}
};

/**
 * @tc.name: SharedSendableModuleTest001
 * @tc.desc: shared sendable module: callback runs once per process; every VM gets the
 *           same shared root (marker readable in brand-new VMs, also after they die).
 * @tc.type: FUNC
 */
HWTEST_F(SharedSendableModuleTest, SharedSendableModuleTest001, testing::ext::TestSize.Level1)
{
    // VM1 (fixture engine): first load runs the callback once.
    ArkNativeEngine* engine = static_cast<ArkNativeEngine*>(engine_);
    LoadResult first = LoadModuleOnEngine(engine, "testshare.sendable");
    ASSERT_TRUE(first.loaded);
    ASSERT_TRUE(first.markerReadOk);
    ASSERT_EQ(first.marker, SHARED_ROOT_MARKER);
    EXPECT_EQ(g_sharedRootInitCount.load(), 1);

    // VM2 (new thread + new VM in the same process): the cached root is shared, the
    // callback is NOT re-run.
    LoadResult second = LoadModuleInNewThread("testshare.sendable");
    ASSERT_TRUE(second.loaded);
    ASSERT_TRUE(second.markerReadOk);
    ASSERT_EQ(second.marker, SHARED_ROOT_MARKER);
    EXPECT_EQ(g_sharedRootInitCount.load(), 1);

    // VM3: the process-level root outlives VM2's destruction.
    LoadResult third = LoadModuleInNewThread("testshare.sendable");
    ASSERT_TRUE(third.loaded);
    ASSERT_TRUE(third.markerReadOk);
    ASSERT_EQ(third.marker, SHARED_ROOT_MARKER);
    EXPECT_EQ(g_sharedRootInitCount.load(), 1);
}

/**
 * @tc.name: SharedSendableModuleBadRootTest001
 * @tc.desc: non-sendable callback return: the load fails and the failure is permanent in
 *           this process (the callback never runs again).
 * @tc.type: FUNC
 */
HWTEST_F(SharedSendableModuleTest, SharedSendableModuleBadRootTest001, testing::ext::TestSize.Level1)
{
    ArkNativeEngine* engine = static_cast<ArkNativeEngine*>(engine_);
    LoadResult first = LoadModuleOnEngine(engine, "testshare.badroot");
    ASSERT_FALSE(first.loaded);
    EXPECT_EQ(g_badRootInitCount.load(), 1);

    // A second VM fails fast without re-running the callback.
    LoadResult second = LoadModuleInNewThread("testshare.badroot");
    ASSERT_FALSE(second.loaded);
    EXPECT_EQ(g_badRootInitCount.load(), 1);
}
