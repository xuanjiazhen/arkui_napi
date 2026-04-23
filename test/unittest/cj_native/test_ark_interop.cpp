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

constexpr uint64_t TEST_STACK_LIMIT = 0x5be506e000;
constexpr int64_t BIGINT_POS_VAL = 0x123'4567'8945;
constexpr int64_t BIGINT_NEG_VAL = -0x123'4567'8945;
constexpr int64_t BIGINT_MAX_VAL = 0x7FFF'FFFF'FFFF'FFFF;
constexpr int64_t BIGINT_MIN_VAL = -0x7FFF'FFFF'FFFF'FFFF;

constexpr int SLEEP_DURATION_MS = 100;

MockContext* MockContext::instance_ = nullptr;

Slab<std::function<ARKTS_Value(ARKTS_CallInfo)>> CycleFreeContext::callbacks_;
std::mutex CycleFreeContext::callbackMutex_;

void ArkInteropTest::TestPrime()
{
    auto env = MockContext::GetInstance()->GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto jsUDef = ARKTS_CreateUndefined();
    EXPECT_TRUE(ARKTS_IsUndefined(jsUDef));
    EXPECT_EQ(ARKTS_GetValueType(env, jsUDef), N_UNDEFINED);

    auto jsNull = ARKTS_CreateNull();
    EXPECT_TRUE(ARKTS_IsNull(jsNull));
    EXPECT_EQ(ARKTS_GetValueType(env, jsNull), N_NULL);

    ARKTS_Value jsBools[] {
        ARKTS_CreateBool(true),
        ARKTS_CreateBool(false)
    };
    EXPECT_TRUE(ARKTS_IsBool(jsBools[0]));
    EXPECT_EQ(ARKTS_GetValueType(env, jsBools[0]), N_BOOL);
    EXPECT_TRUE(ARKTS_GetValueBool(jsBools[0]));
    EXPECT_FALSE(ARKTS_GetValueBool(jsBools[1]));

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestNumber()
{
    auto env = MockContext::GetInstance()->GetEnv();
    auto scope = ARKTS_OpenScope(env);
    double origins[] {
        0.1,
        -12.1,
        12456.126546
    };
    constexpr auto totalCases = std::size(origins);
    ARKTS_Value jsValues[totalCases];
    double received[totalCases];

    for (size_t i = 0;i < totalCases; i++) {
        jsValues[i] = ARKTS_CreateF64(origins[i]);
        EXPECT_EQ(ARKTS_GetValueType(env, jsValues[i]), N_NUMBER);
        EXPECT_TRUE(ARKTS_IsNumber(jsValues[i]));
        received[i] = ARKTS_GetValueNumber(jsValues[i]);
        EXPECT_EQ(origins[i], received[i]);
    }

    auto jsNan = ARKTS_CreateF64(NAN);
    auto nNan = ARKTS_GetValueNumber(jsNan);
    EXPECT_TRUE(std::isnan(nNan));
    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestString()
{
    auto env = MockContext::GetInstance()->GetEnv();
    auto scope = ARKTS_OpenScope(env);
    const char* origins[] {
        "a plain text",
        "`~!@#$%^&*()_+[]\\",
        "涓枃瀛楃",
        "馃槉馃槀馃ぃ鉂わ笍馃槏馃槖馃憣馃槝",
    };
    ARKTS_Value jsValues[] {
        ARKTS_CreateUtf8(env, origins[0], strlen(origins[0])),
        ARKTS_CreateUtf8(env, origins[1], strlen(origins[1])),
        ARKTS_CreateUtf8(env, origins[2], strlen(origins[2])),
        ARKTS_CreateUtf8(env, origins[3], strlen(origins[3])),
    };
    EXPECT_TRUE(ARKTS_IsString(env, jsValues[0]));
    EXPECT_TRUE(ARKTS_IsHeapObject(jsValues[0]));
    EXPECT_EQ(ARKTS_GetValueType(env, jsValues[0]), N_STRING);
    for (auto i = 0;i < sizeof(jsValues)/sizeof(ARKTS_Value); i++) {
        auto size = ARKTS_GetValueUtf8Size(env, jsValues[i]);
        std::string result;
        result.resize(size - 1);
        ARKTS_GetValueUtf8(env, jsValues[i], size - 1, result.data());
        EXPECT_EQ(result, origins[i]);

        auto cStr = ARKTS_GetValueCString(env, jsValues[i]);
        result = cStr;
        EXPECT_EQ(result, origins[i]);
        ARKTS_FreeCString(cStr);
    }
    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestUtf16String()
{
    auto env = MockContext::GetInstance()->GetEnv();
    auto scope = ARKTS_OpenScope(env);

    std::string latin1Case[] {
        "a plain text",
        "hello, world!",
        "./[]124"
    };
    std::u16string utf16Cases[] {
        u"a plain text, 鍜?,
        u"浣犲ソ锛屼笘鐣岋紒",
        u"馃馃榾"
    };

    for (const auto& one : latin1Case) {
        auto value = ARKTS_CreateString(env, true, one.size(), one.data());
        EXPECT_TRUE(ARKTS_IsString(env, value));
        auto header = ARKTS_GetStringInfo(env, value);
        EXPECT_EQ(header.length, one.size());
        EXPECT_TRUE(header.isCompressed);
    }

    for (const auto& one : utf16Cases) {
        auto value = ARKTS_CreateString(env, false, one.size(), one.data());
        EXPECT_TRUE(ARKTS_IsString(env, value));
        auto header = ARKTS_GetStringInfo(env, value);
        EXPECT_EQ(header.length, one.size());
        EXPECT_FALSE(header.isCompressed);
    }

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestObject()
{
    auto env = MockContext::GetInstance()->GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto obj = ARKTS_CreateObject(env);
    EXPECT_TRUE(ARKTS_IsHeapObject(obj));
    EXPECT_TRUE(ARKTS_IsObject(env, obj));
    EXPECT_EQ(ARKTS_GetValueType(env, obj), N_OBJECT);
    auto keyA = ARKTS_CreateUtf8(env, "a", 1);
    EXPECT_FALSE(ARKTS_HasOwnProperty(env, obj, keyA));
    auto valueA = ARKTS_GetProperty(env, obj, keyA);
    EXPECT_TRUE(ARKTS_IsUndefined(valueA));
    valueA = ARKTS_CreateBool(true);
    ARKTS_SetProperty(env, obj, keyA, valueA);
    EXPECT_TRUE(ARKTS_HasOwnProperty(env, obj, keyA));
    auto receivedA = ARKTS_GetProperty(env, obj, keyA);
    EXPECT_TRUE(ARKTS_IsBool(receivedA));
    EXPECT_TRUE(ARKTS_GetValueBool(receivedA));
    auto keys = ARKTS_EnumOwnProperties(env, obj);
    EXPECT_TRUE(ARKTS_IsArray(env, keys));
    EXPECT_EQ(ARKTS_GetArrayLength(env, keys), 1);
    auto key = ARKTS_GetElement(env, keys, 0);
    EXPECT_TRUE(ARKTS_IsString(env, key));
    EXPECT_TRUE(ARKTS_StrictEqual(env, keyA, key));
    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestKeyable()
{
    auto env = MockContext::GetInstance()->GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto obj = ARKTS_CreateObject(env);
    constexpr int32_t numericKey = 12;
    ARKTS_Value allowedKeys[] {
        ARKTS_CreateUtf8(env, "a", 1),
        ARKTS_CreateI32(numericKey),
        ARKTS_CreateSymbol(env, "a", 1)
    };
    auto value = ARKTS_CreateBool(true);
    for (auto key : allowedKeys) {
        ARKTS_SetProperty(env, obj, key, value);
        auto received = ARKTS_GetProperty(env, obj, key);
        EXPECT_TRUE(ARKTS_IsBool(received));
        EXPECT_TRUE(ARKTS_GetValueBool(received));
    }
    ARKTS_CloseScope(env, scope);
}

struct PropCase {
    char k;
    bool writable;
    bool enumerable;
    bool configurable;

    static PropCase cases[];
};

PropCase PropCase::cases[] {
    {'a', false, false, false},
    {'b', true, false, false},
    {'c', false, true, false},
    {'d', false, false, true},
    {'e', true, true, false},
    {'f', true, false, true},
    {'g', false, true, true},
    {'h', true, true, true}
};

void ArkInteropTest::TestDefineProperty()
{
    ErrorCaptureContext errorCtx;
    auto env = errorCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto obj = ARKTS_CreateObject(env);

    constexpr auto totalCases = std::size(PropCase::cases);
    auto valueF = ARKTS_CreateBool(false);
    ARKTS_Value keys[totalCases];
    for (auto i = 0; i < totalCases; i++) {
        keys[i] = ARKTS_CreateUtf8(env, &PropCase::cases[i].k, 1);
        ARKTS_DefineOwnProperty(env, obj, keys[i], valueF, static_cast<ARKTS_PropertyFlag>(
            (PropCase::cases[i].writable ? N_WRITABLE : 0) |
            (PropCase::cases[i].enumerable ? N_ENUMERABLE : 0) |
            (PropCase::cases[i].configurable ? N_CONFIGURABLE : 0)
        ));
    }
    constexpr int expectKeys = 4;
    auto jsKeys = ARKTS_EnumOwnProperties(env, obj);
    EXPECT_TRUE(ARKTS_IsArray(env, jsKeys));
    EXPECT_EQ(ARKTS_GetArrayLength(env, jsKeys), expectKeys);
    for (auto i = 0; i < expectKeys; i++) {
        auto jsKey = ARKTS_GetElement(env, jsKeys, i);
        EXPECT_TRUE(ARKTS_IsString(env, jsKey));
        char buffer = 0;
        ARKTS_GetValueUtf8(env, jsKey, 1, &buffer);
        EXPECT_TRUE(buffer == 'c' || buffer == 'e' || buffer == 'g' || buffer == 'h');
    }

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestDefinePropertyWritable()
{
    ErrorCaptureContext errorCtx;
    auto env = errorCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto obj = ARKTS_CreateObject(env);

    constexpr auto totalCases = std::size(PropCase::cases);
    auto valueT = ARKTS_CreateBool(true);
    auto valueF = ARKTS_CreateBool(false);
    ARKTS_Value keys[totalCases];
    for (auto i = 0; i < totalCases; i++) {
        keys[i] = ARKTS_CreateUtf8(env, &PropCase::cases[i].k, 1);
        ARKTS_DefineOwnProperty(env, obj, keys[i], valueF, static_cast<ARKTS_PropertyFlag>(
            (PropCase::cases[i].writable ? N_WRITABLE : 0) |
            (PropCase::cases[i].enumerable ? N_ENUMERABLE : 0) |
            (PropCase::cases[i].configurable ? N_CONFIGURABLE : 0)
        ));
    }

    for (auto i = 0; i < totalCases; i++) { // writable
        if (PropCase::cases[i].writable && PropCase::cases[i].configurable) {
            ARKTS_SetProperty(env, obj, keys[i], valueT);
            auto receivedJS = ARKTS_GetProperty(env, obj, keys[i]);
            auto recievedN = ARKTS_GetValueBool(receivedJS);
            EXPECT_TRUE(recievedN);
        }
    }

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestDefinePropertyConfigurable()
{
    ErrorCaptureContext errorCtx;
    auto env = errorCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto obj = ARKTS_CreateObject(env);

    constexpr auto totalCases = std::size(PropCase::cases);
    auto valueT = ARKTS_CreateBool(true);
    auto valueF = ARKTS_CreateBool(false);
    ARKTS_Value keys[totalCases];
    for (auto i = 0; i < totalCases; i++) {
        keys[i] = ARKTS_CreateUtf8(env, &PropCase::cases[i].k, 1);
        ARKTS_DefineOwnProperty(env, obj, keys[i], valueF, static_cast<ARKTS_PropertyFlag>(
            (PropCase::cases[i].writable ? N_WRITABLE : 0) |
            (PropCase::cases[i].enumerable ? N_ENUMERABLE : 0) |
            (PropCase::cases[i].configurable ? N_CONFIGURABLE : 0)
        ));
    }

    for (auto i = 0;i < totalCases; ++i) { // configurable
        if (PropCase::cases[i].configurable) {
            ARKTS_DefineOwnProperty(env, obj, keys[i], valueT,
                static_cast<ARKTS_PropertyFlag>(N_WRITABLE | N_ENUMERABLE | N_CONFIGURABLE));
            auto received = ARKTS_GetProperty(env, obj, keys[i]);
            EXPECT_TRUE(ARKTS_IsBool(received));
            EXPECT_TRUE(ARKTS_GetValueBool(received));
        }
    }
    auto jsKeys = ARKTS_EnumOwnProperties(env, obj);
    constexpr int expectLength = 6;
    EXPECT_EQ(ARKTS_GetArrayLength(env, jsKeys), expectLength);

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestDefinePropertyConfigurableV2()
{
    ErrorCaptureContext errorCtx;
    auto env = errorCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);
    auto obj = ARKTS_CreateObject(env);

    constexpr auto totalCases = std::size(PropCase::cases);
    auto valueT = ARKTS_CreateBool(true);
    auto valueF = ARKTS_CreateBool(false);
    ARKTS_Value keys[totalCases];
    for (auto i = 0; i < totalCases; i++) {
        keys[i] = ARKTS_CreateUtf8(env, &PropCase::cases[i].k, 1);
        ARKTS_DefineOwnProperty(env, obj, keys[i], valueF, static_cast<ARKTS_PropertyFlag>(
            (PropCase::cases[i].writable ? N_WRITABLE : 0) |
            (PropCase::cases[i].enumerable ? N_ENUMERABLE : 0) |
            (PropCase::cases[i].configurable ? N_CONFIGURABLE : 0)
        ));
    }

    for (auto i = 0;i < totalCases; ++i) { // configurable
        auto result = ARKTS_DefineOwnPropertyV2(env, obj, keys[i], valueT,
            static_cast<ARKTS_PropertyFlag>(N_WRITABLE | N_ENUMERABLE | N_CONFIGURABLE));
        if (PropCase::cases[i].configurable) {
            EXPECT_TRUE(result);
        } else {
            EXPECT_TRUE(errorCtx.HasAndClearJSError());
        }
    }
    auto jsKeys = ARKTS_EnumOwnProperties(env, obj);
    constexpr int expectLength = 6;
    EXPECT_EQ(ARKTS_GetArrayLength(env, jsKeys), expectLength);

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestInstanceOf()
{
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    auto global = ARKTS_GetGlobalConstant(env);
    char clError[] = "Error";
    auto jError = ARKTS_CreateUtf8(env, clError, sizeof(clError) - 1);
    auto errorCls = ARKTS_GetProperty(env, global, jError);
    auto errorObJ = ARKTS_New(env, errorCls, 0, nullptr);
    auto isError = ARKTS_InstanceOf(env, errorObJ, errorCls);
    EXPECT_TRUE(isError);
    auto jObj = ARKTS_CreateObject(env);
    EXPECT_FALSE(ARKTS_InstanceOf(env, jObj, errorCls));
}

void ArkInteropTest::TestArray()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);

    char c = 'c';
    constexpr double testDoubleVal = 12.02;
    ARKTS_Value origin[] {
        ARKTS_CreateBool(true),
        ARKTS_CreateF64(testDoubleVal),
        ARKTS_CreateUtf8(env, &c, 1),
        ARKTS_CreateObject(env),
    };
    constexpr auto arrSize = std::size(origin);
    auto arr = ARKTS_CreateArray(env, arrSize);
    EXPECT_TRUE(ARKTS_IsArray(env, arr));
    EXPECT_EQ(ARKTS_GetValueType(env, arr), N_OBJECT);
    EXPECT_EQ(ARKTS_GetArrayLength(env, arr), arrSize);
    for (auto i = 0; i < arrSize; i++) {
        auto initialValue = ARKTS_GetElement(env, arr, i);
        EXPECT_TRUE(ARKTS_IsUndefined(initialValue));
        ARKTS_SetElement(env, arr, i, origin[i]);
        auto receivedValue = ARKTS_GetElement(env, arr, i);
        EXPECT_TRUE(ARKTS_StrictEqual(env, origin[i], receivedValue));
    }
}

void ArkInteropTest::TestBigintInt64()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    {
        int64_t origins[] {
            BIGINT_POS_VAL,
            BIGINT_NEG_VAL,
            BIGINT_MAX_VAL,
            BIGINT_MIN_VAL,
        };
        constexpr auto arrSize = std::size(origins);
        ARKTS_Value values[arrSize];
        for (auto i = 0; i < arrSize; i++) {
            values[i] = ARKTS_CreateBigInt(env, origins[i]);
            EXPECT_TRUE(ARKTS_IsBigInt(env, values[i]));
            EXPECT_EQ(ARKTS_GetValueType(env, values[i]), N_BIGINT);
            ARKTS_BigIntGetByteSize(env, values[i]);
        }
    }
}

void ArkInteropTest::TestBigint16Bytes()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    uint8_t origin[] {
        0, 10, 20, 30, 40, 50, 60, 70,
        80, 90, 70, 50, 20, 30, 40, 50
    };
    bool isNegative = false;
    auto value = ARKTS_CreateBigIntWithBytes(env, isNegative, std::size(origin), origin);
    EXPECT_TRUE(ARKTS_IsBigInt(env, value));
    EXPECT_EQ(ARKTS_GetValueType(env, value), N_BIGINT);
    EXPECT_EQ(ARKTS_BigIntGetByteSize(env, value), std::size(origin));
    uint8_t received[std::size(origin)];
    ARKTS_BigIntReadBytes(env, value, &isNegative, std::size(origin), received);
    EXPECT_FALSE(isNegative);
    for (auto i = 0; i < std::size(origin); i++) {
        EXPECT_EQ(origin[i], received[i]);
    }
}

void ArkInteropTest::TestBigint28Bytes()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    uint8_t origin[] {
        0, 10, 20, 30, 40, 50, 60, 70,
        80, 90, 70, 50, 20, 30, 40, 50,
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 20, 30,
    };
    bool isNegative = false;
    constexpr int expectSize = 32;
    // bigint words are 8 bytes aligned, received 32 bytes, and lower 4 bytes would be filled with 0.
    auto value = ARKTS_CreateBigIntWithBytes(env, isNegative, std::size(origin), origin);
    EXPECT_TRUE(ARKTS_IsBigInt(env, value));
    EXPECT_EQ(ARKTS_GetValueType(env, value), N_BIGINT);
    EXPECT_EQ(ARKTS_BigIntGetByteSize(env, value), expectSize);
    uint8_t received[expectSize];
    ARKTS_BigIntReadBytes(env, value, &isNegative, expectSize, received);
    EXPECT_FALSE(isNegative);
    for (auto i = 0; i < std::size(origin); i++) {
        EXPECT_EQ(origin[i], received[i + expectSize - std::size(origin)]);
    }
}

void ArkInteropTest::TestBigint32Bytes()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    uint8_t origin[] {
        0, 10, 20, 30, 40, 50, 60, 70,
        80, 90, 70, 50, 20, 30, 40, 50,
        1, 2, 3, 4, 5, 6, 7, 8,
        9, 10, 20, 30, 40, 50, 60, 70,
    };
    bool isNegative = false;
    auto value = ARKTS_CreateBigIntWithBytes(env, isNegative, std::size(origin), origin);
    EXPECT_TRUE(ARKTS_IsBigInt(env, value));
    EXPECT_EQ(ARKTS_GetValueType(env, value), N_BIGINT);
    EXPECT_EQ(ARKTS_BigIntGetByteSize(env, value), std::size(origin));
    uint8_t received[std::size(origin)];
    ARKTS_BigIntReadBytes(env, value, &isNegative, std::size(origin), received);
    EXPECT_FALSE(isNegative);
    for (auto i = 0; i < std::size(origin); i++) {
        EXPECT_EQ(origin[i], received[i]);
    }
}

void ArkInteropTest::TestSymbol()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);

    const char originDesc[] = "TestSymbol";

    auto symbol = ARKTS_CreateSymbol(env, originDesc, sizeof(originDesc) - 1);
    EXPECT_TRUE(ARKTS_IsSymbol(env, symbol));
    EXPECT_EQ(ARKTS_GetValueType(env, symbol), N_SYMBOL);

    auto retDesc = ARKTS_GetSymbolDesc(env, symbol);
    EXPECT_EQ(std::string(retDesc), originDesc);
    ARKTS_FreeCString(retDesc);

    auto obj = ARKTS_CreateObject(env);
    ARKTS_SetProperty(env, obj, symbol, ARKTS_CreateBool(true));
    auto value = ARKTS_GetProperty(env, obj, symbol);
    EXPECT_TRUE(ARKTS_IsBool(value));
    EXPECT_TRUE(ARKTS_GetValueBool(value));

    auto symbol1 = ARKTS_CreateSymbol(env, ORIGIN_DESC, sizeof(ORIGIN_DESC) - 1);
    ARKTS_SetProperty(env, obj, symbol1, ARKTS_CreateBool(false));
    auto value1 = ARKTS_GetProperty(env, obj, symbol1);
    EXPECT_TRUE(ARKTS_IsBool(value1));
    EXPECT_FALSE(ARKTS_GetValueBool(value1));

    value = ARKTS_GetProperty(env, obj, symbol);
    EXPECT_TRUE(ARKTS_IsBool(value));
    EXPECT_TRUE(ARKTS_GetValueBool(value));
}

