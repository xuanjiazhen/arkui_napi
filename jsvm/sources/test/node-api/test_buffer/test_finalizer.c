#include <node_api.h>
#include <stdlib.h>
#include <string.h>
#include "../../js-native-api/common.h"

static const char theText[] =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";

static void malignDeleter(JSVM_Env env, void* data, void* finalize_hint) {
  NODE_API_ASSERT_RETURN_VOID(
      env, data != NULL && strcmp(data, theText) == 0, "invalid data");
  JSVM_Ref finalizer_ref = (JSVM_Ref)finalize_hint;
  JSVM_Value js_finalizer;
  JSVM_Value recv;
  NODE_API_CALL_RETURN_VOID(
      env, OH_JSVM_GetReferenceValue(env, finalizer_ref, &js_finalizer));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetGlobal(env, &recv));
  NODE_API_CALL_RETURN_VOID(
      env, OH_JSVM_CallFunction(env, recv, js_finalizer, 0, NULL, NULL));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, finalizer_ref));
}

static JSVM_Value malignFinalizerBuffer(JSVM_Env env, JSVM_CallbackInfo info) {
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

  JSVM_Value theBuffer;
  NODE_API_CALL(env,
                OH_JSVM_CreateExternal_buffer(env,
                                            sizeof(theText),
                                            (void*)theText,
                                            malignDeleter,
                                            finalizer_ref,  // finalize_hint
                                            &theBuffer));
  return theBuffer;
}

static JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor methods[] = {
      DECLARE_NODE_API_PROPERTY("malignFinalizerBuffer", malignFinalizerBuffer),
  };

  NODE_API_CALL(
      env,
      OH_JSVM_DefineProperties(
          env, exports, sizeof(methods) / sizeof(methods[0]), methods));

  return exports;
}

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
