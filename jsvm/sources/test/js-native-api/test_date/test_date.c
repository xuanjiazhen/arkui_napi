#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"

static JSVM_Value createDate(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_number,
      "Wrong type of arguments. Expects a number as first argument.");

  double time;
  NODE_API_CALL(env, OH_JSVM_GetValueDouble(env, args[0], &time));

  JSVM_Value date;
  NODE_API_CALL(env, OH_JSVM_CreateDate(env, time, &date));

  return date;
}

static JSVM_Value isDate(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value date, result;
  size_t argc = 1;
  bool is_date;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &date, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_IsDate(env, date, &is_date));
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, is_date, &result));

  return result;
}

static JSVM_Value getDateValue(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value date, result;
  size_t argc = 1;
  double value;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &date, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_GetDateValue(env, date, &value));
  NODE_API_CALL(env, OH_JSVM_CreateDouble(env, value, &result));

  return result;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("createDate", createDate),
    DECLARE_NODE_API_PROPERTY("isDate", isDate),
    DECLARE_NODE_API_PROPERTY("getDateValue", getDateValue),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  return exports;
}
EXTERN_C_END
