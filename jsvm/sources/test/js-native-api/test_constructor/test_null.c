#include <jsvm.h>

#include "../common.h"
#include "test_null.h"

static int some_data = 0;

static JSVM_Value TestConstructor(JSVM_Env env, JSVM_CallbackInfo info) {
  return NULL;
}

static JSVM_Value TestDefineClass(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value return_value, cons;

  const JSVM_PropertyDescriptor prop =
      DECLARE_NODE_API_PROPERTY("testConstructor", TestConstructor);

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));
  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      OH_JSVM_DefineClass(NULL,
                                        "TestClass",
                                        JSVM_AUTO_LENGTH,
                                        TestConstructor,
                                        &some_data,
                                        1,
                                        &prop,
                                        &cons));

  OH_JSVM_DefineClass(env,
                    NULL,
                    JSVM_AUTO_LENGTH,
                    TestConstructor,
                    &some_data,
                    1,
                    &prop,
                    &cons);
  add_last_status(env, "nameIsNull", return_value);

  OH_JSVM_DefineClass(
      env, "TestClass", 0, TestConstructor, &some_data, 1, &prop, &cons);
  add_last_status(env, "lengthIsZero", return_value);

  OH_JSVM_DefineClass(
      env, "TestClass", JSVM_AUTO_LENGTH, NULL, &some_data, 1, &prop, &cons);
  add_last_status(env, "nativeSideIsNull", return_value);

  OH_JSVM_DefineClass(env,
                    "TestClass",
                    JSVM_AUTO_LENGTH,
                    TestConstructor,
                    NULL,
                    1,
                    &prop,
                    &cons);
  add_last_status(env, "dataIsNull", return_value);

  OH_JSVM_DefineClass(env,
                    "TestClass",
                    JSVM_AUTO_LENGTH,
                    TestConstructor,
                    &some_data,
                    0,
                    &prop,
                    &cons);
  add_last_status(env, "propsLengthIsZero", return_value);

  OH_JSVM_DefineClass(env,
                    "TestClass",
                    JSVM_AUTO_LENGTH,
                    TestConstructor,
                    &some_data,
                    1,
                    NULL,
                    &cons);
  add_last_status(env, "propsIsNull", return_value);

  OH_JSVM_DefineClass(env,
                    "TestClass",
                    JSVM_AUTO_LENGTH,
                    TestConstructor,
                    &some_data,
                    1,
                    &prop,
                    NULL);
  add_last_status(env, "resultIsNull", return_value);

  return return_value;
}

void init_test_null(JSVM_Env env, JSVM_Value exports) {
  JSVM_Value test_null;

  const JSVM_PropertyDescriptor test_null_props[] = {
      DECLARE_NODE_API_PROPERTY("testDefineClass", TestDefineClass),
  };

  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_CreateObject(env, &test_null));
  NODE_API_CALL_RETURN_VOID(
      env,
      OH_JSVM_DefineProperties(env,
                             test_null,
                             sizeof(test_null_props) / sizeof(*test_null_props),
                             test_null_props));

  NODE_API_CALL_RETURN_VOID(
      env, OH_JSVM_SetNamedProperty(env, exports, "testNull", test_null));
}
