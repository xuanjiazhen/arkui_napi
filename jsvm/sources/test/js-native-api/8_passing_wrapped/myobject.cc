#include "myobject.h"
#include "../common.h"

size_t finalize_count = 0;

MyObject::MyObject() : env_(nullptr), wrapper_(nullptr) {}

MyObject::~MyObject() {
  finalize_count++;
  OH_JSVM_DeleteReference(env_, wrapper_);
}

void MyObject::Destructor(
  JSVM_Env env, void* nativeObject, void* /*finalize_hint*/) {
  MyObject* obj = static_cast<MyObject*>(nativeObject);
  delete obj;
}

JSVM_Ref MyObject::constructor;

JSVM_Status MyObject::Init(JSVM_Env env) {
  JSVM_Status status;

  JSVM_Value cons;
  status = OH_JSVM_DefineClass(
      env, "MyObject", -1, New, nullptr, 0, nullptr, &cons);
  if (status != JSVM_OK) return status;

  status = OH_JSVM_CreateReference(env, cons, 1, &constructor);
  if (status != JSVM_OK) return status;

  return JSVM_OK;
}

JSVM_Value MyObject::New(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  JSVM_Value _this;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, &_this, nullptr));

  MyObject* obj = new MyObject();

  JSVM_ValueType valuetype;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype));

  if (valuetype == JSVM_UNDEFINED) {
    obj->val_ = 0;
  } else {
    NODE_API_CALL(env, OH_JSVM_GetValueDouble(env, args[0], &obj->val_));
  }

  obj->env_ = env;

  // The below call to OH_JSVM_Wrap() must request a reference to the wrapped
  // object via the out-parameter, because this ensures that we test the code
  // path that deals with a reference that is destroyed from its own finalizer.
  NODE_API_CALL(env,
      OH_JSVM_Wrap(env, _this, obj, MyObject::Destructor,
          nullptr /* finalize_hint */, &obj->wrapper_));

  return _this;
}

JSVM_Status MyObject::NewInstance(JSVM_Env env,
                                  JSVM_Value arg,
                                  JSVM_Value* instance) {
  JSVM_Status status;

  const int argc = 1;
  JSVM_Value argv[argc] = {arg};

  JSVM_Value cons;
  status = OH_JSVM_GetReferenceValue(env, constructor, &cons);
  if (status != JSVM_OK) return status;

  status = OH_JSVM_NewInstance(env, cons, argc, argv, instance);
  if (status != JSVM_OK) return status;

  return JSVM_OK;
}
