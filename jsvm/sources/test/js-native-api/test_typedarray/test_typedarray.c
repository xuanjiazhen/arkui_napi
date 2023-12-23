#include <jsvm.h>
#include <stdlib.h>
#include <string.h>
#include "../common.h"
#include "../entry_point.h"

static JSVM_Value Multiply(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc == 2, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_object,
      "Wrong type of arguments. Expects a typed array as first argument.");

  JSVM_Value input_array = args[0];
  bool is_typedarray;
  NODE_API_CALL(env, OH_JSVM_IsTypedarray(env, input_array, &is_typedarray));

  NODE_API_ASSERT(env, is_typedarray,
      "Wrong type of arguments. Expects a typed array as first argument.");

  JSVM_ValueType valuetype1;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &valuetype1));

  NODE_API_ASSERT(env, valuetype1 == jsvm_number,
      "Wrong type of arguments. Expects a number as second argument.");

  double multiplier;
  NODE_API_CALL(env, OH_JSVM_GetValueDouble(env, args[1], &multiplier));

  JSVM_TypedarrayType type;
  JSVM_Value input_buffer;
  size_t byte_offset;
  size_t i, length;
  NODE_API_CALL(env, OH_JSVM_GetTypedarrayInfo(
      env, input_array, &type, &length, NULL, &input_buffer, &byte_offset));

  void* data;
  size_t byte_length;
  NODE_API_CALL(env, OH_JSVM_GetArraybufferInfo(
      env, input_buffer, &data, &byte_length));

  JSVM_Value output_buffer;
  void* output_ptr = NULL;
  NODE_API_CALL(env, OH_JSVM_CreateArraybuffer(
      env, byte_length, &output_ptr, &output_buffer));

  JSVM_Value output_array;
  NODE_API_CALL(env, OH_JSVM_CreateTypedarray(
      env, type, length, output_buffer, byte_offset, &output_array));

  if (type == JSVM_UINT8_ARRAY) {
    uint8_t* input_bytes = (uint8_t*)(data) + byte_offset;
    uint8_t* output_bytes = (uint8_t*)(output_ptr);
    for (i = 0; i < length; i++) {
      output_bytes[i] = (uint8_t)(input_bytes[i] * multiplier);
    }
  } else if (type == JSVM_FLOAT64_ARRAY) {
    double* input_doubles = (double*)((uint8_t*)(data) + byte_offset);
    double* output_doubles = (double*)(output_ptr);
    for (i = 0; i < length; i++) {
      output_doubles[i] = input_doubles[i] * multiplier;
    }
  } else {
    OH_JSVM_ThrowError(env, NULL,
        "Typed array was of a type not expected by test.");
    return NULL;
  }

  return output_array;
}

static void FinalizeCallback(JSVM_Env env,
                             void* finalize_data,
                             void* finalize_hint)
{
  free(finalize_data);
}

static JSVM_Value External(JSVM_Env env, JSVM_CallbackInfo info) {
  const uint8_t nElem = 3;
  int8_t* externalData = malloc(nElem*sizeof(int8_t));
  externalData[0] = 0;
  externalData[1] = 1;
  externalData[2] = 2;

  JSVM_Value output_buffer;
  NODE_API_CALL(env, OH_JSVM_CreateExternalArraybuffer(
      env,
      externalData,
      nElem*sizeof(int8_t),
      FinalizeCallback,
      NULL,  // finalize_hint
      &output_buffer));

  JSVM_Value output_array;
  NODE_API_CALL(env, OH_JSVM_CreateTypedarray(env,
      JSVM_INT8_ARRAY,
      nElem,
      output_buffer,
      0,
      &output_array));

  return output_array;
}


static JSVM_Value NullArrayBuffer(JSVM_Env env, JSVM_CallbackInfo info) {
  static void* data = NULL;
  JSVM_Value arraybuffer;
  NODE_API_CALL(env,
      OH_JSVM_CreateExternalArraybuffer(env, data, 0, NULL, NULL, &arraybuffer));
  return arraybuffer;
}

