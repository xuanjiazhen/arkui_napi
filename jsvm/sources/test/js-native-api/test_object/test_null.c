#include <jsvm.h>

#include "../common.h"
#include "test_null.h"

static JSVM_Value SetProperty(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value return_value, object, key;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &object));
  NODE_API_CALL(env,
      OH_JSVM_CreateStringUtf8(env, "someString", JSVM_AUTO_LENGTH, &key));

  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      OH_JSVM_SetProperty(NULL, object, key, object));

  OH_JSVM_SetProperty(env, NULL, key, object);
  add_last_status(env, "objectIsNull", return_value);

  OH_JSVM_SetProperty(env, object, NULL, object);
  add_last_status(env, "keyIsNull", return_value);

  OH_JSVM_SetProperty(env, object, key, NULL);
  add_last_status(env, "valueIsNull", return_value);

  return return_value;
}

static JSVM_Value GetProperty(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value return_value, object, key, prop;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &object));
  NODE_API_CALL(env,
      OH_JSVM_CreateStringUtf8(env, "someString", JSVM_AUTO_LENGTH, &key));

  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      OH_JSVM_GetProperty(NULL, object, key, &prop));

  OH_JSVM_GetProperty(env, NULL, key, &prop);
  add_last_status(env, "objectIsNull", return_value);

  OH_JSVM_GetProperty(env, object, NULL, &prop);
  add_last_status(env, "keyIsNull", return_value);

  OH_JSVM_GetProperty(env, object, key, NULL);
  add_last_status(env, "valueIsNull", return_value);

  return return_value;
}

static JSVM_Value TestBoolValuedPropApi(JSVM_Env env,
    JSVM_Status (*api)(JSVM_Env, JSVM_Value, JSVM_Value, bool*)) {
  JSVM_Value return_value, object, key;
  bool result;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &object));
  NODE_API_CALL(env,
      OH_JSVM_CreateStringUtf8(env, "someString", JSVM_AUTO_LENGTH, &key));

  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      api(NULL, object, key, &result));

  api(env, NULL, key, &result);
  add_last_status(env, "objectIsNull", return_value);

  api(env, object, NULL, &result);
  add_last_status(env, "keyIsNull", return_value);

  api(env, object, key, NULL);
  add_last_status(env, "valueIsNull", return_value);

  return return_value;
}

static JSVM_Value HasProperty(JSVM_Env env, JSVM_CallbackInfo info) {
  return TestBoolValuedPropApi(env, OH_JSVM_HasProperty);
}

static JSVM_Value HasOwnProperty(JSVM_Env env, JSVM_CallbackInfo info) {
  return TestBoolValuedPropApi(env, OH_JSVM_HasOwnProperty);
}

static JSVM_Value DeleteProperty(JSVM_Env env, JSVM_CallbackInfo info) {
  return TestBoolValuedPropApi(env, OH_JSVM_DeleteProperty);
}

static JSVM_Value SetNamedProperty(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value return_value, object;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &object));

  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      OH_JSVM_SetNamedProperty(NULL, object, "key", object));

  OH_JSVM_SetNamedProperty(env, NULL, "key", object);
  add_last_status(env, "objectIsNull", return_value);

  OH_JSVM_SetNamedProperty(env, object, NULL, object);
  add_last_status(env, "keyIsNull", return_value);

  OH_JSVM_SetNamedProperty(env, object, "key", NULL);
  add_last_status(env, "valueIsNull", return_value);

  return return_value;
}

static JSVM_Value GetNamedProperty(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value return_value, object, prop;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &object));

  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      OH_JSVM_GetNamedProperty(NULL, object, "key", &prop));

  OH_JSVM_GetNamedProperty(env, NULL, "key", &prop);
  add_last_status(env, "objectIsNull", return_value);

  OH_JSVM_GetNamedProperty(env, object, NULL, &prop);
  add_last_status(env, "keyIsNull", return_value);

  OH_JSVM_GetNamedProperty(env, object, "key", NULL);
  add_last_status(env, "valueIsNull", return_value);

  return return_value;
}

static JSVM_Value HasNamedProperty(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value return_value, object;
  bool result;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &object));

  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      OH_JSVM_HasNamedProperty(NULL, object, "key", &result));

  OH_JSVM_HasNamedProperty(env, NULL, "key", &result);
  add_last_status(env, "objectIsNull", return_value);

  OH_JSVM_HasNamedProperty(env, object, NULL, &result);
  add_last_status(env, "keyIsNull", return_value);

  OH_JSVM_HasNamedProperty(env, object, "key", NULL);
  add_last_status(env, "valueIsNull", return_value);

  return return_value;
}

static JSVM_Value SetElement(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value return_value, object;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &object));

  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      OH_JSVM_SetElement(NULL, object, 0, object));

  OH_JSVM_SetElement(env, NULL, 0, object);
  add_last_status(env, "objectIsNull", return_value);

  OH_JSVM_SetProperty(env, object, 0, NULL);
  add_last_status(env, "valueIsNull", return_value);

  return return_value;
}

static JSVM_Value GetElement(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value return_value, object, prop;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &object));

  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      OH_JSVM_GetElement(NULL, object, 0, &prop));

  OH_JSVM_GetProperty(env, NULL, 0, &prop);
  add_last_status(env, "objectIsNull", return_value);

  OH_JSVM_GetProperty(env, object, 0, NULL);
  add_last_status(env, "valueIsNull", return_value);

  return return_value;
}

