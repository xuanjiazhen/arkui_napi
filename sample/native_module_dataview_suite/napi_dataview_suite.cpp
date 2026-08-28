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

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "securec.h"

// 匿名命名空间，用于文件级常量及 UPPER_CASE 常量定义
namespace {
// 缓冲区容量定义
static constexpr size_t BUFFER_SIZE_ZERO = 0;
static constexpr size_t BUFFER_SIZE_SMALL = 8;
static constexpr size_t BUFFER_SIZE_MEDIUM = 64;
static constexpr size_t BUFFER_SIZE_LARGE = 256;
static constexpr size_t BUFFER_SIZE_HUGE = 1024;
static constexpr size_t BUFFER_SIZE_GIANT = 4096;
static constexpr size_t BUFFER_SIZE_MAX = 1048576;

// 偏移量定义
static constexpr size_t OFFSET_ZERO = 0;
static constexpr size_t OFFSET_ONE = 1;
static constexpr size_t OFFSET_TWO = 2;
static constexpr size_t OFFSET_FOUR = 4;
static constexpr size_t OFFSET_EIGHT = 8;
static constexpr size_t OFFSET_SIXTEEN = 16;
static constexpr size_t OFFSET_THIRTY_TWO = 32;
static constexpr size_t OFFSET_OUT_OF_BOUNDS = 9999;

// 长度定义
static constexpr size_t LENGTH_ZERO = 0;
static constexpr size_t LENGTH_ONE = 1;
static constexpr size_t LENGTH_TWO = 2;
static constexpr size_t LENGTH_FOUR = 4;
static constexpr size_t LENGTH_EIGHT = 8;
static constexpr size_t LENGTH_SIXTEEN = 16;
static constexpr size_t LENGTH_THIRTY_TWO = 32;
static constexpr size_t LENGTH_OUT_OF_BOUNDS = 9999;

// 读写校验使用的特定测试值
static constexpr uint8_t VAL_INT8_POS = 120;
static constexpr int8_t VAL_INT8_NEG = -120;
static constexpr uint8_t VAL_UINT8 = 240;
static constexpr int16_t VAL_INT16_NEG = -30000;
static constexpr uint16_t VAL_UINT16 = 60000;
static constexpr int32_t VAL_INT32_NEG = -2000000000;
static constexpr uint32_t VAL_UINT32 = 4000000000U;
static constexpr float VAL_FLOAT = 1.2345f;
static constexpr double VAL_DOUBLE = 9.87654321;

// 其它常量
static constexpr uint32_t MODULE_VERSION = 1;
static constexpr uint32_t NO_MODULE_FLAGS = 0;

struct TestCallback {
    const char* name;
    napi_callback callback;
};

static bool SetNamedBool(napi_env env, napi_value object, const char* name, bool value)
{
    napi_value napiValue = nullptr;
    if (napi_get_boolean(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

static bool SetNamedInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int32(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

static napi_value CreateResultObject(napi_env env)
{
    napi_value result = nullptr;
    (void)napi_create_object(env, &result);
    return result;
}

static napi_value CreateArrayBufferWithSize(napi_env env, size_t byteLength, void** data)
{
    napi_value arrayBuffer = nullptr;
    (void)napi_create_arraybuffer(env, byteLength, data, &arrayBuffer);
    return arrayBuffer;
}

// ---------------------------------------------------------------------------
// 用例 1: napi_create_dataview 基本创建与常规属性获取
// ---------------------------------------------------------------------------
static napi_value TestCreateDataViewBasic(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_MEDIUM, &data);

    napi_value dataView = nullptr;
    napi_status status = napi_create_dataview(
        env, LENGTH_SIXTEEN, arrayBuffer, OFFSET_ZERO, &dataView);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "created", dataView != nullptr);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 2: napi_create_dataview 带有非零偏移量
// ---------------------------------------------------------------------------
static napi_value TestCreateDataViewOffset(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_MEDIUM, &data);

    napi_value dataView = nullptr;
    napi_status status = napi_create_dataview(
        env, LENGTH_EIGHT, arrayBuffer, OFFSET_FOUR, &dataView);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "created", dataView != nullptr);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 3: napi_create_dataview 长度为0的DataView
// ---------------------------------------------------------------------------
static napi_value TestCreateDataViewZeroLength(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_MEDIUM, &data);

    napi_value dataView = nullptr;
    napi_status status = napi_create_dataview(
        env, LENGTH_ZERO, arrayBuffer, OFFSET_ZERO, &dataView);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "created", dataView != nullptr);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 4: napi_create_dataview 针对空ArrayBuffer (长度为0)
// ---------------------------------------------------------------------------
static napi_value TestCreateDataViewEmptyArrayBuffer(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_ZERO, &data);

    napi_value dataView = nullptr;
    napi_status status = napi_create_dataview(
        env, LENGTH_ZERO, arrayBuffer, OFFSET_ZERO, &dataView);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "created", dataView != nullptr);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 5: napi_create_dataview 偏移量越界 (Offset Out of Bounds)
// ---------------------------------------------------------------------------
static napi_value TestCreateDataViewOffsetOutOfBounds(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    napi_value dataView = nullptr;
    napi_status status = napi_create_dataview(
        env, LENGTH_FOUR, arrayBuffer, OFFSET_OUT_OF_BOUNDS, &dataView);

    SetNamedBool(env, result, "statusNotOk", status != napi_ok);
    SetNamedBool(env, result, "notCreated", dataView == nullptr);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 6: napi_create_dataview 长度越界 (Length Out of Bounds)
// ---------------------------------------------------------------------------
static napi_value TestCreateDataViewLengthOutOfBounds(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    napi_value dataView = nullptr;
    napi_status status = napi_create_dataview(
        env, LENGTH_OUT_OF_BOUNDS, arrayBuffer, OFFSET_ZERO, &dataView);

    SetNamedBool(env, result, "statusNotOk", status != napi_ok);
    SetNamedBool(env, result, "notCreated", dataView == nullptr);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 7: napi_create_dataview 偏移与长度之和超出范围 (Offset + Length Out of Bounds)
// ---------------------------------------------------------------------------
static napi_value TestCreateDataViewSumOutOfBounds(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    napi_value dataView = nullptr;
    napi_status status = napi_create_dataview(
        env, LENGTH_EIGHT, arrayBuffer, OFFSET_FOUR, &dataView);

    SetNamedBool(env, result, "statusNotOk", status != napi_ok);
    SetNamedBool(env, result, "notCreated", dataView == nullptr);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 8: napi_create_dataview 传入无效(空)的ArrayBuffer
// ---------------------------------------------------------------------------
static napi_value TestCreateDataViewNullArrayBuffer(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    napi_value arrayBuffer = nullptr;

    napi_value dataView = nullptr;
    napi_status status = napi_create_dataview(
        env, LENGTH_FOUR, arrayBuffer, OFFSET_ZERO, &dataView);

    SetNamedBool(env, result, "statusNotOk", status != napi_ok);
    SetNamedBool(env, result, "notCreated", dataView == nullptr);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 9: napi_create_dataview 传入无效的 result 指针
// ---------------------------------------------------------------------------
static napi_value TestCreateDataViewNullResult(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    napi_status status = napi_create_dataview(
        env, LENGTH_FOUR, arrayBuffer, OFFSET_ZERO, nullptr);

    SetNamedBool(env, result, "statusNotOk", status != napi_ok);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 10: napi_is_dataview 判定为真 (Is DataView Positive)
// ---------------------------------------------------------------------------
static napi_value TestIsDataViewTrue(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_FOUR, arrayBuffer, OFFSET_ZERO, &dataView));

    bool isDataView = false;
    napi_status status = napi_is_dataview(env, dataView, &isDataView);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "isDataView", isDataView);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 11: napi_is_dataview 判定为假 - 普通 JavaScript 对象
// ---------------------------------------------------------------------------
static napi_value TestIsDataViewFalsePlain(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    napi_value plainObject = nullptr;
    NAPI_CALL(env, napi_create_object(env, &plainObject));

    bool isDataView = true;
    napi_status status = napi_is_dataview(env, plainObject, &isDataView);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "isNotDataView", !isDataView);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 12: napi_is_dataview 判定为假 - ArrayBuffer 实例
// ---------------------------------------------------------------------------
static napi_value TestIsDataViewFalseArrayBuffer(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    bool isDataView = true;
    napi_status status = napi_is_dataview(env, arrayBuffer, &isDataView);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "isNotDataView", !isDataView);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 13: napi_is_dataview 判定为假 - TypedArray 实例 (Int32Array)
// ---------------------------------------------------------------------------
static napi_value TestIsDataViewFalseTypedArray(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    napi_value typedArray = nullptr;
    NAPI_CALL(env, napi_create_typedarray(
        env, napi_int32_array, LENGTH_TWO, arrayBuffer, OFFSET_ZERO, &typedArray));

    bool isDataView = true;
    napi_status status = napi_is_dataview(env, typedArray, &isDataView);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "isNotDataView", !isDataView);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 14: napi_is_dataview 判定为假 - null 值
// ---------------------------------------------------------------------------
static napi_value TestIsDataViewFalseNull(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    napi_value nullValue = nullptr;
    NAPI_CALL(env, napi_get_null(env, &nullValue));

    bool isDataView = true;
    napi_status status = napi_is_dataview(env, nullValue, &isDataView);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "isNotDataView", !isDataView);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 15: napi_is_dataview 判定为假 - undefined 值
// ---------------------------------------------------------------------------
static napi_value TestIsDataViewFalseUndefined(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    napi_value undefinedValue = nullptr;
    NAPI_CALL(env, napi_get_undefined(env, &undefinedValue));

    bool isDataView = true;
    napi_status status = napi_is_dataview(env, undefinedValue, &isDataView);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "isNotDataView", !isDataView);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 16: napi_is_dataview 判定为假 - number 数字
// ---------------------------------------------------------------------------
static napi_value TestIsDataViewFalseNumber(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    napi_value numValue = nullptr;
    NAPI_CALL(env, napi_create_int32(env, static_cast<int32_t>(VAL_INT8_POS), &numValue));

    bool isDataView = true;
    napi_status status = napi_is_dataview(env, numValue, &isDataView);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "isNotDataView", !isDataView);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 17: napi_is_dataview 判定为假 - string 字符串
// ---------------------------------------------------------------------------
static napi_value TestIsDataViewFalseString(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    napi_value strValue = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(
        env, "not_a_dataview", NAPI_AUTO_LENGTH, &strValue));

    bool isDataView = true;
    napi_status status = napi_is_dataview(env, strValue, &isDataView);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "isNotDataView", !isDataView);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 18: napi_is_dataview 传入无效 result 指针
// ---------------------------------------------------------------------------
static napi_value TestIsDataViewNullResult(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_FOUR, arrayBuffer, OFFSET_ZERO, &dataView));

    napi_status status = napi_is_dataview(env, dataView, nullptr);

    SetNamedBool(env, result, "statusNotOk", status != napi_ok);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 19: napi_get_dataview_info 获取信息功能正常 (Normal Info)
// ---------------------------------------------------------------------------
static napi_value TestGetDataViewInfoNormal(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_MEDIUM, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_EIGHT, arrayBuffer, OFFSET_FOUR, &dataView));

