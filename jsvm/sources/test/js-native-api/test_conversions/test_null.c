#include <jsvm.h>

#include "../common.h"
#include "test_null.h"

#define GEN_NULL_CHECK_BINDING(binding_name, output_type, api)            \
  static JSVM_Value binding_name(JSVM_Env env, JSVM_CallbackInfo info) { \
    JSVM_Value return_value;                                              \
    output_type result;                                                   \
    NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));           \
    add_returned_status(env,                                              \
                        "envIsNull",                                      \
                        return_value,                                     \
                        "Invalid argument",                               \
                        JSVM_INVALID_ARG,                                 \
                        api(NULL, return_value, &result));                \
    api(env, NULL, &result);                                              \
    add_last_status(env, "valueIsNull", return_value);                    \
    api(env, return_value, NULL);                                         \
    add_last_status(env, "resultIsNull", return_value);                   \
    api(env, return_value, &result);                                      \
    add_last_status(env, "inputTypeCheck", return_value);                 \
    return return_value;                                                  \
  }

GEN_NULL_CHECK_BINDING(GetValueBool, bool, OH_JSVM_GetValueBool)
GEN_NULL_CHECK_BINDING(GetValueInt32, int32_t, OH_JSVM_GetValueInt32)
GEN_NULL_CHECK_BINDING(GetValueUint32, uint32_t, OH_JSVM_GetValueUint32)
GEN_NULL_CHECK_BINDING(GetValueInt64, int64_t, OH_JSVM_GetValueInt64)
GEN_NULL_CHECK_BINDING(GetValueDouble, double, OH_JSVM_GetValueDouble)
GEN_NULL_CHECK_BINDING(CoerceToBool, JSVM_Value, OH_JSVM_CoerceToBool)
GEN_NULL_CHECK_BINDING(CoerceToObject, JSVM_Value, OH_JSVM_CoerceToObject)
GEN_NULL_CHECK_BINDING(CoerceToString, JSVM_Value, OH_JSVM_CoerceToString)

#define GEN_NULL_CHECK_STRING_BINDING(binding_name, arg_type, api)         \
  static JSVM_Value binding_name(JSVM_Env env, JSVM_CallbackInfo info) {  \
    JSVM_Value return_value;                                               \
    NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));            \
    arg_type buf1[4];                                                      \
    size_t length1 = 3;                                                    \
    add_returned_status(env,                                               \
                        "envIsNull",                                       \
                        return_value,                                      \
                        "Invalid argument",                                \
                        JSVM_INVALID_ARG,                                  \
                        api(NULL, return_value, buf1, length1, &length1)); \
    arg_type buf2[4];                                                      \
    size_t length2 = 3;                                                    \
    api(env, NULL, buf2, length2, &length2);                               \
    add_last_status(env, "valueIsNull", return_value);                     \
    api(env, return_value, NULL, 3, NULL);                                 \
    add_last_status(env, "wrongTypeIn", return_value);                     \
    JSVM_Value string;                                                     \
    NODE_API_CALL(env,                                                     \
              OH_JSVM_CreateStringUtf8(env,                                 \
                                      "Something",                         \
                                      JSVM_AUTO_LENGTH,                    \
                                      &string));                           \
    api(env, string, NULL, 3, NULL);                                       \
    add_last_status(env, "bufAndOutLengthIsNull", return_value);           \
    return return_value;                                                   \
  }

GEN_NULL_CHECK_STRING_BINDING(GetValueStringUtf8,
                              char,
                              OH_JSVM_GetValueStringUtf8)
GEN_NULL_CHECK_STRING_BINDING(GetValueStringLatin1,
                              char,
                              OH_JSVM_GetValueStringLatin1)
GEN_NULL_CHECK_STRING_BINDING(GetValueStringUtf16,
                              char16_t,
                              OH_JSVM_GetValueStringUtf16)

void init_test_null(JSVM_Env env, JSVM_Value exports) {
  JSVM_Value test_null;

  const JSVM_PropertyDescriptor test_null_props[] = {
    DECLARE_NODE_API_PROPERTY("getValueBool", GetValueBool),
    DECLARE_NODE_API_PROPERTY("getValueInt32", GetValueInt32),
    DECLARE_NODE_API_PROPERTY("getValueUint32", GetValueUint32),
    DECLARE_NODE_API_PROPERTY("getValueInt64", GetValueInt64),
    DECLARE_NODE_API_PROPERTY("getValueDouble", GetValueDouble),
    DECLARE_NODE_API_PROPERTY("coerceToBool", CoerceToBool),
    DECLARE_NODE_API_PROPERTY("coerceToObject", CoerceToObject),
    DECLARE_NODE_API_PROPERTY("coerceToString", CoerceToString),
    DECLARE_NODE_API_PROPERTY("getValueStringUtf8", GetValueStringUtf8),
    DECLARE_NODE_API_PROPERTY("getValueStringLatin1", GetValueStringLatin1),
    DECLARE_NODE_API_PROPERTY("getValueStringUtf16", GetValueStringUtf16),
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
