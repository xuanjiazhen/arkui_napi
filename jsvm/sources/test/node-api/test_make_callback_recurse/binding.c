#include <node_api.h>
#include "../../js-native-api/common.h"

static JSVM_Value MakeCallback(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  // NOLINTNEXTLINE (readability/null_usage)
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value recv = args[0];
  JSVM_Value func = args[1];

  JSVM_Status status = jsvm_make_callback(env, NULL /* async_context */,
    recv, func, 0 /* argc */, NULL /* argv */, NULL /* result */);

  bool isExceptionPending;
  NODE_API_CALL(env, OH_JSVM_IsExceptionPending(env, &isExceptionPending));
  if (isExceptionPending && !(status == JSVM_PENDING_EXCEPTION)) {
    // if there is an exception pending we don't expect any
    // other error
    JSVM_Value pending_error;
    status = OH_JSVM_GetAndClearLastException(env, &pending_error);
    NODE_API_CALL(env,
      OH_JSVM_ThrowError((env),
                        NULL,
                        "error when only pending exception expected"));
  }

  return recv;
}

static JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_Value fn;
  NODE_API_CALL(env, OH_JSVM_CreateFunction(
      // NOLINTNEXTLINE (readability/null_usage)
      env, NULL, JSVM_AUTO_LENGTH, MakeCallback, NULL, &fn));
  NODE_API_CALL(env, OH_JSVM_SetNamedProperty(env, exports, "makeCallback", fn));
  return exports;
}

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