    size_t byteLength = LENGTH_ZERO;
    void* viewData = nullptr;
    napi_value outBuffer = nullptr;
    size_t byteOffset = LENGTH_ZERO;
    napi_status status = napi_get_dataview_info(
        env, dataView, &byteLength, &viewData, &outBuffer, &byteOffset);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "lengthMatch", byteLength == LENGTH_EIGHT);
    SetNamedBool(env, result, "dataMatch", viewData == static_cast<uint8_t*>(data) + OFFSET_FOUR);
    SetNamedBool(env, result, "bufferNotNull", outBuffer != nullptr);
    SetNamedBool(env, result, "offsetMatch", byteOffset == OFFSET_FOUR);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 20: napi_get_dataview_info 部分参数传空 - byte_length 传空
// ---------------------------------------------------------------------------
static napi_value TestGetDataViewInfoNullLength(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_MEDIUM, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_EIGHT, arrayBuffer, OFFSET_FOUR, &dataView));

    void* viewData = nullptr;
    napi_value outBuffer = nullptr;
    size_t byteOffset = LENGTH_ZERO;
    napi_status status = napi_get_dataview_info(
        env, dataView, nullptr, &viewData, &outBuffer, &byteOffset);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "dataMatch", viewData == static_cast<uint8_t*>(data) + OFFSET_FOUR);
    SetNamedBool(env, result, "offsetMatch", byteOffset == OFFSET_FOUR);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 21: napi_get_dataview_info 部分参数传空 - data 传空
