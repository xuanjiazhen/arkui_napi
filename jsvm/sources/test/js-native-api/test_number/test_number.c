#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"
#include "test_null.h"

static JSVM_Value Test(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_number,
      "Wrong type of arguments. Expects a number as first argument.");

  double input;
  NODE_API_CALL(env, OH_JSVM_GetValueDouble(env, args[0], &input));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateDouble(env, input, &output));

  return output;
}

static JSVM_Value TestUint32Truncation(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_number,
      "Wrong type of arguments. Expects a number as first argument.");

  uint32_t input;
  NODE_API_CALL(env, OH_JSVM_GetValueUint32(env, args[0], &input));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateUint32(env, input, &output));

  return output;
}

static JSVM_Value TestInt32Truncation(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_number,
      "Wrong type of arguments. Expects a number as first argument.");

  int32_t input;
  NODE_API_CALL(env, OH_JSVM_GetValueInt32(env, args[0], &input));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateInt32(env, input, &output));

  return output;
}

static JSVM_Value TestInt64Truncation(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_number,
      "Wrong type of arguments. Expects a number as first argument.");

  int64_t input;
  NODE_API_CALL(env, OH_JSVM_GetValueInt64(env, args[0], &input));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateInt64(env, input, &output));

  return output;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("Test", Test),
    DECLARE_NODE_API_PROPERTY("TestInt32Truncation", TestInt32Truncation),
    DECLARE_NODE_API_PROPERTY("TestUint32Truncation", TestUint32Truncation),
    DECLARE_NODE_API_PROPERTY("TestInt64Truncation", TestInt64Truncation),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  init_test_null(env, exports);

  return exports;
}
EXTERN_C_END
