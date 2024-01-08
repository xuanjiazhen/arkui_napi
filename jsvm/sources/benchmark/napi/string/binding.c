#include <assert.h>
#define JSVM_EXPERIMENTAL
#include <node_api.h>

#define JSVM_CALL(call)                                                        \
  do {                                                                         \
    JSVM_Status status = call;                                                 \
    assert(status == JSVM_OK && #call " failed");                              \
  } while (0);

#define EXPORT_FUNC(env, exports, name, func)                                  \
  do {                                                                         \
    JSVM_Value js_func;                                                        \
    JSVM_CALL(OH_JSVM_CreateFunction(                                            \
        (env), (name), JSVM_AUTO_LENGTH, (func), NULL, &js_func));             \
    JSVM_CALL(OH_JSVM_SetNamedProperty((env), (exports), (name), js_func));     \
  } while (0);

const char* one_byte_string = "The Quick Brown Fox Jumped Over The Lazy Dog.";
const char16_t* two_byte_string =
    u"The Quick Brown Fox Jumped Over The Lazy Dog.";

#define DECLARE_BINDING(CapName, lowercase_name, var_name)                     \
  static JSVM_Value CreateString##CapName(JSVM_Env env,                        \
                                          JSVM_CallbackInfo info) {           \
    size_t argc = 4;                                                           \
    JSVM_Value argv[4];                                                        \
    uint32_t n;                                                                \
    uint32_t index;                                                            \
    JSVM_HandleScope scope;                                                   \
    JSVM_Value js_string;                                                      \
                                                                               \
    JSVM_CALL(OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));           \
    JSVM_CALL(OH_JSVM_GetValueUint32(env, argv[0], &n));                        \
    JSVM_CALL(OH_JSVM_OpenHandleScope(env, &scope));                            \
    JSVM_CALL(OH_JSVM_CallFunction(env, argv[1], argv[2], 0, NULL, NULL));       \
    for (index = 0; index < n; index++) {                                      \
      JSVM_CALL(jsvm_create_string_##lowercase_name(                           \
          env, (var_name), JSVM_AUTO_LENGTH, &js_string));                     \
    }                                                                          \
    JSVM_CALL(OH_JSVM_CallFunction(env, argv[1], argv[3], 1, &argv[0], NULL));   \
    JSVM_CALL(OH_JSVM_CloseHandleScope(env, scope));                            \
                                                                               \
    return NULL;                                                               \
  }

DECLARE_BINDING(Latin1, latin1, one_byte_string)
DECLARE_BINDING(Utf8, utf8, one_byte_string)
DECLARE_BINDING(Utf16, utf16, two_byte_string)

JSVM_MODULE_INIT() {
  EXPORT_FUNC(env, exports, "createStringLatin1", CreateStringLatin1);
  EXPORT_FUNC(env, exports, "createStringUtf8", CreateStringUtf8);
  EXPORT_FUNC(env, exports, "createStringUtf16", CreateStringUtf16);
  return exports;
}