// ---------------------------------------------------------------------------
static napi_value TestGetDataViewInfoNullData(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_MEDIUM, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_EIGHT, arrayBuffer, OFFSET_FOUR, &dataView));

    size_t byteLength = LENGTH_ZERO;
    napi_value outBuffer = nullptr;
    size_t byteOffset = LENGTH_ZERO;
    napi_status status = napi_get_dataview_info(
        env, dataView, &byteLength, nullptr, &outBuffer, &byteOffset);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "lengthMatch", byteLength == LENGTH_EIGHT);
    SetNamedBool(env, result, "offsetMatch", byteOffset == OFFSET_FOUR);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 22: napi_get_dataview_info 部分参数传空 - arraybuffer 传空
// ---------------------------------------------------------------------------
static napi_value TestGetDataViewInfoNullBuffer(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_MEDIUM, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_EIGHT, arrayBuffer, OFFSET_FOUR, &dataView));

    size_t byteLength = LENGTH_ZERO;
    void* viewData = nullptr;
    size_t byteOffset = LENGTH_ZERO;
    napi_status status = napi_get_dataview_info(
        env, dataView, &byteLength, &viewData, nullptr, &byteOffset);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "lengthMatch", byteLength == LENGTH_EIGHT);
    SetNamedBool(env, result, "dataMatch", viewData == static_cast<uint8_t*>(data) + OFFSET_FOUR);
    SetNamedBool(env, result, "offsetMatch", byteOffset == OFFSET_FOUR);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 23: napi_get_dataview_info 部分参数传空 - byte_offset 传空
