/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "test_ark_interop_common.h"

namespace {

constexpr int32_t invalidIntVal = 1;
constexpr int32_t invalidIntZero = 0;
constexpr int32_t numValValue = 42;
constexpr int32_t zeroInt = 0;
constexpr int32_t bigintTestVal = 42;

constexpr size_t testPathLen = 4;
constexpr size_t curvesPathLen = 20;
constexpr size_t modulePathLen = 18;

constexpr uint8_t hexVal11 = 0x11;
constexpr uint8_t hexVal22 = 0x22;
constexpr uint8_t hexVal33 = 0x33;
constexpr uint8_t hexVal44 = 0x44;
constexpr uint8_t hexValAA = 0xAA;

constexpr int32_t externalTestData = 123;
constexpr int32_t externalTestData2 = 42;
constexpr int32_t bigintTestData = 99;

constexpr uint16_t utf16Char1 = 0x4F60;
constexpr uint16_t utf16Char2 = 0x597D;
constexpr uint16_t utf16Char3 = 0x0041;
constexpr uint16_t utf16CharZhong = 0x4E2D;

constexpr uint8_t hexValBB = 0xBB;
constexpr uint8_t hexValCC = 0xCC;
constexpr uint8_t hexValDD = 0xDD;

constexpr uint8_t bigintBytes[] = {
    0xFF, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
};

constexpr int64_t externalDataValue = 0x12345678ABCDEF00LL;
constexpr uintptr_t dummyEnvAddress = 0xBEEF;

// === Module Loading Tests ===

TEST_F(ArkInteropTest, RequireArkModuleWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_RequireArkModule(nullptr, "test", testPathLen, ARKTS_NativeModule);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, RequireArkModuleWithNullPath)
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto val = ARKTS_RequireArkModule(env, nullptr, 0, ARKTS_NativeModule);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, RequireArkNativeModule)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto result = ARKTS_RequireArkModule(env, "@native:ohos.curves", curvesPathLen, ARKTS_NativeModule);
    (void)result;

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, RequireArkNormalModule)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto result = ARKTS_RequireArkModule(env, "nonexistent_module", modulePathLen, ARKTS_NormalModule);
    (void)result;

    ARKTS_CloseScope(env, scope);
}

// === Scope Tests ===

TEST_F(ArkInteropTest, CloseScopeWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_CloseScope(nullptr, nullptr);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, OpenScopeWithNullEnv)
{
    ErrorCaptureContext local;
    auto scope = ARKTS_OpenScope(nullptr);
    (void)scope;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, ReturnWithScope)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    constexpr int32_t returnVal = 42;
    auto value = ARKTS_CreateI32(returnVal);
    auto result = ARKTS_Return(env, scope, value);
    EXPECT_TRUE(result.value != 0);
}

// === Global Tests ===

TEST_F(ArkInteropTest, CreateGlobalWithNullEnv)
{
    ErrorCaptureContext local;
    auto result = ARKTS_CreateGlobal(nullptr, ARKTS_CreateI32(invalidIntVal));
    (void)result;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, DisposeGlobalAsync)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, obj);
    ARKTS_DisposeGlobal(env, global);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, DisposeGlobalSyncWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_DisposeGlobalSync(nullptr, nullptr);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, DisposeGlobalWithNullEnv)
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    ARKTS_DisposeGlobal(nullptr, nullptr);
    EXPECT_TRUE(local.HasAndClearNativeError());

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetGlobalNapiEnv)
{
    MockContext local;
    auto env = local.GetEnv();
    auto napiEnv = ARKTS_GetGlobalNapiEnv(env);
    EXPECT_TRUE(napiEnv != nullptr);
}

