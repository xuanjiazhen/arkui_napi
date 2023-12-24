#ifndef TEST_JS_NATIVE_API_7_FACTORY_WRAP_MYOBJECT_H_
#define TEST_JS_NATIVE_API_7_FACTORY_WRAP_MYOBJECT_H_

#include <jsvm.h>

class MyObject {
 public:
  static JSVM_Status Init(JSVM_Env env);
  static void Destructor(JSVM_Env env, void* nativeObject, void* finalize_hint);
  static JSVM_Value GetFinalizeCount(JSVM_Env env, JSVM_CallbackInfo info);
  static JSVM_Status NewInstance(JSVM_Env env,
                                 JSVM_Value arg,
                                 JSVM_Value* instance);

 private:
  MyObject();
  ~MyObject();

  static JSVM_Ref constructor;
  static JSVM_Value New(JSVM_Env env, JSVM_CallbackInfo info);
  static JSVM_Value PlusOne(JSVM_Env env, JSVM_CallbackInfo info);
  uint32_t counter_;
  JSVM_Env env_;
  JSVM_Ref wrapper_;
};

#endif  // TEST_JS_NATIVE_API_7_FACTORY_WRAP_MYOBJECT_H_
