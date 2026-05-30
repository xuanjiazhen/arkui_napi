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

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace {

/***********************************************
 * Constants
 ***********************************************/
static constexpr int REQUIRED_ARGS_ONE = 1;
static constexpr int REQUIRED_ARGS_TWO = 2;
static constexpr int ARG_INDEX_ZERO = 0;
static constexpr int ARG_INDEX_ONE = 1;
static constexpr size_t PROMISE_NAME_SIZE = 128;
static constexpr size_t K_NULL_TERMINATOR_SIZE = 1;
static constexpr uint32_t INITIAL_REF_COUNT = 1;
static constexpr uint32_t K_MODULE_VERSION = 1;
static constexpr uint32_t K_NO_MODULE_FLAGS = 0;
static constexpr int32_t TEST_RESOLVE_VALUE = 42;
static constexpr int32_t TEST_NUMBER_VALUE = 100;
static constexpr int32_t TEST_NUMBER_VALUE_LARGE = 200;
static constexpr double TEST_DOUBLE_VALUE = 3.14;
static constexpr int32_t TEST_COERCE_STR_INT = 456;
static constexpr int32_t TEST_COERCE_OBJ_INT = 789;
static constexpr int32_t TEST_PROP_ALPHA = 10;
static constexpr int32_t TEST_PROP_BETA = 20;
static constexpr int32_t TEST_PROP_GAMMA = 30;
static constexpr int32_t TEST_SYMBOL_PROP_VALUE = 999;
static constexpr double TEST_ADDER_ARG_A = 7.5;
static constexpr double TEST_ADDER_ARG_B = 2.5;

/***********************************************
 * Helper: Set named property on object
 ***********************************************/
static bool SetNamedInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int32(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

static bool SetNamedBool(napi_env env, napi_value object, const char* name, bool value)
{
    napi_value napiValue = nullptr;
    if (napi_get_boolean(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

static bool SetNamedString(napi_env env, napi_value object, const char* name, const char* value)
{
    napi_value napiValue = nullptr;
    if (napi_create_string_utf8(env, value, strlen(value), &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

static bool SetNamedDouble(napi_env env, napi_value object, const char* name, double value)
{
    napi_value napiValue = nullptr;
    if (napi_create_double(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

static bool SetNamedValue(napi_env env, napi_value object, const char* name, napi_value value)
{
    return napi_set_named_property(env, object, name, value) == napi_ok;
}

/***********************************************
 * Test 01: Basic Promise Create and Resolve
 *   - napi_create_promise
 *   - napi_resolve_deferred
 *   - napi_is_promise
 ***********************************************/
static napi_value TestPromiseCreateResolve(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));

    bool isPromise = false;
    NAPI_CALL(env, napi_is_promise(env, promise, &isPromise));
    SetNamedBool(env, result, "isPromise", isPromise);

    napi_value resolveValue = nullptr;
    NAPI_CALL(env, napi_create_int32(env, TEST_RESOLVE_VALUE, &resolveValue));
    NAPI_CALL(env, napi_resolve_deferred(env, deferred, resolveValue));

    SetNamedBool(env, result, "resolved", true);
    SetNamedString(env, result, "testName", "promiseCreateResolve");
    return result;
}

/***********************************************
 * Test 02: Promise Create and Reject
 *   - napi_create_promise
 *   - napi_reject_deferred
 ***********************************************/
static napi_value TestPromiseCreateReject(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_deferred deferred = nullptr;
    napi_value promise = nullptr;
    NAPI_CALL(env, napi_create_promise(env, &deferred, &promise));

    bool isPromise = false;
    NAPI_CALL(env, napi_is_promise(env, promise, &isPromise));
    SetNamedBool(env, result, "isPromise", isPromise);

    napi_value errorMsg = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "test rejection", NAPI_AUTO_LENGTH, &errorMsg));
    napi_value errorObj = nullptr;
    NAPI_CALL(env, napi_create_error(env, nullptr, errorMsg, &errorObj));
    NAPI_CALL(env, napi_reject_deferred(env, deferred, errorObj));

    SetNamedBool(env, result, "rejected", true);
    SetNamedString(env, result, "testName", "promiseCreateReject");
    return result;
}

/***********************************************
 * Test 03: napi_is_promise on non-promise values
 ***********************************************/
static napi_value TestIsPromiseNonPromise(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value plainObj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &plainObj));
    bool isPromise = true;
    NAPI_CALL(env, napi_is_promise(env, plainObj, &isPromise));
    SetNamedBool(env, result, "objectIsPromise", isPromise);

    napi_value number = nullptr;
    NAPI_CALL(env, napi_create_int32(env, TEST_NUMBER_VALUE, &number));
    bool numIsPromise = true;
    NAPI_CALL(env, napi_is_promise(env, number, &numIsPromise));
    SetNamedBool(env, result, "numberIsPromise", numIsPromise);

    napi_value str = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "hello", NAPI_AUTO_LENGTH, &str));
    bool strIsPromise = true;
    NAPI_CALL(env, napi_is_promise(env, str, &strIsPromise));
    SetNamedBool(env, result, "stringIsPromise", strIsPromise);

    napi_value undefinedVal = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &undefinedVal));
    bool undefIsPromise = true;
    NAPI_CALL(env, napi_is_promise(env, undefinedVal, &undefIsPromise));
    SetNamedBool(env, result, "undefinedIsPromise", undefIsPromise);

    SetNamedString(env, result, "testName", "isPromiseNonPromise");
    return result;
}

/***********************************************
 * Test 04: napi_typeof for various value types
 *   - napi_typeof
 *   - napi_get_undefined, napi_get_null
 *   - napi_create_double, napi_create_string_utf8
 *   - napi_create_object, napi_get_boolean
 *   - napi_create_symbol
 ***********************************************/
static napi_value TestTypeofValues(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value undefinedVal = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &undefinedVal));
    napi_valuetype typeUndef = napi_object;
    NAPI_CALL(env, napi_typeof(env, undefinedVal, &typeUndef));
    SetNamedBool(env, result, "undefinedCorrect", typeUndef == napi_undefined);

    napi_value nullVal = nullptr;
    NAPI_CALL(env, napi_get_null(env, &nullVal));
    napi_valuetype typeNull = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, nullVal, &typeNull));
    SetNamedBool(env, result, "nullCorrect", typeNull == napi_null);

    napi_value boolVal = nullptr;
    NAPI_CALL(env, napi_get_boolean(env, true, &boolVal));
    napi_valuetype typeBool = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, boolVal, &typeBool));
    SetNamedBool(env, result, "booleanCorrect", typeBool == napi_boolean);

    napi_value numVal = nullptr;
    NAPI_CALL(env, napi_create_double(env, TEST_DOUBLE_VALUE, &numVal));
    napi_valuetype typeNum = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, numVal, &typeNum));
    SetNamedBool(env, result, "numberCorrect", typeNum == napi_number);

    napi_value strVal = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "test", NAPI_AUTO_LENGTH, &strVal));
    napi_valuetype typeStr = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, strVal, &typeStr));
    SetNamedBool(env, result, "stringCorrect", typeStr == napi_string);

    napi_value objVal = nullptr;
    NAPI_CALL(env, napi_create_object(env, &objVal));
    napi_valuetype typeObj = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, objVal, &typeObj));
    SetNamedBool(env, result, "objectCorrect", typeObj == napi_object);

    napi_value symDesc = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "mySymbol", NAPI_AUTO_LENGTH, &symDesc));
    napi_value symVal = nullptr;
    NAPI_CALL(env, napi_create_symbol(env, symDesc, &symVal));
    napi_valuetype typeSym = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, symVal, &typeSym));
    SetNamedBool(env, result, "symbolCorrect", typeSym == napi_symbol);

    SetNamedString(env, result, "testName", "typeofValues");
    return result;
}

