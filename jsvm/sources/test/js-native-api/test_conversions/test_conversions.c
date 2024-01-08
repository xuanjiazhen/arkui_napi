#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"
#include "test_null.h"

static JSVM_Value AsBool(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  bool value;
  NODE_API_CALL(env, OH_JSVM_GetValueBool(env, args[0], &value));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, value, &output));

  return output;
}

static JSVM_Value AsInt32(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  int32_t value;
  NODE_API_CALL(env, OH_JSVM_GetValueInt32(env, args[0], &value));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateInt32(env, value, &output));

  return output;
}

static JSVM_Value AsUInt32(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  uint32_t value;
  NODE_API_CALL(env, OH_JSVM_GetValueUint32(env, args[0], &value));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateUint32(env, value, &output));

  return output;
}

static JSVM_Value AsInt64(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  int64_t value;
  NODE_API_CALL(env, OH_JSVM_GetValueInt64(env, args[0], &value));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateInt64(env, (double)value, &output));

  return output;
}

static JSVM_Value AsDouble(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  double value;
  NODE_API_CALL(env, OH_JSVM_GetValueDouble(env, args[0], &value));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateDouble(env, value, &output));

  return output;
}

static JSVM_Value AsString(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  char value[100];
  NODE_API_CALL(env,
    OH_JSVM_GetValueStringUtf8(env, args[0], value, sizeof(value), NULL));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
      env, value, JSVM_AUTO_LENGTH, &output));

  return output;
}

static JSVM_Value ToBool(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CoerceToBool(env, args[0], &output));

  return output;
}

static JSVM_Value ToNumber(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CoerceToNumber(env, args[0], &output));

  return output;
}

static JSVM_Value ToObject(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CoerceToObject(env, args[0], &output));

  return output;
}

static JSVM_Value ToString(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CoerceToString(env, args[0], &output));

  return output;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("asBool", AsBool),
    DECLARE_NODE_API_PROPERTY("asInt32", AsInt32),
    DECLARE_NODE_API_PROPERTY("asUInt32", AsUInt32),
    DECLARE_NODE_API_PROPERTY("asInt64", AsInt64),
    DECLARE_NODE_API_PROPERTY("asDouble", AsDouble),
    DECLARE_NODE_API_PROPERTY("asString", AsString),
    DECLARE_NODE_API_PROPERTY("toBool", ToBool),
    DECLARE_NODE_API_PROPERTY("toNumber", ToNumber),
    DECLARE_NODE_API_PROPERTY("toObject", ToObject),
    DECLARE_NODE_API_PROPERTY("toString", ToString),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  init_test_null(env, exports);

  return exports;
}
EXTERN_C_END
