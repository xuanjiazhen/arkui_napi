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

#include "napi/native_common.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "securec.h"
#include "test.h"
#include "test_common.h"
#include "utils/log.h"
#ifdef FOR_JERRYSCRIPT_TEST
#include "jerryscript-core.h"
#endif

static constexpr int32_t NAPI_UT_BUFFER_SIZE = 64;

class NapiExtTest : public NativeEngineTest {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "NapiExtTest SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "NapiExtTest TearDownTestCase";
    }

    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: UndefinedTest
 * @tc.desc: Test undefined type.
 * @tc.type: FUNC
 */
HWTEST_F(NapiExtTest, CreateBufferTest001, testing::ext::TestSize.Level1)
{
    napi_env env = (napi_env)engine_;

    napi_value buffer = nullptr;
    void* bufferPtr = nullptr;
    size_t bufferSize = NAPI_UT_BUFFER_SIZE;
    napi_create_buffer(env, bufferSize, &bufferPtr, &buffer);
    void* tmpBufferPtr = nullptr;
    size_t bufferLength = 0;
    napi_get_buffer_info(env, buffer, &tmpBufferPtr, &bufferLength);

    ASSERT_EQ(bufferPtr, tmpBufferPtr);
    ASSERT_EQ(bufferSize, bufferLength);
}

HWTEST_F(NapiExtTest, CreateBufferTest002, testing::ext::TestSize.Level1) 
{
    napi_env env = (napi_env)engine_;

    napi_value buffer = nullptr;
    void* bufferPtr = nullptr;
    size_t bufferSize = -1;
    napi_status creatresult = napi_create_buffer(env, bufferSize, &bufferPtr, &buffer);

    ASSERT_EQ(creatresult, napi_status::napi_invalid_arg);
    ASSERT_EQ(bufferPtr, nullptr);
}

HWTEST_F(NapiExtTest, CreateBufferTest003, testing::ext::TestSize.Level1) 
{
    napi_env env = (napi_env)engine_;

    napi_value buffer = nullptr;
    void* bufferPtr = nullptr;
    const char bufferdata[] = "for test";
    const char* data = bufferdata;
    size_t bufferSize = NAPI_UT_BUFFER_SIZE;
    napi_create_buffer_copy(env, bufferSize, data, &bufferPtr, &buffer);

    void* tmpBufferPtr = nullptr;
    size_t bufferLength = 0;
    napi_get_buffer_info(env, buffer, &tmpBufferPtr, &bufferLength);
//add
    bool isBuffer = false;
    napi_is_buffer(env, buffer, &isBuffer);
    ASSERT_TRUE(isBuffer);  

    ASSERT_EQ(bufferPtr, tmpBufferPtr);
    ASSERT_EQ(bufferSize, bufferLength);
    ASSERT_EQ(0, memcmp(bufferdata, bufferPtr, bufferSize));
}

HWTEST_F(NapiExtTest, CreateBufferTest004, testing::ext::TestSize.Level1) 
{
    napi_env env = (napi_env)engine_;

    napi_value buffer = nullptr;
    void* bufferPtr = nullptr;
    const char* data = nullptr;
    size_t bufferSize = -1;
    napi_status creatresult = napi_create_buffer_copy(env, bufferSize, data, &bufferPtr, &buffer);

    ASSERT_EQ(creatresult, napi_status::napi_invalid_arg);
    ASSERT_EQ(bufferPtr, nullptr);
}

HWTEST_F(NapiExtTest, CreateBufferTest005, testing::ext::TestSize.Level1) 
{
    napi_env env = (napi_env)engine_;

    napi_value buffer = nullptr;
    char testStr[] = "test";
    void* bufferPtr = testStr;

    size_t bufferSize = NAPI_UT_BUFFER_SIZE;
    napi_create_external_buffer(
        env, bufferSize, bufferPtr, [](napi_env env, void* data, void* hint) {}, (void*)testStr, &buffer); 

    void* tmpBufferPtr = nullptr;
    size_t bufferLength = 0;
    napi_get_buffer_info(env, buffer, &tmpBufferPtr, &bufferLength);
    bool isBuffer = false;
    napi_is_buffer(env, buffer, &isBuffer);
//add
    ASSERT_TRUE(isBuffer); 
    ASSERT_EQ(bufferPtr, tmpBufferPtr); 

    ASSERT_EQ(bufferSize, bufferLength);
}

