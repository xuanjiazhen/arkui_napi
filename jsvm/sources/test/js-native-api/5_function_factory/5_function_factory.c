#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"

static JSVM_Value MyFunction(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value str;
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(env, "hello world", -1, &str));
  return str;
}

static JSVM_Value CreateFunction(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value fn;
  NODE_API_CALL(env,
    OH_JSVM_CreateFunction(env, "theFunction", -1, MyFunction, NULL, &fn));
  return fn;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  NODE_API_CALL(env,
      OH_JSVM_CreateFunction(env, "exports", -1, CreateFunction, NULL, &exports));
  return exports;
}
EXTERN_C_END