TEST_F(ArkInteropTest, GetGlobalNapiEnvWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetGlobalNapiEnv(nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetGlobalValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, obj);
    EXPECT_TRUE(ARKTS_GlobalIsAlive(env, global));

    auto val = ARKTS_GetGlobalValue(global);
    EXPECT_TRUE(ARKTS_IsHeapObject(val));

    ARKTS_DisposeGlobalSync(env, global);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetGlobalValueWithNull)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetGlobalValue(nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GlobalClearWeakWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_GlobalClearWeak(nullptr, nullptr);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GlobalConstant)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto globalObj = ARKTS_GetGlobalConstant(env);
    EXPECT_TRUE(ARKTS_IsHeapObject(globalObj));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GlobalFromValueWithNullEnv)
{
    ErrorCaptureContext local;
    auto result = ARKTS_GlobalFromValue(nullptr, ARKTS_CreateI32(invalidIntZero));
    (void)result;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GlobalIsAliveWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_GlobalIsAlive(nullptr, nullptr);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GlobalSetWeakAndClearWeak)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, obj);

    ARKTS_GlobalSetWeak(env, global);
    EXPECT_TRUE(ARKTS_GlobalIsAlive(env, global));

    ARKTS_GlobalClearWeak(env, global);
    EXPECT_TRUE(ARKTS_GlobalIsAlive(env, global));

    ARKTS_DisposeGlobalSync(env, global);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GlobalSetWeakTwice)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, obj);
    ARKTS_GlobalSetWeak(env, global);
    ARKTS_GlobalSetWeak(env, global);
    EXPECT_TRUE(ARKTS_GlobalIsAlive(env, global));

    ARKTS_DisposeGlobalSync(env, global);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GlobalSetWeakWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_GlobalSetWeak(nullptr, nullptr);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GlobalToValueWithNullEnv)
{
    ErrorCaptureContext local;
    auto result = ARKTS_GlobalToValue(nullptr, nullptr);
    (void)result;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

// === Object Tests ===

TEST_F(ArkInteropTest, CreateObjectWithNullEnv)
{
    ErrorCaptureContext local;
    auto obj = ARKTS_CreateObject(nullptr);
    (void)obj;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, DefineAccessors)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto key = ARKTS_CreateUtf8(env, "prop", 4);

    constexpr int32_t getterRetVal = 42;
    auto getterId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_CreateI32(getterRetVal);
    });
    auto getterFunc = ARKTS_CreateFunc(env, getterId);

    ARKTS_Accessor accessor;
    accessor.getter = getterFunc;
    accessor.setter = ARKTS_CreateUndefined();
    accessor.attrs = static_cast<ARKTS_PropertyFlag>(N_CONFIGURABLE | N_ENUMERABLE);

    ARKTS_DefineAccessors(env, obj, key, accessor);

    auto val = ARKTS_GetProperty(env, obj, key);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(val)), getterRetVal);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, DefineAccessorsV2)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto key = ARKTS_CreateUtf8(env, "val", 3);

    constexpr int32_t v2GetterRetVal = 100;
    auto getterId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_CreateI32(v2GetterRetVal);
    });
    auto getterFunc = ARKTS_CreateFunc(env, getterId);

    ARKTS_Accessor accessor;
    accessor.getter = getterFunc;
    accessor.setter = ARKTS_CreateUndefined();
    accessor.attrs = static_cast<ARKTS_PropertyFlag>(N_CONFIGURABLE | N_ENUMERABLE);

    auto result = ARKTS_DefineAccessorsV2(env, obj, key, accessor);
    EXPECT_TRUE(result);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, DefineAccessorsV2WithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_Accessor accessor = {};
    ARKTS_DefineAccessorsV2(nullptr, ARKTS_CreateI32(invalidIntVal), ARKTS_CreateI32(invalidIntVal), accessor);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, DefineAccessorsWithBothGetterSetter)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto key = ARKTS_CreateUtf8(env, "gs", 2);

    constexpr int32_t bothGetterRetVal = 77;
    auto getterId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_CreateI32(bothGetterRetVal);
    });
    auto setterId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_CreateUndefined();
    });

    ARKTS_Accessor accessor;
    accessor.getter = ARKTS_CreateFunc(env, getterId);
    accessor.setter = ARKTS_CreateFunc(env, setterId);
    accessor.attrs = static_cast<ARKTS_PropertyFlag>(N_CONFIGURABLE | N_ENUMERABLE);

    ARKTS_DefineAccessors(env, obj, key, accessor);
    auto val = ARKTS_GetProperty(env, obj, key);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(val)), bothGetterRetVal);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, DefineAccessorsWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_Accessor accessor = {};
    ARKTS_DefineAccessors(nullptr, ARKTS_CreateI32(invalidIntVal), ARKTS_CreateI32(invalidIntVal), accessor);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, DefineOwnProperty)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto key = ARKTS_CreateUtf8(env, "defined", 7);
    constexpr int32_t readBackVal1 = 999;
    auto val = ARKTS_CreateI32(readBackVal1);

    ARKTS_DefineOwnProperty(env, obj, key, val,
        static_cast<ARKTS_PropertyFlag>(N_WRITABLE | N_ENUMERABLE | N_CONFIGURABLE));

    EXPECT_TRUE(ARKTS_HasOwnProperty(env, obj, key));
    auto readBack = ARKTS_GetProperty(env, obj, key);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(readBack)), readBackVal1);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, DefineOwnPropertyV2)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto key = ARKTS_CreateUtf8(env, "myProp", 6);
    constexpr int32_t readBackVal2 = 123;
    auto val = ARKTS_CreateI32(readBackVal2);

    auto result = ARKTS_DefineOwnPropertyV2(env, obj, key, val,
        static_cast<ARKTS_PropertyFlag>(N_WRITABLE | N_ENUMERABLE | N_CONFIGURABLE));
    EXPECT_TRUE(result);

    auto readBack = ARKTS_GetProperty(env, obj, key);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(readBack)), readBackVal2);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, DefineOwnPropertyWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_DefineOwnProperty(nullptr,\r
        ARKTS_CreateI32(invalidIntVal), ARKTS_CreateI32(invalidIntVal), ARKTS_CreateI32(invalidIntVal),\r
        static_cast<ARKTS_PropertyFlag>(N_WRITABLE));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, DefineOwnPropertyWithNumberKey)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    constexpr int32_t numKeyValue = 42;
    auto numKey = ARKTS_CreateI32(numKeyValue);
    constexpr int32_t numPropVal = 999;
    auto val = ARKTS_CreateI32(numPropVal);
    ARKTS_DefineOwnProperty(env, obj, numKey, val,
        static_cast<ARKTS_PropertyFlag>(N_WRITABLE | N_ENUMERABLE | N_CONFIGURABLE));

    EXPECT_TRUE(ARKTS_HasOwnProperty(env, obj, numKey));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, EnumOwnProperties)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto key1 = ARKTS_CreateUtf8(env, "a", 1);
    auto key2 = ARKTS_CreateUtf8(env, "b", 1);
    constexpr int32_t enumPropVal1 = 1;
    constexpr int32_t enumPropVal2 = 2;
    ARKTS_SetProperty(env, obj, key1, ARKTS_CreateI32(enumPropVal1));
    ARKTS_SetProperty(env, obj, key2, ARKTS_CreateI32(enumPropVal2));

    auto props = ARKTS_EnumOwnProperties(env, obj);
    EXPECT_TRUE(ARKTS_IsArray(env, props));
    constexpr int32_t expectedPropCount = 2;
    EXPECT_EQ(ARKTS_GetArrayLength(env, props), expectedPropCount);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, EnumOwnPropertiesWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_EnumOwnProperties(nullptr, ARKTS_CreateI32(invalidIntVal));
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetPropertyWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetProperty(nullptr, ARKTS_CreateI32(invalidIntVal), ARKTS_CreateI32(invalidIntVal));
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, HasOwnPropertyWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_HasOwnProperty(nullptr, ARKTS_CreateI32(invalidIntVal), ARKTS_CreateI32(invalidIntVal));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, HasOwnPropertyWithNumberKey)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    constexpr size_t hasOwnArrSize = 3;
    auto arr = ARKTS_CreateArray(env, hasOwnArrSize);
    constexpr int32_t hasOwnElemVal = 100;
    ARKTS_SetElement(env, arr, 0, ARKTS_CreateI32(hasOwnElemVal));

    auto obj = ARKTS_CreateObject(env);
    auto key = ARKTS_CreateUtf8(env, "x", 1);
    constexpr int32_t setPropVal = 1;
    ARKTS_SetProperty(env, obj, key, ARKTS_CreateI32(setPropVal));
    EXPECT_TRUE(ARKTS_HasOwnProperty(env, obj, key));

    auto missingKey = ARKTS_CreateUtf8(env, "missing", 7);
    EXPECT_FALSE(ARKTS_HasOwnProperty(env, obj, missingKey));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, HasOwnPropertyWithSymbolKey)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto sym = ARKTS_CreateSymbol(env, "propSym", 7);
    ARKTS_SetProperty(env, obj, sym, ARKTS_CreateI32(numValValue));
    EXPECT_TRUE(ARKTS_HasOwnProperty(env, obj, sym));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsObjectWithNonHeapValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsObject(env, ARKTS_CreateI32(numValValue)));
    EXPECT_FALSE(ARKTS_IsObject(env, ARKTS_CreateBool(false)));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsObjectWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_IsObject(nullptr, ARKTS_CreateI32(invalidIntVal));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, SetPropertyWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_SetProperty(nullptr,\r
        ARKTS_CreateI32(invalidIntVal), ARKTS_CreateI32(invalidIntVal), ARKTS_CreateI32(invalidIntVal));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, SetPropertyWithSymbolKey)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto sym = ARKTS_CreateSymbol(env, "mySymbol", 8);

    constexpr int32_t symbolPropertyValue = 777;
    ARKTS_SetProperty(env, obj, sym, ARKTS_CreateI32(symbolPropertyValue));
    auto val = ARKTS_GetProperty(env, obj, sym);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(val)), symbolPropertyValue);

    ARKTS_CloseScope(env, scope);
}

