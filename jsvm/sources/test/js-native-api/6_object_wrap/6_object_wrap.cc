#include "../common.h"
#include "../entry_point.h"
#include "assert.h"
#include "myobject.h"

JSVM_Ref MyObject::constructor;

MyObject::MyObject(double value)
    : value_(value), env_(nullptr), wrapper_(nullptr) {}

MyObject::~MyObject() { OH_JSVM_DeleteReference(env_, wrapper_); }

void MyObject::Destructor(
  JSVM_Env env, void* nativeObject, void* /*finalize_hint*/) {
  MyObject* obj = static_cast<MyObject*>(nativeObject);
  delete obj;
}

void MyObject::Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor properties[] = {
    { "value", nullptr, nullptr, GetValue, SetValue, 0, JSVM_DEFAULT, 0 },
    { "valueReadonly", nullptr, nullptr, GetValue, nullptr, 0, JSVM_DEFAULT,
      0 },
    DECLARE_NODE_API_PROPERTY("plusOne", PlusOne),
    DECLARE_NODE_API_PROPERTY("multiply", Multiply),
  };

  JSVM_Value cons;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DefineClass(
      env, "MyObject", -1, New, nullptr,
      sizeof(properties) / sizeof(JSVM_PropertyDescriptor),
      properties, &cons));

  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_CreateReference(env, cons, 1, &constructor));

  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_SetNamedProperty(env, exports, "MyObject", cons));
}

JSVM_Value MyObject::New(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value new_target;
  NODE_API_CALL(env, OH_JSVM_GetNewTarget(env, info, &new_target));
  bool is_constructor = (new_target != nullptr);

  size_t argc = 1;
  JSVM_Value args[1];
  JSVM_Value _this;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, &_this, nullptr));

  if (is_constructor) {
    // Invoked as constructor: `new MyObject(...)`
    double value = 0;

    JSVM_ValueType valuetype;
    NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype));

    if (valuetype != JSVM_UNDEFINED) {
      NODE_API_CALL(env, OH_JSVM_GetValueDouble(env, args[0], &value));
    }

    MyObject* obj = new MyObject(value);

    obj->env_ = env;
    NODE_API_CALL(env,
        OH_JSVM_Wrap(env, _this, obj, MyObject::Destructor,
            nullptr /* finalize_hint */, &obj->wrapper_));

    return _this;
  }

  // Invoked as plain function `MyObject(...)`, turn into construct call.
  argc = 1;
  JSVM_Value argv[1] = {args[0]};

  JSVM_Value cons;
  NODE_API_CALL(env, OH_JSVM_GetReferenceValue(env, constructor, &cons));

  JSVM_Value instance;
  NODE_API_CALL(env, OH_JSVM_NewInstance(env, cons, argc, argv, &instance));

  return instance;
}

JSVM_Value MyObject::GetValue(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value _this;
  NODE_API_CALL(env,
      OH_JSVM_GetCbInfo(env, info, nullptr, nullptr, &_this, nullptr));

  MyObject* obj;
  NODE_API_CALL(env, OH_JSVM_Unwrap(env, _this, reinterpret_cast<void**>(&obj)));

  JSVM_Value num;
  NODE_API_CALL(env, OH_JSVM_CreateDouble(env, obj->value_, &num));

  return num;
}

JSVM_Value MyObject::SetValue(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  JSVM_Value _this;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, &_this, nullptr));

  MyObject* obj;
  NODE_API_CALL(env, OH_JSVM_Unwrap(env, _this, reinterpret_cast<void**>(&obj)));

  NODE_API_CALL(env, OH_JSVM_GetValueDouble(env, args[0], &obj->value_));

  return nullptr;
}

JSVM_Value MyObject::PlusOne(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value _this;
  NODE_API_CALL(env,
      OH_JSVM_GetCbInfo(env, info, nullptr, nullptr, &_this, nullptr));

  MyObject* obj;
  NODE_API_CALL(env, OH_JSVM_Unwrap(env, _this, reinterpret_cast<void**>(&obj)));

  obj->value_ += 1;

  JSVM_Value num;
  NODE_API_CALL(env, OH_JSVM_CreateDouble(env, obj->value_, &num));

  return num;
}

JSVM_Value MyObject::Multiply(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  JSVM_Value _this;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, &_this, nullptr));

  double multiple = 1;
  if (argc >= 1) {
    NODE_API_CALL(env, OH_JSVM_GetValueDouble(env, args[0], &multiple));
  }

  MyObject* obj;
  NODE_API_CALL(env, OH_JSVM_Unwrap(env, _this, reinterpret_cast<void**>(&obj)));

  JSVM_Value cons;
  NODE_API_CALL(env, OH_JSVM_GetReferenceValue(env, constructor, &cons));

  const int kArgCount = 1;
  JSVM_Value argv[kArgCount];
  NODE_API_CALL(env, OH_JSVM_CreateDouble(env, obj->value_ * multiple, argv));

  JSVM_Value instance;
  NODE_API_CALL(env, OH_JSVM_NewInstance(env, cons, kArgCount, argv, &instance));

  return instance;
}

// This finalizer should never be invoked.
void ObjectWrapDanglingReferenceFinalizer(JSVM_Env env,
                                          void* finalize_data,
                                          void* finalize_hint) {
  assert(0 && "unreachable");
}

JSVM_Ref dangling_ref;
JSVM_Value ObjectWrapDanglingReference(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env,
                OH_JSVM_GetCbInfo(env, info, &argc, args, nullptr, nullptr));

  // Create a OH_JSVM_Wrap and remove it immediately, whilst leaving the out-param
  // ref dangling (not deleted).
  NODE_API_CALL(env,
                OH_JSVM_Wrap(env,
                          args[0],
                          nullptr,
                          ObjectWrapDanglingReferenceFinalizer,
                          nullptr,
                          &dangling_ref));
  NODE_API_CALL(env, OH_JSVM_RemoveWrap(env, args[0], nullptr));

  return args[0];
}

JSVM_Value ObjectWrapDanglingReferenceTest(JSVM_Env env,
                                           JSVM_CallbackInfo info) {
  JSVM_Value out;
  JSVM_Value ret;
  NODE_API_CALL(env, OH_JSVM_GetReferenceValue(env, dangling_ref, &out));

  if (out == nullptr) {
    // If the JSVM_Ref has been invalidated, delete it.
    NODE_API_CALL(env, OH_JSVM_DeleteReference(env, dangling_ref));
    NODE_API_CALL(env, OH_JSVM_GetBoolean(env, true, &ret));
  } else {
    // The dangling JSVM_Ref is still valid.
    NODE_API_CALL(env, OH_JSVM_GetBoolean(env, false, &ret));
  }
  return ret;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  MyObject::Init(env, exports);

  JSVM_PropertyDescriptor descriptors[] = {
      DECLARE_NODE_API_PROPERTY("objectWrapDanglingReference",
                                ObjectWrapDanglingReference),
      DECLARE_NODE_API_PROPERTY("objectWrapDanglingReferenceTest",
                                ObjectWrapDanglingReferenceTest),
  };

  NODE_API_CALL(
      env,
      OH_JSVM_DefineProperties(env,
                             exports,
                             sizeof(descriptors) / sizeof(*descriptors),
                             descriptors));

  return exports;
}
EXTERN_C_END
