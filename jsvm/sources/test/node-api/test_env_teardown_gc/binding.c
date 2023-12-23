#include <stdlib.h>
#include <node_api.h>
#include "../../js-native-api/common.h"

static void MyObject_fini(JSVM_Env env, void* data, void* hint) {
  JSVM_Ref* ref = data;
  JSVM_Value global;
  JSVM_Value cleanup;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetGlobal(env, &global));
  NODE_API_CALL_RETURN_VOID(
      env, OH_JSVM_GetNamedProperty(env, global, "cleanup", &cleanup));
  JSVM_Status status = OH_JSVM_CallFunction(env, global, cleanup, 0, NULL, NULL);
  // We may not be allowed to call into JS, in which case a pending exception
  // will be returned.
  NODE_API_ASSERT_RETURN_VOID(env,
      status == JSVM_OK || status == JSVM_PENDING_EXCEPTION,
      "Unexpected status for OH_JSVM_CallFunction");
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, *ref));
  free(ref);
}

static JSVM_Value MyObject(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value js_this;
  JSVM_Ref* ref = malloc(sizeof(*ref));
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, NULL, NULL, &js_this, NULL));
  NODE_API_CALL(env, OH_JSVM_Wrap(env, js_this, ref, MyObject_fini, NULL, ref));
  return NULL;
}

JSVM_MODULE_INIT() {
  JSVM_Value ctor;
  NODE_API_CALL(
      env, OH_JSVM_DefineClass(
          env, "MyObject", JSVM_AUTO_LENGTH, MyObject, NULL, 0, NULL, &ctor));
  NODE_API_CALL(env, OH_JSVM_SetNamedProperty(env, exports, "MyObject",  ctor));
  return exports;
}
