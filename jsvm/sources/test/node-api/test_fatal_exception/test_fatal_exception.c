#include <node_api.h>
#include "../../js-native-api/common.h"

static JSVM_Value Test(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value err;
  size_t argc = 1;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &err, NULL, NULL));

  NODE_API_CALL(env, jsvm_fatal_exception(env, err));

  return NULL;
}

static JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor properties[] = {
    DECLARE_NODE_API_PROPERTY("Test", Test),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(properties) / sizeof(*properties), properties));

  return exports;
}

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
