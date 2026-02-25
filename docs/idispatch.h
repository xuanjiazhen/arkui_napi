/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with License.
 * You may obtain a copy of License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @addtogroup AbilityRuntime_IDispatch
 * @{
 *
 * @brief Provide definition of C interface for IDispatch late binding mechanism
 *
 * @since 24
 */

/**
 * @file idispatch.h
 *
 * @brief Declare common types and interfaces for IDispatch late binding.
 *
 * @library libidispatch_ndk.so
 * @kit AbilityKit
 * @syscap SystemCapability.Ability.AbilityRuntime.Core
 * @since 24
 */

#ifndef ABILITY_RUNTIME_IDISPATCH_H
#define ABILITY_RUNTIME_IDISPATCH_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Define error codes for IDispatch operations.
 *
 * @since 24
 */
typedef enum {
    /**
     * Operation successful.
     */
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK = 0,
    /**
     * Invalid parameter.
     */
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID = -1,
    /**
     * Method or property not found.
     */
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND = -2,
    /**
     * Type mismatch.
     */
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_TYPE_MISMATCH = -3,
    /**
     * IPC communication failed.
     */
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_IPC_FAILED = -4,
    /**
     * Out of memory.
     */
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OUT_OF_MEMORY = -5,
    /**
     * Permission denied.
     */
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PERMISSION_DENIED = -6,
    /**
     * Context does not exist.
     */
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_CONTEXT_NOT_EXIST = -7
} AbilityRuntime_IDispatch_ErrorCode;

/**
 * @brief Define variant type enumeration.
 *
 * @since 24
 */
typedef enum {
    /**
     * Empty value.
     */
    ABILITY_RUNTIME_IDISPATCH_VT_EMPTY = 0,
    /**
     * Null value.
     */
    ABILITY_RUNTIME_IDISPATCH_VT_NULL = 1,
    /**
     * 8-bit signed integer.
     */
    ABILITY_RUNTIME_IDISPATCH_VT_I8 = 2,
    /**
     * 32-bit signed integer.
     */
    ABILITY_RUNTIME_IDISPATCH_VT_I32 = 3,
    /**
     * 64-bit signed integer.
     */
    ABILITY_RUNTIME_IDISPATCH_VT_I64 = 4,
    /**
     * 64-bit floating point.
     */
    ABILITY_RUNTIME_IDISPATCH_VT_F64 = 5,
    /**
     * String (UTF-8).
     */
    ABILITY_RUNTIME_IDISPATCH_VT_STRING = 6,
    /**
     * Void type.
     */
    ABILITY_RUNTIME_IDISPATCH_VT_VOID = 7,
    /**
     * Enum type.
     */
    ABILITY_RUNTIME_IDISPATCH_VT_ENUM = 8,
    /**
     * Interface type.
     */
    ABILITY_RUNTIME_IDISPATCH_VT_INTERFACE = 9,
    /**
     * Struct type.
     */
    ABILITY_RUNTIME_IDISPATCH_VT_STRUCT = 10
} AbilityRuntime_IDispatch_Vt;

/**
 * @brief Define parameter information structure.
 *
 * @since 24
 */
typedef struct {
    /**
     * Parameter name.
     */
    const char* name;
    /**
     * Variant type.
     */
    AbilityRuntime_IDispatch_Vt vt;
    /**
     * Is input parameter.
     */
    bool isIn;
    /**
     * Is output parameter.
     */
    bool isOut;
} AbilityRuntime_IDispatch_ParamInfo;

/**
 * @brief Define method information structure.
 *
 * @since 24
 */
typedef struct {
    /**
     * Dispatch ID.
     */
    uint32_t dispId;
    /**
     * Method/property name.
     */
    const char* name;
    /**
     * Return type name.
     */
    const char* returnType;
    /**
     * Array of parameter descriptions.
     */
    AbilityRuntime_IDispatch_ParamInfo* params;
    /**
     * Number of parameters.
     */
    uint32_t cParams;
} AbilityRuntime_IDispatch_MethodInfo;

/**
 * @brief Define variant structure using pointer + type tag for ABI compatibility.
 *
 * @since 24
 */
typedef struct {
    /**
     * Variant type.
     */
    AbilityRuntime_IDispatch_Vt vt;
    /**
     * Variant value pointer (opaque pointer to actual value).
     */
    void* value;
} AbilityRuntime_IDispatch_Variant;

/**
 * @brief Define IDispatchTypeInfo structure type.
 *
 * @since 24
 */
struct AbilityRuntime_IDispatch_TypeInfo;

/**
 * @brief Defines pointer to AbilityRuntime_IDispatch_TypeInfo.
 *
 * @since 24
 */
typedef struct AbilityRuntime_IDispatch_TypeInfo* AbilityRuntime_IDispatch_TypeInfoHandle;

/**
 * @brief Define IDispatch structure type.
 *
 * @since 24
 */
struct AbilityRuntime_IDispatch;

/**
 * @brief Defines pointer to AbilityRuntime_IDispatch.
 *
 * @since 24
 */
typedef struct AbilityRuntime_IDispatch* AbilityRuntime_IDispatchHandle;

/**
 * @brief Define IDispatchStruct structure type for struct operations.
 *
 * @since 24
 */
struct AbilityRuntime_IDispatch_Struct;

/**
 * @brief Defines pointer to AbilityRuntime_IDispatch_Struct.
 *
 * @since 24
 */
typedef struct AbilityRuntime_IDispatch_Struct* AbilityRuntime_IDispatch_StructHandle;

/**
 * @brief Define AbilityRuntime_IDispatch_TypeInfo interface for type information query.
 *
 * @since 24
 */
struct AbilityRuntime_IDispatch_TypeInfo {
    /**
     * @brief Get interface name.
     *
     * @param typeInfo The type info object.
     * @param name A pointer to a buffer that receives interface name.
     * @param size The length of buffer.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or name is null.
     * @since 24
     */
    int32_t (*GetInterfaceName)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, char* name, uint32_t size);

    /**
     * @brief Get interface descriptor.
     *
     * @param typeInfo The type info object.
     * @param descriptor A pointer to a buffer that receives interface descriptor.
     * @param size The length of buffer.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or descriptor is null.
     * @since 24
     */
    int32_t (*GetInterfaceDescriptor)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, char* descriptor, uint32_t size);

    /**
     * @brief Get interface version.
     *
     * @param typeInfo The type info object.
     * @param version A pointer to receive interface version.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or version is null.
     * @since 24
     */
    int32_t (*GetInterfaceVersion)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, uint32_t* version);

    /**
     * @brief Get method count.
     *
     * @param typeInfo The type info object.
     * @param count A pointer to receive method count.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or count is null.
     * @since 24
     */
    int32_t (*GetMethodCount)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, uint32_t* count);

    /**
     * @brief Get method name by index.
     *
     * @param typeInfo The type info object.
     * @param index The method index.
     * @param name A pointer to a buffer that receives method name.
     * @param size The length of buffer.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or name is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if index is out of range.
     * @since 24
     */
    int32_t (*GetMethodName)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, uint32_t index, char* name, uint32_t size);

    /**
     * @brief Get method DispID by name.
     *
     * @param typeInfo The type info object.
     * @param name The method name.
     * @param dispId A pointer to receive DispID.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or name or dispId is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if method not found.
     * @since 24
     */
    int32_t (*GetMethodDispId)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, const char* name, uint32_t* dispId);

    /**
     * @brief Get method name by DispID.
     *
     * @param typeInfo The type info object.
     * @param dispId The DispID.
     * @param name A pointer to a buffer that receives method name.
     * @param size The length of buffer.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or name is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if DispID not found.
     * @since 24
     */
    int32_t (*GetMethodNameByDispId)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, uint32_t dispId, char* name, uint32_t size);

    /**
     * @brief Get method parameter count.
     *
     * @param typeInfo The type info object.
     * @param dispId The DispID.
     * @param count A pointer to receive parameter count.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or count is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if DispID not found.
     * @since 24
     */
    int32_t (*GetMethodParamCount)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, uint32_t dispId, uint32_t* count);

    /**
     * @brief Get method parameter name.
     *
     * @param typeInfo The type info object.
     * @param dispId The DispID.
     * @param paramIndex The parameter index.
     * @param name A pointer to a buffer that receives parameter name.
     * @param size The length of buffer.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or name is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if DispID or parameter index not found.
     * @since 24
     */
    int32_t (*GetMethodParamName)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, uint32_t dispId, uint32_t paramIndex,
        char* name, uint32_t size);

    /**
     * @brief Get method parameter type.
     *
     * @param typeInfo The type info object.
     * @param dispId The DispID.
     * @param paramIndex The parameter index.
     * @param type A pointer to a buffer that receives parameter type string.
     * @param size The length of buffer.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or type is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if DispID or parameter index not found.
     * @since 24
     */
    int32_t (*GetMethodParamType)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, uint32_t dispId, uint32_t paramIndex,
        char* type, uint32_t size);

    /**
     * @brief Get method parameter direction.
     *
     * @param typeInfo The type info object.
     * @param dispId The DispID.
     * @param paramIndex The parameter index.
     * @param direction A pointer to a buffer that receives parameter direction ("in" or "out").
     * @param size The length of buffer.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or direction is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if DispID or parameter index not found.
     * @since 24
     */
    int32_t (*GetMethodParamDirection)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, uint32_t dispId, uint32_t paramIndex,
        char* direction, uint32_t size);

    /**
     * @brief Get method return type.
     *
     * @param typeInfo The type info object.
     * @param dispId The DispID.
     * @param type A pointer to a buffer that receives return type string.
     * @param size The length of buffer.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or type is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if DispID not found.
     * @since 24
     */
    int32_t (*GetMethodReturnType)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, uint32_t dispId, char* type, uint32_t size);

    /**
     * @brief Get enum count.
     *
     * @param typeInfo The type info object.
     * @param count A pointer to receive enum count.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or count is null.
     * @since 24
     */
    int32_t (*GetEnumCount)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, uint32_t* count);

    /**
     * @brief Get enum name by index.
     *
     * @param typeInfo The type info object.
     * @param index The enum index.
     * @param name A pointer to a buffer that receives enum name.
     * @param size The length of buffer.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or name is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if index is out of range.
     * @since 24
     */
    int32_t (*GetEnumName)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, uint32_t index, char* name, uint32_t size);

    /**
     * @brief Get enum value count.
     *
     * @param typeInfo The type info object.
     * @param enumName The enum name.
     * @param count A pointer to receive enum value count.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or enumName or count is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if enum not found.
     * @since 24
     */
    int32_t (*GetEnumValueCount)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, const char* enumName, uint32_t* count);

    /**
     * @brief Get enum value name.
     *
     * @param typeInfo The type info object.
     * @param enumName The enum name.
     * @param valueIndex The enum value index.
     * @param name A pointer to a buffer that receives enum value name.
     * @param size The length of buffer.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or enumName or name is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if enum or value index not found.
     * @since 24
     */
    int32_t (*GetEnumValueName)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, const char* enumName, uint32_t valueIndex,
        char* name, uint32_t size);

    /**
     * @ @brief Get enum value.
     *
     * @param typeInfo The type info object.
     * @param enumName The enum name.
     * @param valueIndex The enum value index.
     * @param value A pointer: to receive enum value.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or enumName or value is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if enum or value index not found.
     * @since 24
     */
    int32_t (*GetEnumValue)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, const char* enumName, uint32_t valueIndex, int32_t* value);

    /**
     * @brief Get struct count.
     *
     * @param typeInfo The type info object.
     * @param count A pointer to receive struct count.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or count is null.
     * @since 24
     */
    int32_t (*GetStructCount)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, uint32_t* count);

    /**
     * @brief Get struct name by index.
     *
     * @param typeInfo The type info object.
     * @param index The struct index.
     * @param name A pointer to a buffer that receives struct name.
     * @param size The length of buffer.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or name is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if index is out of range.
     * @since 24
     */
    int32_t (*GetStructName)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, uint32_t index, char* name, uint32_t size);

    /**
     * @brief Get struct field count.
     *
     * @param typeInfo The type info object.
     * @param structName The struct name.
     * @param count A pointer to receive field count.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or structName or count is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if struct not found.
     * @since 24
     */
    int32_t (*GetStructFieldCount)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, const char* structName, uint32_t* count);

    /**
     * @ @brief Get struct field name.
     *
     * @param typeInfo The type info object.
     * @param structName The struct name.
     * @param fieldIndex The field index.
     * @param name A pointer to a buffer that receives field name.
     * @param size The length of buffer.
     * @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or structName or name is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if struct or field index not found.
     * @since 24
     */
    int32_t (*GetStructFieldName)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, const char* structName, uint32_t fieldIndex,
        char* name, uint32_t size);

    /**
     * @brief Get struct field type.
     *
     * @param typeInfo The type info object.
     * @param structName The struct name.
     * @param fieldIndex The field index.
     * @param type A pointer to a buffer that receives field type string.
     * @param size The length of buffer.
     * @ @return The error code.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if typeInfo or structName or type is null.
     *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if struct or field index not found.
     * @since 24
     */
    int32_t (*GetStructFieldType)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo, const char* structName, uint32_t fieldIndex,
        char* type, uint32_t size);

    /**
     * @brief Release type info object.
     *
     * @param typeInfo The type info object to release.
     * @since 24
     */
    void (*Release)(AbilityRuntime_IDispatch_TypeInfoHandle typeInfo);
};

