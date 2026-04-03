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

using namespace panda;
using namespace panda::ecmascript;

/**
 * @file ark_interop_symbol.cpp
 * @brief Symbol creation, type test, and description export (description allocated via `GetValueCString`).
 */

/**
 * @brief Creates a Symbol from a UTF-8 description; empty `description` or `length==0` uses an empty string description.
 */
ARKTS_Value ARKTS_CreateSymbol(ARKTS_Env env, const char* description, int32_t length)
{
    ARKTS_ASSERT_U(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);

    panda::Local<panda::JSValueRef> object;

    if (!description || !length) {
        const char* str = "";
        object = panda::StringRef::NewFromUtf8(vm, str, 0);
    } else {
        object = panda::StringRef::NewFromUtf8(vm, description, length);
    }
    auto symbol = panda::SymbolRef::New(vm, object);
    return ARKTS_FromHandle(symbol);
}

/** @brief Heap object and `IsSymbol`. */
bool ARKTS_IsSymbol(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_F(env, "env is null");
    ARKTS_ASSERT_F(value.value, "value is null");
    auto tag = BIT_CAST(value, JSValueRef);
    if (!tag.IsHeapObject()) {
        return false;
    }
    auto vm = P_CAST(env, EcmaVM*);
    JsiFastNativeScope fastNativeScope(vm);
    auto handle = BIT_CAST(value, Local<JSValueRef>);
    return handle->IsSymbol(vm);
}

/**
 * @brief UTF-8 C string for the description (may be `malloc`d); `nullptr` if description is `undefined` or not a string.
 * @warning Same ownership as `ARKTS_GetValueCString`; caller must `FreeCString` when non-null.
 */
const char* ARKTS_GetSymbolDesc(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_P(ARKTS_IsSymbol(env, value), "value is not a symbol");

    auto vm = P_CAST(env, EcmaVM*);
    JsiFastNativeScope fastNativeScope(vm);
    auto symbol = BIT_CAST(value.pointer, Local<SymbolRef>);
    auto desc = symbol->GetDescription(vm);
    auto desc1 = BIT_CAST(desc, ARKTS_Value);
    // Well-known symbols may use undefined description; only allocate a C string for real strings.
    if (ARKTS_IsString(env, desc1)) {
        return ARKTS_GetValueCString(env, BIT_CAST(desc, ARKTS_Value));
    }
    return nullptr;
}