HWTEST_F(NapiExtTest, IsBufferTest001, testing::ext::TestSize.Level1)
{
    napi_env env = (napi_env)engine_;

    napi_value buffer = nullptr;
    void* bufferPtr = nullptr;
    size_t bufferSize = NAPI_UT_BUFFER_SIZE;
    bool isBuffer = false;

    napi_create_buffer(env, bufferSize, &bufferPtr, &buffer);

    void* tmpBufferPtr = nullptr;
    size_t bufferLength = 0;
    napi_get_buffer_info(env, buffer, &tmpBufferPtr, &bufferLength);
    napi_is_buffer(env, buffer, &isBuffer);
//add
    ASSERT_EQ(bufferSize, bufferLength);
    ASSERT_EQ(bufferPtr, tmpBufferPtr); 

    ASSERT_TRUE(isBuffer);
}

HWTEST_F(NapiExtTest, IsBufferTest002, testing::ext::TestSize.Level1) 
{
    napi_env env = (napi_env)engine_;

    napi_value buffer = nullptr;
    void* bufferPtr = nullptr;
    size_t bufferSize = -1;
    bool isBuffer = false;

    napi_create_buffer(env, bufferSize, &bufferPtr, &buffer);

    void* tmpBufferPtr = nullptr;
    size_t bufferLength = 0;
    napi_get_buffer_info(env, buffer, &tmpBufferPtr, &bufferLength);
    napi_is_buffer(env, buffer, &isBuffer);

    ASSERT_EQ(isBuffer, false);
}

HWTEST_F(NapiExtTest, IsDetachedArrayBufferTest001, testing::ext::TestSize.Level1)
{                                                                                  
    static constexpr size_t arrayBufferSize = 1024;
    napi_env env = (napi_env)engine_;
    napi_value arrayBuffer = nullptr;
    void* arrayBufferPtr = nullptr;
    napi_create_arraybuffer(env, arrayBufferSize, &arrayBufferPtr, &arrayBuffer);

    bool result = false;
    ASSERT_CHECK_CALL(napi_is_detached_arraybuffer(env, arrayBuffer, &result));

    auto out = napi_detach_arraybuffer(env, arrayBuffer);
    if (out == napi_ok) {
        arrayBufferPtr = nullptr;
    }
    ASSERT_EQ(out, napi_ok);

    result = false;
    ASSERT_CHECK_CALL(napi_is_detached_arraybuffer(env, arrayBuffer, &result));
    ASSERT_TRUE(result);
}

#if  (defined(FOR_JERRYSCRIPT_TEST)) &&  (JERRY_API_MINOR_VERSION <= 3)
    // jerryscript 2.3 do nothing
#else
// jerryscript 2.4 or quickjs or V8

static constexpr size_t NAPI_UT_STR_LENGTH = 30;

