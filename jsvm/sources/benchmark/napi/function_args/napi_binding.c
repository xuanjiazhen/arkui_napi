#include <node_api.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static JSVM_Value CallWithString(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Status status;

  size_t argc = 1;
  JSVM_Value args[1];
  status = OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL);
  assert(status == JSVM_OK);

  JSVM_ValueType types[1];
  status = OH_JSVM_Typeof(env, args[0], types);
  assert(status == JSVM_OK);

  assert(types[0] == jsvm_string);
  if (types[0] == jsvm_string) {
    size_t len = 0;
    // Get the length
    status = OH_JSVM_GetValueStringUtf8(env, args[0], NULL, 0, &len);
    assert(status == JSVM_OK);
    char* buf = (char*)malloc(len + 1);
    status = OH_JSVM_GetValueStringUtf8(env, args[0], buf, len + 1, &len);
    assert(status == JSVM_OK);
    free(buf);
  }

  return NULL;
}

static JSVM_Value CallWithArray(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Status status;

  size_t argc = 1;
  JSVM_Value args[1];
  status = OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL);
  assert(status == JSVM_OK);

  JSVM_Value array = args[0];
  bool is_array = false;
  status = OH_JSVM_IsArray(env, array, &is_array);
  assert(status == JSVM_OK);

  assert(is_array);
  if (is_array) {
    uint32_t length;
    status = OH_JSVM_GetArrayLength(env, array, &length);
    assert(status == JSVM_OK);

    uint32_t i;
    for (i = 0; i < length; ++i) {
      JSVM_Value v;
      status = OH_JSVM_GetElement(env, array, i, &v);
      assert(status == JSVM_OK);
    }
  }

  return NULL;
}

static JSVM_Value CallWithNumber(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Status status;

  size_t argc = 1;
  JSVM_Value args[1];
  status = OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL);
  assert(status == JSVM_OK);

  JSVM_ValueType types[1];
  status = OH_JSVM_Typeof(env, args[0], types);
  assert(status == JSVM_OK);

  assert(types[0] == jsvm_number);
  if (types[0] == jsvm_number) {
    double value = 0.0;
    status = OH_JSVM_GetValueDouble(env, args[0], &value);
    assert(status == JSVM_OK);
  }

  return NULL;
}

static JSVM_Value CallWithObject(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Status status;

  size_t argc = 1;
  JSVM_Value args[1];
  status = OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL);
  assert(status == JSVM_OK);

  JSVM_ValueType types[1];
  status = OH_JSVM_Typeof(env, args[0], types);
  assert(status == JSVM_OK);

  assert(argc == 1 && types[0] == jsvm_object);
  if (argc == 1 && types[0] == jsvm_object) {
    JSVM_Value value;

    status = OH_JSVM_GetNamedProperty(env, args[0], "map", &value);
    assert(status == JSVM_OK);

    status = OH_JSVM_GetNamedProperty(env, args[0], "operand", &value);
    assert(status == JSVM_OK);

    status = OH_JSVM_GetNamedProperty(env, args[0], "data", &value);
    assert(status == JSVM_OK);

    status = OH_JSVM_GetNamedProperty(env, args[0], "reduce", &value);
    assert(status == JSVM_OK);
  }

  return NULL;
}

static JSVM_Value CallWithTypedarray(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Status status;

  size_t argc = 1;
  JSVM_Value args[1];
  status = OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL);
  assert(status == JSVM_OK);

  bool is_typedarray = false;
  status = OH_JSVM_IsTypedarray(env, args[0], &is_typedarray);
  assert(status == JSVM_OK);

  assert(is_typedarray);
  if (is_typedarray) {
    JSVM_TypedarrayType type;
    JSVM_Value input_buffer;
    size_t byte_offset = 0;
    size_t length = 0;
    status = OH_JSVM_GetTypedarrayInfo(env, args[0], &type, &length,
        NULL, &input_buffer, &byte_offset);
    assert(status == JSVM_OK);
    assert(length > 0);

    void* data = NULL;
    size_t byte_length = 0;
    status = OH_JSVM_GetArraybufferInfo(env,
        input_buffer, &data, &byte_length);
    assert(status == JSVM_OK);

    uint32_t* input_integers = (uint32_t*)((uint8_t*)(data) + byte_offset);
    assert(input_integers);
  }

  return NULL;
}

static JSVM_Value CallWithArguments(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Status status;

  size_t argc = 1;
  JSVM_Value args[1000];
  // Get the length
  status = OH_JSVM_GetCbInfo(env, info, &argc, NULL, NULL, NULL);
  assert(status == JSVM_OK);

  status = OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL);
  assert(status == JSVM_OK);
  assert(argc <= 1000);

  JSVM_ValueType types[1];
  status = OH_JSVM_Typeof(env, args[0], types);
  assert(status == JSVM_OK);

  assert(argc > 1 && types[0] == jsvm_number);
  if (argc > 1 && types[0] == jsvm_number) {
    uint32_t loop = 0;
    status = OH_JSVM_GetValueUint32(env, args[0], &loop);
    assert(status == JSVM_OK);

    uint32_t i;
    for (i = 1; i < loop; ++i) {
      assert(i < argc);
      status = OH_JSVM_Typeof(env, args[i], types);
      assert(status == JSVM_OK);
      assert(types[0] == jsvm_number);

      uint32_t value = 0;
      status = OH_JSVM_GetValueUint32(env, args[i], &value);
      assert(status == JSVM_OK);
    }
  }

  return NULL;
}


#define EXPORT_FUNC(env, exports, name, func)       \
  do {                                              \
    JSVM_Status status;                             \
    JSVM_Value js_func;                             \
    status = OH_JSVM_CreateFunction((env),            \
                                  (name),           \
                                  JSVM_AUTO_LENGTH, \
                                  (func),           \
                                  NULL,             \
                                  &js_func);        \
    assert(status == JSVM_OK);                      \
    status = OH_JSVM_SetNamedProperty((env),         \
                                     (exports),     \
                                     (name),        \
                                     js_func);      \
    assert(status == JSVM_OK);                      \
  } while (0);


JSVM_MODULE_INIT() {
  EXPORT_FUNC(env, exports, "callWithString", CallWithString);
  EXPORT_FUNC(env, exports, "callWithLongString", CallWithString);

  EXPORT_FUNC(env, exports, "callWithArray", CallWithArray);
  EXPORT_FUNC(env, exports, "callWithLargeArray", CallWithArray);
  EXPORT_FUNC(env, exports, "callWithHugeArray", CallWithArray);

  EXPORT_FUNC(env, exports, "callWithNumber", CallWithNumber);

  EXPORT_FUNC(env, exports, "callWithObject", CallWithObject);
  EXPORT_FUNC(env, exports, "callWithTypedarray", CallWithTypedarray);

  EXPORT_FUNC(env, exports, "callWith10Numbers", CallWithArguments);
  EXPORT_FUNC(env, exports, "callWith100Numbers", CallWithArguments);
  EXPORT_FUNC(env, exports, "callWith1000Numbers", CallWithArguments);

  return exports;
}