// === Array Tests ===

TEST_F(ArkInteropTest, CreateArrayWithInit)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    constexpr int32_t initDataE0 = 10;
    constexpr int32_t initDataE1 = 20;
    constexpr int32_t initDataE2 = 30;
    ARKTS_Value initData[] = {
        ARKTS_CreateI32(initDataE0), ARKTS_CreateI32(initDataE1), ARKTS_CreateI32(initDataE2)
    };
    constexpr size_t initDataSize = 3;
    constexpr uint32_t firstIdx = 0;
    constexpr uint32_t lastIdx = 2;
    auto arr = ARKTS_CreateArrayWithInit(env, initDataSize, initData);
    EXPECT_TRUE(ARKTS_IsArray(env, arr));
    EXPECT_EQ(ARKTS_GetArrayLength(env, arr), initDataSize);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(ARKTS_GetElement(env, arr, firstIdx))), initDataE0);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(ARKTS_GetElement(env, arr, lastIdx))), initDataE2);

    constexpr size_t emptyArraySize = 0;
    auto emptyArr = ARKTS_CreateArrayWithInit(env, emptyArraySize, nullptr);
    EXPECT_TRUE(ARKTS_IsArray(env, emptyArr));
    EXPECT_EQ(ARKTS_GetArrayLength(env, emptyArr), emptyArraySize);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateArrayWithNullEnv)
{
    ErrorCaptureContext local;
    auto arr = ARKTS_CreateArray(nullptr, 0);
    (void)arr;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetArrayLengthBasic)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    constexpr size_t arrLen = 5;
    auto arr = ARKTS_CreateArray(env, arrLen);
    auto len = ARKTS_GetArrayLength(env, arr);
    EXPECT_EQ(len, arrLen);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsArrayWithNonHeapValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsArray(env, ARKTS_CreateI32(invalidIntVal)));
    EXPECT_FALSE(ARKTS_IsArray(env, ARKTS_CreateNull()));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsArrayWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_IsArray(nullptr, ARKTS_CreateI32(invalidIntVal));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

// === ArrayBuffer Tests ===

TEST_F(ArkInteropTest, ArrayBufferReadBytes)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    constexpr int32_t arrayBufferSize = 4;
    auto ab = ARKTS_CreateArrayBuffer(env, arrayBufferSize);
    auto ptr = reinterpret_cast<uint8_t*>(ARKTS_GetArrayBufferRawPtr(env, ab));
    ptr[0] = hexVal11;
    ptr[1] = hexVal22;
    ptr[2] = hexVal33;
    ptr[3] = hexVal44;

    uint8_t dest[arrayBufferSize] = {};
    auto read = ARKTS_ArrayBufferReadBytes(env, ab, dest, arrayBufferSize);
    EXPECT_EQ(read, arrayBufferSize);
    EXPECT_EQ(dest[0], hexVal11);
    EXPECT_EQ(dest[3], hexVal44);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateArrayBufferWithData)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    uint8_t rawData[] = {hexValAA, hexValBB, hexValCC, hexValDD};
    auto buffer = ARKTS_CreateArrayBufferWithData(env, rawData, sizeof(rawData), 0);
    EXPECT_TRUE(ARKTS_IsArrayBuffer(env, buffer));
    EXPECT_EQ(ARKTS_GetArrayBufferLength(env, buffer), static_cast<int32_t>(sizeof(rawData)));

    auto ptr = reinterpret_cast<uint8_t*>(ARKTS_GetArrayBufferRawPtr(env, buffer));
    EXPECT_EQ(ptr[0], hexValAA);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateArrayBufferWithDataNullBuffer)
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto val = ARKTS_CreateArrayBufferWithData(env, nullptr, 1, 0);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, CreateArrayBufferWithDataNullEnv)
{
    ErrorCaptureContext local;
    uint8_t data[] = {1};
    auto val = ARKTS_CreateArrayBufferWithData(nullptr, data, 1, 0);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, CreateArrayBufferWithNullEnv)
{
    ErrorCaptureContext local;
    auto buf = ARKTS_CreateArrayBuffer(nullptr, 0);
    (void)buf;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, DataViewBufferLengthAndPtr)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    constexpr int32_t dataViewBufferSize = 8;
    auto ab = ARKTS_CreateArrayBuffer(env, dataViewBufferSize);
    auto global = ARKTS_GetGlobalConstant(env);
    auto dvName = ARKTS_CreateUtf8(env, "DataView", -1);
    auto dvCtor = ARKTS_GetProperty(env, global, dvName);
    ASSERT_TRUE(ARKTS_IsCallable(env, dvCtor));

    auto dataView = ARKTS_New(env, dvCtor, 1, &ab);
    EXPECT_TRUE(ARKTS_IsArrayBuffer(env, dataView));

    auto len = ARKTS_GetArrayBufferLength(env, dataView);
    EXPECT_EQ(len, dataViewBufferSize);

    auto ptr = ARKTS_GetArrayBufferRawPtr(env, dataView);
    EXPECT_NE(ptr, nullptr);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsArrayBufferWithNonHeapValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsArrayBuffer(env, ARKTS_CreateI32(invalidIntVal)));
    EXPECT_FALSE(ARKTS_IsArrayBuffer(env, ARKTS_CreateBool(true)));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsArrayBufferWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_IsArrayBuffer(nullptr, ARKTS_CreateI32(invalidIntVal));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, TypedArrayBufferLengthAndPtr)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto global = ARKTS_GetGlobalConstant(env);
    auto ctorName = ARKTS_CreateUtf8(env, "Int8Array", -1);
    auto int8Ctor = ARKTS_GetProperty(env, global, ctorName);
    ASSERT_TRUE(ARKTS_IsCallable(env, int8Ctor));

    constexpr int32_t typedArraySize = 4;
    ARKTS_Value sizeArg = ARKTS_CreateI32(typedArraySize);
    auto typedArr = ARKTS_New(env, int8Ctor, 1, &sizeArg);
    EXPECT_TRUE(ARKTS_IsArrayBuffer(env, typedArr));

    auto len = ARKTS_GetArrayBufferLength(env, typedArr);
    EXPECT_EQ(len, typedArraySize);

    auto ptr = ARKTS_GetArrayBufferRawPtr(env, typedArr);
    EXPECT_NE(ptr, nullptr);

    ARKTS_CloseScope(env, scope);
}

