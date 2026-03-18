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

#include <string>
#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

static constexpr int REQUIRED_ARGS_ONE = 1;
static constexpr int REQUIRED_ARGS_TWO = 2;
static constexpr int REQUIRED_ARGS_THREE = 3;
static constexpr int ARG_INDEX_ZERO = 0;
static constexpr int ARG_INDEX_ONE = 1;
static constexpr int ARG_INDEX_TWO = 2;
static constexpr int MAX_PROPERTY_NAME_LENGTH = 255;

static napi_value CreateEmptyObject(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));
    return result;
}

static napi_value GetProperty(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: object, property");

    napi_valuetype type = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[ARG_INDEX_ZERO], &type));
    NAPI_ASSERT(env, type == napi_object, "First argument must be an object");

    NAPI_CALL(env, napi_typeof(env, argv[ARG_INDEX_ONE], &type));
    NAPI_ASSERT(env, type == napi_string, "Second argument must be a string");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_property(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], &result));
    return result;
}

static napi_value SetProperty(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_THREE;
    napi_value argv[REQUIRED_ARGS_THREE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_THREE, "Requires 3 arguments: object, property, value");

    napi_valuetype type = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[ARG_INDEX_ZERO], &type));
    NAPI_ASSERT(env, type == napi_object, "First argument must be an object");

    NAPI_CALL(env, napi_typeof(env, argv[ARG_INDEX_ONE], &type));
    NAPI_ASSERT(env, type == napi_string, "Second argument must be a string");

    NAPI_CALL(env, napi_set_property(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], argv[ARG_INDEX_TWO]));

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &result));
    return result;
}

static napi_value HasProperty(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: object, property");

    napi_valuetype type = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[ARG_INDEX_ZERO], &type));
    NAPI_ASSERT(env, type == napi_object, "First argument must be an object");

    NAPI_CALL(env, napi_typeof(env, argv[ARG_INDEX_ONE], &type));
    NAPI_ASSERT(env, type == napi_string, "Second argument must be a string");

    bool hasProperty = false;
    NAPI_CALL(env, napi_has_property(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], &hasProperty));

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, hasProperty, &result));
    return result;
}

static napi_value DeleteProperty(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: object, property");

    napi_valuetype type = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[ARG_INDEX_ZERO], &type));
    NAPI_ASSERT(env, type == napi_object, "First argument must be an object");

    NAPI_CALL(env, napi_typeof(env, argv[ARG_INDEX_ONE], &type));
    NAPI_ASSERT(env, type == napi_string, "Second argument must be a string");

    bool result = false;
    NAPI_CALL(env, napi_delete_property(env, argv[ARG_INDEX_ZERO], argv[ARG_INDEX_ONE], &result));

    napi_value returnValue = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, result, &returnValue));
    return returnValue;
}

static napi_value GetPropertyNames(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: object");

    napi_valuetype type = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[ARG_INDEX_ZERO], &type));
    NAPI_ASSERT(env, type == napi_object, "Argument must be an object");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_property_names(env, argv[ARG_INDEX_ZERO], &result));
    return result;
}

static napi_value GetOwnPropertyNames(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: object");

    napi_valuetype type = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[ARG_INDEX_ZERO], &type));
    NAPI_ASSERT(env, type == napi_object, "Argument must be an object");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_get_all_property_names(env, argv[ARG_INDEX_ZERO], napi_key_own_only,
                                               napi_key_enumerable | napi_key_skip_symbols,
                                               napi_key_keep_numbers, &result));
    return result;
}

static napi_value MergeObjects(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_TWO, "Requires 2 arguments: target, source");

    napi_valuetype type = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[ARG_INDEX_ZERO], &type));
    NAPI_ASSERT(env, type == napi_object, "First argument must be an object");

    NAPI_CALL(env, napi_typeof(env, argv[ARG_INDEX_ONE], &type));
    NAPI_ASSERT(env, type == napi_object, "Second argument must be an object");

    napi_value keys = nullptr;
    NAPI_CALL(env, napi_get_property_names(env, argv[ARG_INDEX_ONE], &keys));

    uint32_t length = 0;
    NAPI_CALL(env, napi_get_array_length(env, keys, &length));

    for (uint32_t i = 0; i < length; i++) {
        napi_value key = nullptr;
        NAPI_CALL(env, napi_get_element(env, keys, i, &key));

        napi_value value = nullptr;
        NAPI_CALL(env, napi_get_property(env, argv[ARG_INDEX_ONE], key, &value));
        NAPI_CALL(env, napi_set_property(env, argv[ARG_INDEX_ZERO], key, value));
    }

    return argv[ARG_INDEX_ZERO];
}

static napi_value CreateObjectFromProperties(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: properties object");

    napi_valuetype type = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[ARG_INDEX_ZERO], &type));
    NAPI_ASSERT(env, type == napi_object, "Argument must be an object");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value keys = nullptr;
    NAPI_CALL(env, napi_get_property_names(env, argv[ARG_INDEX_ZERO], &keys));

    uint32_t length = 0;
    NAPI_CALL(env, napi_get_array_length(env, keys, &length));

    for (uint32_t i = 0; i < length; i++) {
        napi_value key = nullptr;
        NAPI_CALL(env, napi_get_element(env, keys, i, &key));

        napi_value value = nullptr;
        NAPI_CALL(env, napi_get_property(env, argv[ARG_INDEX_ZERO], key, &value));
        NAPI_CALL(env, napi_set_property(env, result, key, value));
    }

    return result;
}

static napi_value DeepCopyObject(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= REQUIRED_ARGS_ONE, "Requires 1 argument: object");

    napi_valuetype type = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[ARG_INDEX_ZERO], &type));
    NAPI_ASSERT(env, type == napi_object, "Argument must be an object");

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value keys = nullptr;
    NAPI_CALL(env, napi_get_property_names(env, argv[ARG_INDEX_ZERO], &keys));

    uint32_t length = 0;
    NAPI_CALL(env, napi_get_array_length(env, keys, &length));

    for (uint32_t i = 0; i < length; i++) {
        napi_value key = nullptr;
        NAPI_CALL(env, napi_get_element(env, keys, i, &key));

        napi_value value = nullptr;
        NAPI_CALL(env, napi_get_property(env, argv[ARG_INDEX_ZERO], key, &value));
        NAPI_CALL(env, napi_set_property(env, result, key, value));
    }

    return result;
}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("createEmpty", CreateEmptyObject),
        DECLARE_NAPI_FUNCTION("get", GetProperty),
        DECLARE_NAPI_FUNCTION("set", SetProperty),
        DECLARE_NAPI_FUNCTION("has", HasProperty),
        DECLARE_NAPI_FUNCTION("delete", DeleteProperty),
        DECLARE_NAPI_FUNCTION("getKeys", GetPropertyNames),
        DECLARE_NAPI_FUNCTION("getOwnKeys", GetOwnPropertyNames),
        DECLARE_NAPI_FUNCTION("merge", MergeObjects),
        DECLARE_NAPI_FUNCTION("createFrom", CreateObjectFromProperties),
        DECLARE_NAPI_FUNCTION("deepCopy", DeepCopyObject),
    };
    NAPI_CALL(env, napi_define_properties(env, exports,
                                      sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

static napi_module objectModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "object",
    .nm_priv = ((void*)0),
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void ObjectRegisterModule(void)
{
    napi_module_register(&objectModule);
}