/**
 * @brief Initialize variant structure.
 *
 * @param variant The variant to initialize.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant is null.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_Init(
    AbilityRuntime_IDispatch_Variant* variant);

/**
 * @brief Set int8 value to variant.
 *
 * @param variant The variant to set.
 * @param value The int8 value.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant is null.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_SetInt8(
    AbilityRuntime_IDispatch_Variant* variant, int8_t value);

/**
 * @ @brief Set int32 value to variant.
 *
 * @param variant The variant to set.
 * @param value The int32 value.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant is null.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_SetInt32(
    AbilityRuntime_IDispatch_Variant* variant, int32_t value);

/**
 * @ @brief Set int64 value to variant.
 *
 * @param variant The variant to set.
 * @param value The int64 value.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant is null.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_SetInt64(
    AbilityRuntime_IDispatch_Variant* variant, int64_t value);

/**
 * @ @brief Set float value to variant.
 *
 * @param variant The variant to set.
 * @param value The float value.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant is null.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_SetFloat(
    AbilityRuntime_IDispatch_Variant* variant, float value);

/**
 * @brief Set double value to variant.
 *
 * @param variant The variant to set.
 * @param value The double value.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant is null.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_SetDouble(
    AbilityRuntime_IDispatch_Variant* variant, double value);

/**
 * @brief Set string value to variant.
 *
 * @param variant The variant to set.
 * @param value The string value (UTF-8).
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant or value is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OUT_OF_MEMORY} if memory allocation failed.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_SetString(
    AbilityRuntime_IDispatch_Variant* variant, const char* value);

/**
 * @brief Set enum value to variant.
 *
 * @param variant The variant to set.
 * @param value The enum value.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant is null.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_SetEnum(
    AbilityRuntime_IDispatch_Variant* variant, int32_t value);

/**
 * @brief Set interface pointer to variant.
    *
 * @param variant The variant to set.
 * @param value The IDispatch handle.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant is null.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_SetDispatch(
    AbilityRuntime_IDispatch_Variant* variant, AbilityRuntime_IDispatchHandle value);

/**
 * @brief Set struct pointer to variant.
 *
 * @param variant The variant to set.
 * @param value The struct handle.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant is null.
 * @ @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_SetStruct(
    AbilityRuntime_IDispatch_Variant* variant, AbilityRuntime_IDispatch_StructHandle value);

/**
 * @brief Get int8 value from variant.
 *
 * @param variant The variant to get value from.
 * @param value Pointer to receive int8 value.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant or value is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_TYPE_MISMATCH} if variant type is not int8.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_GetInt8(
    const AbilityRuntime_IDispatch_Variant* variant, int8_t* value);

/**
 * @ @brief Get int32 value from variant.
 *
 * @param variant The variant to get value from.
 * @param value Pointer to receive int32 value.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant or value is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_TYPE_MISMATCH} if variant type is not int32.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_GetInt32(
    const AbilityRuntime_IDispatch_Variant* variant, int32_t* value);

/**
 * @brief Get int64 value from variant.
 *
 * @param variant The variant to get value from.
 * @param value Pointer to receive int64 value.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant or value is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_TYPE_MISMATCH} if variant type is not int64.
 * @ @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_GetInt64(
    const AbilityRuntime_IDispatch_Variant* variant, int64_t* value);

/**
 * @brief Get float value from variant.
 *
 * @param variant The variant to get value from.
 * @ @param value Pointer to receive float value.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant or value is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_TYPE_MISMATCH} if variant type is not float.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_GetFloat(
    const AbilityRuntime_IDispatch_Variant* variant, float* value);

/**
 * @brief Get double value from variant.
 *
 * @param variant The variant to get value from.
 * @param value Pointer to receive double value.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant or value is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_TYPE_MISMATCH} if variant type is not double.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_GetDouble(
    const AbilityRuntime_IDispatch_Variant* variant, double* value);

/**
 * @brief Get string value from variant.
 *
 * @param variant The variant to get value from.
 * @param buffer A pointer to a buffer that receives string value.
 * @param bufferSize The length of buffer.
 * @param writeLength The string length actually written to buffer.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant, buffer, or writeLength is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_TYPE_MISMATCH} if variant type is not string.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_GetString(
    const AbilityRuntime_IDispatch_Variant* variant, char* buffer, uint32_t bufferSize, uint32_t* writeLength);

/**
 * @brief Get enum value from variant.
 *
 * @param variant The variant to get value from.
 * @param value Pointer to receive enum value.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant or value is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_TYPE_MISMATCH} if variant type is not enum.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_GetEnum(
    const AbilityRuntime_IDispatch_Variant* variant, int32_t* value);

/**
 * @brief Get interface pointer from variant.
 *
 * @param variant The variant to get value from.
 * @param value Pointer to receive IDispatch handle.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant or value is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_TYPE_MISMATCH} if variant type is not IDispatch.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_GetDispatch(
    const AbilityRuntime_IDispatch_Variant* variant, AbilityRuntime_IDispatchHandle* value);

/**
 * @brief Get struct pointer from variant.
 *
 * @param variant The variant to get value from.
 * @param value Pointer to receive struct handle.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant or value is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_TYPE_MISMATCH} if variant type is not struct.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_GetStruct(
    const AbilityRuntime_IDispatch_Variant* variant, AbilityRuntime_IDispatch_StructHandle* value);

/**
 * @brief Get variant type.
 *
 * @param variant The variant to get type from.
 * @return The variant type.
 * @since 24
 */