// ---------------------------------------------------------------------------
static napi_value TestGetDataViewInfoNullOffset(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_MEDIUM, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_EIGHT, arrayBuffer, OFFSET_FOUR, &dataView));

    size_t byteLength = LENGTH_ZERO;
    void* viewData = nullptr;
    napi_value outBuffer = nullptr;
    napi_status status = napi_get_dataview_info(
        env, dataView, &byteLength, &viewData, &outBuffer, nullptr);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "lengthMatch", byteLength == LENGTH_EIGHT);
    SetNamedBool(env, result, "dataMatch", viewData == static_cast<uint8_t*>(data) + OFFSET_FOUR);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 24: napi_get_dataview_info 全部返回参数传空
// ---------------------------------------------------------------------------
static napi_value TestGetDataViewInfoAllNull(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_MEDIUM, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_EIGHT, arrayBuffer, OFFSET_FOUR, &dataView));

    napi_status status = napi_get_dataview_info(
        env, dataView, nullptr, nullptr, nullptr, nullptr);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 25: napi_get_dataview_info 针对非 DataView 类型对象 (普通 JS Object)
// ---------------------------------------------------------------------------
static napi_value TestGetDataViewInfoInvalidType(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    napi_value plainObject = nullptr;
    NAPI_CALL(env, napi_create_object(env, &plainObject));

    size_t byteLength = LENGTH_ZERO;
    void* viewData = nullptr;
    napi_value outBuffer = nullptr;
    size_t byteOffset = LENGTH_ZERO;
    napi_status status = napi_get_dataview_info(
        env, plainObject, &byteLength, &viewData, &outBuffer, &byteOffset);

    SetNamedBool(env, result, "statusNotOk", status != napi_ok);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 26: napi_get_dataview_info 传入无效的 nullptr dataview
// ---------------------------------------------------------------------------
static napi_value TestGetDataViewInfoNullValue(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    napi_value dataview = nullptr;

    size_t byteLength = LENGTH_ZERO;
    void* viewData = nullptr;
    napi_value outBuffer = nullptr;
    size_t byteOffset = LENGTH_ZERO;
    napi_status status = napi_get_dataview_info(
        env, dataview, &byteLength, &viewData, &outBuffer, &byteOffset);

    SetNamedBool(env, result, "statusNotOk", status != napi_ok);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 27: DataView 底层数据读写校验 - Int8 读写
// ---------------------------------------------------------------------------
static napi_value TestDataViewReadWriteInt8(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_FOUR, arrayBuffer, OFFSET_ZERO, &dataView));

    void* viewData = nullptr;
    NAPI_CALL(env, napi_get_dataview_info(
        env, dataView, nullptr, &viewData, nullptr, nullptr));

    auto* int8Ptr = static_cast<int8_t*>(viewData);
    int8Ptr[OFFSET_ZERO] = VAL_INT8_NEG;
    int8Ptr[OFFSET_ONE] = static_cast<int8_t>(VAL_INT8_POS);

    SetNamedBool(env, result, "negMatch", int8Ptr[OFFSET_ZERO] == VAL_INT8_NEG);
    SetNamedBool(env, result, "posMatch", int8Ptr[OFFSET_ONE] == static_cast<int8_t>(VAL_INT8_POS));
    return result;
}