// === String Tests ===

TEST_F(ArkInteropTest, CreateLongUtf8String)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    constexpr size_t longStrLen = 200;
    std::string longStr(longStrLen, 'A');
    auto str = ARKTS_CreateUtf8(env, longStr.c_str(), longStr.size());
    EXPECT_TRUE(ARKTS_IsString(env, str));

    auto size = ARKTS_GetValueUtf8Size(env, str);
    EXPECT_EQ(size, static_cast<int32_t>(longStr.size() + 1));

    auto cstr = ARKTS_GetValueCString(env, str);
    EXPECT_STREQ(cstr, longStr.c_str());
    ARKTS_FreeCString(cstr);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateStringCompressedLong)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    constexpr size_t compressedStrLen = 200;
    std::string longStr(compressedStrLen, 'B');
    auto str = ARKTS_CreateString(env, true, longStr.size(), longStr.c_str());
    EXPECT_TRUE(ARKTS_IsString(env, str));
    auto info = ARKTS_GetStringInfo(env, str);
    EXPECT_EQ(info.length, compressedStrLen);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateStringShortCompressed)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    const char* text = "short";
    auto str = ARKTS_CreateString(env, true, strlen(text), text);
    EXPECT_TRUE(ARKTS_IsString(env, str));
    auto info = ARKTS_GetStringInfo(env, str);
    EXPECT_TRUE(info.isCompressed);
    EXPECT_EQ(info.length, strlen(text));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateStringShortUncompressed)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    uint16_t utf16Data[] = {utf16Char1, utf16Char2};
    auto str = ARKTS_CreateString(env, false, 2, utf16Data);
    EXPECT_TRUE(ARKTS_IsString(env, str));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateStringUncompressed)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    uint16_t utf16Data[] = {utf16Char1, utf16Char2, utf16Char3};
    auto str = ARKTS_CreateString(env, false, 3, utf16Data);
    EXPECT_TRUE(ARKTS_IsString(env, str));
    auto info = ARKTS_GetStringInfo(env, str);
    EXPECT_FALSE(info.isCompressed);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateStringUncompressedLong)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    constexpr size_t uncompressedStrLen = 200;
    std::vector<uint16_t> longUtf16(uncompressedStrLen, utf16CharZhong);
    auto str = ARKTS_CreateString(env, false, longUtf16.size(), longUtf16.data());
    EXPECT_TRUE(ARKTS_IsString(env, str));
    auto info = ARKTS_GetStringInfo(env, str);
    EXPECT_FALSE(info.isCompressed);
    EXPECT_EQ(info.length, uncompressedStrLen);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateStringWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateString(nullptr, true, 1, "a");
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, CreateUtf8WithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateUtf8(nullptr, "t", 1);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetStringInfoWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetStringInfo(nullptr, ARKTS_CreateI32(invalidIntVal));
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetValueCStringAndFree)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto str = ARKTS_CreateUtf8(env, "hello_world", -1);
    const char* cstr = ARKTS_GetValueCString(env, str);
    ASSERT_NE(cstr, nullptr);
    EXPECT_STREQ(cstr, "hello_world");
    ARKTS_FreeCString(cstr);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetValueCStringWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetValueCString(nullptr, ARKTS_CreateI32(invalidIntVal));
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, StringCopy)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    const char* text = "hello world";
    auto str = ARKTS_CreateUtf8(env, text, strlen(text));
    auto info = ARKTS_GetStringInfo(env, str);
    EXPECT_EQ(info.length, strlen(text));

    char buf[64] = {};
    ARKTS_StringCopy(env, str, buf, info.length);
    EXPECT_EQ(std::string(buf, info.length), text);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, StringCopyUncompressed)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    uint16_t utf16Data[] = {utf16Char1, utf16Char2, utf16Char3};
    auto str = ARKTS_CreateString(env, false, 3, utf16Data);
    auto info = ARKTS_GetStringInfo(env, str);
    EXPECT_FALSE(info.isCompressed);

    uint16_t buf[4] = {};
    ARKTS_StringCopy(env, str, buf, info.length);
    EXPECT_EQ(buf[0], utf16Char1);
    EXPECT_EQ(buf[1], utf16Char2);
    EXPECT_EQ(buf[2], utf16Char3);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, StringCopyWithNullEnv)
{
    ErrorCaptureContext local;
    char buf[4];
    ARKTS_StringCopy(nullptr, ARKTS_CreateI32(invalidIntVal), buf, 1);
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, StringGetValueUtf8)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    const char* text = "test string";
    auto str = ARKTS_CreateUtf8(env, text, strlen(text));

    auto size = ARKTS_GetValueUtf8Size(env, str);
    EXPECT_EQ(size, static_cast<int32_t>(strlen(text) + 1));

    char buf[64] = {};
    auto copied = ARKTS_GetValueUtf8(env, str, sizeof(buf), buf);
    EXPECT_GT(copied, 0);
    EXPECT_STREQ(buf, text);

    ARKTS_CloseScope(env, scope);
}

// === Symbol Tests ===

TEST_F(ArkInteropTest, CreateSymbolWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateSymbol(nullptr, "test", 4);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetValueTypeSymbol)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto sym = ARKTS_CreateSymbol(env, "test_sym", -1);
    auto t = ARKTS_GetValueType(env, sym);
    EXPECT_EQ(t, N_SYMBOL);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsSymbolWithNonHeapValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsSymbol(env, ARKTS_CreateI32(numValValue)));
    EXPECT_FALSE(ARKTS_IsSymbol(env, ARKTS_CreateBool(true)));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, SymbolWithEmptyDescription)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto symbol = ARKTS_CreateSymbol(env, "", 0);
    EXPECT_TRUE(ARKTS_IsSymbol(env, symbol));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, SymbolWithNullDescription)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto symbol = ARKTS_CreateSymbol(env, nullptr, 0);
    EXPECT_TRUE(ARKTS_IsSymbol(env, symbol));

    ARKTS_CloseScope(env, scope);
}

// === Promise Tests ===