AbilityRuntime_IDispatch_Vt OH_AbilityRuntime_IDispatch_Variant_GetType(
    const AbilityRuntime_IDispatch_Variant* variant);

/**
 * @brief Clear variant and release resources.
 *
 * @param variant The variant to clear.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if variant is null.
 * @ @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_Clear(
    AbilityRuntime_IDispatch_Variant* variant);

/**
 * @brief Copy variant.
 *
 * @param src The source variant.
 * @param dest The destination variant.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if src or dest is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OUT_OF_MEMORY} if memory allocation failed.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Variant_Copy(
    const AbilityRuntime_IDispatch_Variant* src, AbilityRuntime_IDispatch_Variant* dest);

/**
 * @brief Create IDispatch instance from IPC remote proxy.
 *
 * @param ipcRemoteProxy The IPC remote proxy handle obtained from connectExtension.
 * @param dispatch A pointer to receive IDispatch handle.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if ipcRemoteProxy or dispatch is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_IPC_FAILED} if IPC connection failed.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_CoCreateInstance(
    void* ipcRemoteProxy, AbilityRuntime_IDispatchHandle* dispatch);

/**
 * @brief Release IPC remote proxy handle.
 *
 * @param ipcRemoteProxy The IPC remote proxy handle to release.
 * @since 24
 */