static JSVM_Value TestBoolValuedElementApi(JSVM_Env env,
    JSVM_Status (*api)(JSVM_Env, JSVM_Value, uint32_t, bool*)) {
  JSVM_Value return_value, object;
  bool result;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &object));

  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      api(NULL, object, 0, &result));

  api(env, NULL, 0, &result);
  add_last_status(env, "objectIsNull", return_value);

  api(env, object, 0, NULL);
  add_last_status(env, "valueIsNull", return_value);

  return return_value;
}

static JSVM_Value HasElement(JSVM_Env env, JSVM_CallbackInfo info) {
  return TestBoolValuedElementApi(env, OH_JSVM_HasElement);
}

static JSVM_Value DeleteElement(JSVM_Env env, JSVM_CallbackInfo info) {
  return TestBoolValuedElementApi(env, OH_JSVM_DeleteElement);
}

static JSVM_Value DefineProperties(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value object, return_value;

  JSVM_PropertyDescriptor desc = {
    "prop", NULL, DefineProperties, NULL, NULL, NULL, JSVM_ENUMERABLE, NULL
  };

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &object));
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));

  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      OH_JSVM_DefineProperties(NULL, object, 1, &desc));

  OH_JSVM_DefineProperties(env, NULL, 1, &desc);
  add_last_status(env, "objectIsNull", return_value);

  OH_JSVM_DefineProperties(env, object, 1, NULL);
  add_last_status(env, "descriptorListIsNull", return_value);

  desc.utf8name = NULL;
  OH_JSVM_DefineProperties(env, object, 1, NULL);
  add_last_status(env, "utf8nameIsNull", return_value);
  desc.utf8name = "prop";

  desc.method = NULL;
  OH_JSVM_DefineProperties(env, object, 1, NULL);
  add_last_status(env, "methodIsNull", return_value);
  desc.method = DefineProperties;

  return return_value;
}

static JSVM_Value GetPropertyNames(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value return_value, props;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));

  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      OH_JSVM_GetPropertyNames(NULL, return_value, &props));

  OH_JSVM_GetPropertyNames(env, NULL, &props);
  add_last_status(env, "objectIsNull", return_value);

  OH_JSVM_GetPropertyNames(env, return_value, NULL);
  add_last_status(env, "valueIsNull", return_value);

  return return_value;
}

static JSVM_Value GetAllPropertyNames(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value return_value, props;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));

  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      OH_JSVM_GetAllPropertyNames(NULL,
                                                  return_value,
                                                  JSVM_KEY_OWN_ONLY,
                                                  JSVM_KEY_WRITABLE,
                                                  JSVM_KEY_KEEP_NUMBERS,
                                                  &props));

  OH_JSVM_GetAllPropertyNames(env,
                              NULL,
                              JSVM_KEY_OWN_ONLY,
                              JSVM_KEY_WRITABLE,
                              JSVM_KEY_KEEP_NUMBERS,
                              &props);
  add_last_status(env, "objectIsNull", return_value);

  OH_JSVM_GetAllPropertyNames(env,
                              return_value,
                              JSVM_KEY_OWN_ONLY,
                              JSVM_KEY_WRITABLE,
                              JSVM_KEY_KEEP_NUMBERS,
                              NULL);
  add_last_status(env, "valueIsNull", return_value);

  return return_value;
}

static JSVM_Value GetPrototype(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value return_value, proto;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));

  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      OH_JSVM_GetPrototype(NULL, return_value, &proto));

  OH_JSVM_GetPrototype(env, NULL, &proto);
  add_last_status(env, "objectIsNull", return_value);

  OH_JSVM_GetPrototype(env, return_value, NULL);
  add_last_status(env, "valueIsNull", return_value);

  return return_value;
}

void init_test_null(JSVM_Env env, JSVM_Value exports) {
  JSVM_Value test_null;

  const JSVM_PropertyDescriptor test_null_props[] = {
    DECLARE_NODE_API_PROPERTY("setProperty", SetProperty),
    DECLARE_NODE_API_PROPERTY("getProperty", GetProperty),
    DECLARE_NODE_API_PROPERTY("hasProperty", HasProperty),
    DECLARE_NODE_API_PROPERTY("hasOwnProperty", HasOwnProperty),
    DECLARE_NODE_API_PROPERTY("deleteProperty", DeleteProperty),
    DECLARE_NODE_API_PROPERTY("setNamedProperty", SetNamedProperty),
    DECLARE_NODE_API_PROPERTY("getNamedProperty", GetNamedProperty),
    DECLARE_NODE_API_PROPERTY("hasNamedProperty", HasNamedProperty),
    DECLARE_NODE_API_PROPERTY("setElement", SetElement),
    DECLARE_NODE_API_PROPERTY("getElement", GetElement),
    DECLARE_NODE_API_PROPERTY("hasElement", HasElement),
    DECLARE_NODE_API_PROPERTY("deleteElement", DeleteElement),
    DECLARE_NODE_API_PROPERTY("defineProperties", DefineProperties),
    DECLARE_NODE_API_PROPERTY("getPropertyNames", GetPropertyNames),
    DECLARE_NODE_API_PROPERTY("getAllPropertyNames", GetAllPropertyNames),
    DECLARE_NODE_API_PROPERTY("getPrototype", GetPrototype),
  };

  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_CreateObject(env, &test_null));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DefineProperties(
      env, test_null, sizeof(test_null_props) / sizeof(*test_null_props),
      test_null_props));

  const JSVM_PropertyDescriptor test_null_set = {
    "testNull", NULL, NULL, NULL, NULL, test_null, JSVM_ENUMERABLE, NULL
  };

  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_DefineProperties(env, exports, 1, &test_null_set));
}
