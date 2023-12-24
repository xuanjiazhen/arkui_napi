#include <assert.h>
#include <jsvm.h>
#include <string.h>
#include "../common.h"
#include "../entry_point.h"

static JSVM_Value TestExceptions(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value target = args[0];
  JSVM_Value exception, key, value;
  JSVM_Status status;
  bool is_exception_pending;
  bool bool_result;

  NODE_API_CALL(env,
                OH_JSVM_CreateStringUtf8(env, "key", JSVM_AUTO_LENGTH, &key));
  NODE_API_CALL(
      env, OH_JSVM_CreateStringUtf8(env, "value", JSVM_AUTO_LENGTH, &value));

#define PROCEDURE(call)                                                        \
  {                                                                            \
    status = (call);                                                           \
    NODE_API_ASSERT(                                                           \
        env, status == JSVM_PENDING_EXCEPTION, "expect exception pending");    \
    NODE_API_CALL(env, OH_JSVM_IsExceptionPending(env, &is_exception_pending)); \
    NODE_API_ASSERT(env, is_exception_pending, "expect exception pending");    \
    NODE_API_CALL(env, OH_JSVM_GetAndClearLastException(env, &exception));    \
  }
  // discard the exception values.

  // properties
  PROCEDURE(OH_JSVM_SetProperty(env, target, key, value));
  PROCEDURE(OH_JSVM_SetNamedProperty(env, target, "key", value));
  PROCEDURE(OH_JSVM_HasProperty(env, target, key, &bool_result));
  PROCEDURE(OH_JSVM_HasOwnProperty(env, target, key, &bool_result));
  PROCEDURE(OH_JSVM_HasNamedProperty(env, target, "key", &bool_result));
  PROCEDURE(OH_JSVM_GetProperty(env, target, key, &value));
  PROCEDURE(OH_JSVM_GetNamedProperty(env, target, "key", &value));
  PROCEDURE(OH_JSVM_DeleteProperty(env, target, key, &bool_result));

  // elements
  PROCEDURE(OH_JSVM_SetElement(env, target, 0, value));
  PROCEDURE(OH_JSVM_HasElement(env, target, 0, &bool_result));
  PROCEDURE(OH_JSVM_GetElement(env, target, 0, &value));
  PROCEDURE(OH_JSVM_DeleteElement(env, target, 0, &bool_result));

  JSVM_PropertyDescriptor descriptors[] = {
      DECLARE_NODE_API_PROPERTY_VALUE("key", value),
  };
  PROCEDURE(OH_JSVM_DefineProperties(
      env, target, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  PROCEDURE(OH_JSVM_GetAllPropertyNames(env,
                                        target,
                                        JSVM_KEY_OWN_ONLY,
                                        JSVM_KEY_ENUMERABLE,
                                        JSVM_KEY_KEEP_NUMBERS,
                                        &value));
  PROCEDURE(OH_JSVM_GetPropertyNames(env, target, &value));

  return NULL;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
      DECLARE_NODE_API_PROPERTY("testExceptions", TestExceptions),
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
