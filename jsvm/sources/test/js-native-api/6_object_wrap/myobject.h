#ifndef TEST_JS_NATIVE_API_6_OBJECT_WRAP_MYOBJECT_H_
#define TEST_JS_NATIVE_API_6_OBJECT_WRAP_MYOBJECT_H_

#include <jsvm.h>

class MyObject {
 public:
  static void Init(JSVM_Env env, JSVM_Value exports);
  static void Destructor(JSVM_Env env, void* nativeObject, void* finalize_hint);

 private:
  explicit MyObject(double value_ = 0);
  ~MyObject();

  static JSVM_Value New(JSVM_Env env, JSVM_CallbackInfo info);
  static JSVM_Value GetValue(JSVM_Env env, JSVM_CallbackInfo info);
  static JSVM_Value SetValue(JSVM_Env env, JSVM_CallbackInfo info);
  static JSVM_Value PlusOne(JSVM_Env env, JSVM_CallbackInfo info);
  static JSVM_Value Multiply(JSVM_Env env, JSVM_CallbackInfo info);
  static JSVM_Ref constructor;
  double value_;
  JSVM_Env env_;
  JSVM_Ref wrapper_;
};

#endif  // TEST_JS_NATIVE_API_6_OBJECT_WRAP_MYOBJECT_H_
