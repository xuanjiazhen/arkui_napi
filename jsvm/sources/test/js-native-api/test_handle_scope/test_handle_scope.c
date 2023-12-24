#include <jsvm.h>
#include <string.h>
#include "../common.h"
#include "../entry_point.h"

// these tests validate the handle scope functions in the normal
// flow.  Forcing gc behavior to fully validate they are doing
// the right right thing would be quite hard so we keep it
// simple for now.

static JSVM_Value NewScope(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_HandleScope scope;
  JSVM_Value output = NULL;

  NODE_API_CALL(env, OH_JSVM_OpenHandleScope(env, &scope));
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &output));
  NODE_API_CALL(env, OH_JSVM_CloseHandleScope(env, scope));
  return NULL;
}

static JSVM_Value NewScopeEscape(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_EscapableHandleScope scope;
  JSVM_Value output = NULL;
  JSVM_Value escapee = NULL;

  NODE_API_CALL(env, OH_JSVM_OpenEscapableHandleScope(env, &scope));
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &output));
  NODE_API_CALL(env, OH_JSVM_EscapeHandle(env, scope, output, &escapee));
  NODE_API_CALL(env, OH_JSVM_CloseEscapableHandleScope(env, scope));
  return escapee;
}

static JSVM_Value NewScopeEscapeTwice(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_EscapableHandleScope scope;
  JSVM_Value output = NULL;
  JSVM_Value escapee = NULL;
  JSVM_Status status;

  NODE_API_CALL(env, OH_JSVM_OpenEscapableHandleScope(env, &scope));
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &output));
  NODE_API_CALL(env, OH_JSVM_EscapeHandle(env, scope, output, &escapee));
  status = OH_JSVM_EscapeHandle(env, scope, output, &escapee);
  NODE_API_ASSERT(env, status == JSVM_ESCAPE_CALLED_TWICE, "Escaping twice fails");
  NODE_API_CALL(env, OH_JSVM_CloseEscapableHandleScope(env, scope));
  return NULL;
}

static JSVM_Value NewScopeWithException(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_HandleScope scope;
  size_t argc;
  JSVM_Value exception_function;
  JSVM_Status status;
  JSVM_Value output = NULL;

  NODE_API_CALL(env, OH_JSVM_OpenHandleScope(env, &scope));
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &output));

  argc = 1;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(
      env, info, &argc, &exception_function, NULL, NULL));

  status = OH_JSVM_CallFunction(
      env, output, exception_function, 0, NULL, NULL);
  NODE_API_ASSERT(env, status == JSVM_PENDING_EXCEPTION,
      "Function should have thrown.");

  // Closing a handle scope should still work while an exception is pending.
  NODE_API_CALL(env, OH_JSVM_CloseHandleScope(env, scope));
  return NULL;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor properties[] = {
    DECLARE_NODE_API_PROPERTY("NewScope", NewScope),
    DECLARE_NODE_API_PROPERTY("NewScopeEscape", NewScopeEscape),
    DECLARE_NODE_API_PROPERTY("NewScopeEscapeTwice", NewScopeEscapeTwice),
    DECLARE_NODE_API_PROPERTY("NewScopeWithException", NewScopeWithException),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(properties) / sizeof(*properties), properties));

  return exports;
}
EXTERN_C_END