/***********************************************
 * Test 05: Coercion APIs
 *   - napi_coerce_to_bool
 *   - napi_coerce_to_number
 *   - napi_coerce_to_string
 *   - napi_coerce_to_object
 ***********************************************/
static napi_value TestCoercionApis(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value numVal = nullptr;
    NAPI_CALL(env, napi_create_int32(env, TEST_RESOLVE_VALUE, &numVal));
    napi_value coercedBool = nullptr;
    NAPI_CALL(env, napi_coerce_to_bool(env, numVal, &coercedBool));
    bool boolResult = false;
    NAPI_CALL(env, napi_get_value_bool(env, coercedBool, &boolResult));
    SetNamedBool(env, result, "numberToBool", boolResult);

    napi_value strVal = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "123", NAPI_AUTO_LENGTH, &strVal));
    napi_value coercedNum = nullptr;
    NAPI_CALL(env, napi_coerce_to_number(env, strVal, &coercedNum));
    double numResult = 0.0;
    NAPI_CALL(env, napi_get_value_double(env, coercedNum, &numResult));
    SetNamedDouble(env, result, "stringToNumber", numResult);

    napi_value intVal = nullptr;
    NAPI_CALL(env, napi_create_int32(env, TEST_COERCE_STR_INT, &intVal));
    napi_value coercedStr = nullptr;
    NAPI_CALL(env, napi_coerce_to_string(env, intVal, &coercedStr));
    char strBuf[PROMISE_NAME_SIZE] = { 0 };
    size_t strLen = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, coercedStr, strBuf, PROMISE_NAME_SIZE, &strLen));
    SetNamedString(env, result, "numberToString", strBuf);

    napi_value primitiveNum = nullptr;
    NAPI_CALL(env, napi_create_int32(env, TEST_COERCE_OBJ_INT, &primitiveNum));
    napi_value coercedObj = nullptr;
    NAPI_CALL(env, napi_coerce_to_object(env, primitiveNum, &coercedObj));
    napi_valuetype coercedType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, coercedObj, &coercedType));
    SetNamedBool(env, result, "numberToObjectIsObject", coercedType == napi_object);

    SetNamedString(env, result, "testName", "coercionApis");
    return result;
}

/***********************************************
 * Test 06: napi_strict_equals
 ***********************************************/
static napi_value TestStrictEquals(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value a = nullptr;
    napi_value b = nullptr;
    NAPI_CALL(env, napi_create_int32(env, TEST_NUMBER_VALUE, &a));
    NAPI_CALL(env, napi_create_int32(env, TEST_NUMBER_VALUE, &b));
    bool eq = false;
    NAPI_CALL(env, napi_strict_equals(env, a, b, &eq));
    SetNamedBool(env, result, "sameIntEquals", eq);

    napi_value c = nullptr;
    NAPI_CALL(env, napi_create_int32(env, TEST_NUMBER_VALUE_LARGE, &c));
    bool neq = true;
    NAPI_CALL(env, napi_strict_equals(env, a, c, &neq));
    SetNamedBool(env, result, "diffIntNotEquals", !neq);

    napi_value s1 = nullptr;
    napi_value s2 = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "hello", NAPI_AUTO_LENGTH, &s1));
    NAPI_CALL(env, napi_create_string_utf8(env, "hello", NAPI_AUTO_LENGTH, &s2));
    bool strEq = false;
    NAPI_CALL(env, napi_strict_equals(env, s1, s2, &strEq));
    SetNamedBool(env, result, "sameStringEquals", strEq);

    napi_value undVal = nullptr;
    napi_value nullVal = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &undVal));
    NAPI_CALL(env, napi_get_null(env, &nullVal));
    bool undNullEq = true;
    NAPI_CALL(env, napi_strict_equals(env, undVal, nullVal, &undNullEq));
    SetNamedBool(env, result, "undefinedNullNotStrictEqual", !undNullEq);

    SetNamedString(env, result, "testName", "strictEquals");
    return result;
}