// ---------------------------------------------------------------------------
// 用例 28: DataView 底层数据读写校验 - Uint8 读写
// ---------------------------------------------------------------------------
static napi_value TestDataViewReadWriteUint8(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_FOUR, arrayBuffer, OFFSET_ZERO, &dataView));

    void* viewData = nullptr;
    NAPI_CALL(env, napi_get_dataview_info(
        env, dataView, nullptr, &viewData, nullptr, nullptr));

    auto* uint8Ptr = static_cast<uint8_t*>(viewData);
    uint8Ptr[OFFSET_ZERO] = VAL_UINT8;

    SetNamedBool(env, result, "valMatch", uint8Ptr[OFFSET_ZERO] == VAL_UINT8);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 29: DataView 底层数据读写校验 - Int16 读写
// ---------------------------------------------------------------------------
static napi_value TestDataViewReadWriteInt16(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_FOUR, arrayBuffer, OFFSET_ZERO, &dataView));

    void* viewData = nullptr;
    NAPI_CALL(env, napi_get_dataview_info(
        env, dataView, nullptr, &viewData, nullptr, nullptr));

    auto* int16Ptr = static_cast<int16_t*>(viewData);
    int16Ptr[OFFSET_ZERO] = VAL_INT16_NEG;

    SetNamedBool(env, result, "valMatch", int16Ptr[OFFSET_ZERO] == VAL_INT16_NEG);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 30: DataView 底层数据读写校验 - Uint16 读写
// ---------------------------------------------------------------------------
static napi_value TestDataViewReadWriteUint16(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_FOUR, arrayBuffer, OFFSET_ZERO, &dataView));

    void* viewData = nullptr;
    NAPI_CALL(env, napi_get_dataview_info(
        env, dataView, nullptr, &viewData, nullptr, nullptr));

    auto* uint16Ptr = static_cast<uint16_t*>(viewData);
    uint16Ptr[OFFSET_ZERO] = VAL_UINT16;

    SetNamedBool(env, result, "valMatch", uint16Ptr[OFFSET_ZERO] == VAL_UINT16);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 31: DataView 底层数据读写校验 - Int32 读写
// ---------------------------------------------------------------------------
static napi_value TestDataViewReadWriteInt32(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_EIGHT, arrayBuffer, OFFSET_ZERO, &dataView));

    void* viewData = nullptr;
    NAPI_CALL(env, napi_get_dataview_info(
        env, dataView, nullptr, &viewData, nullptr, nullptr));

    auto* int32Ptr = static_cast<int32_t*>(viewData);
    int32Ptr[OFFSET_ZERO] = VAL_INT32_NEG;

    SetNamedBool(env, result, "valMatch", int32Ptr[OFFSET_ZERO] == VAL_INT32_NEG);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 32: DataView 底层数据读写校验 - Uint32 读写
// ---------------------------------------------------------------------------
static napi_value TestDataViewReadWriteUint32(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_EIGHT, arrayBuffer, OFFSET_ZERO, &dataView));

    void* viewData = nullptr;
    NAPI_CALL(env, napi_get_dataview_info(
        env, dataView, nullptr, &viewData, nullptr, nullptr));

    auto* uint32Ptr = static_cast<uint32_t*>(viewData);
    uint32Ptr[OFFSET_ZERO] = VAL_UINT32;

    SetNamedBool(env, result, "valMatch", uint32Ptr[OFFSET_ZERO] == VAL_UINT32);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 33: DataView 底层数据读写校验 - Float32 读写
// ---------------------------------------------------------------------------
static napi_value TestDataViewReadWriteFloat32(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_EIGHT, arrayBuffer, OFFSET_ZERO, &dataView));

    void* viewData = nullptr;
    NAPI_CALL(env, napi_get_dataview_info(
        env, dataView, nullptr, &viewData, nullptr, nullptr));

    auto* floatPtr = static_cast<float*>(viewData);
    floatPtr[OFFSET_ZERO] = VAL_FLOAT;

    SetNamedBool(env, result, "valMatch", floatPtr[OFFSET_ZERO] == VAL_FLOAT);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 34: DataView 底层数据读写校验 - Float64 读写
