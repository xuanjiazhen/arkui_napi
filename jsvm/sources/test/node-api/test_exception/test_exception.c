#include <node_api.h>
#include "../../js-native-api/common.h"

static void finalizer(JSVM_Env env, void *data, void *hint) {
  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_ThrowError(env, NULL, "Error during Finalize"));
}

static char buffer_data[12];

static JSVM_Value createExternalBuffer(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value buffer;
  NODE_API_CALL(env, OH_JSVM_CreateExternal_buffer(env, sizeof(buffer_data),
      buffer_data, finalizer, NULL, &buffer));
  return buffer;
}

static JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("createExternalBuffer", createExternalBuffer),
  };
  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));
  return exports;
}

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
