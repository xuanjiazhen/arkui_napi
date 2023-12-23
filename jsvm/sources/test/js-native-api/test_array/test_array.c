#include <jsvm.h>
#include <string.h>
#include "../common.h"
#include "../entry_point.h"

static JSVM_Value TestGetElement(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 2, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_object,
      "Wrong type of arguments. Expects an array as first argument.");

  JSVM_ValueType valuetype1;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &valuetype1));

  NODE_API_ASSERT(env, valuetype1 == jsvm_number,
      "Wrong type of arguments. Expects an integer as second argument.");

  JSVM_Value array = args[0];
  int32_t index;
  NODE_API_CALL(env, OH_JSVM_GetValueInt32(env, args[1], &index));

  NODE_API_ASSERT(env, index >= 0, "Invalid index. Expects a positive integer.");

  bool isarray;
  NODE_API_CALL(env, OH_JSVM_IsArray(env, array, &isarray));

  if (!isarray) {
    return NULL;
  }

  uint32_t length;
  NODE_API_CALL(env, OH_JSVM_GetArrayLength(env, array, &length));

  NODE_API_ASSERT(env, ((uint32_t)index < length), "Index out of bounds!");

  JSVM_Value ret;
  NODE_API_CALL(env, OH_JSVM_GetElement(env, array, index, &ret));

  return ret;
}

static JSVM_Value TestHasElement(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 2, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_object,
      "Wrong type of arguments. Expects an array as first argument.");

  JSVM_ValueType valuetype1;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &valuetype1));

  NODE_API_ASSERT(env, valuetype1 == jsvm_number,
      "Wrong type of arguments. Expects an integer as second argument.");

  JSVM_Value array = args[0];
  int32_t index;
  NODE_API_CALL(env, OH_JSVM_GetValueInt32(env, args[1], &index));

  bool isarray;
  NODE_API_CALL(env, OH_JSVM_IsArray(env, array, &isarray));

  if (!isarray) {
    return NULL;
  }

  bool has_element;
  NODE_API_CALL(env, OH_JSVM_HasElement(env, array, index, &has_element));

  JSVM_Value ret;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, has_element, &ret));

  return ret;
}

static JSVM_Value TestDeleteElement(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));
  NODE_API_ASSERT(env, argc == 2, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));
  NODE_API_ASSERT(env, valuetype0 == jsvm_object,
      "Wrong type of arguments. Expects an array as first argument.");

  JSVM_ValueType valuetype1;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &valuetype1));
  NODE_API_ASSERT(env, valuetype1 == jsvm_number,
      "Wrong type of arguments. Expects an integer as second argument.");

  JSVM_Value array = args[0];
  int32_t index;
  bool result;
  JSVM_Value ret;

  NODE_API_CALL(env, OH_JSVM_GetValueInt32(env, args[1], &index));
  NODE_API_CALL(env, OH_JSVM_IsArray(env, array, &result));

  if (!result) {
    return NULL;
  }

  NODE_API_CALL(env, OH_JSVM_DeleteElement(env, array, index, &result));
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, result, &ret));

  return ret;
}

static JSVM_Value New(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_object,
      "Wrong type of arguments. Expects an array as first argument.");

  JSVM_Value ret;
  NODE_API_CALL(env, OH_JSVM_CreateArray(env, &ret));

  uint32_t i, length;
  NODE_API_CALL(env, OH_JSVM_GetArrayLength(env, args[0], &length));

  for (i = 0; i < length; i++) {
    JSVM_Value e;
    NODE_API_CALL(env, OH_JSVM_GetElement(env, args[0], i, &e));
    NODE_API_CALL(env, OH_JSVM_SetElement(env, ret, i, e));
  }

  return ret;
}

static JSVM_Value NewWithLength(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_number,
      "Wrong type of arguments. Expects an integer the first argument.");

  int32_t array_length;
  NODE_API_CALL(env, OH_JSVM_GetValueInt32(env, args[0], &array_length));

  JSVM_Value ret;
  NODE_API_CALL(env, OH_JSVM_CreateArrayWithLength(env, array_length, &ret));

  return ret;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("TestGetElement", TestGetElement),
    DECLARE_NODE_API_PROPERTY("TestHasElement", TestHasElement),
    DECLARE_NODE_API_PROPERTY("TestDeleteElement", TestDeleteElement),
    DECLARE_NODE_API_PROPERTY("New", New),
    DECLARE_NODE_API_PROPERTY("NewWithLength", NewWithLength),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  return exports;
}
EXTERN_C_END