TEST_F(ArkInteropTest, CreatePromiseCapabilityWithNullEnv)
{
    ErrorCaptureContext local;
    auto p = ARKTS_CreatePromiseCapability(nullptr);
    (void)p;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, IsPromiseWithNonHeapValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsPromise(env, ARKTS_CreateI32(invalidIntZero)));
    EXPECT_FALSE(ARKTS_IsPromise(env, ARKTS_CreateBool(true)));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsPromiseWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_IsPromise(nullptr, ARKTS_CreateI32(invalidIntVal));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, PromiseCapabilityResolveWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_PromiseCapabilityResolve(nullptr, nullptr, ARKTS_CreateI32(invalidIntVal));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, PromiseCatch)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto promiseCap = ARKTS_CreatePromiseCapability(env);
    auto promise = ARKTS_GetPromiseFromCapability(env, promiseCap);

    bool caught = false;
    auto catchId = local.StoreFunc([&caught](ARKTS_CallInfo info) -> ARKTS_Value {
        caught = true;
        return ARKTS_CreateUndefined();
    });
    auto catchFunc = ARKTS_CreateFunc(env, catchId);
    auto catchedPromise = ARKTS_PromiseCatch(env, promise, catchFunc);
    EXPECT_TRUE(ARKTS_IsPromise(env, catchedPromise));

    constexpr int32_t promiseRejectVal = 99;
    ARKTS_PromiseCapabilityReject(env, promiseCap, ARKTS_CreateI32(promiseRejectVal));
    EXPECT_TRUE(caught);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, PromiseCatchWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_PromiseCatch(nullptr, ARKTS_CreateI32(invalidIntVal), ARKTS_CreateI32(invalidIntVal));
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, PromiseThenWithRejectHandler)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto promiseCap = ARKTS_CreatePromiseCapability(env);
    auto promise = ARKTS_GetPromiseFromCapability(env, promiseCap);

    bool fulfilled = false;
    bool rejected = false;

    auto fulfilledId = local.StoreFunc([&fulfilled](ARKTS_CallInfo) -> ARKTS_Value {
        fulfilled = true;
        return ARKTS_CreateUndefined();
    });
    auto rejectedId = local.StoreFunc([&rejected](ARKTS_CallInfo) -> ARKTS_Value {
        rejected = true;
        return ARKTS_CreateUndefined();
    });

    auto onFulfilled = ARKTS_CreateFunc(env, fulfilledId);
    auto onRejected = ARKTS_CreateFunc(env, rejectedId);
    auto thenPromise = ARKTS_PromiseThen(env, promise, onFulfilled, onRejected);
    EXPECT_TRUE(ARKTS_IsPromise(env, thenPromise));

    constexpr int32_t promiseThenRejectVal = 42;
    ARKTS_PromiseCapabilityReject(env, promiseCap, ARKTS_CreateI32(promiseThenRejectVal));
    EXPECT_TRUE(rejected);

    ARKTS_CloseScope(env, scope);
}

// === BigInt Tests ===

TEST_F(ArkInteropTest, BigIntWithBytes)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    uint8_t bytes[sizeof(bigintBytes)];
    std::copy(std::begin(bigintBytes), std::end(bigintBytes), bytes);
    auto bigint = ARKTS_CreateBigIntWithBytes(env, true, sizeof(bytes), bytes);
    EXPECT_TRUE(ARKTS_IsBigInt(env, bigint));

    auto byteSize = ARKTS_BigIntGetByteSize(env, bigint);
    EXPECT_GT(byteSize, 0);

    bool isNeg = false;
    uint8_t readBack[32] = {};
    ARKTS_BigIntReadBytes(env, bigint, &isNeg, sizeof(readBack), readBack);
    EXPECT_TRUE(isNeg);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateBigIntWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateBigInt(nullptr, 1);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetValueTypeBigInt)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto bi = ARKTS_CreateBigInt(env, bigintTestVal);
    auto t = ARKTS_GetValueType(env, bi);
    EXPECT_EQ(t, N_BIGINT);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsBigIntWithNonBigInt)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsBigInt(env, ARKTS_CreateI32(bigintTestVal)));
    EXPECT_FALSE(ARKTS_IsBigInt(env, ARKTS_CreateNull()));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, NegativeBigInt)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    int64_t value = -9876543210LL;
    auto bigint = ARKTS_CreateBigInt(env, value);
    EXPECT_TRUE(ARKTS_IsBigInt(env, bigint));

    bool isNeg = false;
    auto byteSize = ARKTS_BigIntGetByteSize(env, bigint);
    EXPECT_GT(byteSize, 0);

    uint8_t bytes[16] = {};
    ARKTS_BigIntReadBytes(env, bigint, &isNeg, sizeof(bytes), bytes);
    EXPECT_TRUE(isNeg);

    ARKTS_CloseScope(env, scope);
}

// === External Tests ===

TEST_F(ArkInteropTest, CreateExternalWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateExternal(nullptr, 1);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, ExternalCreateAndGet)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    int64_t data = externalDataValue;
    auto ext = ARKTS_CreateExternal(env, data);
    EXPECT_TRUE(ARKTS_IsExternal(env, ext));
    EXPECT_EQ(ARKTS_GetExternalData(env, ext), data);
    EXPECT_EQ(ARKTS_GetValueType(env, ext), N_EXTERNAL);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetExternalDataWithNonExternal)
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto val = ARKTS_GetExternalData(env, ARKTS_CreateI32(invalidIntVal));
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetValueTypeExternal)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto ext = ARKTS_CreateExternal(env, externalTestData);
    auto t = ARKTS_GetValueType(env, ext);
    EXPECT_EQ(t, N_EXTERNAL);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsExternalWithNonHeapValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsExternal(env, ARKTS_CreateI32(invalidIntVal)));
    EXPECT_FALSE(ARKTS_IsExternal(env, ARKTS_CreateNull()));

    ARKTS_CloseScope(env, scope);
}

// === Function Tests ===

TEST_F(ArkInteropTest, CallFuncWithArgs)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto addId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        auto a = ARKTS_GetValueNumber(ARKTS_GetArg(info, 0));
        auto b = ARKTS_GetValueNumber(ARKTS_GetArg(info, 1));
        return ARKTS_CreateF64(a + b);
    });
    auto addFunc = ARKTS_CreateFunc(env, addId);

    constexpr double argA = 10.0;
    constexpr double argB = 20.0;
    constexpr double expectedSum = 30.0;
    constexpr double tolerance = 0.001;
    ARKTS_Value args[] = {ARKTS_CreateF64(argA), ARKTS_CreateF64(argB)};
    auto result = ARKTS_Call(env, addFunc, ARKTS_CreateUndefined(), 2, args);
    EXPECT_NEAR(ARKTS_GetValueNumber(result), expectedSum, tolerance);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CallWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_Call(nullptr, ARKTS_CreateI32(invalidIntVal), ARKTS_CreateUndefined(), 0, nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, CreateFuncWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateFunc(nullptr, 1);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, IsCallableWithNonFunction)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsCallable(env, ARKTS_CreateI32(invalidIntVal)));
    EXPECT_FALSE(ARKTS_IsCallable(env, ARKTS_CreateObject(env)));

    ARKTS_CloseScope(env, scope);
}

