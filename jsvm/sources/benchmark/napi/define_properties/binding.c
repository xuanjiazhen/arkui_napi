#include <node_api.h>
#include <stdio.h>
#include <stdlib.h>

#define NODE_API_CALL(call)                                                    \
  do {                                                                         \
    JSVM_Status status = call;                                                 \
    if (status != JSVM_OK) {                                                   \
      fprintf(stderr, #call " failed: %d\n", status);                          \
      abort();                                                                 \
    }                                                                          \
  } while (0)

#define ABORT_IF_FALSE(condition)                                              \
  if (!(condition)) {                                                          \
    fprintf(stderr, #condition " failed\n");                                   \
    abort();                                                                   \
  }

static JSVM_Value Runner(JSVM_Env env,
                         JSVM_CallbackInfo info,
                         jsvm_property_attributes attr) {
  JSVM_Value argv[2], undefined, js_array_length, start, end;
  size_t argc = 2;
  JSVM_ValueType val_type = JSVM_UNDEFINED;
  bool is_array = false;
  uint32_t array_length = 0;
  JSVM_Value* native_array;

  // Validate params and retrieve start and end function.
  NODE_API_CALL(OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));
  ABORT_IF_FALSE(argc == 2);
  NODE_API_CALL(OH_JSVM_Typeof(env, argv[0], &val_type));
  ABORT_IF_FALSE(val_type == jsvm_object);
  NODE_API_CALL(OH_JSVM_IsArray(env, argv[1], &is_array));
  ABORT_IF_FALSE(is_array);
  NODE_API_CALL(OH_JSVM_GetArrayLength(env, argv[1], &array_length));
  NODE_API_CALL(OH_JSVM_GetNamedProperty(env, argv[0], "start", &start));
  NODE_API_CALL(OH_JSVM_Typeof(env, start, &val_type));
  ABORT_IF_FALSE(val_type == jsvm_function);
  NODE_API_CALL(OH_JSVM_GetNamedProperty(env, argv[0], "end", &end));
  NODE_API_CALL(OH_JSVM_Typeof(env, end, &val_type));
  ABORT_IF_FALSE(val_type == jsvm_function);

  NODE_API_CALL(OH_JSVM_GetUndefined(env, &undefined));
  NODE_API_CALL(OH_JSVM_CreateUint32(env, array_length, &js_array_length));

  // Copy objects into a native array.
  native_array = malloc(array_length * sizeof(*native_array));
  for (uint32_t idx = 0; idx < array_length; idx++) {
    NODE_API_CALL(OH_JSVM_GetElement(env, argv[1], idx, &native_array[idx]));
  }

  const JSVM_PropertyDescriptor desc = {
      "prop", NULL, NULL, NULL, NULL, js_array_length, attr, NULL};

  // Start the benchmark.
  OH_JSVM_CallFunction(env, argv[0], start, 0, NULL, NULL);

  for (uint32_t idx = 0; idx < array_length; idx++) {
    NODE_API_CALL(OH_JSVM_DefineProperties(env, native_array[idx], 1, &desc));
  }

  // Conclude the benchmark.
  NODE_API_CALL(
      OH_JSVM_CallFunction(env, argv[0], end, 1, &js_array_length, NULL));

  free(native_array);

  return undefined;
}

static JSVM_Value RunFastPath(JSVM_Env env, JSVM_CallbackInfo info) {
  return Runner(env, info, JSVM_WRITABLE | JSVM_ENUMERABLE | JSVM_CONFIGURABLE);
}

static JSVM_Value RunSlowPath(JSVM_Env env, JSVM_CallbackInfo info) {
  return Runner(env, info, JSVM_WRITABLE | JSVM_ENUMERABLE);
}

JSVM_MODULE_INIT() {
  JSVM_PropertyDescriptor props[] = {
      {"runFastPath",
       NULL,
       RunFastPath,
       NULL,
       NULL,
       NULL,
       JSVM_WRITABLE | JSVM_CONFIGURABLE | JSVM_ENUMERABLE,
       NULL},
      {"runSlowPath",
       NULL,
       RunSlowPath,
       NULL,
       NULL,
       NULL,
       JSVM_WRITABLE | JSVM_CONFIGURABLE | JSVM_ENUMERABLE,
       NULL},
  };

  NODE_API_CALL(OH_JSVM_DefineProperties(
      env, exports, sizeof(props) / sizeof(*props), props));
  return exports;
}
