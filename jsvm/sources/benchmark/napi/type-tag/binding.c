#include <assert.h>
#define JSVM_EXPERIMENTAL
#include <node_api.h>

#define JSVM_CALL(call)                           \
  do {                                            \
    JSVM_Status status = call;                    \
    assert(status == JSVM_OK && #call " failed"); \
  } while (0);

#define EXPORT_FUNC(env, exports, name, func)       \
  do {                                              \
    JSVM_Value js_func;                             \
    JSVM_CALL(OH_JSVM_CreateFunction((env),           \
                                  (name),           \
                                  JSVM_AUTO_LENGTH, \
                                  (func),           \
                                  NULL,             \
                                  &js_func));       \
    JSVM_CALL(OH_JSVM_SetNamedProperty((env),        \
                                     (exports),     \
                                     (name),        \
                                     js_func));     \
  } while (0);

static const JSVM_TypeTag tag = {
  0xe7ecbcd5954842f6, 0x9e75161c9bf27282
};

static JSVM_Value TagObject(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 4;
  JSVM_Value argv[4];
  uint32_t n;
  uint32_t index;
  JSVM_HandleScope scope;

  JSVM_CALL(OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));
  JSVM_CALL(OH_JSVM_GetValueUint32(env, argv[0], &n));
  JSVM_CALL(OH_JSVM_OpenHandleScope(env, &scope));
  JSVM_Value objects[n];
  for (index = 0; index < n; index++) {
    JSVM_CALL(OH_JSVM_CreateObject(env, &objects[index]));
  }

  // Time the object tag creation.
  JSVM_CALL(OH_JSVM_CallFunction(env, argv[1], argv[2], 0, NULL, NULL));
  for (index = 0; index < n; index++) {
    JSVM_CALL(OH_JSVM_TypeTagObject(env, objects[index], &tag));
  }
  JSVM_CALL(OH_JSVM_CallFunction(env, argv[1], argv[3], 1, &argv[0], NULL));

  JSVM_CALL(OH_JSVM_CloseHandleScope(env, scope));
  return NULL;
}

static JSVM_Value CheckObjectTag(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 4;
  JSVM_Value argv[4];
  uint32_t n;
  uint32_t index;
  bool is_of_type;

  JSVM_CALL(OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));
  JSVM_CALL(OH_JSVM_GetValueUint32(env, argv[0], &n));
  JSVM_Value object;
  JSVM_CALL(OH_JSVM_CreateObject(env, &object));
  JSVM_CALL(OH_JSVM_TypeTagObject(env, object, &tag));

  // Time the object tag checking.
  JSVM_CALL(OH_JSVM_CallFunction(env, argv[1], argv[2], 0, NULL, NULL));
  for (index = 0; index < n; index++) {
    JSVM_CALL(OH_JSVM_CheckObjectTypeTag(env, object, &tag, &is_of_type));
    assert(is_of_type && " type mismatch");
  }
  JSVM_CALL(OH_JSVM_CallFunction(env, argv[1], argv[3], 1, &argv[0], NULL));

  return NULL;
}

JSVM_MODULE_INIT() {
  EXPORT_FUNC(env, exports, "tagObject", TagObject);
  EXPORT_FUNC(env, exports, "checkObjectTag", CheckObjectTag);
  return exports;
}
