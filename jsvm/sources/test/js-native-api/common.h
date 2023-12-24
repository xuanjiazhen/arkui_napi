#ifndef JS_NATIVE_API_COMMON_H_
#define JS_NATIVE_API_COMMON_H_

#include <jsvm.h>

// Empty value so that macros here are able to return NULL or void
#define NODE_API_RETVAL_NOTHING  // Intentionally blank #define

#define GET_AND_THROW_LAST_ERROR(env)                                    \
  do {                                                                   \
    const JSVM_ExtendedErrorInfo *error_info;                          \
    OH_JSVM_GetLastErrorInfo((env), &error_info);                        \
    bool is_pending;                                                     \
    const char* err_message = error_info->error_message;                  \
    OH_JSVM_IsExceptionPending((env), &is_pending);                       \
    /* If an exception is already pending, don't rethrow it */           \
    if (!is_pending) {                                                   \
      const char* error_message = err_message != NULL ?                  \
                       err_message :                                     \
                      "empty error message";                             \
      OH_JSVM_ThrowError((env), NULL, error_message);                      \
    }                                                                    \
  } while (0)

#define NODE_API_ASSERT_BASE(env, assertion, message, ret_val)           \
  do {                                                                   \
    if (!(assertion)) {                                                  \
      OH_JSVM_ThrowError(                                                  \
          (env),                                                         \
        NULL,                                                            \
          "assertion (" #assertion ") failed: " message);                \
      return ret_val;                                                    \
    }                                                                    \
  } while (0)

// Returns NULL on failed assertion.
// This is meant to be used inside JSVM_Callback methods.
#define NODE_API_ASSERT(env, assertion, message)                         \
  NODE_API_ASSERT_BASE(env, assertion, message, NULL)

// Returns empty on failed assertion.
// This is meant to be used inside functions with void return type.
#define NODE_API_ASSERT_RETURN_VOID(env, assertion, message)             \
  NODE_API_ASSERT_BASE(env, assertion, message, NODE_API_RETVAL_NOTHING)

#define NODE_API_CALL_BASE(env, the_call, ret_val)                       \
  do {                                                                   \
    if ((the_call) != JSVM_OK) {                                         \
      GET_AND_THROW_LAST_ERROR((env));                                   \
      return ret_val;                                                    \
    }                                                                    \
  } while (0)

// Returns NULL if the_call doesn't return JSVM_OK.
#define NODE_API_CALL(env, the_call)                                     \
  NODE_API_CALL_BASE(env, the_call, NULL)

// Returns empty if the_call doesn't return JSVM_OK.
#define NODE_API_CALL_RETURN_VOID(env, the_call)                         \
  NODE_API_CALL_BASE(env, the_call, NODE_API_RETVAL_NOTHING)

#define NODE_API_CHECK_STATUS(the_call)                                   \
  do {                                                                         \
    JSVM_Status status = (the_call);                                           \
    if (status != JSVM_OK) {                                                   \
      return status;                                                           \
    }                                                                          \
  } while (0)

#define NODE_API_ASSERT_STATUS(env, assertion, message)                        \
  NODE_API_ASSERT_BASE(env, assertion, message, JSVM_GENERIC_FAILURE)

#define DECLARE_NODE_API_PROPERTY(name, func)                            \
  { (name), NULL, (func), NULL, NULL, NULL, JSVM_DEFAULT, NULL }

#define DECLARE_NODE_API_GETTER(name, func)                              \
  { (name), NULL, NULL, (func), NULL, NULL, JSVM_DEFAULT, NULL }

#define DECLARE_NODE_API_PROPERTY_VALUE(name, value)                     \
  { (name), NULL, NULL, NULL, NULL, (value), JSVM_DEFAULT, NULL }

static inline void add_returned_status(JSVM_Env env,
                                       const char* key,
                                       JSVM_Value object,
                                       char* expected_message,
                                       JSVM_Status expected_status,
                                       JSVM_Status actual_status);

static inline void add_last_status(JSVM_Env env,
                                   const char* key,
                                   JSVM_Value return_value);

#include "common-inl.h"

#endif  // JS_NATIVE_API_COMMON_H_