// === Class Tests ===

TEST_F(ArkInteropTest, CreateClassNoBase)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto classId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_GetThisArg(info);
    });
    auto clazz = ARKTS_CreateClass(env, classId, ARKTS_CreateI32(invalidIntZero));
    EXPECT_TRUE(ARKTS_IsClass(env, clazz));

    auto proto = ARKTS_GetPrototype(env, clazz);
    EXPECT_TRUE(ARKTS_IsHeapObject(proto));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateClassWithInheritance)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto baseId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_GetThisArg(info);
    });
    auto baseClass = ARKTS_CreateClass(env, baseId, ARKTS_CreateUndefined());
    EXPECT_TRUE(ARKTS_IsClass(env, baseClass));

    auto childId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_GetThisArg(info);
    });
    auto childClass = ARKTS_CreateClass(env, childId, baseClass);
    EXPECT_TRUE(ARKTS_IsClass(env, childClass));

    auto instance = ARKTS_New(env, childClass, 0, nullptr);
    EXPECT_TRUE(ARKTS_InstanceOf(env, instance, childClass));
    EXPECT_TRUE(ARKTS_InstanceOf(env, instance, baseClass));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetPrototypeOfClass)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto classId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_GetThisArg(info);
    });
    auto clazz = ARKTS_CreateClass(env, classId, ARKTS_CreateUndefined());
    EXPECT_TRUE(ARKTS_IsClass(env, clazz));

    auto proto = ARKTS_GetPrototype(env, clazz);
    EXPECT_TRUE(ARKTS_IsHeapObject(proto));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsClassWithNonClass)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsClass(env, ARKTS_CreateI32(invalidIntVal)));
    EXPECT_FALSE(ARKTS_IsClass(env, ARKTS_CreateObject(env)));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, NewInstanceAndInstanceOf)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto classId = local.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_GetThisArg(info);
    });
    auto clazz = ARKTS_CreateClass(env, classId, ARKTS_CreateUndefined());
    EXPECT_TRUE(ARKTS_IsClass(env, clazz));

    auto instance = ARKTS_New(env, clazz, 0, nullptr);
    EXPECT_TRUE(ARKTS_IsObject(env, instance));
    EXPECT_TRUE(ARKTS_InstanceOf(env, instance, clazz));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, NewWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_New(nullptr, ARKTS_CreateI32(invalidIntVal), 0, nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

// === CycleFree Tests ===

TEST_F(ArkInteropTest, CreateCycleFreeExternAndVerify)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto ext = ARKTS_CreateCycleFreeExtern(env, externalTestData2);
    EXPECT_TRUE(ARKTS_IsExternal(env, ext));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateCycleFreeExternWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateCycleFreeExtern(nullptr, 1);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, CreateCycleFreeFuncAndCall)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto func = ARKTS_CreateCycleFreeFunc(env, 1);
    EXPECT_TRUE(ARKTS_IsCallable(env, func));
    auto result = ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);
    (void)result;

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateCycleFreeFuncWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_CreateCycleFreeFunc(nullptr, 1);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, RegisterCycleFreeCallbackDuplicate)
{
    ARKTS_CycleFreeCallback cb;
    cb.funcInvoker = [](ARKTS_CallInfo, int64_t) -> ARKTS_Value { return ARKTS_CreateUndefined(); };
    cb.refRelease = [](int64_t) {};
    ARKTS_RegisterCycleFreeCallback(cb);
}

// === Throw/Exception Tests ===

TEST_F(ArkInteropTest, GetExceptionAndClearNoException)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto result = ARKTS_GetExceptionAndClear(env);
    (void)result;

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetExceptionAndClearWithNullEnv)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetExceptionAndClear(nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, ThrowDuplicateErrorConflict)
{
    ErrorCaptureContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto err1 = ARKTS_CreateUtf8(env, "first", -1);
    ARKTS_Throw(env, err1);
    auto err2 = ARKTS_CreateUtf8(env, "second", -1);
    ARKTS_Throw(env, err2);

    auto caught = ARKTS_GetExceptionAndClear(env);
    EXPECT_TRUE(ARKTS_IsHeapObject(caught));
    local.HasAndClearNativeError();
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ThrowError)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto strErr = ARKTS_CreateUtf8(env, "test error", -1);
    ARKTS_Throw(env, strErr);

    auto exception = ARKTS_GetExceptionAndClear(env);
    EXPECT_FALSE(ARKTS_IsUndefined(exception));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ThrowObjectError)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    ARKTS_Throw(env, obj);

    auto exception = ARKTS_GetExceptionAndClear(env);
    EXPECT_FALSE(ARKTS_IsUndefined(exception));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ThrowObjectErrorAndCatch)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto global = ARKTS_GetGlobalConstant(env);
    auto errorCtorName = ARKTS_CreateUtf8(env, "Error", -1);
    auto errorCtor = ARKTS_GetProperty(env, global, errorCtorName);
    auto msg = ARKTS_CreateUtf8(env, "test throw", -1);
    auto errorObj = ARKTS_New(env, errorCtor, 1, &msg);
    ARKTS_Throw(env, errorObj);
    auto caught = ARKTS_GetExceptionAndClear(env);
    EXPECT_TRUE(ARKTS_IsHeapObject(caught));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ThrowWithNullEnv)
{
    ErrorCaptureContext local;
    ARKTS_Throw(nullptr, ARKTS_CreateI32(invalidIntVal));
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, ThrowWithPendingException)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto strErr = ARKTS_CreateUtf8(env, "first error", -1);
    ARKTS_Throw(env, strErr);

    auto strErr2 = ARKTS_CreateUtf8(env, "second error", -1);
    ARKTS_Throw(env, strErr2);

    auto exception = ARKTS_GetExceptionAndClear(env);
    (void)exception;

    ARKTS_CloseScope(env, scope);
}

// === CallInfo Tests ===

TEST_F(ArkInteropTest, GetArgCountWithNull)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetArgCount(nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetArgWithNull)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetArg(nullptr, 0);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetCallEnvWithNull)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetCallEnv(nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetThisArgWithNull)
{
    ErrorCaptureContext local;
    auto val = ARKTS_GetThisArg(nullptr);
    (void)val;
    EXPECT_TRUE(local.HasAndClearNativeError());
}

// === Value/Type Tests ===

TEST_F(ArkInteropTest, GetValueNumberF64)
{
    constexpr double expectedEuler = 2.718;
    auto val = ARKTS_CreateF64(expectedEuler);
    constexpr double tolerance = 0.001;
    EXPECT_NEAR(ARKTS_GetValueNumber(val), expectedEuler, tolerance);

    constexpr int32_t expectedIntVal = 100;
    auto intVal = ARKTS_CreateI32(expectedIntVal);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(intVal)), expectedIntVal);
}