HWTEST_F(NapiExtTest, FreezeObjectTest001, testing::ext::TestSize.Level1)
{
    constexpr int dataSize = 60;
    napi_env env = (napi_env)engine_;
    napi_value object = nullptr;
    napi_create_object(env, &object);

    const char testStr[] = "1234567";
    napi_value strAttribute = nullptr;
    napi_create_string_utf8(env, testStr, strlen(testStr), &strAttribute);
    napi_set_named_property(env, object, "strAttribute", strAttribute);

    int32_t testNumber = 12345;
    napi_value numberAttribute = nullptr;
    napi_create_int32(env, testNumber, &numberAttribute);
    napi_set_named_property(env, object, "numberAttribute", numberAttribute);

    ASSERT_CHECK_CALL(napi_object_freeze(env, object));

    int32_t testNumber2 = 111111;
    napi_value numberAttribute2 = nullptr;
    napi_create_int32(env, testNumber2, &numberAttribute2);
    ASSERT_CHECK_CALL(napi_set_named_property(env, object, "test", numberAttribute2));

    napi_key_collection_mode keyMode = napi_key_own_only;
    napi_key_filter keyFilter = napi_key_all_properties;
    napi_key_conversion keyConversion = napi_key_keep_numbers;
    napi_value propNames = nullptr;
    ASSERT_CHECK_CALL(napi_get_all_property_names(env, object, keyMode, keyFilter, keyConversion, &propNames));

    uint32_t arrayLength = 0;
    ASSERT_CHECK_CALL(napi_get_array_length(env, propNames, &arrayLength));
    ASSERT_EQ(arrayLength, (uint32_t)2);

    char names[2][30];
    memset_s(names, dataSize, 0, dataSize);
    auto ret = memcpy_s(names[0], strlen("strAttribute"), "strAttribute", strlen("strAttribute"));
    ASSERT_EQ(ret, EOK);
    ret = memcpy_s(names[1], strlen("numberAttribute"), "numberAttribute", strlen("numberAttribute"));
    ASSERT_EQ(ret, EOK);
    for (uint32_t i = 0; i < arrayLength; i++) {
        bool hasElement = false;
        ASSERT_CHECK_CALL(napi_has_element(env, propNames, i, &hasElement));

        napi_value propName = nullptr;
        ASSERT_CHECK_CALL(napi_get_element(env, propNames, i, &propName));
        ASSERT_CHECK_VALUE_TYPE(env, propName, napi_string);

        size_t testStrLength = NAPI_UT_STR_LENGTH;
        char testStrInner[NAPI_UT_STR_LENGTH + 1];
        size_t outStrLength = 0;
        memset_s(testStrInner, testStrLength + 1, 0, testStrLength + 1);
        ASSERT_CHECK_CALL(napi_get_value_string_utf8(env, propName, testStrInner, testStrLength, &outStrLength));

        int ret = strcmp(testStrInner, names[i]);
        ASSERT_EQ(ret, 0);
    }
}

HWTEST_F(NapiExtTest, SealObjectTest001, testing::ext::TestSize.Level1) 
{
    napi_env env = (napi_env)engine_;
    napi_value object = nullptr;

    napi_create_object(env, &object);

    const char testStr[] = "1234567";
    napi_value strAttribute = nullptr;
    napi_create_string_utf8(env, testStr, strlen(testStr), &strAttribute);
    napi_set_named_property(env, object, "strAttribute", strAttribute);

    int32_t testNumber = 12345;
    napi_value numberAttribute = nullptr;
    napi_create_int32(env, testNumber, &numberAttribute);
    napi_set_named_property(env, object, "numberAttribute", numberAttribute);

    ASSERT_CHECK_CALL(napi_object_seal(env, object));

    int32_t testNumber2 = 111111;
    napi_value numberAttribute2 = nullptr;
    napi_create_int32(env, testNumber2, &numberAttribute2);
    ASSERT_CHECK_CALL(napi_set_named_property(env, object, "test", numberAttribute2));

    napi_key_collection_mode keyMode = napi_key_own_only;
    napi_key_filter keyFilter = napi_key_all_properties;
    napi_key_conversion keyConversion = napi_key_keep_numbers;
    napi_value propNames = nullptr;
    ASSERT_CHECK_CALL(napi_get_all_property_names(env, object, keyMode, keyFilter, keyConversion, &propNames));

    uint32_t arrayLength = 0;
    ASSERT_CHECK_CALL(napi_get_array_length(env, propNames, &arrayLength));
    ASSERT_EQ(arrayLength, (uint32_t)2);

    char names[2][NAPI_UT_STR_LENGTH];
    memset_s(names, NAPI_UT_STR_LENGTH * 2, 0, NAPI_UT_STR_LENGTH * 2);
    auto ret = memcpy_s(names[0], strlen("strAttribute"), "strAttribute", strlen("strAttribute"));
    ASSERT_EQ(ret, EOK);
    ret = memcpy_s(names[1], strlen("numberAttribute"), "numberAttribute", strlen("numberAttribute"));
    ASSERT_EQ(ret, EOK);

    for (uint32_t i = 0; i < arrayLength; i++) {
        bool hasElement = false;
        ASSERT_CHECK_CALL(napi_has_element(env, propNames, i, &hasElement));

        napi_value propName = nullptr;
        ASSERT_CHECK_CALL(napi_get_element(env, propNames, i, &propName));
        ASSERT_CHECK_VALUE_TYPE(env, propName, napi_string);

        size_t testStrLength = NAPI_UT_STR_LENGTH;
        char testStrInner[NAPI_UT_STR_LENGTH + 1];
        size_t outStrLength = 0;
        memset_s(testStrInner, testStrLength + 1, 0, testStrLength + 1);
        ASSERT_CHECK_CALL(napi_get_value_string_utf8(env, propName, testStrInner, testStrLength, &outStrLength));

        int ret = strcmp(testStrInner, names[i]);
        ASSERT_EQ(ret, 0);
    }
}