static JSVM_Value CreateTypedArray(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 4;
  JSVM_Value args[4];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc == 2 || argc == 4, "Wrong number of arguments");

  JSVM_Value input_array = args[0];
  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, input_array, &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_object,
      "Wrong type of arguments. Expects a typed array as first argument.");

  bool is_typedarray;
  NODE_API_CALL(env, OH_JSVM_IsTypedarray(env, input_array, &is_typedarray));

  NODE_API_ASSERT(env, is_typedarray,
      "Wrong type of arguments. Expects a typed array as first argument.");

  JSVM_ValueType valuetype1;
  JSVM_Value input_buffer = args[1];
  NODE_API_CALL(env, OH_JSVM_Typeof(env, input_buffer, &valuetype1));

  NODE_API_ASSERT(env, valuetype1 == jsvm_object,
      "Wrong type of arguments. Expects an array buffer as second argument.");

  bool is_arraybuffer;
  NODE_API_CALL(env, OH_JSVM_IsArraybuffer(env, input_buffer, &is_arraybuffer));

  NODE_API_ASSERT(env, is_arraybuffer,
      "Wrong type of arguments. Expects an array buffer as second argument.");

  JSVM_TypedarrayType type;
  JSVM_Value in_array_buffer;
  size_t byte_offset;
  size_t length;
  NODE_API_CALL(env, OH_JSVM_GetTypedarrayInfo(
      env, input_array, &type, &length, NULL, &in_array_buffer, &byte_offset));

  if (argc == 4) {
    JSVM_ValueType valuetype2;
    NODE_API_CALL(env, OH_JSVM_Typeof(env, args[2], &valuetype2));

    NODE_API_ASSERT(env, valuetype2 == jsvm_number,
        "Wrong type of arguments. Expects a number as third argument.");

    uint32_t uint32_length;
    NODE_API_CALL(env, OH_JSVM_GetValueUint32(env, args[2], &uint32_length));
    length = uint32_length;

    JSVM_ValueType valuetype3;
    NODE_API_CALL(env, OH_JSVM_Typeof(env, args[3], &valuetype3));

    NODE_API_ASSERT(env, valuetype3 == jsvm_number,
        "Wrong type of arguments. Expects a number as third argument.");

    uint32_t uint32_byte_offset;
    NODE_API_CALL(env, OH_JSVM_GetValueUint32(env, args[3], &uint32_byte_offset));
    byte_offset = uint32_byte_offset;
  }

  JSVM_Value output_array;
  NODE_API_CALL(env, OH_JSVM_CreateTypedarray(
      env, type, length, input_buffer, byte_offset, &output_array));

  return output_array;
}

static JSVM_Value Detach(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));
  NODE_API_ASSERT(env, argc == 1, "Wrong number of arguments.");

  bool is_typedarray;
  NODE_API_CALL(env, OH_JSVM_IsTypedarray(env, args[0], &is_typedarray));
  NODE_API_ASSERT(
      env, is_typedarray,
      "Wrong type of arguments. Expects a typedarray as first argument.");

  JSVM_Value arraybuffer;
  NODE_API_CALL(env,
      OH_JSVM_GetTypedarrayInfo(
          env, args[0], NULL, NULL, NULL, &arraybuffer, NULL));
  NODE_API_CALL(env, OH_JSVM_DetachArraybuffer(env, arraybuffer));

  return NULL;
}

static JSVM_Value IsDetached(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));
  NODE_API_ASSERT(env, argc == 1, "Wrong number of arguments.");

  JSVM_Value array_buffer = args[0];
  bool is_arraybuffer;
  NODE_API_CALL(env, OH_JSVM_IsArraybuffer(env, array_buffer, &is_arraybuffer));
  NODE_API_ASSERT(env, is_arraybuffer,
      "Wrong type of arguments. Expects an array buffer as first argument.");

  bool is_detached;
  NODE_API_CALL(env,
      OH_JSVM_IsDetachedArraybuffer(env, array_buffer, &is_detached));

  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, is_detached, &result));

  return result;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("Multiply", Multiply),
    DECLARE_NODE_API_PROPERTY("External", External),
    DECLARE_NODE_API_PROPERTY("NullArrayBuffer", NullArrayBuffer),
    DECLARE_NODE_API_PROPERTY("CreateTypedArray", CreateTypedArray),
    DECLARE_NODE_API_PROPERTY("Detach", Detach),
    DECLARE_NODE_API_PROPERTY("IsDetached", IsDetached),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  return exports;
}
EXTERN_C_END
