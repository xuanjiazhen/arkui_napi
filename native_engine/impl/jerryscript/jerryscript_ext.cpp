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

#include "jerryscript_headers.h"

#include "jerryscript-ext/handler.h"

#include "utils/log.h"

struct jerry_external_context {
    void* value = nullptr;
    jerry_finalizer_t callback = nullptr;
    void* hint = nullptr;
};

namespace {
static jerry_object_native_info_t g_objectNativeInfo = {
    .free_cb = [](void *nativePointer) {
        auto externalCtx = (jerry_external_context*)nativePointer;
        if (externalCtx != nullptr) {
            if (externalCtx->callback != nullptr) {
                externalCtx->callback(externalCtx->value, externalCtx->hint);
            }
            delete externalCtx;
            externalCtx = nullptr;
        }
    }
};
} // namspace

bool jerry_add_external()
{
    bool result = false;
    auto callback = [](const jerry_value_t function,
                       const jerry_value_t thisVal,
                       const jerry_value_t args[],
                       const jerry_length_t argc)
                       -> jerry_value_t
                       { 
                           return jerry_acquire_value(thisVal);
                       };

    auto global = jerry_get_global_object();
    auto constructor = jerry_create_external_function(callback);
    auto prototype = jerry_create_object();
    auto resultVal = jerryx_set_property_str(constructor, "prototype", prototype);

    if (jerry_value_is_error(resultVal)) {
        HILOG_ERROR("jerry_add_external failed");
    } else {
        auto setExternalRet = jerryx_set_property_str(global, "External", constructor);
        if (jerry_value_is_error(setExternalRet)) {
            HILOG_ERROR("jerry_add_external failed");
        } else {
            result = true;
        }
        jerry_release_value(setExternalRet);
    }
    jerry_release_value(resultVal);
    jerry_release_value(prototype);
    jerry_release_value(constructor);
    jerry_release_value(global);
    return result;
}

jerry_value_t jerry_create_external(void* value, jerry_finalizer_t finalizer, void* hint)
{
    auto externalCtx = new jerry_external_context{
        .value = value,
        .callback = finalizer,
        .hint = hint,
    };

    jerry_value_t object = 0;
    jerry_value_t global = jerry_get_global_object();
    jerry_value_t external = jerryx_get_property_str(global, "External");

    if (!jerry_value_is_undefined(external)) {
        object = jerry_construct_object(external, nullptr, 0);
        jerry_set_object_native_pointer(object, (void*)externalCtx, &g_objectNativeInfo);
    } else {
        object = jerry_create_undefined();
    }

    jerry_release_value(external);
    jerry_release_value(global);

    return object;
}

bool jerry_value_is_external(const jerry_value_t object)
{
    bool result = false;
    jerry_value_t global = jerry_get_global_object();
    jerry_value_t external = jerryx_get_property_str(global, "External");

    if (!jerry_value_is_undefined(external)) {
        jerry_value_t op = jerry_binary_operation(JERRY_BIN_OP_INSTANCEOF, object, external);
        result = jerry_get_boolean_value(op);
        jerry_release_value(op);
    }

    jerry_release_value(external);
    jerry_release_value(global);
    return result;
}

void* jerry_value_get_external(const jerry_value_t object)
{
    jerry_external_context* externalCtx = nullptr;
    jerry_get_object_native_pointer(object, (void**)&externalCtx, &g_objectNativeInfo);
    if (externalCtx != nullptr) {
        return externalCtx->value;
    } else {
        return nullptr;
    }
}