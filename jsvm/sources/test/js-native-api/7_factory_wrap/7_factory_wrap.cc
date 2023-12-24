#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"
#include "myobject.h"

JSVM_Value CreateObject(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env,
      OH_JSVM_GetCbInfo(env, info, &argc, args, nullptr, nullptr));

  JSVM_Value instance;
  NODE_API_CALL(env, MyObject::NewInstance(env, args[0], &instance));

  return instance;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  NODE_API_CALL(env, MyObject::Init(env));

  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_GETTER("finalizeCount", MyObject::GetFinalizeCount),
    DECLARE_NODE_API_PROPERTY("createObject", CreateObject),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  return exports;
}
EXTERN_C_END
