#include <stdio.h>
#include <node_api.h>
#include <assert.h>
#include "../../js-native-api/common.h"

JSVM_Value Test(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value recv;
  JSVM_Value argv[1];
  JSVM_Status status;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, argv, &recv, NULL));
  NODE_API_ASSERT(env, argc >= 1, "Not enough arguments, expected 1.");

  JSVM_ValueType t;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, argv[0], &t));
  NODE_API_ASSERT(env, t == jsvm_function,
      "Wrong first argument, function expected.");

  status = OH_JSVM_CallFunction(env, recv, argv[0], 0, NULL, NULL);
  assert(status == JSVM_PENDING_EXCEPTION);

  return NULL;
}

JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor properties[] = {
    DECLARE_NODE_API_PROPERTY("Test", Test)
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(properties) / sizeof(*properties), properties));

  return exports;
}

// Do not start using JSVM_MODULE_INIT() here, so that we can test
// compatibility of Workers with JSVM_MODULE().
JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