void ArkInteropTest::TestFunction()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);

    bool called = false;
    constexpr size_t zeroArgCount = 0;
    auto id = MockContext::GetInstance()->StoreFunc([&called, env](ARKTS_CallInfo info)->ARKTS_Value {
        called = true;
        EXPECT_EQ(ARKTS_GetArgCount(info), zeroArgCount);
        EXPECT_TRUE(ARKTS_IsUndefined(ARKTS_GetThisArg(info)));
        return ARKTS_CreateUndefined();
    });
    auto func = ARKTS_CreateFunc(env, id);
    EXPECT_TRUE(ARKTS_IsCallable(env, func));
    EXPECT_EQ(ARKTS_GetValueType(env, func), N_FUNCTION);
    ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);
    EXPECT_TRUE(called);
    called = false;
    constexpr double callArgDouble = 45.1;
    ARKTS_Value args[] {
        ARKTS_CreateNull(),
        ARKTS_CreateBool(true),
        ARKTS_CreateF64(callArgDouble),
        ARKTS_CreateObject(env)
    };
    static constexpr auto totalArgs = std::size(args);
    id = MockContext::GetInstance()->StoreFunc([env, &called, origin = args](ARKTS_CallInfo info)->ARKTS_Value {
        called = true;
        auto self = ARKTS_GetThisArg(info);
        EXPECT_TRUE(ARKTS_IsObject(env, self));
        EXPECT_EQ(totalArgs, ARKTS_GetArgCount(info));
        for (auto i = 0; i < totalArgs; i++) {
            EXPECT_TRUE(ARKTS_StrictEqual(env, ARKTS_GetArg(info, i), origin[i]));
        }
        return ARKTS_CreateBool(true);
    });
    func = ARKTS_CreateFunc(env, id);
    ARKTS_Call(env, func, ARKTS_CreateObject(env), std::size(args), args);
    EXPECT_TRUE(called);
}

