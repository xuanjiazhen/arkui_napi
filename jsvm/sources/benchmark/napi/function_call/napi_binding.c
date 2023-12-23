#include <assert.h>
#include <node_api.h>

static int32_t increment = 0;

static JSVM_Value Hello(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  JSVM_Status status = OH_JSVM_CreateInt32(env, increment++, &result);
  assert(status == JSVM_OK);
  return result;
}

JSVM_MODULE_INIT() {
  JSVM_Value hello;
  JSVM_Status status =
      OH_JSVM_CreateFunction(env,
                           "hello",
                           JSVM_AUTO_LENGTH,
                           Hello,
                           NULL,
                           &hello);
  assert(status == JSVM_OK);
  status = OH_JSVM_SetNamedProperty(env, exports, "hello", hello);
  assert(status == JSVM_OK);
  return exports;
}
