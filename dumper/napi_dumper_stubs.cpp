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
 *
 * napi_dumper_stubs.cpp — NAPI function stub implementations.
 *
 * This file ONLY contains stub functions that return napi_ok / dummy values.
 * It does NOT reference any global variables, avoiding ThinLTO relocation issues.
 * Uses real napi headers — type signatures match libace_napi.z.so exactly.
 * Compiled as C++ → C++ name mangling is automatic, no asm aliases needed.
 */

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include <unistd.h>
#include <cstring>

static napi_value g_dummy = reinterpret_cast<napi_value>(0xDEAD);

/* Create family */
napi_status napi_create_object(napi_env, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_array(napi_env, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_array_with_length(napi_env, size_t, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_arraybuffer(napi_env, size_t, void** data, napi_value* result)
{
    if (data != nullptr) {
        *data = nullptr;
    }
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_string_utf8(napi_env, const char*, size_t, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_string_utf16(napi_env, const char16_t*, size_t, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_string_latin1(napi_env, const char*, size_t, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_int32(napi_env, int32_t, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_uint32(napi_env, uint32_t, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_int64(napi_env, int64_t, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_double(napi_env, double, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_bigint_int64(napi_env, int64_t, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_bigint_uint64(napi_env, uint64_t, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_bigint_words(napi_env, int, size_t, const uint64_t*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_boolean(napi_env, bool, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_symbol(napi_env, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_error(napi_env, napi_value, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_typedarray(
    napi_env, napi_typedarray_type, size_t, napi_value, size_t, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_dataview(napi_env, size_t, napi_value, size_t, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_promise(napi_env, napi_deferred* deferred, napi_value* result)
{
    if (deferred != nullptr) {
        *deferred = nullptr;
    }
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_reference(napi_env, napi_value, uint32_t, napi_ref* result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_create_async_work(napi_env, napi_value, napi_value,
    napi_async_execute_callback, napi_async_complete_callback, void*, napi_async_work* result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_create_external(
    napi_env, void*, napi_finalize, void*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_external_arraybuffer(
    napi_env, void*, size_t, napi_finalize, void*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_external_buffer(
    napi_env, size_t, void*, napi_finalize, void*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_buffer(napi_env, size_t, void** data, napi_value* result)
{
    if (data != nullptr) {
        *data = nullptr;
    }
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_buffer_copy(
    napi_env, size_t, const void*, void** data, napi_value* result)
{
    if (data != nullptr) {
        *data = nullptr;
    }
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_new_instance(
    napi_env, napi_value, size_t, const napi_value*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_object_with_properties(
    napi_env, napi_value* result, size_t, const napi_property_descriptor*)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_object_with_named_properties(
    napi_env, napi_value* result, size_t, const char**, const napi_value*)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_create_sendable_object_with_properties(
    napi_env, size_t, const napi_property_descriptor*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_define_sendable_class(napi_env, const char*, size_t, napi_callback,
    void*, size_t, const napi_property_descriptor*, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

/* Get family */
napi_status napi_get_null(napi_env, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_get_undefined(napi_env, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_get_global(napi_env, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_get_boolean(napi_env, bool, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_get_reference_value(napi_env, napi_ref, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_get_named_property(napi_env, napi_value, const char*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_get_element(napi_env, napi_value, uint32_t, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_get_array_length(napi_env, napi_value, uint32_t* result)
{
    if (result != nullptr) {
        *result = 0;
    }
    return napi_ok;
}

napi_status napi_get_cb_info(napi_env, napi_callback_info, size_t* argc,
    napi_value*, napi_value* thisArg, void** data)
{
    if (argc != nullptr) {
        *argc = 0;
    }
    if (data != nullptr) {
        *data = nullptr;
    }
    if (thisArg != nullptr) {
        *thisArg = g_dummy;
    }
    return napi_ok;
}

napi_status napi_get_property(napi_env, napi_value, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_get_prototype(napi_env, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_get_property_names(napi_env, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_get_all_property_names(napi_env, napi_value, napi_key_collection_mode,
    napi_key_filter, napi_key_conversion, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_get_value_int32(napi_env, napi_value, int32_t* result)
{
    if (result != nullptr) {
        *result = 0;
    }
    return napi_ok;
}

napi_status napi_get_value_uint32(napi_env, napi_value, uint32_t* result)
{
    if (result != nullptr) {
        *result = 0;
    }
    return napi_ok;
}

napi_status napi_get_value_int64(napi_env, napi_value, int64_t* result)
{
    if (result != nullptr) {
        *result = 0;
    }
    return napi_ok;
}

napi_status napi_get_value_double(napi_env, napi_value, double* result)
{
    if (result != nullptr) {
        *result = 0;
    }
    return napi_ok;
}

napi_status napi_get_value_bool(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_get_value_string_utf8(
    napi_env, napi_value, char* buf, size_t bufSize, size_t* result)
{
    if (buf != nullptr && bufSize > 0) {
        buf[0] = '\0';
    }
    if (result != nullptr) {
        *result = 0;
    }
    return napi_ok;
}

napi_status napi_get_value_string_utf16(
    napi_env, napi_value, char16_t* buf, size_t bufSize, size_t* result)
{
    if (buf != nullptr && bufSize > 0) {
        buf[0] = '\0';
    }
    if (result != nullptr) {
        *result = 0;
    }
    return napi_ok;
}

napi_status napi_get_typedarray_info(napi_env, napi_value, napi_typedarray_type* type,
    size_t* length, void** data, napi_value* arraybuffer, size_t* offset)
{
    if (type != nullptr) {
        *type = static_cast<napi_typedarray_type>(0);
    }
    if (length != nullptr) {
        *length = 0;
    }
    if (data != nullptr) {
        *data = nullptr;
    }
    if (arraybuffer != nullptr) {
        *arraybuffer = g_dummy;
    }
    if (offset != nullptr) {
        *offset = 0;
    }
    return napi_ok;
}

napi_status napi_get_arraybuffer_info(napi_env, napi_value, void** data, size_t* length)
{
    if (data != nullptr) {
        *data = nullptr;
    }
    if (length != nullptr) {
        *length = 0;
    }
    return napi_ok;
}

napi_status napi_get_buffer_info(napi_env, napi_value, void** data, size_t* length)
{
    if (data != nullptr) {
        *data = nullptr;
    }
    if (length != nullptr) {
        *length = 0;
    }
    return napi_ok;
}

napi_status napi_get_version(napi_env, uint32_t* result)
{
    if (result != nullptr) {
        *result = 8;
    }
    return napi_ok;
}

napi_status napi_get_last_error_info(napi_env, const napi_extended_error_info** result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_get_and_clear_last_exception(napi_env, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

/* Is family */
napi_status napi_typeof(napi_env, napi_value, napi_valuetype* result)
{
    if (result != nullptr) {
        *result = napi_undefined;
    }
    return napi_ok;
}

napi_status napi_instanceof(napi_env, napi_value, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_array(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_error(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_typedarray(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_dataview(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_arraybuffer(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_buffer(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_promise(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_exception_pending(napi_env, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_strict_equals(napi_env, napi_value, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = true;
    }
    return napi_ok;
}

/* Coerce */
napi_status napi_coerce_to_bool(napi_env, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_coerce_to_number(napi_env, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_coerce_to_object(napi_env, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_coerce_to_string(napi_env, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

/* Wrap */
napi_status napi_wrap(napi_env, napi_value, void*, napi_finalize, void*, napi_ref* result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_unwrap(napi_env, napi_value, void** result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_remove_wrap(napi_env, napi_value, void** result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_wrap_s(napi_env, napi_value, void*, napi_finalize, void*,
    const napi_type_tag*, napi_ref* result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_unwrap_s(napi_env, napi_value, const napi_type_tag*, void** result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_wrap_sendable_s(napi_env, napi_value, void*, napi_finalize, void*,
    const napi_type_tag*, napi_ref* result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_unwrap_sendable_s(
    napi_env, napi_value, const napi_type_tag*, void** result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_wrap_enhance_s(napi_env, napi_value, void*, napi_finalize, bool,
    void*, size_t, const napi_type_tag*, napi_ref* result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_wrap_async_finalizer(
    napi_env, napi_value, void*, napi_finalize, void*, size_t, napi_ref* result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_create_external_s(
    napi_env, void*, napi_finalize, void*, const napi_type_tag*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_get_value_external_s(
    napi_env, napi_value, const napi_type_tag*, void** result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_coerce_to_native_binding_object(napi_env, napi_value,
    napi_native_binding_detach_callback, napi_native_binding_attach_callback, void*, void*)
{
    return napi_ok;
}

/* Reference */
napi_status napi_delete_reference(napi_env, napi_ref)
{
    return napi_ok;
}

napi_status napi_reference_ref(napi_env, napi_ref, uint32_t* result)
{
    if (result != nullptr) {
        *result = 1;
    }
    return napi_ok;
}

napi_status napi_reference_unref(napi_env, napi_ref, uint32_t* result)
{
    if (result != nullptr) {
        *result = 0;
    }
    return napi_ok;
}

/* Async */
napi_status napi_delete_async_work(napi_env, napi_async_work)
{
    return napi_ok;
}

napi_status napi_queue_async_work(napi_env, napi_async_work)
{
    return napi_ok;
}

napi_status napi_queue_async_work_with_qos(napi_env, napi_async_work, napi_qos_t)
{
    return napi_ok;
}

napi_status napi_create_async_resource(
    napi_env, napi_value, napi_value, napi_async_context* result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_destroy_async_resource(napi_env, napi_async_context)
{
    return napi_ok;
}

/* Promise */
napi_status napi_resolve_deferred(napi_env, napi_deferred, napi_value)
{
    return napi_ok;
}

napi_status napi_reject_deferred(napi_env, napi_deferred, napi_value)
{
    return napi_ok;
}

napi_status napi_delete_deferred(napi_env, napi_deferred)
{
    return napi_ok;
}

napi_status napi_set_promise_rejection_callback(napi_env, napi_ref, napi_ref)
{
    return napi_ok;
}

/* Set/Delete/Has */
napi_status napi_set_property(napi_env, napi_value, napi_value, napi_value)
{
    return napi_ok;
}

napi_status napi_set_named_property(napi_env, napi_value, const char*, napi_value)
{
    return napi_ok;
}

napi_status napi_set_element(napi_env, napi_value, uint32_t, napi_value)
{
    return napi_ok;
}

napi_status napi_delete_property(napi_env, napi_value, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = true;
    }
    return napi_ok;
}

napi_status napi_delete_element(napi_env, napi_value, uint32_t, bool* result)
{
    if (result != nullptr) {
        *result = true;
    }
    return napi_ok;
}

napi_status napi_has_property(napi_env, napi_value, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_has_named_property(napi_env, napi_value, const char*, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_has_element(napi_env, napi_value, uint32_t, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

/* Call/Script */
napi_status napi_call_function(
    napi_env, napi_value, napi_value, size_t, const napi_value*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_run_script(napi_env, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

/* Throw */
napi_status napi_throw(napi_env, napi_value)
{
    return napi_ok;
}

napi_status napi_throw_error(napi_env, const char*, const char*)
{
    return napi_ok;
}

napi_status napi_throw_type_error(napi_env, const char*, const char*)
{
    return napi_ok;
}

napi_status napi_throw_range_error(napi_env, const char*, const char*)
{
    return napi_ok;
}

napi_status napi_throw_jsvalue(napi_env, napi_value)
{
    return napi_ok;
}

napi_status napi_fatal_exception(napi_env, napi_value)
{
    return napi_ok;
}

void napi_fatal_error(const char*, size_t, const char*, size_t)
{
    _exit(1);
}

/* Instance data */
napi_status napi_set_instance_data(napi_env, void*, napi_finalize, void*)
{
    return napi_ok;
}

napi_status napi_get_instance_data(napi_env, void** data)
{
    if (data != nullptr) {
        *data = nullptr;
    }
    return napi_ok;
}

/* Finalizer */
napi_status napi_add_finalizer(
    napi_env, napi_value, void*, napi_finalize, void*, napi_ref* result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_add_env_cleanup_hook(napi_env, void (*)(void*), void*)
{
    return napi_ok;
}

napi_status napi_remove_env_cleanup_hook(napi_env, void (*)(void*), void*)
{
    return napi_ok;
}

napi_status napi_add_detached_finalizer(
    napi_env, napi_value, void*, napi_detach_finalize_callback, void*)
{
    return napi_ok;
}

/* Type tag */
napi_status napi_type_tag_object(napi_env, napi_value, const napi_type_tag*)
{
    return napi_ok;
}

napi_status napi_check_object_type_tag(
    napi_env, napi_value, const napi_type_tag*, bool* result)
{
    if (result != nullptr) {
        *result = true;
    }
    return napi_ok;
}

/* Object freeze/seal */
napi_status napi_object_freeze(napi_env, napi_value)
{
    return napi_ok;
}

napi_status napi_object_seal(napi_env, napi_value)
{
    return napi_ok;
}

/* Handle scope */
napi_status napi_open_handle_scope(napi_env, napi_handle_scope* result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_close_handle_scope(napi_env, napi_handle_scope)
{
    return napi_ok;
}

napi_status napi_open_escapable_handle_scope(
    napi_env, napi_escapable_handle_scope* result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_close_escapable_handle_scope(napi_env, napi_escapable_handle_scope)
{
    return napi_ok;
}

napi_status napi_escape_handle(
    napi_env, napi_escapable_handle_scope, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

/* Callback scope */
napi_status napi_open_callback_scope(
    napi_env, napi_value, napi_async_context, napi_callback_scope* result)
{
    if (result != nullptr) {
        *result = nullptr;
    }
    return napi_ok;
}

napi_status napi_close_callback_scope(napi_env, napi_callback_scope)
{
    return napi_ok;
}

/* Event loop */
napi_status napi_run_event_loop(napi_env, napi_event_mode)
{
    return napi_ok;
}

napi_status napi_stop_event_loop(napi_env)
{
    return napi_ok;
}

/* Module load */
napi_status napi_load_module(napi_env, const char*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_load_module_with_path(napi_env, const char*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_load_module_with_info(
    napi_env, const char*, const char*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

/* Threadsafe */
napi_status napi_acquire_threadsafe_function(napi_threadsafe_function)
{
    return napi_ok;
}

napi_status napi_release_threadsafe_function(
    napi_threadsafe_function, napi_threadsafe_function_release_mode)
{
    return napi_ok;
}

napi_status napi_call_threadsafe_function(
    napi_threadsafe_function, void*, napi_threadsafe_function_call_mode)
{
    return napi_ok;
}

napi_status napi_call_threadsafe_function_with_priority(
    napi_threadsafe_function, void*, napi_task_priority, bool)
{
    return napi_ok;
}

/* Inner API: is_* family */
napi_status napi_is_callable(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = true;
    }
    return napi_ok;
}

napi_status napi_is_arguments_object(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_async_function(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_generator_function(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_generator_object(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_map(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_map_iterator(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_module_namespace_object(
    napi_env, napi_value, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_proxy(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_reg_exp(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_set(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_set_iterator(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_big_int64_array(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_big_uint64_array(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_shared_array_buffer(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_date(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_number_object(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_string_object(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_symbol_object(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_weak_map(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_weak_set(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_bitvector(napi_env, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_is_hybrid_vm(napi_env, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

/* Inner API: misc */
napi_status napi_get_stack_trace(napi_env, std::string&)
{
    return napi_ok;
}

napi_status napi_get_hybrid_stack_trace(napi_env, std::string&)
{
    return napi_ok;
}

napi_status napi_get_own_property_descriptor(
    napi_env, napi_value, const char*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_object_get_keys(napi_env, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_get_print_string(napi_env, napi_value, std::string&)
{
    return napi_ok;
}

napi_status napi_encode(napi_env, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_run_script_path(napi_env, const char*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_run_actor(napi_env, const char*, char*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_get_value_bigint_words(
    napi_env, napi_value, int* sign, size_t* wordCount, uint64_t* words)
{
    if (sign != nullptr) {
        *sign = 0;
    }
    if (wordCount != nullptr) {
        *wordCount = 0;
    }
    (void)words;
    return napi_ok;
}

napi_status napi_get_binary_options(napi_env, napi_value, int* result)
{
    if (result != nullptr) {
        *result = 0;
    }
    return napi_ok;
}

napi_status napi_get_shared_array_buffer_info(napi_env, napi_value, void**, size_t*)
{
    return napi_ok;
}

napi_status napi_open_fast_native_scope(napi_env, napi_fast_native_scope*)
{
    return napi_ok;
}

napi_status napi_close_fast_native_scope(napi_env, napi_fast_native_scope)
{
    return napi_ok;
}

napi_status napi_set_module_validate_callback(napi_module_validate_callback)
{
    return napi_ok;
}

napi_status napi_send_event(napi_env, const std::function<void()>&,
    napi_event_priority, const char*, napi_event_barrier_option)
{
    return napi_ok;
}

napi_status napi_send_cancelable_event(napi_env, const std::function<void(void*)>&,
    void*, napi_event_priority, uint64_t*, const char*, napi_event_barrier_option)
{
    return napi_ok;
}

napi_status napi_cancel_event(napi_env, uint64_t, const char*)
{
    return napi_ok;
}

napi_status napi_adjust_external_memory(napi_env, int64_t, int64_t* result)
{
    if (result != nullptr) {
        *result = 0;
    }
    return napi_ok;
}

napi_status napi_node_api_get_module_file_name(napi_env, const char** result)
{
    if (result != nullptr) {
        *result = "";
    }
    return napi_ok;
}

/* Inner API: map */
napi_status napi_create_map(napi_env, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_map_set_property(napi_env, napi_value, napi_value, napi_value)
{
    return napi_ok;
}

napi_status napi_map_set_named_property(napi_env, napi_value, const char*, napi_value)
{
    return napi_ok;
}

napi_status napi_map_get_property(napi_env, napi_value, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_map_get_named_property(
    napi_env, napi_value, const char*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_map_has_property(napi_env, napi_value, napi_value, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_map_has_named_property(
    napi_env, napi_value, const char*, bool* result)
{
    if (result != nullptr) {
        *result = false;
    }
    return napi_ok;
}

napi_status napi_map_delete_property(napi_env, napi_value, napi_value)
{
    return napi_ok;
}

napi_status napi_map_clear(napi_env, napi_value)
{
    return napi_ok;
}

napi_status napi_map_get_size(napi_env, napi_value, uint32_t* result)
{
    if (result != nullptr) {
        *result = 0;
    }
    return napi_ok;
}

napi_status napi_map_get_entries(napi_env, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_map_get_keys(napi_env, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_map_get_values(napi_env, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

napi_status napi_map_iterator_get_next(napi_env, napi_value, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}

/* Inner API: runtime */
napi_status napi_create_runtime(napi_env, napi_env* result)
{
    if (result != nullptr) {
        *result = reinterpret_cast<napi_env>(0x3);
    }
    return napi_ok;
}

napi_status napi_create_limit_runtime(napi_env, napi_env* result)
{
    if (result != nullptr) {
        *result = reinterpret_cast<napi_env>(0x3);
    }
    return napi_ok;
}

napi_status napi_destroy_runtime(napi_env)
{
    return napi_ok;
}

napi_status napi_create_ark_runtime(napi_env* result)
{
    if (result != nullptr) {
        *result = reinterpret_cast<napi_env>(0x3);
    }
    return napi_ok;
}

napi_status napi_destroy_ark_runtime(napi_env* result)
{
    (void)result;
    return napi_ok;
}

/* Inner API: misc cleanup */
napi_status napi_set_store_global_ref(napi_env, bool)
{
    return napi_ok;
}

napi_status napi_add_cleanup_finalizer(napi_env, void (*)(void*), void*)
{
    return napi_ok;
}

napi_status napi_remove_cleanup_finalizer(napi_env, void (*)(void*), void*)
{
    return napi_ok;
}

napi_status napi_get_dataview_info(
    napi_env, napi_value, size_t*, void**, napi_value*, size_t*)
{
    return napi_ok;
}

napi_status napi_create_copyback_arraybuffer(napi_env, size_t, size_t, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    return napi_ok;
}