/***********************************************
 * Test 07: instanceof check
 *   - napi_instanceof
 *   - napi_create_function
 ***********************************************/
static napi_value DummyConstructor(napi_env env, napi_callback_info info)
{
    napi_value thisVar = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, &thisVar, nullptr));
    return thisVar;
}

static napi_value TestInstanceof(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value ctor = nullptr;
    NAPI_CALL(env, napi_create_function(env, "MyClass", NAPI_AUTO_LENGTH, DummyConstructor, nullptr, &ctor));

    napi_value instance = nullptr;
    NAPI_CALL(env, napi_new_instance(env, ctor, 0, nullptr, &instance));

    bool isInstance = false;
    NAPI_CALL(env, napi_instanceof(env, instance, ctor, &isInstance));
    SetNamedBool(env, result, "instanceOfCtor", isInstance);

    napi_value plainObj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &plainObj));
    bool plainIsInstance = true;
    NAPI_CALL(env, napi_instanceof(env, plainObj, ctor, &plainIsInstance));
    SetNamedBool(env, result, "plainNotInstanceOfCtor", !plainIsInstance);

    SetNamedString(env, result, "testName", "instanceof");
    return result;
}

/***********************************************
 * Test 08: napi_create_reference / napi_get_reference_value / napi_delete_reference
 ***********************************************/
static napi_value TestReferenceLifecycle(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value targetObj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &targetObj));
    SetNamedString(env, targetObj, "key", "refTestValue");

    napi_ref ref = nullptr;
    NAPI_CALL(env, napi_create_reference(env, targetObj, INITIAL_REF_COUNT, &ref));

    napi_value retrieved = nullptr;
    NAPI_CALL(env, napi_get_reference_value(env, ref, &retrieved));

    bool isEqual = false;
    NAPI_CALL(env, napi_strict_equals(env, targetObj, retrieved, &isEqual));
    SetNamedBool(env, result, "refValueEquals", isEqual);

    NAPI_CALL(env, napi_delete_reference(env, ref));
    SetNamedBool(env, result, "refDeleted", true);

    SetNamedString(env, result, "testName", "referenceLifecycle");
    return result;
}

/***********************************************
 * Test 09: napi_create_external / napi_get_value_external
 ***********************************************/
static constexpr int32_t EXTERNAL_MAGIC_TAG = 0xCAFE;

static void ExternalFinalize(napi_env env, void* data, void* hint)
{
    (void)env;
    (void)hint;
    (void)data;
}

static napi_value TestExternalData(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    int32_t externalData = EXTERNAL_MAGIC_TAG;
    napi_value externalVal = nullptr;
    NAPI_CALL(env, napi_create_external(env, &externalData, ExternalFinalize, nullptr, &externalVal));

    napi_valuetype externalType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, externalVal, &externalType));
    SetNamedBool(env, result, "typeIsExternal", externalType == napi_external);

    void* retrievedData = nullptr;
    NAPI_CALL(env, napi_get_value_external(env, externalVal, &retrievedData));

    int32_t* pData = static_cast<int32_t*>(retrievedData);
    SetNamedBool(env, result, "dataMatches", pData != nullptr && *pData == EXTERNAL_MAGIC_TAG);

    SetNamedString(env, result, "testName", "externalData");
    return result;
}

/***********************************************
 * Test 10: napi_is_array and napi_get_array_length
 ***********************************************/
static constexpr uint32_t TEST_ARRAY_LENGTH = 5;

static napi_value TestArrayApis(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value arr = nullptr;
    NAPI_CALL(env, napi_create_array_with_length(env, TEST_ARRAY_LENGTH, &arr));

    bool isArray = false;
    NAPI_CALL(env, napi_is_array(env, arr, &isArray));
    SetNamedBool(env, result, "isArray", isArray);

    uint32_t length = 0;
    NAPI_CALL(env, napi_get_array_length(env, arr, &length));
    SetNamedInt32(env, result, "arrayLength", static_cast<int32_t>(length));

    for (uint32_t i = 0; i < TEST_ARRAY_LENGTH; i++) {
        napi_value elem = nullptr;
        NAPI_CALL(env, napi_create_int32(env, static_cast<int32_t>(i * i), &elem));
        NAPI_CALL(env, napi_set_element(env, arr, i, elem));
    }

    napi_value elemTwo = nullptr;
    NAPI_CALL(env, napi_get_element(env, arr, REQUIRED_ARGS_TWO, &elemTwo));
    int32_t elemTwoVal = 0;
    NAPI_CALL(env, napi_get_value_int32(env, elemTwo, &elemTwoVal));
    SetNamedInt32(env, result, "element2Value", elemTwoVal);

    bool hasElem = false;
    NAPI_CALL(env, napi_has_element(env, arr, 0, &hasElem));
    SetNamedBool(env, result, "hasElement0", hasElem);

    napi_value notArr = nullptr;
    NAPI_CALL(env, napi_create_object(env, &notArr));
    bool notArrResult = true;
    NAPI_CALL(env, napi_is_array(env, notArr, &notArrResult));
    SetNamedBool(env, result, "objectIsNotArray", !notArrResult);

    SetNamedString(env, result, "testName", "arrayApis");
    return result;
}

/***********************************************
 * Test 11: napi_get_prototype
 ***********************************************/
static napi_value TestGetPrototype(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value obj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &obj));

    napi_value proto = nullptr;
    NAPI_CALL(env, napi_get_prototype(env, obj, &proto));

    napi_valuetype protoType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, proto, &protoType));
    SetNamedBool(env, result, "prototypeIsObject", protoType == napi_object);

    SetNamedString(env, result, "testName", "getPrototype");
    return result;
}

/***********************************************
 * Test 12: Property name operations
 *   - napi_get_property_names
 *   - napi_set_named_property
 *   - napi_get_named_property
 *   - napi_has_named_property
 *   - napi_delete_property
 ***********************************************/
