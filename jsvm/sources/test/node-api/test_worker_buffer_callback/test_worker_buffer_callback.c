#include <stdio.h>
#include <stdlib.h>
#include <node_api.h>
#include <assert.h>
#include "../../js-native-api/common.h"

uint32_t free_call_count = 0;

JSVM_Value GetFreeCallCount(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value value;
  NODE_API_CALL(env, OH_JSVM_CreateUint32(env, free_call_count, &value));
  return value;
}

static void finalize_cb(JSVM_Env env, void* finalize_data, void* hint) {
  free(finalize_data);
  free_call_count++;
}

JSVM_MODULE_INIT() {
  JSVM_PropertyDescriptor properties[] = {
    DECLARE_NODE_API_PROPERTY("getFreeCallCount", GetFreeCallCount)
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(properties) / sizeof(*properties), properties));

  // This is a slight variation on the non-N-API test: We create an ArrayBuffer
  // rather than a Node.js Buffer, since testing the latter would only test
  // the same code paths and not the ones specific to N-API.
  JSVM_Value buffer;

  char* data = malloc(sizeof(char));

  NODE_API_CALL(env, OH_JSVM_CreateExternalArraybuffer(
      env,
      data,
      sizeof(char),
      finalize_cb,
      NULL,
      &buffer));

  NODE_API_CALL(env, OH_JSVM_SetNamedProperty(env, exports, "buffer", buffer));

  return exports;
}
