#ifndef JS_NATIVE_API_COMMON_INL_H_
#define JS_NATIVE_API_COMMON_INL_H_

#include <jsvm.h>
#include "common.h"

#include <stdio.h>

inline void add_returned_status(JSVM_Env env,
                                const char* key,
                                JSVM_Value object,
                                char* expected_message,
                                JSVM_Status expected_status,
                                JSVM_Status actual_status) {
  char jsvm_message_string[100] = "";
  JSVM_Value prop_value;

  if (actual_status != expected_status) {
    snprintf(jsvm_message_string,
             sizeof(jsvm_message_string),
             "Invalid status [%d]",
             actual_status);
  }

  NODE_API_CALL_RETURN_VOID(
      env,
      OH_JSVM_CreateStringUtf8(
          env,
          (actual_status == expected_status ? expected_message
                                            : jsvm_message_string),
          JSVM_AUTO_LENGTH,
          &prop_value));
  NODE_API_CALL_RETURN_VOID(
      env, OH_JSVM_SetNamedProperty(env, object, key, prop_value));
}

inline void add_last_status(JSVM_Env env,
                            const char* key,
                            JSVM_Value return_value) {
  JSVM_Value prop_value;
  JSVM_Value exception;
  const JSVM_ExtendedErrorInfo* p_last_error;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetLastErrorInfo(env, &p_last_error));
  // Content of p_last_error can be updated in subsequent node-api calls.
  // Retrieve it immediately.
  const char* error_message = p_last_error->error_message == NULL
                                  ? "JSVM_OK"
                                  : p_last_error->error_message;

  bool is_exception_pending;
  NODE_API_CALL_RETURN_VOID(
      env, OH_JSVM_IsExceptionPending(env, &is_exception_pending));
  if (is_exception_pending) {
    NODE_API_CALL_RETURN_VOID(
        env, OH_JSVM_GetAndClearLastException(env, &exception));
    char exception_key[50];
    snprintf(exception_key, sizeof(exception_key), "%s%s", key, "Exception");
    NODE_API_CALL_RETURN_VOID(
        env,
        OH_JSVM_SetNamedProperty(env, return_value, exception_key, exception));
  }

  NODE_API_CALL_RETURN_VOID(
      env,
      OH_JSVM_CreateStringUtf8(
          env, error_message, JSVM_AUTO_LENGTH, &prop_value));
  NODE_API_CALL_RETURN_VOID(
      env, OH_JSVM_SetNamedProperty(env, return_value, key, prop_value));
}

#endif  // JS_NATIVE_API_COMMON_INL_H_