static napi_value TestPropertyNames(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value obj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &obj));

    napi_value valA = nullptr;
    NAPI_CALL(env, napi_create_int32(env, TEST_PROP_ALPHA, &valA));
    NAPI_CALL(env, napi_set_named_property(env, obj, "alpha", valA));

    napi_value valB = nullptr;
    NAPI_CALL(env, napi_create_int32(env, TEST_PROP_BETA, &valB));
    NAPI_CALL(env, napi_set_named_property(env, obj, "beta", valB));

    napi_value valG = nullptr;
    NAPI_CALL(env, napi_create_int32(env, TEST_PROP_GAMMA, &valG));
    NAPI_CALL(env, napi_set_named_property(env, obj, "gamma", valG));

    bool hasAlpha = false;
    NAPI_CALL(env, napi_has_named_property(env, obj, "alpha", &hasAlpha));
    SetNamedBool(env, result, "hasAlpha", hasAlpha);

    bool hasDelta = false;
    NAPI_CALL(env, napi_has_named_property(env, obj, "delta", &hasDelta));
    SetNamedBool(env, result, "hasDelta", hasDelta);

    napi_value names = nullptr;
    NAPI_CALL(env, napi_get_property_names(env, obj, &names));
    uint32_t namesLen = 0;
    NAPI_CALL(env, napi_get_array_length(env, names, &namesLen));
    SetNamedInt32(env, result, "propertyCount", static_cast<int32_t>(namesLen));

    napi_value alphaKey = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "gamma", NAPI_AUTO_LENGTH, &alphaKey));
    bool deleted = false;
    NAPI_CALL(env, napi_delete_property(env, obj, alphaKey, &deleted));
    SetNamedBool(env, result, "gammaDeleted", deleted);

    napi_value namesAfter = nullptr;
    NAPI_CALL(env, napi_get_property_names(env, obj, &namesAfter));
    uint32_t namesAfterLen = 0;
    NAPI_CALL(env, napi_get_array_length(env, namesAfter, &namesAfterLen));
    SetNamedInt32(env, result, "propertyCountAfterDelete", static_cast<int32_t>(namesAfterLen));

    SetNamedString(env, result, "testName", "propertyNames");
    return result;
}

/***********************************************
 * Test 13: Dynamic property via key (napi_set_property / napi_get_property / napi_has_property)
 ***********************************************/
static napi_value TestDynamicProperty(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value obj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &obj));

    napi_value key = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "dynamicKey", NAPI_AUTO_LENGTH, &key));
    napi_value val = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "dynamicValue", NAPI_AUTO_LENGTH, &val));
    NAPI_CALL(env, napi_set_property(env, obj, key, val));

    bool hasProp = false;
    NAPI_CALL(env, napi_has_property(env, obj, key, &hasProp));
    SetNamedBool(env, result, "hasProperty", hasProp);

    napi_value got = nullptr;
    NAPI_CALL(env, napi_get_property(env, obj, key, &got));
    char buf[PROMISE_NAME_SIZE] = { 0 };
    size_t len = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, got, buf, PROMISE_NAME_SIZE, &len));
    SetNamedString(env, result, "retrievedValue", buf);

    SetNamedString(env, result, "testName", "dynamicProperty");
    return result;
}

/***********************************************
 * Test 14: napi_create_symbol and use as property key
 ***********************************************/
static napi_value TestSymbolAsKey(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value desc = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "uniqueSymbol", NAPI_AUTO_LENGTH, &desc));
    napi_value sym = nullptr;
    NAPI_CALL(env, napi_create_symbol(env, desc, &sym));

    napi_valuetype symType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, sym, &symType));
    SetNamedBool(env, result, "isSymbol", symType == napi_symbol);

    napi_value obj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &obj));
    napi_value symVal = nullptr;
    NAPI_CALL(env, napi_create_int32(env, TEST_SYMBOL_PROP_VALUE, &symVal));
    NAPI_CALL(env, napi_set_property(env, obj, sym, symVal));

    bool hasSym = false;
    NAPI_CALL(env, napi_has_property(env, obj, sym, &hasSym));
    SetNamedBool(env, result, "hasSymbolProperty", hasSym);

    napi_value retrievedSym = nullptr;
    NAPI_CALL(env, napi_get_property(env, obj, sym, &retrievedSym));
    int32_t retrievedVal = 0;
    NAPI_CALL(env, napi_get_value_int32(env, retrievedSym, &retrievedVal));
    SetNamedInt32(env, result, "symbolPropertyValue", retrievedVal);

    SetNamedString(env, result, "testName", "symbolAsKey");
    return result;
}

/***********************************************
 * Test 15: napi_create_error / napi_create_type_error / napi_create_range_error
 *   - napi_is_error
 ***********************************************/
static napi_value TestErrorCreation(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value errMsg = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "generic error", NAPI_AUTO_LENGTH, &errMsg));
    napi_value errObj = nullptr;
    NAPI_CALL(env, napi_create_error(env, nullptr, errMsg, &errObj));
    bool isError = false;
    NAPI_CALL(env, napi_is_error(env, errObj, &isError));
    SetNamedBool(env, result, "genericIsError", isError);

    napi_value typeErrMsg = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "type error", NAPI_AUTO_LENGTH, &typeErrMsg));
    napi_value typeErr = nullptr;
    NAPI_CALL(env, napi_create_type_error(env, nullptr, typeErrMsg, &typeErr));
    bool isTypeErr = false;
    NAPI_CALL(env, napi_is_error(env, typeErr, &isTypeErr));
    SetNamedBool(env, result, "typeErrorIsError", isTypeErr);

    napi_value rangeErrMsg = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "range error", NAPI_AUTO_LENGTH, &rangeErrMsg));
    napi_value rangeErr = nullptr;
    NAPI_CALL(env, napi_create_range_error(env, nullptr, rangeErrMsg, &rangeErr));
    bool isRangeErr = false;
    NAPI_CALL(env, napi_is_error(env, rangeErr, &isRangeErr));
    SetNamedBool(env, result, "rangeErrorIsError", isRangeErr);

    napi_value notErr = nullptr;
    NAPI_CALL(env, napi_create_object(env, &notErr));
    bool objIsErr = true;
    NAPI_CALL(env, napi_is_error(env, notErr, &objIsErr));
    SetNamedBool(env, result, "objectIsNotError", !objIsErr);

    SetNamedString(env, result, "testName", "errorCreation");
    return result;
}