// ---------------------------------------------------------------------------
static napi_value TestDataViewReadWriteFloat64(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_SMALL, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_EIGHT, arrayBuffer, OFFSET_ZERO, &dataView));

    void* viewData = nullptr;
    NAPI_CALL(env, napi_get_dataview_info(
        env, dataView, nullptr, &viewData, nullptr, nullptr));

    auto* doublePtr = static_cast<double*>(viewData);
    doublePtr[OFFSET_ZERO] = VAL_DOUBLE;

    SetNamedBool(env, result, "valMatch", doublePtr[OFFSET_ZERO] == VAL_DOUBLE);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 35: 多重视图 - 相同 Buffer 创建不重叠的两个 DataView 读写校验
// ---------------------------------------------------------------------------
static napi_value TestMultipleViewsSameBuffer(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_MEDIUM, &data);

    napi_value viewA = nullptr;
    napi_value viewB = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_FOUR, arrayBuffer, OFFSET_ZERO, &viewA));
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_FOUR, arrayBuffer, OFFSET_FOUR, &viewB));

    void* dataA = nullptr;
    void* dataB = nullptr;
    NAPI_CALL(env, napi_get_dataview_info(
        env, viewA, nullptr, &dataA, nullptr, nullptr));
    NAPI_CALL(env, napi_get_dataview_info(
        env, viewB, nullptr, &dataB, nullptr, nullptr));

    auto* ptrA = static_cast<uint8_t*>(dataA);
    auto* ptrB = static_cast<uint8_t*>(dataB);
    ptrA[OFFSET_ZERO] = VAL_INT8_POS;
    ptrB[OFFSET_ZERO] = VAL_UINT8;

    SetNamedBool(env, result, "viewAMatch", ptrA[OFFSET_ZERO] == VAL_INT8_POS);
    SetNamedBool(env, result, "viewBMatch", ptrB[OFFSET_ZERO] == VAL_UINT8);
    SetNamedBool(env, result, "distinct", ptrA != ptrB);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 36: 重叠视图 - 相同 Buffer 创建有重叠区域的两个 DataView 读写校验
// ---------------------------------------------------------------------------
static napi_value TestOverlappingViewsSameBuffer(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_MEDIUM, &data);

    napi_value viewA = nullptr;
    napi_value viewB = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_EIGHT, arrayBuffer, OFFSET_ZERO, &viewA));
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_EIGHT, arrayBuffer, OFFSET_FOUR, &viewB));

    void* dataA = nullptr;
    void* dataB = nullptr;
    NAPI_CALL(env, napi_get_dataview_info(
        env, viewA, nullptr, &dataA, nullptr, nullptr));
    NAPI_CALL(env, napi_get_dataview_info(
        env, viewB, nullptr, &dataB, nullptr, nullptr));

    auto* ptrA = static_cast<uint8_t*>(dataA);
    auto* ptrB = static_cast<uint8_t*>(dataB);
    ptrA[OFFSET_FOUR] = VAL_UINT8;

    SetNamedBool(env, result, "overlapMatch", ptrB[OFFSET_ZERO] == VAL_UINT8);
    return result;
}

// ---------------------------------------------------------------------------
// 外部 ArrayBuffer 终结器 (用于 External ArrayBuffer)
// ---------------------------------------------------------------------------
static uint8_t g_externalRawData[BUFFER_SIZE_MEDIUM] = {};

static void ExternalBufferCleanup(napi_env env, void* finalizeData, void* finalizeHint)
{
    (void)env;
    (void)finalizeData;
    (void)finalizeHint;
}

// ---------------------------------------------------------------------------
// 用例 37: 在外部外部分配的 ArrayBuffer 上创建 DataView 并进行读写校验
// ---------------------------------------------------------------------------
static napi_value TestCreateDataViewOnExternalBuffer(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    napi_value arrayBuffer = nullptr;
    NAPI_CALL(env, napi_create_external_arraybuffer(
        env, g_externalRawData, BUFFER_SIZE_MEDIUM, ExternalBufferCleanup, nullptr, &arrayBuffer));

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_SIXTEEN, arrayBuffer, OFFSET_ZERO, &dataView));

    void* viewData = nullptr;
    NAPI_CALL(env, napi_get_dataview_info(
        env, dataView, nullptr, &viewData, nullptr, nullptr));

    auto* ptr = static_cast<uint8_t*>(viewData);
    ptr[OFFSET_ZERO] = VAL_UINT8;

    SetNamedBool(env, result, "statusOk", dataView != nullptr);
    SetNamedBool(env, result, "externalRawMatch", g_externalRawData[OFFSET_ZERO] == VAL_UINT8);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 38: 大容量 ArrayBuffer (如 1MB) 创建 DataView 校验
