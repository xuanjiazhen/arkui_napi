#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"
#include "test_null.h"

static double value_ = 1;
static double static_value_ = 10;

static JSVM_Value TestDefineClass(JSVM_Env env,
                                  JSVM_CallbackInfo info) {
  JSVM_Status status;
  JSVM_Value result, return_value;

  JSVM_PropertyDescriptor property_descriptor = {
    "TestDefineClass",
    NULL,
    TestDefineClass,
    NULL,
    NULL,
    NULL,
    JSVM_ENUMERABLE | JSVM_STATIC,
    NULL};

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));

  status = OH_JSVM_DefineClass(NULL,
                             "TrackedFunction",
                             JSVM_AUTO_LENGTH,
                             TestDefineClass,
                             NULL,
                             1,
                             &property_descriptor,
                             &result);

  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      status);

  OH_JSVM_DefineClass(env,
                    NULL,
                    JSVM_AUTO_LENGTH,
                    TestDefineClass,
                    NULL,
                    1,
                    &property_descriptor,
                    &result);

  add_last_status(env, "nameIsNull", return_value);

  OH_JSVM_DefineClass(env,
                    "TrackedFunction",
                    JSVM_AUTO_LENGTH,
                    NULL,
                    NULL,
                    1,
                    &property_descriptor,
                    &result);

  add_last_status(env, "cbIsNull", return_value);

  OH_JSVM_DefineClass(env,
                    "TrackedFunction",
                    JSVM_AUTO_LENGTH,
                    TestDefineClass,
                    NULL,
                    1,
                    &property_descriptor,
                    &result);

  add_last_status(env, "cbDataIsNull", return_value);

  OH_JSVM_DefineClass(env,
                    "TrackedFunction",
                    JSVM_AUTO_LENGTH,
                    TestDefineClass,
                    NULL,
                    1,
                    NULL,
                    &result);

  add_last_status(env, "propertiesIsNull", return_value);


  OH_JSVM_DefineClass(env,
                    "TrackedFunction",
                    JSVM_AUTO_LENGTH,
                    TestDefineClass,
                    NULL,
                    1,
                    &property_descriptor,
                    NULL);

  add_last_status(env, "resultIsNull", return_value);

  return return_value;
}

static JSVM_Value GetValue(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 0;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, NULL, NULL, NULL));

  NODE_API_ASSERT(env, argc == 0, "Wrong number of arguments");

  JSVM_Value number;
  NODE_API_CALL(env, OH_JSVM_CreateDouble(env, value_, &number));

  return number;
}

static JSVM_Value SetValue(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc == 1, "Wrong number of arguments");

  NODE_API_CALL(env, OH_JSVM_GetValueDouble(env, args[0], &value_));

  return NULL;
}

static JSVM_Value Echo(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc == 1, "Wrong number of arguments");

  return args[0];
}

static JSVM_Value New(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value _this;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, NULL, NULL, &_this, NULL));

  return _this;
}

static JSVM_Value GetStaticValue(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 0;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, NULL, NULL, NULL));

  NODE_API_ASSERT(env, argc == 0, "Wrong number of arguments");

  JSVM_Value number;
  NODE_API_CALL(env, OH_JSVM_CreateDouble(env, static_value_, &number));

  return number;
}


static JSVM_Value NewExtra(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value _this;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, NULL, NULL, &_this, NULL));

  return _this;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_Value number, cons;
  NODE_API_CALL(env, OH_JSVM_CreateDouble(env, value_, &number));

  NODE_API_CALL(env, OH_JSVM_DefineClass(
      env, "MyObject_Extra", 8, NewExtra, NULL, 0, NULL, &cons));

  JSVM_PropertyDescriptor properties[] = {
    { "echo", NULL, Echo, NULL, NULL, NULL, JSVM_ENUMERABLE, NULL },
    { "readwriteValue", NULL, NULL, NULL, NULL, number,
        JSVM_ENUMERABLE | JSVM_WRITABLE, NULL },
    { "readonlyValue", NULL, NULL, NULL, NULL, number, JSVM_ENUMERABLE,
        NULL },
    { "hiddenValue", NULL, NULL, NULL, NULL, number, JSVM_DEFAULT, NULL },
    { "readwriteAccessor1", NULL, NULL, GetValue, SetValue, NULL, JSVM_DEFAULT,
        NULL },
    { "readwriteAccessor2", NULL, NULL, GetValue, SetValue, NULL,
        JSVM_WRITABLE, NULL },
    { "readonlyAccessor1", NULL, NULL, GetValue, NULL, NULL, JSVM_DEFAULT,
        NULL },
    { "readonlyAccessor2", NULL, NULL, GetValue, NULL, NULL, JSVM_WRITABLE,
        NULL },
    { "staticReadonlyAccessor1", NULL, NULL, GetStaticValue, NULL, NULL,
        JSVM_DEFAULT | JSVM_STATIC, NULL},
    { "constructorName", NULL, NULL, NULL, NULL, cons,
        JSVM_ENUMERABLE | JSVM_STATIC, NULL },
    { "TestDefineClass", NULL, TestDefineClass, NULL, NULL, NULL,
        JSVM_ENUMERABLE | JSVM_STATIC, NULL },
  };

  NODE_API_CALL(env, OH_JSVM_DefineClass(env, "MyObject", JSVM_AUTO_LENGTH, New,
      NULL, sizeof(properties)/sizeof(*properties), properties, &cons));

  init_test_null(env, cons);

  return cons;
}
EXTERN_C_END