void ArkInteropTest::TestClass()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    auto id = MockContext::GetInstance()->StoreFunc([](ARKTS_CallInfo info)->ARKTS_Value {
        return ARKTS_GetThisArg(info);
    });
    auto clazz = ARKTS_CreateClass(env, id, ARKTS_CreateUndefined());
    EXPECT_TRUE(ARKTS_IsClass(env, clazz));
    EXPECT_EQ(ARKTS_GetValueType(env, clazz), N_FUNCTION);
    auto obj = ARKTS_New(env, clazz, 0, nullptr);
    EXPECT_TRUE(ARKTS_IsObject(env, obj));
    EXPECT_TRUE(ARKTS_InstanceOf(env, obj, clazz));
}

void ArkInteropTest::TestPromise()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    auto scope = ARKTS_OpenScope(env);

    auto promiseCap = ARKTS_CreatePromiseCapability(env);
    auto promise = ARKTS_GetPromiseFromCapability(env, promiseCap);
    EXPECT_TRUE(ARKTS_IsPromise(env, promise));
    EXPECT_EQ(ARKTS_GetValueType(env, promise), N_OBJECT);

    bool resolved = false;
    constexpr size_t singleArgCount = 1;
    auto id = MockContext::GetInstance()->StoreFunc([&resolved, env](ARKTS_CallInfo info)->ARKTS_Value {
        resolved = true;
        EXPECT_EQ(ARKTS_GetArgCount(info), singleArgCount);
        auto arg = ARKTS_GetArg(info, 0);
        EXPECT_TRUE(ARKTS_IsUndefined(arg));
        return ARKTS_CreateBool(true);
    });
    auto onResolved = ARKTS_CreateFunc(env, id);
    ARKTS_PromiseThen(env, promise, onResolved, ARKTS_CreateUndefined());
    ARKTS_PromiseCapabilityResolve(env, promiseCap, ARKTS_CreateUndefined());
    EXPECT_TRUE(resolved);

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestArrayBuffer()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    auto scope = ARKTS_OpenScope(env);

    uint8_t origin[] {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
        'a', 'b', 'c', 'd', 'e', 'f', 'g'
    };
    auto arrSize = std::size(origin);
    auto buffer = ARKTS_CreateArrayBuffer(env, arrSize);
    EXPECT_TRUE(ARKTS_IsArrayBuffer(env, buffer));
    EXPECT_EQ(ARKTS_GetValueType(env, buffer), N_OBJECT);
    EXPECT_EQ(ARKTS_GetArrayBufferLength(env, buffer), arrSize);
    auto dst = reinterpret_cast<uint8_t*>(ARKTS_GetArrayBufferRawPtr(env, buffer));
    for (auto i = 0; i < arrSize; i++) {
        dst[i] = origin[i];
    }
    uint8_t received[std::size(origin)];
    auto endpoint = ARKTS_ArrayBufferReadBytes(env, buffer, received, arrSize);
    EXPECT_EQ(endpoint, arrSize);

    for (auto i = 0; i < arrSize; i++) {
        EXPECT_EQ(origin[i], received[i]);
    }

    ARKTS_CloseScope(env, scope);
}

