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

  JSVM_Value resource = args[0];
  JSVM_Value recv = args[1];
  JSVM_Value func = args[2];

  JSVM_Value argv[MAX_ARGUMENTS - RESERVED_ARGS];
  for (n = RESERVED_ARGS; n < argc; n += 1) {
    argv[n - RESERVED_ARGS] = args[n];
  }

  JSVM_ValueType func_type;

  NODE_API_CALL(env, OH_JSVM_Typeof(env, func, &func_type));

  JSVM_Value resource_name;
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
      env, "test", JSVM_AUTO_LENGTH, &resource_name));

  jsvm_async_context context;
  NODE_API_CALL(env, jsvm_async_init(env, resource, resource_name, &context));

  JSVM_Value result;
  if (func_type == jsvm_function) {
    NODE_API_CALL(env, jsvm_make_callback(
        env, context, recv, func, argc - RESERVED_ARGS, argv, &result));
  } else {
    NODE_API_ASSERT(env, false, "Unexpected argument type");
  }

  NODE_API_CALL(env, jsvm_async_destroy(env, context));

  return result;
}

static
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_Value fn;
  NODE_API_CALL(env, OH_JSVM_CreateFunction(
      // NOLINTNEXTLINE (readability/null_usage)
      env, NULL, JSVM_AUTO_LENGTH, MakeCallback, NULL, &fn));
  NODE_API_CALL(env, OH_JSVM_SetNamedProperty(env, exports, "makeCallback", fn));
  return exports;
}

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