void OH_AbilityRuntime_IDispatch_ReleaseExtensionProxy(void* ipcRemoteProxy);

/**
 * @brief Get DispIDs of method/property names.
 *
 * @param dispatch The IDispatch handle.
 * @param rgszNames Array of method/property names.
 * @param cNames Number of names in array.
 * @param rgDispId Array to receive DispIDs.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if dispatch, rgszNames, or rgDispId is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if any name not found.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_GetIDsOfNames(
    AbilityRuntime_IDispatchHandle dispatch, const char** rgszNames, uint32_t cNames, uint32_t* rgDispId);

/**
 * @brief Invoke method or access property.
 *
 * @param dispatch The IDispatch handle.
 * @param dispId The DispID (prioritized) or 0.
 * @param lpszMemberName The method/property name (used when dispId is 0).
 * @param cArgs Number of arguments.
 * @param pArgs Array of argument variants.
 * @param pVarResult Pointer to receive result variant.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if dispatch is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if method not found.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_TYPE_MISMATCH} if parameter type mismatch.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_IPC_FAILED} if IPC communication failed.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_Invoke(
    AbilityRuntime_IDispatchHandle dispatch, uint32_t dispId, const char* lpszMemberName,
    uint32_t cArgs, AbilityRuntime_IDispatch_Variant* pArgs, AbilityRuntime_IDispatch_Variant* pVarResult);

/**
 * @brief Get type information.
 *
 * @param dispatch The IDispatch handle.
 * @param ppTypeInfo Pointer to receive type info handle.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if dispatch or ppTypeInfo is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_IPC_FAILED} if IPC communication failed.
 * @since
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_GetTypeInfo(
    AbilityRuntime_IDispatchHandle dispatch, AbilityRuntime_IDispatch_TypeInfoHandle* ppTypeInfo);

/**
 * @brief Release IDispatch instance.
 *
 * @param dispatch The IDispatch handle to release.
 * @since 24
 */
