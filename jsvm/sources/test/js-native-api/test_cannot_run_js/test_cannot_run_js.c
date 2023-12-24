#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"
#include "stdlib.h"

static void Finalize(JSVM_Env env, void* data, void* hint) {
  JSVM_Value global, set_timeout;
  JSVM_Ref* ref = data;
#ifdef JSVM_EXPERIMENTAL
  JSVM_Status expected_status = JSVM_CANNOT_RUN_JS;
#else
  JSVM_Status expected_status = JSVM_PENDING_EXCEPTION;
#endif  // JSVM_EXPERIMENTAL

  if (OH_JSVM_DeleteReference(env, *ref) != JSVM_OK) abort();
  if (OH_JSVM_GetGlobal(env, &global) != JSVM_OK) abort();
  if (OH_JSVM_GetNamedProperty(env, global, "setTimeout", &set_timeout) !=
      expected_status)
    abort();
  free(ref);
}

static JSVM_Value CreateRef(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value cb;
  JSVM_ValueType value_type;
  JSVM_Ref* ref = malloc(sizeof(*ref));
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &cb, NULL, NULL));
  NODE_API_ASSERT(env, argc == 1, "Function takes only one argument");
  NODE_API_CALL(env, OH_JSVM_Typeof(env, cb, &value_type));
  NODE_API_ASSERT(
      env, value_type == jsvm_function, "argument must be function");
  NODE_API_CALL(env, OH_JSVM_AddFinalizer(env, cb, ref, Finalize, NULL, ref));
  return cb;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor properties[] = {
      DECLARE_NODE_API_PROPERTY("createRef", CreateRef),
  };

  NODE_API_CALL(
      env,
      OH_JSVM_DefineProperties(
          env, exports, sizeof(properties) / sizeof(*properties), properties));

  return exports;
}
EXTERN_C_END
