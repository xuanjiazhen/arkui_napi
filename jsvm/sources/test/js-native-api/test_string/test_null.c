#include <jsvm.h>

#include "../common.h"
#include "test_null.h"

#define DECLARE_TEST(charset, str_arg)                                    \
  static JSVM_Value                                                       \
  test_create_##charset(JSVM_Env env, JSVM_CallbackInfo info) {          \
    JSVM_Value return_value, result;                                      \
    NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));           \
                                                                          \
    add_returned_status(env,                                              \
                        "envIsNull",                                      \
                        return_value,                                     \
                        "Invalid argument",                               \
                        JSVM_INVALID_ARG,                                 \
                        jsvm_create_string_##charset(NULL,                \
                                                     (str_arg),           \
                                                     JSVM_AUTO_LENGTH,    \
                                                     &result));           \
                                                                          \
    jsvm_create_string_##charset(env, NULL, JSVM_AUTO_LENGTH, &result);   \
    add_last_status(env, "stringIsNullNonZeroLength", return_value);      \
                                                                          \
    jsvm_create_string_##charset(env, NULL, 0, &result);                  \
    add_last_status(env, "stringIsNullZeroLength", return_value);         \
                                                                          \
    jsvm_create_string_##charset(env, (str_arg), JSVM_AUTO_LENGTH, NULL); \
    add_last_status(env, "resultIsNull", return_value);                   \
                                                                          \
    return return_value;                                                  \
  }

static const char16_t something[] = {
  (char16_t)'s',
  (char16_t)'o',
  (char16_t)'m',
  (char16_t)'e',
  (char16_t)'t',
  (char16_t)'h',
  (char16_t)'i',
  (char16_t)'n',
  (char16_t)'g',
  (char16_t)'\0'
};

DECLARE_TEST(utf8, "something")
DECLARE_TEST(latin1, "something")
DECLARE_TEST(utf16, something)

void init_test_null(JSVM_Env env, JSVM_Value exports) {
  JSVM_Value test_null;

  const JSVM_PropertyDescriptor test_null_props[] = {
    DECLARE_NODE_API_PROPERTY("test_create_utf8", test_create_utf8),
    DECLARE_NODE_API_PROPERTY("test_create_latin1", test_create_latin1),
    DECLARE_NODE_API_PROPERTY("test_create_utf16", test_create_utf16),
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
