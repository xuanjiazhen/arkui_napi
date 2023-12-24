#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"
#include "myobject.h"

extern size_t finalize_count;

static JSVM_Value CreateObject(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env,
      OH_JSVM_GetCbInfo(env, info, &argc, args, nullptr, nullptr));

  JSVM_Value instance;
  NODE_API_CALL(env, MyObject::NewInstance(env, args[0], &instance));

  return instance;
}

static JSVM_Value Add(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  NODE_API_CALL(env,
      OH_JSVM_GetCbInfo(env, info, &argc, args, nullptr, nullptr));

  MyObject* obj1;
  NODE_API_CALL(env,
      OH_JSVM_Unwrap(env, args[0], reinterpret_cast<void**>(&obj1)));

  MyObject* obj2;
  NODE_API_CALL(env,
      OH_JSVM_Unwrap(env, args[1], reinterpret_cast<void**>(&obj2)));

  JSVM_Value sum;
  NODE_API_CALL(env, OH_JSVM_CreateDouble(env, obj1->Val() + obj2->Val(), &sum));

  return sum;
}

static JSVM_Value FinalizeCount(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value return_value;
  NODE_API_CALL(env, OH_JSVM_CreateUint32(env, finalize_count, &return_value));
  return return_value;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  MyObject::Init(env);

  JSVM_PropertyDescriptor desc[] = {
    DECLARE_NODE_API_PROPERTY("createObject", CreateObject),
    DECLARE_NODE_API_PROPERTY("add", Add),
    DECLARE_NODE_API_PROPERTY("finalizeCount", FinalizeCount),
  };

  NODE_API_CALL(env,
      OH_JSVM_DefineProperties(env, exports, sizeof(desc) / sizeof(*desc), desc));

  return exports;
}
EXTERN_C_END
