#include <node_api.h>
#include "../../js-native-api/common.h"
#include <string.h>

static JSVM_Value Method(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value world;
  const char* str = "world";
  size_t str_len = strlen(str);
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(env, str, str_len, &world));
  return world;
}

JSVM_MODULE_INIT() {
  JSVM_PropertyDescriptor desc = DECLARE_NODE_API_PROPERTY("hello", Method);
  NODE_API_CALL(env, OH_JSVM_DefineProperties(env, exports, 1, &desc));
  return exports;
}
