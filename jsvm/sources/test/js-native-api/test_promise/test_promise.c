#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"

JSVM_Deferred deferred = NULL;

static JSVM_Value createPromise(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value promise;

  // We do not overwrite an existing deferred.
  if (deferred != NULL) {
    return NULL;
  }

  NODE_API_CALL(env, OH_JSVM_CreatePromise(env, &deferred, &promise));

  return promise;
}

static JSVM_Value
concludeCurrentPromise(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value argv[2];
  size_t argc = 2;
  bool resolution;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_GetValueBool(env, argv[1], &resolution));
  if (resolution) {
    NODE_API_CALL(env, OH_JSVM_ResolveDeferred(env, deferred, argv[0]));
  } else {
    NODE_API_CALL(env, OH_JSVM_RejectDeferred(env, deferred, argv[0]));
  }

  deferred = NULL;

  return NULL;
}

static JSVM_Value isPromise(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value promise, result;
  size_t argc = 1;
  bool is_promise;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &promise, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_IsPromise(env, promise, &is_promise));
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, is_promise, &result));

  return result;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("createPromise", createPromise),
    DECLARE_NODE_API_PROPERTY("concludeCurrentPromise", concludeCurrentPromise),
    DECLARE_NODE_API_PROPERTY("isPromise", isPromise),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  return exports;
}
EXTERN_C_END