// ---------------------------------------------------------------------------
static napi_value TestCreateDataViewHugeArrayBuffer(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_MAX, &data);

    napi_value dataView = nullptr;
    napi_status status = napi_create_dataview(
        env, BUFFER_SIZE_MAX, arrayBuffer, OFFSET_ZERO, &dataView);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "created", dataView != nullptr);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 39: 验证 get_dataview_info 返回的 ArrayBuffer 是否为同一个对象
// ---------------------------------------------------------------------------
static napi_value TestGetDataViewInfoBufferIdentity(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_MEDIUM, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_EIGHT, arrayBuffer, OFFSET_FOUR, &dataView));

    napi_value outBuffer = nullptr;
    NAPI_CALL(env, napi_get_dataview_info(
        env, dataView, nullptr, nullptr, &outBuffer, nullptr));

    bool identity = false;
    napi_status status = napi_strict_equals(env, arrayBuffer, outBuffer, &identity);

    SetNamedBool(env, result, "statusOk", status == napi_ok);
    SetNamedBool(env, result, "isIdentical", identity);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 40: 多位置读写与复合运算校验
// ---------------------------------------------------------------------------
static napi_value TestDataViewMultipleReads(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_MEDIUM, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_THIRTY_TWO, arrayBuffer, OFFSET_ZERO, &dataView));

    void* viewData = nullptr;
    NAPI_CALL(env, napi_get_dataview_info(
        env, dataView, nullptr, &viewData, nullptr, nullptr));

    auto* int32Ptr = static_cast<int32_t*>(viewData);
    constexpr int32_t valOne = 100;
    constexpr int32_t valTwo = 200;
    constexpr int32_t valThree = 300;
    constexpr int32_t expectedSum = 600;

    int32Ptr[OFFSET_ZERO] = valOne;
    int32Ptr[OFFSET_ONE] = valTwo;
    int32Ptr[OFFSET_TWO] = valThree;

    int32_t sum = int32Ptr[OFFSET_ZERO] + int32Ptr[OFFSET_ONE] + int32Ptr[OFFSET_TWO];

    SetNamedBool(env, result, "sumMatch", sum == expectedSum);
    return result;
}

// ---------------------------------------------------------------------------
// 用例 41: 边界位置读写校验
// ---------------------------------------------------------------------------
static napi_value TestDataViewBoundaryWrites(napi_env env, napi_callback_info info)
{
    (void)info;
    napi_value result = CreateResultObject(env);
    void* data = nullptr;
    napi_value arrayBuffer = CreateArrayBufferWithSize(env, BUFFER_SIZE_MEDIUM, &data);

    napi_value dataView = nullptr;
    NAPI_CALL(env, napi_create_dataview(
        env, LENGTH_SIXTEEN, arrayBuffer, OFFSET_FOUR, &dataView));

    void* viewData = nullptr;
    NAPI_CALL(env, napi_get_dataview_info(
        env, dataView, nullptr, &viewData, nullptr, nullptr));

    auto* ptr = static_cast<uint8_t*>(viewData);
    constexpr size_t lastIndex = 15;
    ptr[lastIndex] = VAL_UINT8;

    SetNamedBool(env, result, "boundaryMatch", ptr[lastIndex] == VAL_UINT8);
    return result;
}