void ArkInteropTest::TestUpdateStackInfo()
{
    EXPECT_TRUE(MockContext::GetInstance());
    ARKTS_Env env = MockContext::GetInstance()->GetEnv();
    EXPECT_TRUE(env);
    auto scope = ARKTS_OpenScope(env);
    unsigned long long vmAddress = reinterpret_cast<unsigned long long>(env);
    StackInfo stackInfo;
    stackInfo.stackLimit = TEST_STACK_LIMIT;
    stackInfo.lastLeaveFrame = 0;
    ARKTS_UpdateStackInfo(vmAddress, &stackInfo, SWITCH_TO_SUB_STACK_INFO);
    ARKTS_UpdateStackInfo(vmAddress, &stackInfo, SWITCH_TO_MAIN_STACK_INFO);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, Types)
{
    RunLocalTest();
}

TEST_F(ArkInteropTest, PromiseThen)
{
    MockContext mockCtx(ARKTS_CreateEngineWithNewThread());
    auto env = mockCtx.GetEnv();
    EXPECT_TRUE(env);

    std::condition_variable cv;
    bool isComplete = false;
    auto funcId = mockCtx.StoreAsyncFunc([&mockCtx, &cv, &isComplete, env] {
        auto scope = ARKTS_OpenScope(env);
        auto promiseCap = ARKTS_CreatePromiseCapability(env);
        auto promise = ARKTS_GetPromiseFromCapability(env, promiseCap);
        auto thenFuncId = mockCtx.StoreFunc([&cv, &isComplete](ARKTS_CallInfo info) {
            isComplete = true;
            cv.notify_one();
            return ARKTS_CreateUndefined();
        });
        auto func = ARKTS_CreateFunc(env, thenFuncId);
        ARKTS_PromiseThen(env, promise, func, ARKTS_CreateUndefined());
        ARKTS_PromiseCapabilityResolve(env, promiseCap, ARKTS_CreateUndefined());
        ARKTS_CloseScope(env, scope);
    });
    ARKTS_CreateAsyncTask(env, funcId);
    std::mutex mutex;
    std::unique_lock lock(mutex);

    constexpr int checkDuration = 100;
    int waitTimes = 50; // set 100ms timeout
    while (!isComplete && waitTimes--) {
        cv.wait_for(lock, std::chrono::milliseconds(checkDuration));
    }
    // EXPECT no core dump, no timeout
    EXPECT_TRUE(waitTimes > 0);
}

TEST_F(ArkInteropTest, CycleFreeFunc)
{
    CycleFreeContext cycleFreeCtx;
    auto env = cycleFreeCtx.GetEnv();
    auto funcCount = cycleFreeCtx.GetCycleFreeFuncCount();
    auto scope = ARKTS_OpenScope(env);
    auto isCalled = false;
    auto id = cycleFreeCtx.StoreCycleFreeFunc([&isCalled](ARKTS_CallInfo callInfo) {
        isCalled = true;
        return ARKTS_CreateUndefined();
    });
    EXPECT_EQ(cycleFreeCtx.GetCycleFreeFuncCount(), funcCount + 1);
    auto func = ARKTS_CreateCycleFreeFunc(cycleFreeCtx.GetEnv(), id);
    EXPECT_TRUE(ARKTS_IsCallable(env, func));
    ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);
    EXPECT_TRUE(isCalled);
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CycleFreeExtern)
{
    CycleFreeContext cycleFreeCtx;
    auto env = cycleFreeCtx.GetEnv();
    auto funcCount = cycleFreeCtx.GetCycleFreeFuncCount();
    {
        auto scope = ARKTS_OpenScope(env);
        auto id = cycleFreeCtx.StoreCycleFreeFunc(nullptr);
        EXPECT_EQ(cycleFreeCtx.GetCycleFreeFuncCount(), funcCount + 1);
        auto object = ARKTS_CreateCycleFreeExtern(cycleFreeCtx.GetEnv(), id);
        EXPECT_TRUE(ARKTS_IsExternal(env, object));
        auto resid = ARKTS_GetExternalData(env, object);
        EXPECT_EQ(resid, id);

        ARKTS_CloseScope(env, scope);
    }
}

class GlobalWeakTest {
public:
    GlobalWeakTest(): cycleFreeCtx(ARKTS_CreateEngineWithNewThread()), status(CREATING)
    {
    }
    ~GlobalWeakTest()
    {
        ReleaseGlobals();
    }

    void Start()
    {
        ScheduleNext();
    }

    void WaitForComplete()
    {
        std::mutex mutex;
        std::unique_lock lock(mutex);
        while (status != COMPLETE) {
            cv.wait(lock);
        }
    }

    bool IsComplete() const
    {
        return status == COMPLETE;
    }

    bool IsAllDisposed() const
    {
        for (auto one : globals) {
            if (ARKTS_GlobalIsAlive(cycleFreeCtx.GetEnv(), one)) {
                return false;
            }
        }
        return true;
    }

private:
    static constexpr int OBJECT_CNT = 1000;

    void CreateWeakObjects()
    {
        auto env = cycleFreeCtx.GetEnv();
        auto scope = ARKTS_OpenScope(env);
        for (auto i = 0; i < OBJECT_CNT; i++) {
            auto object = ARKTS_CreateObject(env);
            auto global = ARKTS_CreateGlobal(env, object);
            ARKTS_GlobalSetWeak(env, global);
            globals.push_back(global);
        }
        ARKTS_CloseScope(env, scope);
        panda::JSNApi::TriggerGC(P_CAST(env, EcmaVM*), panda::JSNApi::TRIGGER_GC_TYPE::FULL_GC);
    }

    void DoAssertion()
    {
        for (auto one : globals) {
            EXPECT_TRUE(!ARKTS_GlobalIsAlive(cycleFreeCtx.GetEnv(), one));
        }
    }

    void ReleaseGlobals()
    {
        for (auto one : globals) {
            ARKTS_DisposeGlobalSync(cycleFreeCtx.GetEnv(), one);
        }
    }

    enum Status {
        CREATING,
        ASSERTION,
        DISPOSE,
        COMPLETE
    };

    void ScheduleNext()
    {
        auto id = cycleFreeCtx.StoreAsyncFunc([this] {
            DoNext();
        });
        ARKTS_CreateAsyncTask(cycleFreeCtx.GetEnv(), id);
    }

    void DoNext()
    {
        switch (status) {
            case CREATING:
                CreateWeakObjects();
                status = ASSERTION;
                break;
            case ASSERTION:
                DoAssertion();
                status = DISPOSE;
                break;
            case DISPOSE:
                status = COMPLETE;
                cv.notify_all();
                return;
            default: ;
        }
        ScheduleNext();
    }

    CycleFreeContext cycleFreeCtx;
    Status status;
    std::condition_variable cv;
    std::vector<ARKTS_Global> globals;
};

TEST_F(ArkInteropTest, GlobalWeak)
{
    GlobalWeakTest weakTest;
    weakTest.Start();
    weakTest.WaitForComplete();
    EXPECT_TRUE(weakTest.IsComplete());
}