TEST_F(ArkInteropTest, GetValueTypeFunction)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto funcId = local.StoreFunc([](ARKTS_CallInfo) { return ARKTS_CreateUndefined(); });
    auto func = ARKTS_CreateFunc(env, funcId);
    auto t = ARKTS_GetValueType(env, func);
    EXPECT_EQ(t, N_FUNCTION);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetValueTypeNull)
{
    MockContext local;
    auto env = local.GetEnv();
    auto t = ARKTS_GetValueType(env, ARKTS_CreateNull());
    EXPECT_EQ(t, N_NULL);
}

TEST_F(ArkInteropTest, GetValueTypeObject)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto obj = ARKTS_CreateObject(env);
    auto t = ARKTS_GetValueType(env, obj);
    EXPECT_EQ(t, N_OBJECT);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetValueTypeVariants)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_EQ(ARKTS_GetValueType(env, ARKTS_CreateUndefined()), N_UNDEFINED);
    EXPECT_EQ(ARKTS_GetValueType(env, ARKTS_CreateNull()), N_NULL);
    EXPECT_EQ(ARKTS_GetValueType(env, ARKTS_CreateBool(true)), N_BOOL);
    EXPECT_EQ(ARKTS_GetValueType(env, ARKTS_CreateI32(numValValue)), N_NUMBER);
    constexpr double piValue = 3.14;
    EXPECT_EQ(ARKTS_GetValueType(env, ARKTS_CreateF64(piValue)), N_NUMBER);

    auto str = ARKTS_CreateUtf8(env, "test", 4);
    EXPECT_EQ(ARKTS_GetValueType(env, str), N_STRING);

    auto sym = ARKTS_CreateSymbol(env, "sym", 3);
    EXPECT_EQ(ARKTS_GetValueType(env, sym), N_SYMBOL);

    auto obj = ARKTS_CreateObject(env);
    EXPECT_EQ(ARKTS_GetValueType(env, obj), N_OBJECT);

    auto funcId = local.StoreFunc([](ARKTS_CallInfo) -> ARKTS_Value {
        return ARKTS_CreateUndefined();
    });
    auto func = ARKTS_CreateFunc(env, funcId);
    EXPECT_EQ(ARKTS_GetValueType(env, func), N_FUNCTION);

    auto ext = ARKTS_CreateExternal(env, externalTestData2);
    EXPECT_EQ(ARKTS_GetValueType(env, ext), N_EXTERNAL);

    auto bigint = ARKTS_CreateBigInt(env, bigintTestData);
    EXPECT_EQ(ARKTS_GetValueType(env, bigint), N_BIGINT);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsHeapObjectWithPrimitive)
{
    EXPECT_FALSE(ARKTS_IsHeapObject(ARKTS_CreateI32(numValValue)));
    EXPECT_FALSE(ARKTS_IsHeapObject(ARKTS_CreateBool(true)));
}

TEST_F(ArkInteropTest, IsNumberVariants)
{
    EXPECT_TRUE(ARKTS_IsNumber(ARKTS_CreateI32(zeroInt)));
    constexpr double zeroDouble = 0.0;
    EXPECT_TRUE(ARKTS_IsNumber(ARKTS_CreateF64(zeroDouble)));
    EXPECT_FALSE(ARKTS_IsNumber(ARKTS_CreateBool(true)));
    EXPECT_FALSE(ARKTS_IsNumber(ARKTS_CreateNull()));
    EXPECT_FALSE(ARKTS_IsNumber(ARKTS_CreateUndefined()));
}

TEST_F(ArkInteropTest, StrictEqualBooleans)
{
    MockContext local;
    auto env = local.GetEnv();
    EXPECT_TRUE(ARKTS_StrictEqual(env, ARKTS_CreateBool(true), ARKTS_CreateBool(true)));
    EXPECT_FALSE(ARKTS_StrictEqual(env, ARKTS_CreateBool(true), ARKTS_CreateBool(false)));
}

TEST_F(ArkInteropTest, StrictEqualDifferentTypes)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    constexpr int32_t diffTypeNum = 1;
    EXPECT_FALSE(ARKTS_StrictEqual(env, ARKTS_CreateI32(diffTypeNum), ARKTS_CreateUtf8(env, "1", 1)));
    EXPECT_FALSE(ARKTS_StrictEqual(env, ARKTS_CreateBool(true), ARKTS_CreateI32(diffTypeNum)));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, StrictEqualHeapObjects)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto str1 = ARKTS_CreateUtf8(env, "hello", 5);
    auto str2 = ARKTS_CreateUtf8(env, "hello", 5);
    EXPECT_TRUE(ARKTS_StrictEqual(env, str1, str2));

    auto str3 = ARKTS_CreateUtf8(env, "world", 5);
    EXPECT_FALSE(ARKTS_StrictEqual(env, str1, str3));

    auto obj1 = ARKTS_CreateObject(env);
    auto obj2 = ARKTS_CreateObject(env);
    EXPECT_TRUE(ARKTS_StrictEqual(env, obj1, obj1));
    EXPECT_FALSE(ARKTS_StrictEqual(env, obj1, obj2));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, StrictEqualNumbers)
{
    MockContext local;
    auto env = local.GetEnv();
    constexpr int32_t testNum = 42;
    constexpr double testPi = 3.14;
    EXPECT_TRUE(ARKTS_StrictEqual(env, ARKTS_CreateI32(testNum), ARKTS_CreateI32(testNum)));
    constexpr int32_t unequalNum1 = 1;
    constexpr int32_t unequalNum2 = 2;
    EXPECT_FALSE(ARKTS_StrictEqual(env, ARKTS_CreateI32(unequalNum1), ARKTS_CreateI32(unequalNum2)));
    EXPECT_TRUE(ARKTS_StrictEqual(env, ARKTS_CreateF64(testPi), ARKTS_CreateF64(testPi)));
}

TEST_F(ArkInteropTest, StrictEqualSameUndefined)
{
    MockContext local;
    auto env = local.GetEnv();
    EXPECT_TRUE(ARKTS_StrictEqual(env, ARKTS_CreateUndefined(), ARKTS_CreateUndefined()));
    EXPECT_TRUE(ARKTS_StrictEqual(env, ARKTS_CreateNull(), ARKTS_CreateNull()));
}

// === Primitive Types Tests ===