HWTEST_F(NapiExtTest, AllPropertyNamesTest001, testing::ext::TestSize.Level1) 
{
    napi_env env = (napi_env)engine_;
    napi_key_collection_mode keyMode = napi_key_own_only;
    napi_key_filter keyFilter = napi_key_all_properties;
    napi_key_conversion keyConversion = napi_key_keep_numbers;
    napi_value result = nullptr;
    napi_value propNames = nullptr;

    ASSERT_CHECK_CALL(napi_create_object(env, &result));
    ASSERT_CHECK_VALUE_TYPE(env, result, napi_object);

    const char testStr[] = "1234567";
    napi_value strAttribute = nullptr;
    napi_create_string_utf8(env, testStr, strlen(testStr), &strAttribute);
    napi_set_named_property(env, result, "strAttribute", strAttribute);

    int32_t testNumber = 12345;
    napi_value numberAttribute = nullptr;
    napi_create_int32(env, testNumber, &numberAttribute);
    napi_set_named_property(env, result, "numberAttribute", numberAttribute);

    ASSERT_CHECK_CALL(napi_get_all_property_names(env, result, keyMode, keyFilter, keyConversion, &propNames));

    ASSERT_CHECK_VALUE_TYPE(env, propNames, napi_object);
    bool isArray = false;
    ASSERT_CHECK_CALL(napi_is_array(env, propNames, &isArray));
    ASSERT_TRUE(isArray);
    uint32_t arrayLength = 0;
    ASSERT_CHECK_CALL(napi_get_array_length(env, propNames, &arrayLength));
    ASSERT_EQ(arrayLength, (uint32_t)2);

    char names[2][NAPI_UT_STR_LENGTH];
    memset_s(names, NAPI_UT_STR_LENGTH * 2, 0, NAPI_UT_STR_LENGTH * 2);
    auto ret = memcpy_s(names[0], strlen("strAttribute"), "strAttribute", strlen("strAttribute"));
    ASSERT_EQ(ret, EOK);
    ret = memcpy_s(names[1], strlen("numberAttribute"), "numberAttribute", strlen("numberAttribute"));
    ASSERT_EQ(ret, EOK);

    for (uint32_t i = 0; i < arrayLength; i++) {
        bool hasElement = false;
        ASSERT_CHECK_CALL(napi_has_element(env, propNames, i, &hasElement));

        napi_value propName = nullptr;
        ASSERT_CHECK_CALL(napi_get_element(env, propNames, i, &propName));
        ASSERT_CHECK_VALUE_TYPE(env, propName, napi_string);

        size_t testStrLength = NAPI_UT_STR_LENGTH;
        char testStrInner[NAPI_UT_STR_LENGTH + 1];
        size_t outStrLength = 0;
        memset_s(testStrInner, testStrLength + 1, 0, testStrLength + 1);
        ASSERT_CHECK_CALL(napi_get_value_string_utf8(env, propName, testStrInner, testStrLength, &outStrLength));

        int ret = strcmp(testStrInner, names[i]);
        ASSERT_EQ(ret, 0);
    }
}
#endif