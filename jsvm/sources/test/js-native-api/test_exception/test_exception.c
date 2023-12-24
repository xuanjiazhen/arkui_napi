#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"

static bool exceptionWasPending = false;
static int num = 0x23432;

static JSVM_Value returnException(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value global;
  NODE_API_CALL(env, OH_JSVM_GetGlobal(env, &global));

  JSVM_Value result;
  JSVM_Status status = OH_JSVM_CallFunction(env, global, args[0], 0, 0, &result);
  if (status == JSVM_PENDING_EXCEPTION) {
    JSVM_Value ex;
    NODE_API_CALL(env, OH_JSVM_GetAndClearLastException(env, &ex));
    return ex;
  }

  return NULL;
}

static JSVM_Value constructReturnException(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value result;
  JSVM_Status status = OH_JSVM_NewInstance(env, args[0], 0, 0, &result);
  if (status == JSVM_PENDING_EXCEPTION) {
    JSVM_Value ex;
    NODE_API_CALL(env, OH_JSVM_GetAndClearLastException(env, &ex));
    return ex;
  }

  return NULL;
}

static JSVM_Value allowException(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value global;
  NODE_API_CALL(env, OH_JSVM_GetGlobal(env, &global));

  JSVM_Value result;
  OH_JSVM_CallFunction(env, global, args[0], 0, 0, &result);
  // Ignore status and check OH_JSVM_IsExceptionPending() instead.

  NODE_API_CALL(env, OH_JSVM_IsExceptionPending(env, &exceptionWasPending));
  return NULL;
}

static JSVM_Value constructAllowException(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value result;
  OH_JSVM_NewInstance(env, args[0], 0, 0, &result);
  // Ignore status and check OH_JSVM_IsExceptionPending() instead.

  NODE_API_CALL(env, OH_JSVM_IsExceptionPending(env, &exceptionWasPending));
  return NULL;
}

static JSVM_Value wasPending(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, exceptionWasPending, &result));

  return result;
}

static void finalizer(JSVM_Env env, void *data, void *hint) {
  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_ThrowError(env, NULL, "Error during Finalize"));
}

static JSVM_Value createExternal(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value external;

  NODE_API_CALL(env,
      OH_JSVM_CreateExternal(env, &num, finalizer, NULL, &external));

  return external;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("returnException", returnException),
    DECLARE_NODE_API_PROPERTY("allowException", allowException),
    DECLARE_NODE_API_PROPERTY("constructReturnException", constructReturnException),
    DECLARE_NODE_API_PROPERTY("constructAllowException", constructAllowException),
    DECLARE_NODE_API_PROPERTY("wasPending", wasPending),
    DECLARE_NODE_API_PROPERTY("createExternal", createExternal),
  };
  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  JSVM_Value error, code, message;
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(env, "Error during Init",
      JSVM_AUTO_LENGTH, &message));
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(env, "", JSVM_AUTO_LENGTH, &code));
  NODE_API_CALL(env, OH_JSVM_CreateError(env, code, message, &error));
  NODE_API_CALL(env, OH_JSVM_SetNamedProperty(env, error, "binding", exports));
  NODE_API_CALL(env, OH_JSVM_Throw(env, error));

  return exports;
}
EXTERN_C_END
