#include <stdio.h>
#include <node_api.h>
#include <assert.h>
#include <stdlib.h>
#include "../../js-native-api/common.h"

#define BUFFER_SIZE 4

int wrappedNativeData;
JSVM_Ref ref;
void WrapFinalizer(JSVM_Env env, void* data, void* hint) {
  uint32_t count;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_ReferenceUnref(env, ref, &count));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, ref));
}

void BufferFinalizer(JSVM_Env env, void* data, void* hint) {
  free(hint);
}

JSVM_Value Test(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value argv[1];
  JSVM_Value result;
  void* bufferData = malloc(BUFFER_SIZE);

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));
  NODE_API_CALL(env,
      OH_JSVM_CreateExternal_buffer(
          env, BUFFER_SIZE, bufferData, BufferFinalizer, bufferData, &result));
  NODE_API_CALL(env, OH_JSVM_CreateReference(env, result, 1, &ref));
  NODE_API_CALL(env,
      OH_JSVM_Wrap(
          env, argv[0], (void*) &wrappedNativeData, WrapFinalizer, NULL, NULL));
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