TEST_F(ArkInteropTest, BoolOperations)
{
    auto t = ARKTS_CreateBool(true);
    auto f = ARKTS_CreateBool(false);
    EXPECT_TRUE(ARKTS_IsBool(t));
    EXPECT_TRUE(ARKTS_IsBool(f));
    EXPECT_TRUE(ARKTS_GetValueBool(t));
    EXPECT_FALSE(ARKTS_GetValueBool(f));
    EXPECT_FALSE(ARKTS_IsBool(ARKTS_CreateI32(zeroInt)));
}

TEST_F(ArkInteropTest, CreateF64)
{
    constexpr double piValue = 3.14;
    auto val = ARKTS_CreateF64(piValue);
    EXPECT_TRUE(ARKTS_IsNumber(val));
    constexpr double tolerance = 0.001;
    EXPECT_NEAR(ARKTS_GetValueNumber(val), piValue, tolerance);
}

TEST_F(ArkInteropTest, NullAndUndefined)
{
    auto n = ARKTS_CreateNull();
    auto u = ARKTS_CreateUndefined();
    EXPECT_TRUE(ARKTS_IsNull(n));
    EXPECT_TRUE(ARKTS_IsUndefined(u));
    EXPECT_FALSE(ARKTS_IsNull(u));
    EXPECT_FALSE(ARKTS_IsUndefined(n));
    EXPECT_FALSE(ARKTS_IsNull(ARKTS_CreateI32(zeroInt)));
    EXPECT_FALSE(ARKTS_IsUndefined(ARKTS_CreateI32(zeroInt)));
}

// === CJ Callbacks Tests ===

TEST_F(ArkInteropTest, SetCJModuleCallbackDuplicate)
{
    ARKTS_ModuleCallbacks dummyCallbacks {};
    ARKTS_SetCJModuleCallback(&dummyCallbacks);
}

// === Event/Async Tests ===

TEST_F(ArkInteropTest, CreateAsyncTaskViaEventHandler)
{
    auto dummyEnv = reinterpret_cast<ARKTS_Env>(static_cast<uintptr_t>(dummyEnvAddress));
    ARKTS_InitEventHandle(dummyEnv);
    ARKTS_CreateAsyncTask(dummyEnv, 0);
}

TEST_F(ArkInteropTest, OnTriggeredNoTimer)
{
    MockContext local;
    auto engine = local.GetEngine();
    void* napiEnv = ARKTS_GetNAPIEnv(engine);
    ASSERT_NE(napiEnv, nullptr);

    auto nativeEngine = reinterpret_cast<NativeEngine*>(napiEnv);
    auto loop = nativeEngine->GetUVLoop();
    ASSERT_NE(loop, nullptr);

    auto runner = OHOS::AppExecFwk::EventRunner::Create(false);
    auto eventHandler = std::make_shared<OHOS::AppExecFwk::EventHandler>(runner);
    ASSERT_NE(eventHandler, nullptr);

    auto uvHandler = std::make_shared<UVLoopHandler>(loop);
    int pipefd[2];
    if (pipe(pipefd) == 0) {
        uint32_t events = OHOS::AppExecFwk::FILE_DESCRIPTOR_INPUT_EVENT;
        eventHandler->AddFileDescriptorListener(pipefd[0], events, uvHandler, "testNoTimer");

        uvHandler->OnReadable(pipefd[0]);
        uvHandler->OnWritable(pipefd[0]);

        eventHandler->RemoveFileDescriptorListener(pipefd[0]);
        close(pipefd[0]);
        close(pipefd[1]);
    }
}

// === Lifecycle Tests ===

TEST_F(ArkInteropTest, DisposeEventHandlerViaEngineDestroy)
{
    MockContext local(ARKTS_CreateEngineWithNewThread(), true);
}

TEST_F(ArkInteropTest, DisposeJSContextViaEngineDestroy)
{
    MockContext local(ARKTS_CreateEngineWithNewThread(), true);
}

// === Misc Tests ===

TEST_F(ArkInteropTest, FreeCStringNull)
{
    ARKTS_FreeCString(nullptr);
}

TEST_F(ArkInteropTest, GetPosixThreadId)
{
    auto id = ARKTS_GetPosixThreadId();
    EXPECT_GT(id, 0ULL);
}

// === Other Tests ===

TEST_F(ArkInteropTest, ClearWeakOnNonWeak)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, obj);
    ARKTS_GlobalClearWeak(env, global);
    EXPECT_TRUE(ARKTS_GlobalIsAlive(env, global));

    ARKTS_DisposeGlobalSync(env, global);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, IsStringWithNonHeapValue)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    EXPECT_FALSE(ARKTS_IsString(env, ARKTS_CreateI32(numValValue)));
    EXPECT_FALSE(ARKTS_IsString(env, ARKTS_CreateBool(true)));
    EXPECT_FALSE(ARKTS_IsString(env, ARKTS_CreateNull()));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, SetAndGetElement)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    constexpr size_t arraySize = 5;
    constexpr uint32_t firstElementIdx = 0;
    constexpr uint32_t lastElementIdx = 4;
    auto arr = ARKTS_CreateArray(env, arraySize);
    constexpr int32_t firstElementValue = 100;
    constexpr int32_t lastElementValue = 500;
    ARKTS_SetElement(env, arr, firstElementIdx, ARKTS_CreateI32(firstElementValue));
    ARKTS_SetElement(env, arr, lastElementIdx, ARKTS_CreateI32(lastElementValue));

    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(ARKTS_GetElement(env, arr, firstElementIdx))), firstElementValue);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(ARKTS_GetElement(env, arr, lastElementIdx))), lastElementValue);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, SetAndGetElementValues)
{
    MockContext local;
    auto env = local.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    constexpr size_t smallArraySize = 3;
    auto arr = ARKTS_CreateArray(env, smallArraySize);
    constexpr int32_t elem0Val = 10;
    constexpr int32_t elem1Val = 20;
    constexpr int32_t elem2Val = 30;
    constexpr uint32_t elemIdx0 = 0;
    constexpr uint32_t elemIdx1 = 1;
    constexpr uint32_t elemIdx2 = 2;
    ARKTS_SetElement(env, arr, elemIdx0, ARKTS_CreateI32(elem0Val));
    ARKTS_SetElement(env, arr, elemIdx1, ARKTS_CreateI32(elem1Val));
    ARKTS_SetElement(env, arr, elemIdx2, ARKTS_CreateI32(elem2Val));

    auto v0 = ARKTS_GetElement(env, arr, elemIdx0);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(v0)), elem0Val);
    auto v2 = ARKTS_GetElement(env, arr, elemIdx2);
    EXPECT_EQ(static_cast<int>(ARKTS_GetValueNumber(v2)), elem2Val);

    ARKTS_CloseScope(env, scope);
}


} // namespace
