/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "demo_javascript_class.h"

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "utils/log.h"

#include "itest_dialog_ability.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "if_system_ability_manager.h"
#include "system_ability_definition.h"

#include "ui_service_mgr_interface.h"
#include "test_dialog_ability_proxy.h"

namespace {
constexpr size_t VALUE_BUFFER_SIZE = 128;
} // namespace

/*
 * Sync callback
 */
static napi_value Add(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 2;
    size_t argc = 2;
    napi_value args[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, args[0], &valuetype0));

    napi_valuetype valuetype1;
    NAPI_CALL(env, napi_typeof(env, args[1], &valuetype1));

    NAPI_ASSERT(env, valuetype0 == napi_number && valuetype1 == napi_number, "Wrong argument type. Numbers expected.");

    double value0;
    NAPI_CALL(env, napi_get_value_double(env, args[0], &value0));

    double value1;
    NAPI_CALL(env, napi_get_value_double(env, args[1], &value1));

    napi_value sum;
    NAPI_CALL(env, napi_create_double(env, value0 + value1, &sum));

    return sum;
}

struct AsyncCallbackInfo {
    napi_async_work asyncWork = nullptr;
    napi_deferred deferred = nullptr;
    napi_ref callback[2] = { 0 };
};

/**
 * Promise
 */
static napi_value TestPromise(napi_env env, napi_callback_info)
{
    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));

    auto asyncCallbackInfo = new AsyncCallbackInfo {
        .asyncWork = nullptr,
        .deferred = deferred,
    };

    napi_value resourceName = nullptr;
    napi_create_string_latin1(env, "TestPromise", NAPI_AUTO_LENGTH, &resourceName);
    napi_create_async_work(
        env, nullptr, resourceName, [](napi_env env, void* data) {},
        [](napi_env env, napi_status status, void* data) {
            AsyncCallbackInfo* asyncCallbackInfo = (AsyncCallbackInfo*)data;
            napi_value result = nullptr;
            napi_create_string_utf8(env, "TestPromise", NAPI_AUTO_LENGTH, &result);
            napi_resolve_deferred(env, asyncCallbackInfo->deferred, result);
            napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
            delete asyncCallbackInfo;
        },
        (void*)asyncCallbackInfo, &asyncCallbackInfo->asyncWork);
    napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
    return promise;
}

/*
 * Promise or async callback
 */
static napi_value TestPromiseOrAsyncCallback(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = { 0 };
    napi_value thisArg = nullptr;
    void* data = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, &thisArg, &data));

    auto asyncCallbackInfo = new AsyncCallbackInfo {
        .asyncWork = nullptr,
        .deferred = nullptr,
    };

    if (argc != 0) {
        napi_value resourceName = nullptr;
        napi_create_string_latin1(env, "TestPromiseOrAsyncCallback1", NAPI_AUTO_LENGTH, &resourceName);

        for (size_t i = 0; i < argc; i++) {
            napi_valuetype valuetype = napi_undefined;
            NAPI_CALL(env, napi_typeof(env, args[i], &valuetype));
            NAPI_ASSERT(env, valuetype == napi_function, "Wrong argument type. Function expected.");
            napi_create_reference(env, args[i], 1, &asyncCallbackInfo->callback[i]);
        }

        napi_create_async_work(
            env, nullptr, resourceName, [](napi_env env, void* data) {},
            [](napi_env env, napi_status status, void* data) {
                AsyncCallbackInfo* asyncCallbackInfo = (AsyncCallbackInfo*)data;

                napi_value callback = nullptr;
                napi_value undefined = nullptr;
                napi_value result = nullptr;
                napi_value callbackResult = nullptr;
                napi_create_string_utf8(env, "TestPromiseOrAsyncCallback", NAPI_AUTO_LENGTH, &result);
                napi_get_undefined(env, &undefined);

                if (true) {
                    napi_get_reference_value(env, asyncCallbackInfo->callback[0], &callback);
                    napi_call_function(env, undefined, callback, 1, &result, &callbackResult);
                } else {
                    if (asyncCallbackInfo->callback[1]) {
                        napi_get_reference_value(env, asyncCallbackInfo->callback[1], &callback);
                        napi_call_function(env, undefined, callback, 1, &result, &callbackResult);
                    } else {
                        napi_throw_error(env, "error", "foo");
                    }
                }

                if (asyncCallbackInfo->callback[0] != nullptr) {
                    napi_delete_reference(env, asyncCallbackInfo->callback[0]);
                }
                if (asyncCallbackInfo->callback[1] != nullptr) {
                    napi_delete_reference(env, asyncCallbackInfo->callback[1]);
                }
                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void*)asyncCallbackInfo, &asyncCallbackInfo->asyncWork);

        NAPI_CALL(env, napi_queue_async_work(env, asyncCallbackInfo->asyncWork));

        return nullptr;
    } else {
        napi_value resourceName = nullptr;
        napi_create_string_latin1(env, "TestPromiseOrAsyncCallback2", NAPI_AUTO_LENGTH, &resourceName);

        napi_deferred deferred = nullptr;
        napi_value promise = nullptr;
        NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));

        asyncCallbackInfo->deferred = deferred;

        napi_create_async_work(
            env, nullptr, resourceName, [](napi_env env, void* data) {},
            [](napi_env env, napi_status status, void* data) {
                AsyncCallbackInfo* asyncCallbackInfo = (AsyncCallbackInfo*)data;

                napi_value result = nullptr;
                napi_create_string_utf8(env, "TestPromiseOrAsyncCallback", NAPI_AUTO_LENGTH, &result);
                if (true) {
                    napi_resolve_deferred(env, asyncCallbackInfo->deferred, result);
                } else {
                    napi_reject_deferred(env, asyncCallbackInfo->deferred, result);
                }

                napi_delete_async_work(env, asyncCallbackInfo->asyncWork);
                delete asyncCallbackInfo;
            },
            (void*)asyncCallbackInfo, &asyncCallbackInfo->asyncWork);
        napi_queue_async_work(env, asyncCallbackInfo->asyncWork);
        return promise;
    }
}