static const TestCallback DATAVIEW_TESTS[] = {
    { "testCreateDataViewBasic", TestCreateDataViewBasic },
    { "testCreateDataViewOffset", TestCreateDataViewOffset },
    { "testCreateDataViewZeroLength", TestCreateDataViewZeroLength },
    { "testCreateDataViewEmptyArrayBuffer", TestCreateDataViewEmptyArrayBuffer },
    { "testCreateDataViewOffsetOutOfBounds", TestCreateDataViewOffsetOutOfBounds },
    { "testCreateDataViewLengthOutOfBounds", TestCreateDataViewLengthOutOfBounds },
    { "testCreateDataViewSumOutOfBounds", TestCreateDataViewSumOutOfBounds },
    { "testCreateDataViewNullArrayBuffer", TestCreateDataViewNullArrayBuffer },
    { "testCreateDataViewNullResult", TestCreateDataViewNullResult },
    { "testIsDataViewTrue", TestIsDataViewTrue },
    { "testIsDataViewFalsePlain", TestIsDataViewFalsePlain },
    { "testIsDataViewFalseArrayBuffer", TestIsDataViewFalseArrayBuffer },
    { "testIsDataViewFalseTypedArray", TestIsDataViewFalseTypedArray },
    { "testIsDataViewFalseNull", TestIsDataViewFalseNull },
    { "testIsDataViewFalseUndefined", TestIsDataViewFalseUndefined },
    { "testIsDataViewFalseNumber", TestIsDataViewFalseNumber },
    { "testIsDataViewFalseString", TestIsDataViewFalseString },
    { "testIsDataViewNullResult", TestIsDataViewNullResult },
    { "testGetDataViewInfoNormal", TestGetDataViewInfoNormal },
    { "testGetDataViewInfoNullLength", TestGetDataViewInfoNullLength },
    { "testGetDataViewInfoNullData", TestGetDataViewInfoNullData },
    { "testGetDataViewInfoNullBuffer", TestGetDataViewInfoNullBuffer },
    { "testGetDataViewInfoNullOffset", TestGetDataViewInfoNullOffset },
    { "testGetDataViewInfoAllNull", TestGetDataViewInfoAllNull },
    { "testGetDataViewInfoInvalidType", TestGetDataViewInfoInvalidType },
    { "testGetDataViewInfoNullValue", TestGetDataViewInfoNullValue },
    { "testDataViewReadWriteInt8", TestDataViewReadWriteInt8 },
    { "testDataViewReadWriteUint8", TestDataViewReadWriteUint8 },
    { "testDataViewReadWriteInt16", TestDataViewReadWriteInt16 },
    { "testDataViewReadWriteUint16", TestDataViewReadWriteUint16 },
    { "testDataViewReadWriteInt32", TestDataViewReadWriteInt32 },
    { "testDataViewReadWriteUint32", TestDataViewReadWriteUint32 },
    { "testDataViewReadWriteFloat32", TestDataViewReadWriteFloat32 },
    { "testDataViewReadWriteFloat64", TestDataViewReadWriteFloat64 },
    { "testMultipleViewsSameBuffer", TestMultipleViewsSameBuffer },
    { "testOverlappingViewsSameBuffer", TestOverlappingViewsSameBuffer },
    { "testCreateDataViewOnExternalBuffer", TestCreateDataViewOnExternalBuffer },
    { "testCreateDataViewHugeArrayBuffer", TestCreateDataViewHugeArrayBuffer },
    { "testGetDataViewInfoBufferIdentity", TestGetDataViewInfoBufferIdentity },
    { "testDataViewMultipleReads", TestDataViewMultipleReads },
    { "testDataViewBoundaryWrites", TestDataViewBoundaryWrites }
};

static constexpr size_t DATAVIEW_TEST_COUNT = sizeof(DATAVIEW_TESTS) / sizeof(DATAVIEW_TESTS[0]);
} // namespace

static napi_value InitDataViewSuite(napi_env env, napi_value exports)
{
    napi_property_descriptor descriptors[DATAVIEW_TEST_COUNT];
    for (size_t i = 0; i < DATAVIEW_TEST_COUNT; i++) {
        descriptors[i] = napi_property_descriptor {
            DATAVIEW_TESTS[i].name,
            nullptr,
            DATAVIEW_TESTS[i].callback,
            nullptr,
            nullptr,
            nullptr,
            napi_default,
            nullptr
        };
    }
    NAPI_CALL(env, napi_define_properties(
        env, exports, DATAVIEW_TEST_COUNT, descriptors));
    return exports;
}

static napi_module g_dataviewSuiteModule = {
    .nm_version = MODULE_VERSION,
    .nm_flags = NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = InitDataViewSuite,
    .nm_modname = "dataview_suite",
    .nm_priv = nullptr,
    .reserved = { 0 },
};

extern "C" __attribute__((constructor)) void RegisterDataviewSuiteModule(void)
{
    napi_module_register(&g_dataviewSuiteModule);
}
