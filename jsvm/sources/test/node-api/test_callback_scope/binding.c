#include <stdlib.h>
#include "node_api.h"
#include "uv.h"
#include "../../js-native-api/common.h"

static JSVM_Value RunInCallbackScope(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc;
  JSVM_Value args[3];

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, NULL, NULL, NULL));
  NODE_API_ASSERT(env, argc == 3 , "Wrong number of arguments");

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_ValueType valuetype;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype));
  NODE_API_ASSERT(env, valuetype == jsvm_object,
      "Wrong type of arguments. Expects an object as first argument.");

  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &valuetype));
  NODE_API_ASSERT(env, valuetype == jsvm_string,
      "Wrong type of arguments. Expects a string as second argument.");

  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[2], &valuetype));
  NODE_API_ASSERT(env, valuetype == jsvm_function,
      "Wrong type of arguments. Expects a function as third argument.");

  jsvm_async_context context;
  NODE_API_CALL(env, jsvm_async_init(env, args[0], args[1], &context));

  jsvm_callback_scope scope = NULL;
  NODE_API_CALL(env,
      jsvm_open_callback_scope(env, args[0], context, &scope));

  // If the function has an exception pending after the call that is ok
  // so we don't use NODE_API_CALL as we must close the callback scope
  // regardless.
  JSVM_Value result = NULL;
  JSVM_Status function_call_result =
      OH_JSVM_CallFunction(env, args[0], args[2], 0, NULL, &result);
  if (function_call_result != JSVM_OK) {
    GET_AND_THROW_LAST_ERROR((env));
  }

  NODE_API_CALL(env, jsvm_close_callback_scope(env, scope));
  NODE_API_CALL(env, jsvm_async_destroy(env, context));

  return result;
}

static JSVM_Env shared_env = NULL;
static JSVM_Deferred deferred = NULL;

static void Callback(uv_work_t* req, int ignored) {
  JSVM_Env env = shared_env;

  JSVM_HandleScope handle_scope = NULL;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_OpenHandleScope(env, &handle_scope));

  JSVM_Value resource_name;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_CreateStringUtf8(
      env, "test", JSVM_AUTO_LENGTH, &resource_name));
  jsvm_async_context context;
  NODE_API_CALL_RETURN_VOID(env,
      jsvm_async_init(env, NULL, resource_name, &context));

  JSVM_Value resource_object;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_CreateObject(env, &resource_object));

  JSVM_Value undefined_value;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetUndefined(env, &undefined_value));

  jsvm_callback_scope scope = NULL;
  NODE_API_CALL_RETURN_VOID(env,
      jsvm_open_callback_scope(env, resource_object, context, &scope));

  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_ResolveDeferred(env, deferred, undefined_value));

  NODE_API_CALL_RETURN_VOID(env, jsvm_close_callback_scope(env, scope));

  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_CloseHandleScope(env, handle_scope));
  NODE_API_CALL_RETURN_VOID(env, jsvm_async_destroy(env, context));
  free(req);
}

static void NoopWork(uv_work_t* work) { (void) work; }

static JSVM_Value TestResolveAsync(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value promise = NULL;
  if (deferred == NULL) {
    shared_env = env;
    NODE_API_CALL(env, OH_JSVM_CreatePromise(env, &deferred, &promise));

    uv_loop_t* loop = NULL;
    NODE_API_CALL(env, jsvm_get_uv_event_loop(env, &loop));

    uv_work_t* req = malloc(sizeof(*req));
    uv_queue_work(loop,
                  req,
                  NoopWork,
                  Callback);
  }
  return promise;
}

static JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("runInCallbackScope", RunInCallbackScope),
    DECLARE_NODE_API_PROPERTY("testResolveAsync", TestResolveAsync)
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  return exports;
}

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
