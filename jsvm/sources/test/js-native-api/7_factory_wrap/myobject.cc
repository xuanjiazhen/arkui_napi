#include "myobject.h"
#include "../common.h"

static int finalize_count = 0;

MyObject::MyObject() : env_(nullptr), wrapper_(nullptr) {}

MyObject::~MyObject() { OH_JSVM_DeleteReference(env_, wrapper_); }

void MyObject::Destructor(JSVM_Env env,
                          void* nativeObject,
                          void* /*finalize_hint*/) {
  ++finalize_count;
  MyObject* obj = static_cast<MyObject*>(nativeObject);
  delete obj;
}

JSVM_Value MyObject::GetFinalizeCount(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_CreateInt32(env, finalize_count, &result));
  return result;
}

JSVM_Ref MyObject::constructor;

JSVM_Status MyObject::Init(JSVM_Env env) {
  JSVM_Status status;
  JSVM_PropertyDescriptor properties[] = {
    DECLARE_NODE_API_PROPERTY("plusOne", PlusOne),
  };

  JSVM_Value cons;
  status = OH_JSVM_DefineClass(
      env, "MyObject", -1, New, nullptr, 1, properties, &cons);
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

  JSVM_ValueType valuetype;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype));

  MyObject* obj = new MyObject();

  if (valuetype == JSVM_UNDEFINED) {
    obj->counter_ = 0;
  } else {
    NODE_API_CALL(env, OH_JSVM_GetValueUint32(env, args[0], &obj->counter_));
  }

  obj->env_ = env;
  NODE_API_CALL(env,
      OH_JSVM_Wrap(
          env, _this, obj, MyObject::Destructor, nullptr /* finalize_hint */,
          &obj->wrapper_));

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

JSVM_Value MyObject::PlusOne(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value _this;
  NODE_API_CALL(env,
      OH_JSVM_GetCbInfo(env, info, nullptr, nullptr, &_this, nullptr));

  MyObject* obj;
  NODE_API_CALL(env, OH_JSVM_Unwrap(env, _this, reinterpret_cast<void**>(&obj)));

  obj->counter_ += 1;

  JSVM_Value num;
  NODE_API_CALL(env, OH_JSVM_CreateUint32(env, obj->counter_, &num));

  return num;
}
