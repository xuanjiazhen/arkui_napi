#include <jsvm.h>
#include <string.h>
#include "../common.h"
#include "../entry_point.h"

static JSVM_Value RunCallback(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc == 1,
      "Wrong number of arguments. Expects a single argument.");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));
  NODE_API_ASSERT(env, valuetype0 == jsvm_function,
      "Wrong type of arguments. Expects a function as first argument.");

  JSVM_ValueType valuetype1;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &valuetype1));
  NODE_API_ASSERT(env, valuetype1 == JSVM_UNDEFINED,
      "Additional arguments should be undefined.");

  JSVM_Value argv[1];
  const char* str = "hello world";
  size_t str_len = strlen(str);
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(env, str, str_len, argv));

  JSVM_Value global;
  NODE_API_CALL(env, OH_JSVM_GetGlobal(env, &global));

  JSVM_Value cb = args[0];
  NODE_API_CALL(env, OH_JSVM_CallFunction(env, global, cb, 1, argv, NULL));

  return NULL;
}

static JSVM_Value RunCallbackWithRecv(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value cb = args[0];
  JSVM_Value recv = args[1];
  NODE_API_CALL(env, OH_JSVM_CallFunction(env, recv, cb, 0, NULL, NULL));
  return NULL;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor desc[2] = {
    DECLARE_NODE_API_PROPERTY("RunCallback", RunCallback),
    DECLARE_NODE_API_PROPERTY("RunCallbackWithRecv", RunCallbackWithRecv),
  };
  NODE_API_CALL(env, OH_JSVM_DefineProperties(env, exports, 2, desc));
  return exports;
}
EXTERN_C_END
