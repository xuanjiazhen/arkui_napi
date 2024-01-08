#include <node_api.h>
#include "../../js-native-api/common.h"

// Testing calling into JavaScript
static void ThreadSafeFunctionFinalize(JSVM_Env env,
                                       void* finalize_data,
                                       void* finalize_hint) {
  JSVM_Ref js_func_ref = (JSVM_Ref)finalize_data;
  JSVM_Value js_func;
  JSVM_Value recv;
  NODE_API_CALL_RETURN_VOID(
      env, OH_JSVM_GetReferenceValue(env, js_func_ref, &js_func));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetGlobal(env, &recv));
  NODE_API_CALL_RETURN_VOID(
      env, OH_JSVM_CallFunction(env, recv, js_func, 0, NULL, NULL));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, js_func_ref));
}

// Testing calling into JavaScript
static JSVM_Value CallIntoModule(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 4;
  JSVM_Value argv[4];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));

  JSVM_Ref finalize_func;
  NODE_API_CALL(env, OH_JSVM_CreateReference(env, argv[3], 1, &finalize_func));

  jsvm_threadsafe_function tsfn;
  NODE_API_CALL(env,
                jsvm_create_threadsafe_function(env,
                                                argv[0],
                                                argv[1],
                                                argv[2],
                                                0,
                                                1,
                                                finalize_func,
                                                ThreadSafeFunctionFinalize,
                                                NULL,
                                                NULL,
                                                &tsfn));
  NODE_API_CALL(env,
                jsvm_call_threadsafe_function(tsfn, NULL, jsvm_tsfn_blocking));
  NODE_API_CALL(env, jsvm_release_threadsafe_function(tsfn, jsvm_tsfn_release));
  return NULL;
}

// Module init
static JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor properties[] = {
      DECLARE_NODE_API_PROPERTY("CallIntoModule", CallIntoModule),
  };

  NODE_API_CALL(
      env,
      OH_JSVM_DefineProperties(env,
                             exports,
                             sizeof(properties) / sizeof(properties[0]),
                             properties));

  return exports;
}
JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