TEST_F(ArkInteropTest, GlobalToValue)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    {
        auto scope = ARKTS_OpenScope(env);
        auto object = ARKTS_CreateObject(env);
        auto global = ARKTS_CreateGlobal(env, object);
        auto value = ARKTS_GlobalToValue(env, global);
        auto received = ARKTS_GlobalFromValue(env, value);
        EXPECT_EQ(received, global);
        ARKTS_DisposeGlobalSync(env, global);
        ARKTS_CloseScope(env, scope);
    }
}

HWTEST_F(ArkInteropTest, ArkTSInteropNapiCreateEngineNew, TestSize.Level1)
{
    MockContext mockCtx(ARKTS_CreateEngineWithNewThread());

    auto curTid = ARKTS_GetPosixThreadId();
    auto engineTid = ARKTS_GetThreadIdOfEngine(mockCtx.GetEngine());

    EXPECT_NE(curTid, engineTid);

    auto env = mockCtx.GetEnv();
    EXPECT_TRUE(env);
    bool isComplete = false;
    std::condition_variable cv;
    auto funcId = mockCtx.StoreAsyncFunc([&isComplete, &cv] {
        ArkInteropTest::RunLocalTest();
        isComplete = true;
        cv.notify_one();
    });
    ARKTS_CreateAsyncTask(env, funcId);
    std::mutex mutex;
    std::unique_lock lock(mutex);
    constexpr int checkDuration = 10;
    int waitTimes = 100; // set 1000ms timeout
    while (!isComplete && waitTimes--) {
        cv.wait_for(lock, std::chrono::milliseconds(checkDuration));
    }
    EXPECT_TRUE(waitTimes > 0);
}

TEST_F(ArkInteropTest, ScopeMT)
{
    constexpr int threadCount = 1000;
    std::thread threads[threadCount];
    for (int i = 0; i < threadCount; i++) {
        threads[i] = std::thread([] {
            panda::RuntimeOption options;
            auto vm = panda::JSNApi::CreateJSVM(options);
            EXPECT_TRUE(vm);
            auto env = P_CAST(vm, ARKTS_Env);
            auto scope = ARKTS_OpenScope(env);
            EXPECT_TRUE(scope);
            ARKTS_CloseScope(env, scope);
            panda::JSNApi::DestroyJSVM(vm);
        });
    }
    for (auto& thread : threads) {
        thread.join();
    }
}

TEST_F(ArkInteropTest, GlobalRelease)
{
    MockContext mockCtx(ARKTS_CreateEngineWithNewThread());
    auto env = mockCtx.GetEnv();

    bool isComplete = false;
    int loops = 10;
    std::condition_variable cv;
    std::function <void()> callback;
    callback = [&isComplete, &cv, &loops, env, &callback] {
        if (loops == 0) {
            isComplete = true;
            cv.notify_one();
            return;
        }
        auto scope = ARKTS_OpenScope(env);
        auto totalRepeat = 200000;
        for (int i = 0;i < totalRepeat; ++i) {
            auto object = ARKTS_CreateObject(env);
            auto global = ARKTS_CreateGlobal(env, object);
            ARKTS_DisposeGlobal(env, global);
        }
        ARKTS_CloseScope(env, scope);
        --loops;
        auto innerFuncId = MockContext::GetInstance()->StoreAsyncFunc(callback);
        ARKTS_CreateAsyncTask(env, innerFuncId);
    };
    auto funcId = mockCtx.StoreAsyncFunc(callback);
    ARKTS_CreateAsyncTask(env, funcId);
    std::mutex mutex;
    std::unique_lock lock(mutex);
    int waitTimes = 200;
    int msEachTime = 100;
    while (!isComplete && waitTimes--) {
        cv.wait_for(lock, std::chrono::milliseconds(msEachTime));
    }
    EXPECT_TRUE(isComplete);
}

TEST_F(ArkInteropTest, GlobalReleaseSync)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    int loops = 10;
    auto totalRepeat = 200000;
    for (int i = 0;i < loops; ++i) {
        auto scope = ARKTS_OpenScope(env);
        for (int j = 0;j < totalRepeat; ++j) {
            auto object = ARKTS_CreateObject(env);
            auto global = ARKTS_CreateGlobal(env, object);
            ARKTS_DisposeGlobalSync(env, global);
        }
        EXPECT_FALSE(panda::JSNApi::HasPendingException(P_CAST(env, EcmaVM*)));
        ARKTS_CloseScope(env, scope);
    }
}

TEST_F(ArkInteropTest, PromiseRelease)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    int loops = 20;
    auto totalRepeat = 20000;
    auto undefined = ARKTS_CreateUndefined();
    for (int i = 0;i < loops; ++i) {
        auto scope = ARKTS_OpenScope(env);
        for (int j = 0;j < totalRepeat; ++j) {
            auto promiseCap = ARKTS_CreatePromiseCapability(env);
            ARKTS_PromiseCapabilityResolve(env, promiseCap, undefined);
        }
        EXPECT_FALSE(panda::JSNApi::HasPendingException(P_CAST(env, EcmaVM*)));
        ARKTS_CloseScope(env, scope);
    }
}

TEST_F(ArkInteropTest, CheckFreeContext)
{
    bool isDisposed = false;
    {
        MockContext mockCtx(ARKTS_CreateEngineWithNewThread());
        mockCtx.SetFinalizerCallback([&isDisposed](ARKTS_Env) {
            isDisposed = true;
        });
    }
    EXPECT_TRUE(isDisposed);
}

TEST_F(ArkInteropTest, DebugMode)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto id = mockCtx.StoreFunc([](ARKTS_CallInfo) {
        return ARKTS_CreateUndefined();
    });
    auto func = ARKTS_CreateFunc(env, id);
    mockCtx.SetNativeCallingCallback([&mockCtx, id](const void* addr) {
        EXPECT_EQ(addr, mockCtx.GetFuncAddr(id));
    });
    ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);
}

TEST_F(ArkInteropTest, SetCJModuleCallbackWithNull)
{
    ARKTS_SetCJModuleCallback(nullptr);
}

TEST_F(ArkInteropTest, CJLambdaInvoker)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    bool invoked = false;
    auto lambdaId = mockCtx.StoreFunc([&invoked](ARKTS_CallInfo info) -> ARKTS_Value {
        invoked = true;
        return ARKTS_CreateBool(true);
    });

    auto func = ARKTS_CreateFunc(env, lambdaId);
    ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);
    EXPECT_TRUE(invoked);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CJLambdaInvokerWithoutCallback)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto func = ARKTS_CreateFunc(env, 12345);
    ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CJLambdaDeleter)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto lambdaId = mockCtx.StoreFunc([](ARKTS_CallInfo info) -> ARKTS_Value {
        return ARKTS_CreateUndefined();
    });

    auto func = ARKTS_CreateFunc(env, lambdaId);
    ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CJAsyncCallback)
{
    auto engine = ARKTS_CreateEngineWithNewThread();
    ASSERT_TRUE(engine);
    {
        MockContext mockCtx(engine, false);
        auto env = mockCtx.GetEnv();
        auto scope = ARKTS_OpenScope(env);

        bool invoked = false;
        std::condition_variable cv;
        auto asyncId = mockCtx.StoreAsyncFunc([&invoked, &cv] {
            invoked = true;
            cv.notify_one();
        });

        ARKTS_CreateAsyncTask(env, asyncId);

        std::mutex mutex;
        std::unique_lock lock(mutex);
        constexpr int checkDuration = 100;
        int waitTimes = 50;
        while (!invoked && waitTimes--) {
            cv.wait_for(lock, std::chrono::milliseconds(checkDuration));
        }
        EXPECT_TRUE(invoked);

        ARKTS_CloseScope(env, scope);
    }
    std::thread destroyer([engine] { ARKTS_DestroyEngine(engine); });
    destroyer.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_DURATION_MS));
}

TEST_F(ArkInteropTest, GetNAPIEnv)
{
    MockContext mockCtx;
    auto engine = mockCtx.GetEngine();
    auto napiEnv = ARKTS_GetNAPIEnv(engine);
    EXPECT_TRUE(napiEnv);
}

