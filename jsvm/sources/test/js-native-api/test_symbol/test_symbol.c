#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"

static JSVM_Value New(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value description = NULL;
  if (argc >= 1) {
    JSVM_ValueType valuetype;
    NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype));

    NODE_API_ASSERT(env, valuetype == jsvm_string,
        "Wrong type of arguments. Expects a string.");

    description = args[0];
  }

  JSVM_Value symbol;
  NODE_API_CALL(env, OH_JSVM_CreateSymbol(env, description, &symbol));

  return symbol;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor properties[] = {
    DECLARE_NODE_API_PROPERTY("New", New),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(properties) / sizeof(*properties), properties));

  return exports;
}
EXTERN_C_END