static napi_value CheckPermission(napi_env env, napi_callback_info info)
{
    size_t requireArgc = 1;
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= requireArgc, "Wrong number of arguments");

    napi_valuetype valuetype0;
    NAPI_CALL(env, napi_typeof(env, argv[0], &valuetype0));

    NAPI_ASSERT(env, valuetype0 == napi_string, "Wrong argument type. string expected.");

    size_t valueLen = 0;
    char value[VALUE_BUFFER_SIZE] = { 0 };
    napi_get_value_string_utf8(env, argv[0], value, VALUE_BUFFER_SIZE, &valueLen);
    HILOG_INFO("#zhb# native_module_demo: CheckPermission: pinCode: %{public}s", value);

    OHOS::sptr<OHOS::ISystemAbilityManager> systemManager = OHOS::SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (systemManager == nullptr) {
        HILOG_ERROR("#zhb# native_module_demo: %{public}s:fail to get Registry", __func__);
        return nullptr;
    }
    OHOS::sptr<OHOS::IRemoteObject> remoteObject = systemManager->GetSystemAbility(7001);
    if (remoteObject == nullptr) {
        HILOG_ERROR("#zhb# native_module_demo: %{public}s:fail to connect UIMgrService", __func__);
        return nullptr;
    }
    HILOG_INFO("#zhb# native_module_demo: connect UIMgrService success");

    OHOS::sptr<OHOS::Ace::IUIServiceMgr> doms = OHOS::iface_cast<OHOS::Ace::IUIServiceMgr>(remoteObject);
    if (doms == nullptr) {
        HILOG_INFO("#zhb# native_module_demo: doms is null");
        return nullptr;
    }
    HILOG_INFO("#zhb# native_module_demo: get UIMgrService proxy");

    OHOS::sptr<OHOS::IRemoteObject> remoteObject2 = systemManager->GetSystemAbility(1494);
    if (remoteObject2 == nullptr) {
        HILOG_ERROR("#zhb# native_module_demo: %{public}s:fail to connect TestDialogService", __func__);
        return nullptr;
    }
    HILOG_INFO("#zhb# native_module_demo: connect TestDialogService success, IsProxyObject: %{public}d", remoteObject2->IsProxyObject());

    OHOS::sptr<OHOS::ITestDialogAbility> doms2 = OHOS::iface_cast<OHOS::ITestDialogAbility>(remoteObject2);
    if (doms2 == nullptr) {
        HILOG_INFO("#zhb# native_module_demo: doms2 is null");
        return nullptr;
    }

    auto itestDesc = OHOS::ITestDialogAbility::GetDescriptor();
    HILOG_INFO("#zhb# native_module_demo: get TestDialogService proxy, empty: %{public}d, %{public}d", itestDesc.empty(), OHOS::TestDialogAbilityProxy::GetDescriptor().empty());

    std::string pinCode = value;
    HILOG_INFO("#zhb# native_module_demo: CheckPermission begin");
    bool ret = doms2->CheckPermission(pinCode);
    HILOG_INFO("#zhb# native_module_demo: CheckPermission result: %{public}d", ret);

    napi_value retValue = nullptr;
    napi_create_int32(env, ret, &retValue);

    return retValue;
    return nullptr;
}

EXTERN_C_START
/*
 * function for module exports
 */
static napi_value Init(napi_env env, napi_value exports)
{
    /*
     * Properties define
     */
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("add", Add),
        DECLARE_NAPI_FUNCTION("TestPromise", TestPromise),
        DECLARE_NAPI_FUNCTION("TestPromiseOrAsyncCallback", TestPromiseOrAsyncCallback),
        DECLARE_NAPI_FUNCTION("checkPermission", CheckPermission),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));

    DemoJavascriptClassInit(env, exports);

    return exports;
}
EXTERN_C_END

/*
 * Module define
 */
static napi_module demoModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "demo",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};
/*
 * Module register function
 */
extern "C" __attribute__((constructor)) void RegisterModule(void)
{
    napi_module_register(&demoModule);
}
