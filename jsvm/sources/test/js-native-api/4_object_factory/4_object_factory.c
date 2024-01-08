#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"

static JSVM_Value CreateObject(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value obj;
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &obj));

  NODE_API_CALL(env, OH_JSVM_SetNamedProperty(env, obj, "msg", args[0]));

  return obj;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  NODE_API_CALL(env,
      OH_JSVM_CreateFunction(env, "exports", -1, CreateObject, NULL, &exports));
  return exports;
}
EXTERN_C_END
