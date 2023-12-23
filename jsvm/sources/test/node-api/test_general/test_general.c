#define JSVM_VERSION 9
// we define NODE_API_NO_EXTERNAL_BUFFERS_ALLOWED here to validate that it can
// be used as a form of test itself. It is
// not related to any of the other tests
// defined in the file
#define NODE_API_NO_EXTERNAL_BUFFERS_ALLOWED
#include <node_api.h>
#include <stdlib.h>
#include "../../js-native-api/common.h"

static JSVM_Value testGetNodeVersion(JSVM_Env env, JSVM_CallbackInfo info) {
  const jsvm_node_version* node_version;
  JSVM_Value result, major, minor, patch, release;
  NODE_API_CALL(env, jsvm_get_node_version(env, &node_version));
  NODE_API_CALL(env, OH_JSVM_CreateUint32(env, node_version->major, &major));
  NODE_API_CALL(env, OH_JSVM_CreateUint32(env, node_version->minor, &minor));
  NODE_API_CALL(env, OH_JSVM_CreateUint32(env, node_version->patch, &patch));
  NODE_API_CALL(env,
      OH_JSVM_CreateStringUtf8(
          env, node_version->release, JSVM_AUTO_LENGTH, &release));
  NODE_API_CALL(env, OH_JSVM_CreateArrayWithLength(env, 4, &result));
  NODE_API_CALL(env, OH_JSVM_SetElement(env, result, 0, major));
  NODE_API_CALL(env, OH_JSVM_SetElement(env, result, 1, minor));
  NODE_API_CALL(env, OH_JSVM_SetElement(env, result, 2, patch));
  NODE_API_CALL(env, OH_JSVM_SetElement(env, result, 3, release));
  return result;
}

static JSVM_Value GetFilename(JSVM_Env env, JSVM_CallbackInfo info) {
  const char* filename;
  JSVM_Value result;

  NODE_API_CALL(env, node_api_get_module_file_name(env, &filename));
  NODE_API_CALL(env,
      OH_JSVM_CreateStringUtf8(env, filename, JSVM_AUTO_LENGTH, &result));

  return result;
}

static JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("testGetNodeVersion", testGetNodeVersion),
    DECLARE_NODE_API_GETTER("filename", GetFilename),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  return exports;
}

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
