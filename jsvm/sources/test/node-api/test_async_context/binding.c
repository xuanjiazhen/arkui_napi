#include <node_api.h>
#include <assert.h>
#include "../../js-native-api/common.h"

#define MAX_ARGUMENTS 10
#define RESERVED_ARGS 3

static JSVM_Value MakeCallback(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = MAX_ARGUMENTS;
  size_t n;
  JSVM_Value args[MAX_ARGUMENTS];
  // NOLINTNEXTLINE (readability/null_usage)
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc > 0, "Wrong number of arguments");

  JSVM_Value async_context_wrap = args[0];
  JSVM_Value recv = args[1];
  JSVM_Value func = args[2];

  JSVM_Value argv[MAX_ARGUMENTS - RESERVED_ARGS];
  for (n = RESERVED_ARGS; n < argc; n += 1) {
    argv[n - RESERVED_ARGS] = args[n];
  }

  JSVM_ValueType func_type;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, func, &func_type));

  jsvm_async_context context;
  NODE_API_CALL(env, OH_JSVM_Unwrap(env, async_context_wrap, (void**)&context));

  JSVM_Value result;
  if (func_type == jsvm_function) {
    NODE_API_CALL(env, jsvm_make_callback(
        env, context, recv, func, argc - RESERVED_ARGS, argv, &result));
  } else {
    NODE_API_ASSERT(env, false, "Unexpected argument type");
  }

  return result;
}

static void AsyncDestroyCb(JSVM_Env env, void* data, void* hint) {
  JSVM_Status status = jsvm_async_destroy(env, (jsvm_async_context)data);
  // We cannot use NODE_API_ASSERT_RETURN_VOID because we need to have a JS
  // stack below in order to use exceptions.
  assert(status == JSVM_OK);
}

#define CREATE_ASYNC_RESOURCE_ARGC 2

static JSVM_Value CreateAsyncResource(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value async_context_wrap;
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &async_context_wrap));

  size_t argc = CREATE_ASYNC_RESOURCE_ARGC;
  JSVM_Value args[CREATE_ASYNC_RESOURCE_ARGC];
  // NOLINTNEXTLINE (readability/null_usage)
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value resource = args[0];
  JSVM_Value js_destroy_on_finalizer = args[1];
  JSVM_ValueType resource_type;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, resource, &resource_type));
  if (resource_type != jsvm_object) {
    resource = NULL;
  }

  JSVM_Value resource_name;
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
      env, "test_async", JSVM_AUTO_LENGTH, &resource_name));

  jsvm_async_context context;
  NODE_API_CALL(env, jsvm_async_init(env, resource, resource_name, &context));

  bool destroy_on_finalizer = true;
  if (argc == 2) {
    NODE_API_CALL(env, OH_JSVM_GetValueBool(env, js_destroy_on_finalizer, &destroy_on_finalizer));
  }
  if (resource_type == jsvm_object && destroy_on_finalizer) {
    NODE_API_CALL(env, OH_JSVM_AddFinalizer(
        env, resource, (void*)context, AsyncDestroyCb, NULL, NULL));
  }
  NODE_API_CALL(env, OH_JSVM_Wrap(env, async_context_wrap, context, NULL, NULL, NULL));
  return async_context_wrap;
}

#define DESTROY_ASYNC_RESOURCE_ARGC 1

static JSVM_Value DestroyAsyncResource(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = DESTROY_ASYNC_RESOURCE_ARGC;
  JSVM_Value args[DESTROY_ASYNC_RESOURCE_ARGC];
  // NOLINTNEXTLINE (readability/null_usage)
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));
  NODE_API_ASSERT(env, argc == 1, "Wrong number of arguments");

  JSVM_Value async_context_wrap = args[0];

  jsvm_async_context async_context;
  NODE_API_CALL(env,
            OH_JSVM_RemoveWrap(env, async_context_wrap, (void**)&async_context));
  NODE_API_CALL(env, jsvm_async_destroy(env, async_context));

  return async_context_wrap;
}

static
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_Value fn;
  NODE_API_CALL(env, OH_JSVM_CreateFunction(
      // NOLINTNEXTLINE (readability/null_usage)
      env, NULL, JSVM_AUTO_LENGTH, MakeCallback, NULL, &fn));
  NODE_API_CALL(env, OH_JSVM_SetNamedProperty(env, exports, "makeCallback", fn));
  NODE_API_CALL(env, OH_JSVM_CreateFunction(
      // NOLINTNEXTLINE (readability/null_usage)
      env, NULL, JSVM_AUTO_LENGTH, CreateAsyncResource, NULL, &fn));
  NODE_API_CALL(env, OH_JSVM_SetNamedProperty(
      env, exports, "createAsyncResource", fn));

  NODE_API_CALL(env, OH_JSVM_CreateFunction(
      // NOLINTNEXTLINE (readability/null_usage)
      env, NULL, JSVM_AUTO_LENGTH, DestroyAsyncResource, NULL, &fn));
  NODE_API_CALL(env, OH_JSVM_SetNamedProperty(
      env, exports, "destroyAsyncResource", fn));

  return exports;
}

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