/***********************************************
 * Test 16: napi_create_int64 / napi_get_value_int64
 *          napi_create_uint32 / napi_get_value_uint32
 ***********************************************/
static constexpr int64_t LARGE_INT64_VALUE = 9007199254740992LL;
static constexpr uint32_t LARGE_UINT32_VALUE = 4000000000U;

static napi_value TestInt64Uint32(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value int64Val = nullptr;
    NAPI_CALL(env, napi_create_int64(env, LARGE_INT64_VALUE, &int64Val));
    int64_t readBack = 0;
    NAPI_CALL(env, napi_get_value_int64(env, int64Val, &readBack));
    SetNamedBool(env, result, "int64RoundTrip", readBack == LARGE_INT64_VALUE);

    napi_value uint32Val = nullptr;
    NAPI_CALL(env, napi_create_uint32(env, LARGE_UINT32_VALUE, &uint32Val));
    uint32_t readBackU32 = 0;
    NAPI_CALL(env, napi_get_value_uint32(env, uint32Val, &readBackU32));
    SetNamedBool(env, result, "uint32RoundTrip", readBackU32 == LARGE_UINT32_VALUE);

    napi_value negInt64Val = nullptr;
    NAPI_CALL(env, napi_create_int64(env, -LARGE_INT64_VALUE, &negInt64Val));
    int64_t negReadBack = 0;
    NAPI_CALL(env, napi_get_value_int64(env, negInt64Val, &negReadBack));
    SetNamedBool(env, result, "negInt64RoundTrip", negReadBack == -LARGE_INT64_VALUE);

    SetNamedString(env, result, "testName", "int64Uint32");
    return result;
}

/***********************************************
 * Test 17: napi_create_string_latin1 / napi_get_value_string_latin1
 ***********************************************/
static constexpr size_t LATIN1_BUF_SIZE = 256;

static napi_value TestStringLatin1(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    const char* testStr = "Hello Latin1";
    napi_value latin1Str = nullptr;
    NAPI_CALL(env, napi_create_string_latin1(env, testStr, NAPI_AUTO_LENGTH, &latin1Str));

    napi_valuetype strType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, latin1Str, &strType));
    SetNamedBool(env, result, "isString", strType == napi_string);

    char buf[LATIN1_BUF_SIZE] = { 0 };
    size_t readLen = 0;
    NAPI_CALL(env, napi_get_value_string_latin1(env, latin1Str, buf, LATIN1_BUF_SIZE, &readLen));
    SetNamedString(env, result, "readValue", buf);
    SetNamedInt32(env, result, "readLength", static_cast<int32_t>(readLen));

    SetNamedString(env, result, "testName", "stringLatin1");
    return result;
}

/***********************************************
 * Test 18: napi_get_global and global property access
 ***********************************************/
static napi_value TestGetGlobal(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value global = nullptr;
    NAPI_CALL(env, napi_get_global(env, &global));

    napi_valuetype globalType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, global, &globalType));
    SetNamedBool(env, result, "globalIsObject", globalType == napi_object);

    napi_value undKey = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "undefined", NAPI_AUTO_LENGTH, &undKey));
    bool hasUndefined = false;
    NAPI_CALL(env, napi_has_property(env, global, undKey, &hasUndefined));
    SetNamedBool(env, result, "globalHasUndefined", hasUndefined);

    SetNamedString(env, result, "testName", "getGlobal");
    return result;
}

/***********************************************
 * Test 19: napi_define_properties
 ***********************************************/
static napi_value GetterCallback(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int32(env, TEST_RESOLVE_VALUE, &result));
    return result;
}

static napi_value TestDefineProperties(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value obj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &obj));

    napi_property_descriptor props[] = {
        { "readonlyProp", nullptr, nullptr, GetterCallback, nullptr, nullptr, napi_default, nullptr },
    };
    NAPI_CALL(env, napi_define_properties(env, obj, sizeof(props) / sizeof(props[0]), props));

    bool hasProp = false;
    NAPI_CALL(env, napi_has_named_property(env, obj, "readonlyProp", &hasProp));
    SetNamedBool(env, result, "hasReadonlyProp", hasProp);

    napi_value propVal = nullptr;
    NAPI_CALL(env, napi_get_named_property(env, obj, "readonlyProp", &propVal));
    int32_t propIntVal = 0;
    NAPI_CALL(env, napi_get_value_int32(env, propVal, &propIntVal));
    SetNamedInt32(env, result, "readonlyPropValue", propIntVal);

    SetNamedString(env, result, "testName", "defineProperties");
    return result;
}

/***********************************************
 * Test 20: napi_wrap / napi_unwrap
 ***********************************************/
struct WrappedData {
    int32_t value;
    char tag[PROMISE_NAME_SIZE];
};

static constexpr int32_t WRAP_TEST_VALUE = 12345;

static void WrappedDataFinalize(napi_env env, void* data, void* hint)
{
    (void)env;
    (void)hint;
    auto* wrapped = static_cast<WrappedData*>(data);
    delete wrapped;
}

