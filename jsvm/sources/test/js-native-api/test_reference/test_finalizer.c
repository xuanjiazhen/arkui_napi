#include <assert.h>
#include <jsvm.h>
#include <stdlib.h>
#include "../common.h"
#include "../entry_point.h"

static int test_value = 1;
static int finalize_count = 0;

static void FinalizeExternalCallJs(JSVM_Env env, void* data, void* hint) {
  int* actual_value = data;
  NODE_API_ASSERT_RETURN_VOID(
      env,
      actual_value == &test_value,
      "The correct pointer was passed to the finalizer");

  JSVM_Ref finalizer_ref = (JSVM_Ref)hint;
  JSVM_Value js_finalizer;
  JSVM_Value recv;
  NODE_API_CALL_RETURN_VOID(
      env, OH_JSVM_GetReferenceValue(env, finalizer_ref, &js_finalizer));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetGlobal(env, &recv));
  NODE_API_CALL_RETURN_VOID(
      env, OH_JSVM_CallFunction(env, recv, js_finalizer, 0, NULL, NULL));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, finalizer_ref));
}

static JSVM_Value CreateExternalWithJsFinalize(JSVM_Env env,
                                               JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));
  NODE_API_ASSERT(env, argc == 1, "Wrong number of arguments");
  JSVM_Value finalizer = args[0];
  JSVM_ValueType finalizer_valuetype;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, finalizer, &finalizer_valuetype));
  NODE_API_ASSERT(env,
                  finalizer_valuetype == jsvm_function,
                  "Wrong type of first argument");
  JSVM_Ref finalizer_ref;
  NODE_API_CALL(env, OH_JSVM_CreateReference(env, finalizer, 1, &finalizer_ref));

  JSVM_Value result;
  NODE_API_CALL(env,
                OH_JSVM_CreateExternal(env,
                                     &test_value,
                                     FinalizeExternalCallJs,
                                     finalizer_ref, /* finalize_hint */
                                     &result));

  finalize_count = 0;
  return result;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
      DECLARE_NODE_API_PROPERTY("createExternalWithJsFinalize",
                                CreateExternalWithJsFinalize),
  };

  NODE_API_CALL(
      env,
      OH_JSVM_DefineProperties(env,
                             exports,
                             sizeof(descriptors) / sizeof(*descriptors),
                             descriptors));

  return exports;
}
EXTERN_C_END