void OH_AbilityRuntime_IDispatch_Release(AbilityRuntime_IDispatchHandle dispatch);

/**
 * @brief Create struct instance.
 *
 * @param dispatch The IDispatch handle (used to get metadata).
 * @param structName The struct name from tlb.json.
 * @param pStruct Pointer to receive struct handle.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if dispatch, structName, or pStruct is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if struct not found in metadata.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OUT_OF_MEMORY} if memory allocation failed.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_CreateStruct(
    AbilityRuntime_IDispatchHandle dispatch, const char* structName, AbilityRuntime_IDispatch_StructHandle* pStruct);

/**
 * @brief Set struct field value.
 *
 * @param pStruct The struct handle.
 * @param fieldName The field name.
 * @param pValue Pointer to field value variant.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if pStruct, fieldName, or pValue is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if field not found in struct.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_TYPE_MISMATCH} if field type mismatch.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_SetStructField(
    AbilityRuntime_IDispatch_StructHandle pStruct, const char* fieldName, const AbilityRuntime_IDispatch_Variant* pValue);

/**
 * @brief Get struct field value.
 *
 * @param pStruct The struct handle.
 * @param fieldName The field name.
 * @param pValue Pointer to receive field value variant.
 * @return The error code.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK} if operation is successful.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID} if pStruct, fieldName, or pValue is null.
 *         {@link ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND} if field not found in struct.
 * @since 24
 */
AbilityRuntime_IDispatch_ErrorCode OH_AbilityRuntime_IDispatch_GetStructField(
    AbilityRuntime_IDispatch_StructHandle pStruct, const char* fieldName, AbilityRuntime_IDispatch_Variant* pValue);

/**
 * @brief Release struct instance.
 *
 * @param pStruct The struct handle to release.
 * @since 24
 */
void OH_AbilityRuntime_IDispatch_ReleaseStruct(AbilityRuntime_IDispatch_StructHandle pStruct);

#ifdef __cplusplus
}
#endif

/** @} */
#endif // ABILITY_RUNTIME_IDISPATCH_H
