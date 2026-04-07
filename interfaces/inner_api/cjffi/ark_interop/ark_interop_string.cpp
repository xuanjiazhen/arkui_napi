/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ark_interop_internal.h"
#include "ark_interop_napi.h"
#include "ark_interop_log.h"

#include <cstdint>
#include <cassert>
#include <cstdlib>
#include <vector>

using namespace panda;
using namespace panda::ecmascript;

static constexpr int32_t STRING_TABLE_THRESHOLD = 128;

/**
 * @file ark_interop_string.cpp
 * @brief UTF-8/UTF-16/Latin1 conversion with ArkTS strings; short strings use the VM string table, long ones use `WithoutStringTable`.
 */

/**
 * @brief Creates a string from UTF-8 bytes. `size < 0` usually means NUL-terminated (see `StringRef::NewFromUtf8`).
 *        Below `STRING_TABLE_THRESHOLD` the VM interns; otherwise allocates without the table.
 */
ARKTS_Value ARKTS_CreateUtf8(ARKTS_Env env, const char* value, int32_t size)
{
    ARKTS_ASSERT_U(env, "env is null");
    ARKTS_ASSERT_U(value, "value is null");

    auto vm = P_CAST(env, EcmaVM*);
    Local<JSValueRef> result;
    // Intern/consolidate short literals; long buffers skip the global string table for memory and CPU.
    if (size < STRING_TABLE_THRESHOLD) {
        result = StringRef::NewFromUtf8(vm, value, size);
    } else {
        result = StringRef::NewFromUtf8WithoutStringTable(vm, value, size);
    }
    return ARKTS_FromHandle(result);
}

/** @brief After skipping hole, loads the heap slot and checks `IsString`. */
bool ARKTS_IsString(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_F(env, "env is null");
    ARKTS_ASSERT_F(value.value, "value is null");
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    auto v = BIT_CAST(value, JSValueRef);
    if (v.IsHole() || !v.IsHeapObject()) {
        return false;
    }
    v = *P_CAST(value.pointer, JSValueRef*);
    return v.IsString(vm);
}

/** @brief UTF-8 length from `Utf8Length(..., true)` (VM semantics for surrogate pairs, etc.). */
int32_t ARKTS_GetValueUtf8Size(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_I(env, "env is null");
    ARKTS_ASSERT_I(value.value, "value is null");
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    ARKTS_ASSERT_I(ARKTS_IsString(env, value), "not a string");
    auto v = BIT_CAST(value, Local<StringRef>);
    return v->Utf8Length(vm, true);
}

/**
 * @brief Writes UTF-8 into `buffer` (at most `capacity` bytes; NUL semantics per VM); returns bytes written.
 */
int32_t ARKTS_GetValueUtf8(ARKTS_Env env, ARKTS_Value value, int32_t capacity, char* buffer)
{
    ARKTS_ASSERT_I(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    ARKTS_ASSERT_I(ARKTS_IsString(env, value), "not a string");
    ARKTS_ASSERT_I(capacity == 0 || buffer, "buffer is null when length not 0");
    auto v = BIT_CAST(value, Local<StringRef>);
    return v->WriteUtf8(vm, buffer, capacity, true);
}

/**
 * @brief `malloc`s one buffer and writes UTF-8; caller must free with `ARKTS_FreeCString`.
 * @return `nullptr` for empty string; logs and returns `nullptr` on OOM.
 */
const char* ARKTS_GetValueCString(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_I(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    ARKTS_ASSERT_I(ARKTS_IsString(env, value), "not a string");
    auto v = BIT_CAST(value, Local<StringRef>);
    auto size = v->Utf8Length(vm, true);
    if (size <= 0) {
        return nullptr;
    }
    auto result = (char*) malloc(sizeof(char) * size);
    if (!result) {
        LOGE("ARKTS_GetValueCString fail, out of memory");
        return nullptr;
    }
    v->WriteUtf8(vm, result, size, true);
    return result;
}

/** @brief Frees memory from `GetValueCString` (`free`). */
void ARKTS_FreeCString(const char* src)
{
    free(reinterpret_cast<void*>(const_cast<char*>(src)));
}

/** @brief Whether the string uses compressed storage and its UTF-16 code-unit `Length`. */
ARKTS_StringInfo ARKTS_GetStringInfo(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT(env, "env is null", {});
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    ARKTS_ASSERT(ARKTS_IsString(env, value), "not a string", {});

    auto string = BIT_CAST(value, Local<StringRef>);

    auto isCompressed = string->IsCompressed(vm);
    auto length = string->Length(vm);

    return { isCompressed, length };
}

/**
 * @brief Copies raw units in internal layout: Latin1 if `IsCompressed`, else UTF-16 into `dst` (length is code units).
 */
void ARKTS_StringCopy(ARKTS_Env env, ARKTS_Value value, void* dst, uint32_t length)
{
    ARKTS_ASSERT_V(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    ARKTS_ASSERT_V(ARKTS_IsString(env, value), "not a string");
    ARKTS_ASSERT_V(dst, "dst is null");

    auto string = BIT_CAST(value, Local<StringRef>);
    if (string->IsCompressed(vm)) {
        string->WriteLatin1(vm, reinterpret_cast<char*>(dst), length);
    } else {
        string->WriteUtf16(vm, reinterpret_cast<char16_t*>(dst), length);
    }
}

/**
 * @brief Creates a string from a raw buffer: Latin1 (UTF-8 APIs) vs UTF-16 from `isCompressed`; long strings also bypass the string table.
 */
ARKTS_Value ARKTS_CreateString(ARKTS_Env env, bool isCompressed, uint32_t length, const void* data)
{
    ARKTS_ASSERT_U(env, "env is null");
    ARKTS_ASSERT_U(data, "data is null");

    auto vm = P_CAST(env, EcmaVM*);

    Local<StringRef> result;
    if (length < STRING_TABLE_THRESHOLD) {
        if (isCompressed) {
            result = StringRef::NewFromUtf8(vm, reinterpret_cast<const char *>(data), length);
        } else {
            result = StringRef::NewFromUtf16(vm, reinterpret_cast<const char16_t *>(data), length);
        }
    } else {
        if (isCompressed) {
            result = StringRef::NewFromUtf8WithoutStringTable(vm, reinterpret_cast<const char *>(data), length);
        } else {
            result = StringRef::NewFromUtf16WithoutStringTable(vm, reinterpret_cast<const char16_t *>(data), length);
        }
    }
    return ARKTS_FromHandle(result);
}
