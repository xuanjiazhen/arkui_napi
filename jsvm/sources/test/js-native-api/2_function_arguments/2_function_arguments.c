#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"

static JSVM_Value Add(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 2, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  JSVM_ValueType valuetype1;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &valuetype1));

  NODE_API_ASSERT(env, valuetype0 == jsvm_number && valuetype1 == jsvm_number,
      "Wrong argument type. Numbers expected.");

  double value0;
  NODE_API_CALL(env, OH_JSVM_GetValueDouble(env, args[0], &value0));

  double value1;
  NODE_API_CALL(env, OH_JSVM_GetValueDouble(env, args[1], &value1));

  JSVM_Value sum;
  NODE_API_CALL(env, OH_JSVM_CreateDouble(env, value0 + value1, &sum));

  return sum;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor desc = DECLARE_NODE_API_PROPERTY("add", Add);
  NODE_API_CALL(env, OH_JSVM_DefineProperties(env, exports, 1, &desc));
  return exports;
}
EXTERN_C_END