TEST_F(ArkInteropTest, GetNAPIEnvWithNullEngine)
{
    ErrorCaptureContext errorCtx;
    auto napiEnv = ARKTS_GetNAPIEnv(nullptr);
    EXPECT_FALSE(napiEnv);
    EXPECT_TRUE(errorCtx.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetContext)
{
    MockContext mockCtx;
    auto engine = mockCtx.GetEngine();
    auto context = ARKTS_GetContext(engine);
    EXPECT_TRUE(context);
}

TEST_F(ArkInteropTest, GetContextWithNullEngine)
{
    ErrorCaptureContext errorCtx;
    auto context = ARKTS_GetContext(nullptr);
    EXPECT_FALSE(context);
    EXPECT_TRUE(errorCtx.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, GetThreadIdOfEngine)
{
    MockContext mockCtx;
    auto engine = mockCtx.GetEngine();
    auto threadId = ARKTS_GetThreadIdOfEngine(engine);
    EXPECT_GT(threadId, 0);
}

TEST_F(ArkInteropTest, GetThreadIdOfEngineWithNullEngine)
{
    ErrorCaptureContext errorCtx;
    constexpr uint64_t invalidThreadId = 0;
    auto threadId = ARKTS_GetThreadIdOfEngine(nullptr);
    EXPECT_EQ(threadId, invalidThreadId);
    EXPECT_TRUE(errorCtx.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, CreateEngine)
{
    auto engine = ARKTS_CreateEngineWithNewThread();
    ASSERT_TRUE(engine);
    std::thread destroyer([engine] { ARKTS_DestroyEngine(engine); });
    destroyer.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_DURATION_MS));
}

TEST_F(ArkInteropTest, DestroyEngineWithNullEngine)
{
    ErrorCaptureContext errorCtx;
    ARKTS_DestroyEngine(nullptr);
    EXPECT_TRUE(errorCtx.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, LoadEntryFromAbc)
{
    MockContext mockCtx;
    auto engine = mockCtx.GetEngine();

    const char* testFile = "/data/test_module.abc";
    const char* entryPoint = "test_entry";

    bool result = ARKTS_LoadEntryFromAbc(engine, testFile, entryPoint, false);
    EXPECT_FALSE(result);
}

TEST_F(ArkInteropTest, ImportFromEntry)
{
    MockContext mockCtx;
    auto engine = mockCtx.GetEngine();

    auto value = ARKTS_ImportFromEntry(engine, "test_entry", "test_import");
    EXPECT_EQ(value.value, 0ULL);
}

TEST_F(ArkInteropTest, RequireNativeModule)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    const char* target = "test_module";

    (void)ARKTS_Require(env, target, true, false, nullptr);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, RequireNativeModuleWithNullEnv)
{
    ErrorCaptureContext errorCtx;
    const char* target = "test_module";

    (void)ARKTS_Require(nullptr, target, true, false, nullptr);
    EXPECT_TRUE(errorCtx.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, RequireNativeModuleWithRelativePath)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    const char* target = "test_module";
    const char* relativePath = "./test_module";

    (void)ARKTS_Require(env, target, true, false, relativePath);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, RequireAppModule)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    const char* target = "test_module";

    (void)ARKTS_Require(env, target, true, true, nullptr);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, RequireJsModule)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    const char* target = "test_module";

    (void)ARKTS_Require(env, target, false, false, nullptr);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, LoadModuleByNapiEnv)
{
    MockContext mockCtx;
    auto engine = mockCtx.GetEngine();
    void* napiEnv = ARKTS_GetNAPIEnv(engine);
    ASSERT_NE(napiEnv, nullptr);
    // nullptr dllName: early return in ARKTS_LoadModuleByNapiEnv (avoids CJ loader crash in host UT).
    EXPECT_EQ(ARKTS_LoadModuleByNapiEnv(napiEnv, nullptr), nullptr);
}

TEST_F(ArkInteropTest, LoadModuleByNapiEnvWithNullEnv)
{
    const char* dllName = "test_module";

    auto result = ARKTS_LoadModuleByNapiEnv(nullptr, dllName);
    EXPECT_FALSE(result);
}

TEST_F(ArkInteropTest, LoadModuleByNapiEnvWithValidParams)
{
    ModuleRegisteredContext moduleCtx;
    auto engine = moduleCtx.GetEngine();
    void* napiEnv = ARKTS_GetNAPIEnv(engine);
    ASSERT_NE(napiEnv, nullptr);

    auto result = ARKTS_LoadModuleByNapiEnv(napiEnv, "test_module");
    (void)result;
    moduleCtx.HasAndClearNativeError();
    moduleCtx.HasAndClearJSError();
}

TEST_F(ArkInteropTest, UpdateStackInfoWithNullAddress)
{
    ErrorCaptureContext errorCtx;
    auto env = errorCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    StackInfo stackInfo;
    stackInfo.stackLimit = TEST_STACK_LIMIT;
    stackInfo.lastLeaveFrame = 0;
    ARKTS_UpdateStackInfo(0, &stackInfo, SWITCH_TO_SUB_STACK_INFO);
    EXPECT_TRUE(errorCtx.HasAndClearNativeError());
    ARKTS_UpdateStackInfo(0, &stackInfo, SWITCH_TO_MAIN_STACK_INFO);
    EXPECT_TRUE(errorCtx.HasAndClearNativeError());

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, UpdateStackInfoWithNullStackInfo)
{
    ErrorCaptureContext errorCtx;
    auto env = errorCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    unsigned long long vmAddress = reinterpret_cast<unsigned long long>(env);
    ARKTS_UpdateStackInfo(vmAddress, nullptr, SWITCH_TO_SUB_STACK_INFO);
    EXPECT_TRUE(errorCtx.HasAndClearNativeError());
    ARKTS_UpdateStackInfo(vmAddress, nullptr, SWITCH_TO_MAIN_STACK_INFO);
    EXPECT_TRUE(errorCtx.HasAndClearNativeError());

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateCycleFreeFunc)
{
    CycleFreeContext cycleFreeCtx;
    auto env = cycleFreeCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    bool called = false;
    auto id = cycleFreeCtx.StoreCycleFreeFunc([&called](ARKTS_CallInfo callInfo) {
        called = true;
        return ARKTS_CreateUndefined();
    });

    auto func = ARKTS_CreateCycleFreeFunc(env, id);
    EXPECT_TRUE(ARKTS_IsCallable(env, func));

    ARKTS_Call(env, func, ARKTS_CreateUndefined(), 0, nullptr);
    EXPECT_TRUE(called);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateCycleFreeExtern)
{
    CycleFreeContext cycleFreeCtx;
    auto env = cycleFreeCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto id = cycleFreeCtx.StoreCycleFreeFunc([](ARKTS_CallInfo callInfo) {
        return ARKTS_CreateUndefined();
    });

    auto external = ARKTS_CreateCycleFreeExtern(env, id);
    EXPECT_TRUE(ARKTS_IsExternal(env, external));

    auto data = ARKTS_GetExternalData(env, external);
    EXPECT_EQ(data, id);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateGlobal)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto object = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, object);

    auto value = ARKTS_GlobalToValue(env, global);
    auto global2 = ARKTS_GlobalFromValue(env, value);

    EXPECT_EQ(global, global2);

    ARKTS_DisposeGlobal(env, global);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateGlobalWithWeak)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto object = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, object);

    ARKTS_GlobalSetWeak(env, global);

    auto isAlive = ARKTS_GlobalIsAlive(env, global);
    EXPECT_TRUE(isAlive);

    ARKTS_DisposeGlobal(env, global);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, DisposeGlobalSync)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto object = ARKTS_CreateObject(env);
    auto global = ARKTS_CreateGlobal(env, object);

    ARKTS_DisposeGlobalSync(env, global);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateEngineWithNewThread)
{
    auto engine = ARKTS_CreateEngineWithNewThread();
    ASSERT_TRUE(engine);
    auto env = ARKTS_GetContext(engine);
    EXPECT_TRUE(env);

    std::thread destroyer([engine] { ARKTS_DestroyEngine(engine); });
    destroyer.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_DURATION_MS));
}

TEST_F(ArkInteropTest, CreateEngineWithNewThreadNull)
{
    auto engine = ARKTS_CreateEngineWithNewThread();
    if (!engine) {
        return;
    }
    std::thread destroyer([engine] { ARKTS_DestroyEngine(engine); });
    destroyer.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_DURATION_MS));
}

