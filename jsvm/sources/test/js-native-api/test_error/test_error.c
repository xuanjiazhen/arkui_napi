#define JSVM_VERSION 9
#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"

static JSVM_Value checkError(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  bool r;
  NODE_API_CALL(env, OH_JSVM_IsError(env, args[0], &r));

  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, r, &result));

  return result;
}

static JSVM_Value throwExistingError(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value message;
  JSVM_Value error;
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
      env, "existing error", JSVM_AUTO_LENGTH, &message));
  NODE_API_CALL(env, OH_JSVM_CreateError(env, NULL,  message, &error));
  NODE_API_CALL(env, OH_JSVM_Throw(env, error));
  return NULL;
}

static JSVM_Value throwError(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_CALL(env, OH_JSVM_ThrowError(env, NULL, "error"));
  return NULL;
}

static JSVM_Value throwRangeError(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_CALL(env, OH_JSVM_ThrowRangeError(env, NULL, "range error"));
  return NULL;
}

static JSVM_Value throwTypeError(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_CALL(env, OH_JSVM_ThrowTypeError(env, NULL, "type error"));
  return NULL;
}

static JSVM_Value throwSyntaxError(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_CALL(env, OH_JSVM_ThrowSyntaxError(env, NULL, "syntax error"));
  return NULL;
}

static JSVM_Value throwErrorCode(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_CALL(env, OH_JSVM_ThrowError(env, "ERR_TEST_CODE", "Error [error]"));
  return NULL;
}

static JSVM_Value throwRangeErrorCode(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_CALL(env,
      OH_JSVM_ThrowRangeError(env, "ERR_TEST_CODE", "RangeError [range error]"));
  return NULL;
}

static JSVM_Value throwTypeErrorCode(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_CALL(env,
      OH_JSVM_ThrowTypeError(env, "ERR_TEST_CODE", "TypeError [type error]"));
  return NULL;
}

static JSVM_Value throwSyntaxErrorCode(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_CALL(env,
      OH_JSVM_ThrowSyntaxError(env, "ERR_TEST_CODE", "SyntaxError [syntax error]"));
  return NULL;
}

static JSVM_Value createError(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  JSVM_Value message;
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
      env, "error", JSVM_AUTO_LENGTH, &message));
  NODE_API_CALL(env, OH_JSVM_CreateError(env, NULL, message, &result));
  return result;
}

static JSVM_Value createRangeError(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  JSVM_Value message;
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
      env, "range error", JSVM_AUTO_LENGTH, &message));
  NODE_API_CALL(env, OH_JSVM_CreateRangeError(env, NULL, message, &result));
  return result;
}

static JSVM_Value createTypeError(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  JSVM_Value message;
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
      env, "type error", JSVM_AUTO_LENGTH, &message));
  NODE_API_CALL(env, OH_JSVM_CreateTypeError(env, NULL, message, &result));
  return result;
}

static JSVM_Value createSyntaxError(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  JSVM_Value message;
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
      env, "syntax error", JSVM_AUTO_LENGTH, &message));
  NODE_API_CALL(env, OH_JSVM_CreateSyntaxError(env, NULL, message, &result));
  return result;
}

static JSVM_Value createErrorCode(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  JSVM_Value message;
  JSVM_Value code;
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
      env, "Error [error]", JSVM_AUTO_LENGTH, &message));
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
      env, "ERR_TEST_CODE", JSVM_AUTO_LENGTH, &code));
  NODE_API_CALL(env, OH_JSVM_CreateError(env, code, message, &result));
  return result;
}

static JSVM_Value createRangeErrorCode(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  JSVM_Value message;
  JSVM_Value code;
  NODE_API_CALL(env,
      OH_JSVM_CreateStringUtf8(
          env, "RangeError [range error]", JSVM_AUTO_LENGTH, &message));
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
      env, "ERR_TEST_CODE", JSVM_AUTO_LENGTH, &code));
  NODE_API_CALL(env, OH_JSVM_CreateRangeError(env, code, message, &result));
  return result;
}

static JSVM_Value createTypeErrorCode(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  JSVM_Value message;
  JSVM_Value code;
  NODE_API_CALL(env,
      OH_JSVM_CreateStringUtf8(
          env, "TypeError [type error]", JSVM_AUTO_LENGTH, &message));
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
      env, "ERR_TEST_CODE", JSVM_AUTO_LENGTH, &code));
  NODE_API_CALL(env, OH_JSVM_CreateTypeError(env, code, message, &result));
  return result;
}

static JSVM_Value createSyntaxErrorCode(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  JSVM_Value message;
  JSVM_Value code;
  NODE_API_CALL(env,
      OH_JSVM_CreateStringUtf8(
          env, "SyntaxError [syntax error]", JSVM_AUTO_LENGTH, &message));
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
      env, "ERR_TEST_CODE", JSVM_AUTO_LENGTH, &code));
  NODE_API_CALL(env, OH_JSVM_CreateSyntaxError(env, code, message, &result));
  return result;
}

static JSVM_Value throwArbitrary(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value arbitrary;
  size_t argc = 1;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &arbitrary, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_Throw(env, arbitrary));
  return NULL;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("checkError", checkError),
    DECLARE_NODE_API_PROPERTY("throwExistingError", throwExistingError),
    DECLARE_NODE_API_PROPERTY("throwError", throwError),
    DECLARE_NODE_API_PROPERTY("throwRangeError", throwRangeError),
    DECLARE_NODE_API_PROPERTY("throwTypeError", throwTypeError),
    DECLARE_NODE_API_PROPERTY("throwSyntaxError", throwSyntaxError),
    DECLARE_NODE_API_PROPERTY("throwErrorCode", throwErrorCode),
    DECLARE_NODE_API_PROPERTY("throwRangeErrorCode", throwRangeErrorCode),
    DECLARE_NODE_API_PROPERTY("throwTypeErrorCode", throwTypeErrorCode),
    DECLARE_NODE_API_PROPERTY("throwSyntaxErrorCode", throwSyntaxErrorCode),
    DECLARE_NODE_API_PROPERTY("throwArbitrary", throwArbitrary),
    DECLARE_NODE_API_PROPERTY("createError", createError),
    DECLARE_NODE_API_PROPERTY("createRangeError", createRangeError),
    DECLARE_NODE_API_PROPERTY("createTypeError", createTypeError),
    DECLARE_NODE_API_PROPERTY("createSyntaxError", createSyntaxError),
    DECLARE_NODE_API_PROPERTY("createErrorCode", createErrorCode),
    DECLARE_NODE_API_PROPERTY("createRangeErrorCode", createRangeErrorCode),
    DECLARE_NODE_API_PROPERTY("createTypeErrorCode", createTypeErrorCode),
    DECLARE_NODE_API_PROPERTY("createSyntaxErrorCode", createSyntaxErrorCode),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  return exports;
}
EXTERN_C_END