static napi_value TestWrapUnwrap(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value obj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &obj));

    auto* wrapData = new WrappedData();
    wrapData->value = WRAP_TEST_VALUE;
    const char* tagSource = "wrapTest";
    size_t tagIdx = 0;
    for (; tagIdx < PROMISE_NAME_SIZE - 1 && tagSource[tagIdx] != '\0'; ++tagIdx) {
        wrapData->tag[tagIdx] = tagSource[tagIdx];
    }
    wrapData->tag[tagIdx] = '\0';

    napi_status wrapStatus = napi_wrap(env, obj, wrapData, WrappedDataFinalize, nullptr, nullptr);
    SetNamedBool(env, result, "wrapOk", wrapStatus == napi_ok);

    void* unwrapped = nullptr;
    napi_status unwrapStatus = napi_unwrap(env, obj, &unwrapped);
    SetNamedBool(env, result, "unwrapOk", unwrapStatus == napi_ok);

    if (unwrapped != nullptr) {
        auto* retrieved = static_cast<WrappedData*>(unwrapped);
        SetNamedInt32(env, result, "unwrappedValue", retrieved->value);
        SetNamedString(env, result, "unwrappedTag", retrieved->tag);
    }

    SetNamedString(env, result, "testName", "wrapUnwrap");
    return result;
}

/***********************************************
 * Test 21: napi_create_function and napi_call_function
 ***********************************************/
static napi_value AdderFunction(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_TWO;
    napi_value argv[REQUIRED_ARGS_TWO] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    double a = 0.0;
    double b = 0.0;
    NAPI_CALL(env, napi_get_value_double(env, argv[ARG_INDEX_ZERO], &a));
    NAPI_CALL(env, napi_get_value_double(env, argv[ARG_INDEX_ONE], &b));

    napi_value sum = nullptr;
    NAPI_CALL(env, napi_create_double(env, a + b, &sum));
    return sum;
}

static napi_value TestCreateCallFunction(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value fn = nullptr;
    NAPI_CALL(env, napi_create_function(env, "adder", NAPI_AUTO_LENGTH, AdderFunction, nullptr, &fn));

    napi_valuetype fnType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, fn, &fnType));
    SetNamedBool(env, result, "isFunction", fnType == napi_function);

    napi_value argA = nullptr;
    napi_value argB = nullptr;
    NAPI_CALL(env, napi_create_double(env, TEST_ADDER_ARG_A, &argA));
    NAPI_CALL(env, napi_create_double(env, TEST_ADDER_ARG_B, &argB));

    napi_value callArgs[REQUIRED_ARGS_TWO] = { argA, argB };
    napi_value global = nullptr;
    NAPI_CALL(env, napi_get_global(env, &global));
    napi_value callResult = nullptr;
    NAPI_CALL(env, napi_call_function(env, global, fn, REQUIRED_ARGS_TWO, callArgs, &callResult));

    double sumResult = 0.0;
    NAPI_CALL(env, napi_get_value_double(env, callResult, &sumResult));
    SetNamedDouble(env, result, "sumResult", sumResult);

    SetNamedString(env, result, "testName", "createCallFunction");
    return result;
}

/***********************************************
 * Test 22: napi_new_instance
 ***********************************************/
static napi_value TestNewInstance(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value ctor = nullptr;
    NAPI_CALL(env, napi_create_function(env, "TestCtor", NAPI_AUTO_LENGTH, DummyConstructor, nullptr, &ctor));

    napi_value instance = nullptr;
    NAPI_CALL(env, napi_new_instance(env, ctor, 0, nullptr, &instance));

    napi_valuetype instanceType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, instance, &instanceType));
    SetNamedBool(env, result, "instanceIsObject", instanceType == napi_object);

    bool isInst = false;
    NAPI_CALL(env, napi_instanceof(env, instance, ctor, &isInst));
    SetNamedBool(env, result, "instanceOfCtor", isInst);

    SetNamedString(env, result, "testName", "newInstance");
    return result;
}

/***********************************************
 * Test 23: Multiple promises lifecycle
 ***********************************************/
static constexpr size_t MULTI_PROMISE_COUNT = 3;

static napi_value TestMultiplePromises(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_deferred deferreds[MULTI_PROMISE_COUNT];
    napi_value promises[MULTI_PROMISE_COUNT];
    for (size_t i = 0; i < MULTI_PROMISE_COUNT; i++) {
        NAPI_CALL(env, napi_create_promise(env, &deferreds[i], &promises[i]));
    }

    int32_t promiseCount = 0;
    for (size_t i = 0; i < MULTI_PROMISE_COUNT; i++) {
        bool isProm = false;
        NAPI_CALL(env, napi_is_promise(env, promises[i], &isProm));
        if (isProm) {
            promiseCount++;
        }
    }
    SetNamedInt32(env, result, "promiseCount", promiseCount);

    for (size_t i = 0; i < MULTI_PROMISE_COUNT; i++) {
        napi_value resolveVal = nullptr;
        NAPI_CALL(env, napi_create_int32(env, static_cast<int32_t>(i) + REQUIRED_ARGS_ONE, &resolveVal));
        NAPI_CALL(env, napi_resolve_deferred(env, deferreds[i], resolveVal));
    }

    SetNamedBool(env, result, "allResolved", true);
    SetNamedString(env, result, "testName", "multiplePromises");
    return result;
}

/***********************************************
 * Test 24: napi_object_freeze / napi_object_seal
 ***********************************************/
static napi_value TestObjectFreezeAndSeal(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value frozenObj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &frozenObj));
    napi_value frozenVal = nullptr;
    NAPI_CALL(env, napi_create_int32(env, TEST_NUMBER_VALUE, &frozenVal));
    NAPI_CALL(env, napi_set_named_property(env, frozenObj, "x", frozenVal));
    napi_status freezeStatus = napi_object_freeze(env, frozenObj);
    SetNamedBool(env, result, "freezeOk", freezeStatus == napi_ok);

    napi_value sealedObj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &sealedObj));
    napi_value sealedVal = nullptr;
    NAPI_CALL(env, napi_create_int32(env, TEST_NUMBER_VALUE_LARGE, &sealedVal));
    NAPI_CALL(env, napi_set_named_property(env, sealedObj, "y", sealedVal));
    napi_status sealStatus = napi_object_seal(env, sealedObj);
    SetNamedBool(env, result, "sealOk", sealStatus == napi_ok);

    SetNamedString(env, result, "testName", "objectFreezeAndSeal");
    return result;
}