TEST_F(ArkInteropTest, PromiseCapabilityReject)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto promiseCap = ARKTS_CreatePromiseCapability(env);
    auto promise = ARKTS_GetPromiseFromCapability(env, promiseCap);

    bool rejected = false;
    auto noopId = MockContext::GetInstance()->StoreFunc([](ARKTS_CallInfo info)->ARKTS_Value {
        (void)info;
        return ARKTS_CreateUndefined();
    });
    auto rejectId = MockContext::GetInstance()->StoreFunc([&rejected, env](ARKTS_CallInfo info)->ARKTS_Value {
        rejected = true;
        EXPECT_EQ(ARKTS_GetArgCount(info), singleArgCount);
        auto arg = ARKTS_GetArg(info, 0);
        EXPECT_TRUE(ARKTS_IsUndefined(arg));
        return ARKTS_CreateUndefined();
    });
    auto onFulfilled = ARKTS_CreateFunc(env, noopId);
    auto onRejected = ARKTS_CreateFunc(env, rejectId);
    ARKTS_PromiseThen(env, promise, onFulfilled, onRejected);
    ARKTS_PromiseCapabilityReject(env, promiseCap, ARKTS_CreateUndefined());
    EXPECT_TRUE(rejected);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ArrayBufferWithDeleter)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    uint8_t data[] = {1, 2, 3, 4, 5};
    auto buffer = ARKTS_CreateArrayBuffer(env, sizeof(data));
    auto ptr = reinterpret_cast<uint8_t*>(ARKTS_GetArrayBufferRawPtr(env, buffer));
    std::copy(data, data + sizeof(data), ptr);

    bool deleted = false;
    (void)mockCtx.StoreArrayBufferDeleter([&deleted](void* buffer) {
        deleted = true;
    });

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ExternalWithDeleter)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    int* data = new int(42);
    auto deleterId = mockCtx.StoreNativeData(data, [](void* ptr) {
        delete static_cast<int*>(ptr);
    });

    auto external = ARKTS_CreateExternal(env, deleterId);
    EXPECT_TRUE(ARKTS_IsExternal(env, external));

    auto retrievedData = ARKTS_GetExternalData(env, external);
    EXPECT_EQ(retrievedData, deleterId);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetPropertyNames)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto obj = ARKTS_CreateObject(env);
    auto key1 = ARKTS_CreateUtf8(env, "key1", 4);
    auto key2 = ARKTS_CreateUtf8(env, "key2", 4);

    constexpr int32_t propVal1 = 1;
    constexpr int32_t propVal2 = 2;
    ARKTS_SetProperty(env, obj, key1, ARKTS_CreateI32(propVal1));
    ARKTS_SetProperty(env, obj, key2, ARKTS_CreateI32(propVal2));

    auto keys = ARKTS_EnumOwnProperties(env, obj);
    EXPECT_TRUE(ARKTS_IsArray(env, keys));
    constexpr int32_t expectedKeyCount = 2;
    EXPECT_EQ(ARKTS_GetArrayLength(env, keys), expectedKeyCount);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, StrictEqual)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    constexpr int32_t testNumValue = 42;
    constexpr int32_t differentNumValue = 43;
    auto num1 = ARKTS_CreateI32(testNumValue);
    auto num2 = ARKTS_CreateI32(testNumValue);
    auto num3 = ARKTS_CreateI32(differentNumValue);

    EXPECT_TRUE(ARKTS_StrictEqual(env, num1, num2));
    EXPECT_FALSE(ARKTS_StrictEqual(env, num1, num3));

    auto str1 = ARKTS_CreateUtf8(env, "test", 4);
    auto str2 = ARKTS_CreateUtf8(env, "test", 4);
    auto str3 = ARKTS_CreateUtf8(env, "test2", 5);

    EXPECT_TRUE(ARKTS_StrictEqual(env, str1, str2));
    EXPECT_FALSE(ARKTS_StrictEqual(env, str1, str3));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CreateInt32)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    int32_t values[] = {0, 1, -1, 2147483647, -2147483648};

    for (auto value : values) {
        auto jsValue = ARKTS_CreateI32(value);
        auto numValue = ARKTS_GetValueNumber(jsValue);
        EXPECT_EQ(static_cast<int32_t>(numValue), value);
    }
    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, GetValueType)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto undefined = ARKTS_CreateUndefined();
    EXPECT_EQ(ARKTS_GetValueType(env, undefined), N_UNDEFINED);

    auto null = ARKTS_CreateNull();
    EXPECT_EQ(ARKTS_GetValueType(env, null), N_NULL);

    auto boolVal = ARKTS_CreateBool(true);
    EXPECT_EQ(ARKTS_GetValueType(env, boolVal), N_BOOL);

    constexpr int32_t numValValue = 42;
    auto numVal = ARKTS_CreateI32(numValValue);
    EXPECT_EQ(ARKTS_GetValueType(env, numVal), N_NUMBER);

    auto strVal = ARKTS_CreateUtf8(env, "test", 4);
    EXPECT_EQ(ARKTS_GetValueType(env, strVal), N_STRING);

    auto objVal = ARKTS_CreateObject(env);
    EXPECT_EQ(ARKTS_GetValueType(env, objVal), N_OBJECT);

    auto arrVal = ARKTS_CreateArray(env, 0);
    EXPECT_EQ(ARKTS_GetValueType(env, arrVal), N_OBJECT);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, CallWithArguments)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    bool called = false;
    auto id = mockCtx.StoreFunc([&called, env](ARKTS_CallInfo info) -> ARKTS_Value {
        called = true;
        EXPECT_EQ(ARKTS_GetArgCount(info), callArgCount);

        auto arg0 = ARKTS_GetArg(info, 0);
        auto arg1 = ARKTS_GetArg(info, 1);
        auto arg2 = ARKTS_GetArg(info, 2);

        EXPECT_TRUE(ARKTS_IsNumber(arg0));
        EXPECT_TRUE(ARKTS_IsString(env, arg1));
        EXPECT_TRUE(ARKTS_IsBool(arg2));

        return ARKTS_CreateUndefined();
    });

    auto func = ARKTS_CreateFunc(env, id);

    constexpr int32_t callArgNum = 42;
    ARKTS_Value args[] = {
        ARKTS_CreateI32(callArgNum),
        ARKTS_CreateUtf8(env, "test", 4),
        ARKTS_CreateBool(true)
    };

    constexpr size_t callArgCount = 3;
    ARKTS_Call(env, func, ARKTS_CreateUndefined(), callArgCount, args);
    EXPECT_TRUE(called);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, NewWithArguments)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    bool constructed = false;
    auto id = mockCtx.StoreFunc([&constructed](ARKTS_CallInfo info) -> ARKTS_Value {
        constructed = true;
        return ARKTS_GetThisArg(info);
    });

    auto clazz = ARKTS_CreateClass(env, id, ARKTS_CreateUndefined());

    constexpr int32_t newArgNum = 42;
    ARKTS_Value args[] = {
        ARKTS_CreateI32(newArgNum),
        ARKTS_CreateUtf8(env, "test", 4)
    };

    constexpr size_t newArgCount = 2;
    auto instance = ARKTS_New(env, clazz, newArgCount, args);
    EXPECT_TRUE(constructed);
    EXPECT_TRUE(ARKTS_IsObject(env, instance));
    EXPECT_TRUE(ARKTS_InstanceOf(env, instance, clazz));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ArrayOperations)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    constexpr size_t arrSize = 5;
    auto arr = ARKTS_CreateArray(env, arrSize);
    EXPECT_EQ(ARKTS_GetArrayLength(env, arr), arrSize);

    for (size_t i = 0; i < arrSize; i++) {
        ARKTS_SetElement(env, arr, i, ARKTS_CreateI32(i));
    }

    for (size_t i = 0; i < arrSize; i++) {
        auto elem = ARKTS_GetElement(env, arr, i);
        auto numValue = ARKTS_GetValueNumber(elem);
        EXPECT_EQ(static_cast<int>(numValue), i);
    }

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ObjectGetPrototypeOf)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto classId = MockContext::GetInstance()->StoreFunc([](ARKTS_CallInfo info)->ARKTS_Value {
        return ARKTS_GetThisArg(info);
    });
    auto clazz = ARKTS_CreateClass(env, classId, ARKTS_CreateUndefined());
    EXPECT_TRUE(ARKTS_IsClass(env, clazz));
    auto proto = ARKTS_GetPrototype(env, clazz);
    EXPECT_TRUE(ARKTS_IsHeapObject(proto));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, ArrayBufferCreateWithData)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    uint8_t data[] = {1, 2, 3, 4, 5, 6, 7, 8};
    auto buffer = ARKTS_CreateArrayBuffer(env, sizeof(data));

    auto ptr = reinterpret_cast<uint8_t*>(ARKTS_GetArrayBufferRawPtr(env, buffer));
    std::copy(data, data + sizeof(data), ptr);

    EXPECT_EQ(ARKTS_GetArrayBufferLength(env, buffer), sizeof(data));

    uint8_t received[sizeof(data)];
    auto bytesRead = ARKTS_ArrayBufferReadBytes(env, buffer, received, sizeof(data));
    EXPECT_EQ(bytesRead, sizeof(data));

    for (size_t i = 0; i < sizeof(data); i++) {
        EXPECT_EQ(received[i], data[i]);
    }

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, BigIntOperations)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    int64_t bigIntValue = 1234567890123456789LL;
    auto bigInt = ARKTS_CreateBigInt(env, bigIntValue);
    EXPECT_TRUE(ARKTS_IsBigInt(env, bigInt));

    auto byteSize = ARKTS_BigIntGetByteSize(env, bigInt);
    EXPECT_GT(byteSize, 0);

    bool isNegative = false;
    uint8_t bytes[16];
    ARKTS_BigIntReadBytes(env, bigInt, &isNegative, sizeof(bytes), bytes);
    EXPECT_FALSE(isNegative);

    auto bigInt2 = ARKTS_CreateBigIntWithBytes(env, false, byteSize, bytes);
    EXPECT_TRUE(ARKTS_IsBigInt(env, bigInt2));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, SymbolOperations)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    const char* desc = "test_symbol";
    auto symbol = ARKTS_CreateSymbol(env, desc, strlen(desc));
    EXPECT_TRUE(ARKTS_IsSymbol(env, symbol));

    auto symbolDesc = ARKTS_GetSymbolDesc(env, symbol);
    EXPECT_STREQ(symbolDesc, desc);
    ARKTS_FreeCString(symbolDesc);

    auto symbol2 = ARKTS_CreateSymbol(env, desc, strlen(desc));
    EXPECT_FALSE(ARKTS_StrictEqual(env, symbol, symbol2));

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, PromiseChaining)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    auto scope = ARKTS_OpenScope(env);

    auto promiseCap = ARKTS_CreatePromiseCapability(env);
    auto promise = ARKTS_GetPromiseFromCapability(env, promiseCap);

    bool resolved = false;
    bool chained = false;

    auto resolveId = mockCtx.StoreFunc([&resolved, env](ARKTS_CallInfo info) -> ARKTS_Value {
        resolved = true;
        return ARKTS_GetArg(info, 0);
    });

    auto chainId = mockCtx.StoreFunc([&chained, env](ARKTS_CallInfo info) -> ARKTS_Value {
        chained = true;
        return ARKTS_GetArg(info, 0);
    });

    auto onResolved = ARKTS_CreateFunc(env, resolveId);
    auto onChain = ARKTS_CreateFunc(env, chainId);

    auto chainedPromise = ARKTS_PromiseThen(env, promise, onResolved, ARKTS_CreateUndefined());
    (void)ARKTS_PromiseThen(env, chainedPromise, onChain, ARKTS_CreateUndefined());

    constexpr int32_t promiseResolveVal = 42;
    ARKTS_PromiseCapabilityResolve(env, promiseCap, ARKTS_CreateI32(promiseResolveVal));

    EXPECT_TRUE(resolved);
    EXPECT_TRUE(chained);

    ARKTS_CloseScope(env, scope);
}