/***********************************************
 * Test 25: Callback with user-provided data
 *   Accepts a callback argument; calls it and returns the callback result
 ***********************************************/
static napi_value TestCallbackExecution(napi_env env, napi_callback_info info)
{
    size_t argc = REQUIRED_ARGS_ONE;
    napi_value argv[REQUIRED_ARGS_ONE] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    if (argc < static_cast<size_t>(REQUIRED_ARGS_ONE)) {
        SetNamedBool(env, result, "callbackCalled", false);
        SetNamedString(env, result, "testName", "callbackExecution");
        return result;
    }

    napi_valuetype cbType = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[ARG_INDEX_ZERO], &cbType));
    if (cbType != napi_function) {
        SetNamedBool(env, result, "callbackCalled", false);
        SetNamedString(env, result, "testName", "callbackExecution");
        return result;
    }

    napi_value undefined = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &undefined));

    napi_value testArg = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, "promiseSuiteCallback", NAPI_AUTO_LENGTH, &testArg));
    napi_value cbArgs[REQUIRED_ARGS_ONE] = { testArg };
    napi_value cbResult = nullptr;
    NAPI_CALL(env, napi_call_function(env, undefined, argv[ARG_INDEX_ZERO], REQUIRED_ARGS_ONE, cbArgs, &cbResult));

    SetNamedBool(env, result, "callbackCalled", true);
    if (cbResult != nullptr) {
        SetNamedValue(env, result, "callbackResult", cbResult);
    }

    SetNamedString(env, result, "testName", "callbackExecution");
    return result;
}

/***********************************************
 * Test 26: napi_is_dataview / napi_is_typedarray / napi_is_arraybuffer
 *   - napi_create_arraybuffer
 *   - napi_create_typedarray
 *   - napi_create_dataview
 ***********************************************/
static constexpr size_t ARRAYBUF_BYTE_LENGTH = 16;
static constexpr size_t TYPEDARRAY_LENGTH = 4;
static constexpr size_t DATAVIEW_BYTE_LENGTH = 8;

static napi_value TestBufferTypeChecks(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    void* bufData = nullptr;
    napi_value arrayBuf = nullptr;
    NAPI_CALL(env, napi_create_arraybuffer(env, ARRAYBUF_BYTE_LENGTH, &bufData, &arrayBuf));

    bool isArrayBuf = false;
    NAPI_CALL(env, napi_is_arraybuffer(env, arrayBuf, &isArrayBuf));
    SetNamedBool(env, result, "isArrayBuffer", isArrayBuf);

    napi_value typedArr = nullptr;
    NAPI_CALL(env, napi_create_typedarray(env, napi_int32_array, TYPEDARRAY_LENGTH, arrayBuf, 0, &typedArr));

    bool isTypedArr = false;
    NAPI_CALL(env, napi_is_typedarray(env, typedArr, &isTypedArr));
    SetNamedBool(env, result, "isTypedArray", isTypedArr);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(env, DATAVIEW_BYTE_LENGTH, arrayBuf, 0, &dataView));

    bool isDataView = false;
    NAPI_CALL(env, napi_is_dataview(env, dataView, &isDataView));
    SetNamedBool(env, result, "isDataView", isDataView);

    napi_value plainObj = nullptr;
    NAPI_CALL(env, napi_create_object(env, &plainObj));
    bool objIsArrayBuf = true;
    NAPI_CALL(env, napi_is_arraybuffer(env, plainObj, &objIsArrayBuf));
    SetNamedBool(env, result, "objectIsNotArrayBuffer", !objIsArrayBuf);

    SetNamedString(env, result, "testName", "bufferTypeChecks");
    return result;
}

/***********************************************
 * Test 27: napi_get_cb_info with data pointer
 ***********************************************/
static constexpr int32_t CB_DATA_SENTINEL = 7777;
static int32_t g_cbTestData = CB_DATA_SENTINEL;

static napi_value DataBoundCallback(napi_env env, napi_callback_info info)
{
    void* data = nullptr;
    NAPI_CALL(env, napi_get_cb_info(env, info, nullptr, nullptr, nullptr, &data));

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    if (data != nullptr) {
        int32_t* pData = static_cast<int32_t*>(data);
        SetNamedInt32(env, result, "dataValue", *pData);
        SetNamedBool(env, result, "dataValid", *pData == CB_DATA_SENTINEL);
    } else {
        SetNamedBool(env, result, "dataValid", false);
    }

    SetNamedString(env, result, "testName", "cbInfoDataPointer");
    return result;
}

static napi_value TestCbInfoData(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value fn = nullptr;
    NAPI_CALL(env, napi_create_function(env, "dataBound", NAPI_AUTO_LENGTH,
        DataBoundCallback, &g_cbTestData, &fn));

    napi_value global = nullptr;
    NAPI_CALL(env, napi_get_global(env, &global));
    napi_value fnResult = nullptr;
    NAPI_CALL(env, napi_call_function(env, global, fn, 0, nullptr, &fnResult));

    SetNamedValue(env, result, "innerResult", fnResult);
    SetNamedString(env, result, "testName", "cbInfoData");
    return result;
}

/***********************************************
 * Test 28: napi_get_value_string_utf8 partial read (length query)
 ***********************************************/
static napi_value TestStringLengthQuery(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    const char* testStr = "OpenHarmony NAPI Test";
    napi_value strVal = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, testStr, NAPI_AUTO_LENGTH, &strVal));

    size_t length = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, strVal, nullptr, 0, &length));
    SetNamedInt32(env, result, "stringLength", static_cast<int32_t>(length));

    std::string buf(length + K_NULL_TERMINATOR_SIZE, '\0');
    size_t readLen = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, strVal, buf.data(), buf.size(), &readLen));
    buf.resize(readLen);
    SetNamedString(env, result, "readString", buf.c_str());
    SetNamedBool(env, result, "lengthMatches", length == readLen);

    SetNamedString(env, result, "testName", "stringLengthQuery");
    return result;
}

/***********************************************
 * Test 29: napi_set_element / napi_get_element / napi_has_element / napi_delete_element
 ***********************************************/
static constexpr uint32_t ELEM_TEST_SIZE = 3;

static napi_value TestElementOperations(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value arr = nullptr;
    NAPI_CALL(env, napi_create_array(env, &arr));

    for (uint32_t i = 0; i < ELEM_TEST_SIZE; i++) {
        napi_value elem = nullptr;
        NAPI_CALL(env, napi_create_int32(env, static_cast<int32_t>((i + REQUIRED_ARGS_ONE) * REQUIRED_ARGS_TWO),
            &elem));
        NAPI_CALL(env, napi_set_element(env, arr, i, elem));
    }

    uint32_t arrayLen = 0;
    NAPI_CALL(env, napi_get_array_length(env, arr, &arrayLen));
    SetNamedInt32(env, result, "length", static_cast<int32_t>(arrayLen));

    napi_value elem1 = nullptr;
    NAPI_CALL(env, napi_get_element(env, arr, REQUIRED_ARGS_ONE, &elem1));
    int32_t elem1Val = 0;
    NAPI_CALL(env, napi_get_value_int32(env, elem1, &elem1Val));
    SetNamedInt32(env, result, "element1", elem1Val);

    bool hasElem = false;
    NAPI_CALL(env, napi_has_element(env, arr, 0, &hasElem));
    SetNamedBool(env, result, "hasElement0", hasElem);

    bool deleted = false;
    NAPI_CALL(env, napi_delete_element(env, arr, 0, &deleted));
    SetNamedBool(env, result, "element0Deleted", deleted);

    SetNamedString(env, result, "testName", "elementOperations");
    return result;
}

/***********************************************
 * Test 30: napi_get_last_error_info
 ***********************************************/
static napi_value TestLastErrorInfo(napi_env env, napi_callback_info info)
{
    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    const napi_extended_error_info* errorInfo = nullptr;
    napi_status status = napi_get_last_error_info(env, &errorInfo);
    SetNamedBool(env, result, "getErrorInfoOk", status == napi_ok);
    SetNamedBool(env, result, "errorInfoNotNull", errorInfo != nullptr);

    SetNamedString(env, result, "testName", "lastErrorInfo");
    return result;
}

}  // namespace

/***********************************************
 * Module Initialization
 ***********************************************/

struct PromiseTestEntry {
    const char* name;
    napi_callback callback;
};

static const PromiseTestEntry PROMISE_TESTS[] = {
    { "testPromise01_CreateResolve", TestPromiseCreateResolve },
    { "testPromise02_CreateReject", TestPromiseCreateReject },
    { "testPromise03_IsPromiseNonPromise", TestIsPromiseNonPromise },
    { "testPromise04_TypeofValues", TestTypeofValues },
    { "testPromise05_CoercionApis", TestCoercionApis },
    { "testPromise06_StrictEquals", TestStrictEquals },
    { "testPromise07_Instanceof", TestInstanceof },
    { "testPromise08_ReferenceLifecycle", TestReferenceLifecycle },
    { "testPromise09_ExternalData", TestExternalData },
    { "testPromise10_ArrayApis", TestArrayApis },
    { "testPromise11_GetPrototype", TestGetPrototype },
    { "testPromise12_PropertyNames", TestPropertyNames },
    { "testPromise13_DynamicProperty", TestDynamicProperty },
    { "testPromise14_SymbolAsKey", TestSymbolAsKey },
    { "testPromise15_ErrorCreation", TestErrorCreation },
    { "testPromise16_Int64Uint32", TestInt64Uint32 },
    { "testPromise17_StringLatin1", TestStringLatin1 },
    { "testPromise18_GetGlobal", TestGetGlobal },
    { "testPromise19_DefineProperties", TestDefineProperties },
    { "testPromise20_WrapUnwrap", TestWrapUnwrap },
    { "testPromise21_CreateCallFunction", TestCreateCallFunction },
    { "testPromise22_NewInstance", TestNewInstance },
    { "testPromise23_MultiplePromises", TestMultiplePromises },
    { "testPromise24_ObjectFreezeAndSeal", TestObjectFreezeAndSeal },
    { "testPromise25_CallbackExecution", TestCallbackExecution },
    { "testPromise26_BufferTypeChecks", TestBufferTypeChecks },
    { "testPromise27_CbInfoData", TestCbInfoData },
    { "testPromise28_StringLengthQuery", TestStringLengthQuery },
    { "testPromise29_ElementOperations", TestElementOperations },
    { "testPromise30_LastErrorInfo", TestLastErrorInfo },
};

static constexpr size_t K_PROMISE_TEST_COUNT = sizeof(PROMISE_TESTS) / sizeof(PROMISE_TESTS[0]);

static napi_value InitPromiseSuite(napi_env env, napi_value exports)
{
    std::vector<napi_property_descriptor> descriptors;
    descriptors.reserve(K_PROMISE_TEST_COUNT);

    for (size_t i = 0; i < K_PROMISE_TEST_COUNT; i++) {
        descriptors.push_back(napi_property_descriptor{
            PROMISE_TESTS[i].name, nullptr, PROMISE_TESTS[i].callback, nullptr,
            nullptr, nullptr, napi_default, nullptr
        });
    }

    NAPI_CALL(env, napi_define_properties(env, exports, descriptors.size(), descriptors.data()));
    return exports;
}

static napi_module g_promiseSuiteModule = {
    .nm_version = K_MODULE_VERSION,
    .nm_flags = K_NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitPromiseSuite,
    .nm_modname = "promise_suite",
    .nm_priv = nullptr,
    .reserved = { nullptr },
};

extern "C" __attribute__((constructor)) void RegisterPromiseSuiteModule(void)
{
    napi_module_register(&g_promiseSuiteModule);
}