TEST_F(ArkInteropTest, HiTraceStartTrace)
{
    ARKTS_HiTraceStartTrace("test_trace", 1);
}

TEST_F(ArkInteropTest, HiTraceFinishTrace)
{
    ARKTS_HiTraceFinishTrace("test_trace", 1);
}

TEST_F(ArkInteropTest, HiTraceCountTrace)
{
    constexpr int32_t hiTraceCount = 100;
    ARKTS_HiTraceCountTrace("test_trace", hiTraceCount);
}

TEST_F(ArkInteropTest, InitEventHandle)
{
    MockContext mockCtx;
    auto env = mockCtx.GetEnv();
    ARKTS_InitEventHandle(env);
}

TEST_F(ArkInteropTest, InitEventHandleWithNullEnv)
{
    ErrorCaptureContext errorCtx;
    ARKTS_InitEventHandle(nullptr);
    EXPECT_TRUE(errorCtx.HasAndClearNativeError());
}

TEST_F(ArkInteropTest, CreateAsyncTask)
{
    GTEST_SKIP() << "async task on global engine may prevent clean shutdown";
}

TEST_F(ArkInteropTest, CreateAsyncTaskWithNullEnv)
{
    ErrorCaptureContext errorCtx;
    constexpr int64_t asyncTaskId = 12345;
    ARKTS_CreateAsyncTask(nullptr, asyncTaskId);
    EXPECT_TRUE(errorCtx.HasAndClearNativeError());
}

void MockContext::InitModuleCallbacks()
{
    static ARKTS_ModuleCallbacks callbacks {
        .exportModule = ExportModuleCallback,
        .hasModuleHandle = HasModuleHandleCallback,
        .throwJSError = ThrowJSErrorCallback,
        .throwNativeError = ThrowNativeErrorCallback,
        .deleteArrayBufferRawData = [](void* buffer, int64_t lambdaId)->void {
            if (instance_) instance_->DeleteArrayBufferRawData(buffer, lambdaId);
        },
        .deleteExternal = [](int64_t id, ARKTS_Env env)->void {
            if (instance_) instance_->DeleteExternal(id, env);
        },
        .invokerLambda = [](ARKTS_CallInfo callInfo, int64_t lambdaId)->ARKTS_Value {
            return instance_ ? instance_->InvokeLambda(callInfo, lambdaId) : ARKTS_CreateUndefined();
        },
        .deleteLambda = [](ARKTS_Env env, int64_t id)->void {
            if (instance_) instance_->DeleteLambda(env, id);
        },
        .invokeAsyncLambda = [](ARKTS_Env env, int64_t id)->void {
            if (instance_) instance_->InvokeAsyncLambda(env, id);
        },
        .deleteJSContext = [](ARKTS_Env env)->void {
            if (instance_) instance_->DeleteJSContext(env);
        }
    };
    ARKTS_SetCJModuleCallback(&callbacks);
}

void MockContext::InitCycleFreeCallbacks()
{
    static ARKTS_CycleFreeCallback cycleFreeCallback {
        .funcInvoker = [](ARKTS_CallInfo callInfo, int64_t id) {
            if (instance_) {
                return instance_->InvokeCycleFreeFunc(callInfo, id);
            }
            return ARKTS_CreateUndefined();
        },
        .refRelease = [](int64_t id) {
            if (instance_) {
                instance_->ReleaseCycleFreeExt(id);
            }
        }
    };
    ARKTS_RegisterCycleFreeCallback(cycleFreeCallback);
}

void MockContext::InitDebugModeCallbacks()
{
}

} // namespace
bool JSNApi::IsMixedDebugEnabled(const EcmaVM* vm)
{
    return true;
}

void JSNApi::NotifyNativeCalling(const EcmaVM* vm, const void* nativeAddress)
{
    MockContext::GetInstance()->NotifyNativeCalling(nativeAddress);
}

void JSNApi::NotifyNativeReturn(const EcmaVM* vm, const void* nativeAddress)
{
    MockContext::GetInstance()->NotifyNativeReturn(nativeAddress);
}
}

int main(int argc, char** argv)
{
    LOGI("main in");
    testing::GTEST_FLAG(output) = "xml:./";
    testing::InitGoogleTest(&argc, argv);

    OHOS::CJEnvironment::SetAppPath("/data/test");

    auto runner = OHOS::AppExecFwk::EventRunner::Create(true);
    EXPECT_TRUE(runner);
    auto handler = std::make_shared<OHOS::AppExecFwk::EventHandler>(runner);
    EXPECT_TRUE(handler);

    TriggerArktsAssertEarlyReturnBranches();

    MockContext::Init();

    int ret = -1;
    std::condition_variable cv;

    ARKTS_Engine globalEngine;

    auto success = handler->PostTask([&ret, &cv, &globalEngine] {
        MockContext global(ARKTS_CreateEngine(), false);
        globalEngine = global.GetEngine();
        ret = testing::UnitTest::GetInstance()->Run();
        cv.notify_all();
    });

    EXPECT_TRUE(success);

    std::mutex mutex;
    std::unique_lock<std::mutex> lock(mutex);
    auto status = cv.wait_for(lock, std::chrono::seconds(30));

    EXPECT_EQ(status, std::cv_status::no_timeout);
    ARKTS_DestroyEngine(globalEngine);

    runner->Stop();

    LOGI("main out");
    return ret;
}
