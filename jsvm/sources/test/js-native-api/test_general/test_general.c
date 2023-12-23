// we define NODE_API_NO_EXTERNAL_BUFFERS_ALLOWED here to
// validate that it can be used as a form of test itself. It is
// not related to any of the other tests
// defined in the file
#define NODE_API_NO_EXTERNAL_BUFFERS_ALLOWED
#include <jsvm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "../common.h"
#include "../entry_point.h"

static JSVM_Value testStrictEquals(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  bool bool_result;
  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_StrictEquals(env, args[0], args[1], &bool_result));
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, bool_result, &result));

  return result;
}

static JSVM_Value testGetPrototype(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_GetPrototype(env, args[0], &result));

  return result;
}

static JSVM_Value testGetVersion(JSVM_Env env, JSVM_CallbackInfo info) {
  uint32_t version;
  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_GetVersion(env, &version));
  NODE_API_CALL(env, OH_JSVM_CreateUint32(env, version, &result));
  return result;
}

static JSVM_Value doInstanceOf(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  bool instanceof;
  NODE_API_CALL(env, OH_JSVM_Instanceof(env, args[0], args[1], &instanceof));

  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, instanceof, &result));

  return result;
}

static JSVM_Value getNull(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_GetNull(env, &result));
  return result;
}

static JSVM_Value getUndefined(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_GetUndefined(env, &result));
  return result;
}

static JSVM_Value createNapiError(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value value;
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(env, "xyz", 3, &value));

  double double_value;
  JSVM_Status status = OH_JSVM_GetValueDouble(env, value, &double_value);

  NODE_API_ASSERT(env, status != JSVM_OK, "Failed to produce error condition");

  const JSVM_ExtendedErrorInfo *error_info = 0;
  NODE_API_CALL(env, OH_JSVM_GetLastErrorInfo(env, &error_info));

  NODE_API_ASSERT(env, error_info->error_code == status,
      "Last error info code should match last status");
  NODE_API_ASSERT(env, error_info->error_message,
      "Last error info message should not be null");

  return NULL;
}

static JSVM_Value testNapiErrorCleanup(JSVM_Env env, JSVM_CallbackInfo info) {
  const JSVM_ExtendedErrorInfo *error_info = 0;
  NODE_API_CALL(env, OH_JSVM_GetLastErrorInfo(env, &error_info));

  JSVM_Value result;
  bool is_ok = error_info->error_code == JSVM_OK;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, is_ok, &result));

  return result;
}

static JSVM_Value testNapiTypeof(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_ValueType argument_type;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &argument_type));

  JSVM_Value result = NULL;
  if (argument_type == jsvm_number) {
    NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
        env, "number", JSVM_AUTO_LENGTH, &result));
  } else if (argument_type == JSVM_STRING) {
    NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
        env, "string", JSVM_AUTO_LENGTH, &result));
  } else if (argument_type == JSVM_FUNCTION) {
    NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
        env, "function", JSVM_AUTO_LENGTH, &result));
  } else if (argument_type == JSVM_OBJECT) {
    NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
        env, "object", JSVM_AUTO_LENGTH, &result));
  } else if (argument_type == JSVM_BOOLEAN) {
    NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
        env, "boolean", JSVM_AUTO_LENGTH, &result));
  } else if (argument_type == JSVM_UNDEFINED) {
    NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
        env, "undefined", JSVM_AUTO_LENGTH, &result));
  } else if (argument_type == JSVM_SYMBOL) {
    NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
        env, "symbol", JSVM_AUTO_LENGTH, &result));
  } else if (argument_type == JSVM_NULL) {
    NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
        env, "null", JSVM_AUTO_LENGTH, &result));
  }
  return result;
}

static bool deref_item_called = false;
static void deref_item(JSVM_Env env, void* data, void* hint) {
  (void) hint;

  NODE_API_ASSERT_RETURN_VOID(env, data == &deref_item_called,
    "Finalize callback was called with the correct pointer");

  deref_item_called = true;
}

static JSVM_Value deref_item_was_called(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value it_was_called;

  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, deref_item_called, &it_was_called));

  return it_was_called;
}

static JSVM_Value wrap_first_arg(JSVM_Env env,
                                 JSVM_CallbackInfo info,
                                 JSVM_Finalize finalizer,
                                 void* data) {
  size_t argc = 1;
  JSVM_Value to_wrap;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &to_wrap, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_Wrap(env, to_wrap, data, finalizer, NULL, NULL));

  return to_wrap;
}

static JSVM_Value wrap(JSVM_Env env, JSVM_CallbackInfo info) {
  deref_item_called = false;
  return wrap_first_arg(env, info, deref_item, &deref_item_called);
}

static JSVM_Value unwrap(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value wrapped;
  void* data;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &wrapped, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_Unwrap(env, wrapped, &data));

  return NULL;
}

static JSVM_Value remove_wrap(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value wrapped;
  void* data;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &wrapped, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_RemoveWrap(env, wrapped, &data));

  return NULL;
}

static bool finalize_called = false;
static void test_finalize(JSVM_Env env, void* data, void* hint) {
  finalize_called = true;
}

static JSVM_Value test_finalize_wrap(JSVM_Env env, JSVM_CallbackInfo info) {
  return wrap_first_arg(env, info, test_finalize, NULL);
}

static JSVM_Value finalize_was_called(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value it_was_called;

  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, finalize_called, &it_was_called));

  return it_was_called;
}

static JSVM_Value testAdjustExternalMemory(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  int64_t adjustedValue;

  NODE_API_CALL(env, OH_JSVM_AdjustExternalMemory(env, 1, &adjustedValue));
  NODE_API_CALL(env, OH_JSVM_CreateDouble(env, (double)adjustedValue, &result));

  return result;
}

static JSVM_Value testNapiRun(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value script, result;
  size_t argc = 1;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &script, NULL, NULL));

  NODE_API_CALL(env, OH_JSVMRunScript(env, script, &result));

  return result;
}

static void finalizer_only_callback(JSVM_Env env, void* data, void* hint) {
  JSVM_Ref js_cb_ref = data;
  JSVM_Value js_cb, undefined;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetReferenceValue(env, js_cb_ref, &js_cb));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetUndefined(env, &undefined));
  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_CallFunction(env, undefined, js_cb, 0, NULL, NULL));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, js_cb_ref));
}

static JSVM_Value add_finalizer_only(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value argv[2];
  JSVM_Ref js_cb_ref;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_CreateReference(env, argv[1], 1, &js_cb_ref));
  NODE_API_CALL(env,
      OH_JSVM_AddFinalizer(
          env, argv[0], js_cb_ref, finalizer_only_callback, NULL, NULL));
  return NULL;
}

static const char* env_cleanup_finalizer_messages[] = {
  "simple wrap",
  "wrap, removeWrap",
  "first wrap",
  "second wrap"
};

static void cleanup_env_finalizer(JSVM_Env env, void* data, void* hint) {
  (void) env;
  (void) hint;

  printf("finalize at env cleanup for %s\n",
      env_cleanup_finalizer_messages[(uintptr_t)data]);
}

static JSVM_Value env_cleanup_wrap(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value argv[2];
  uint32_t value;
  uintptr_t ptr_value;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));

  NODE_API_CALL(env, OH_JSVM_GetValueUint32(env, argv[1], &value));

  ptr_value = value;
  return wrap_first_arg(env, info, cleanup_env_finalizer, (void*)ptr_value);
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("testStrictEquals", testStrictEquals),
    DECLARE_NODE_API_PROPERTY("testGetPrototype", testGetPrototype),
    DECLARE_NODE_API_PROPERTY("testGetVersion", testGetVersion),
    DECLARE_NODE_API_PROPERTY("testNapiRun", testNapiRun),
    DECLARE_NODE_API_PROPERTY("doInstanceOf", doInstanceOf),
    DECLARE_NODE_API_PROPERTY("getUndefined", getUndefined),
    DECLARE_NODE_API_PROPERTY("getNull", getNull),
    DECLARE_NODE_API_PROPERTY("createNapiError", createNapiError),
    DECLARE_NODE_API_PROPERTY("testNapiErrorCleanup", testNapiErrorCleanup),
    DECLARE_NODE_API_PROPERTY("testNapiTypeof", testNapiTypeof),
    DECLARE_NODE_API_PROPERTY("wrap", wrap),
    DECLARE_NODE_API_PROPERTY("envCleanupWrap", env_cleanup_wrap),
    DECLARE_NODE_API_PROPERTY("unwrap", unwrap),
    DECLARE_NODE_API_PROPERTY("removeWrap", remove_wrap),
    DECLARE_NODE_API_PROPERTY("addFinalizerOnly", add_finalizer_only),
    DECLARE_NODE_API_PROPERTY("testFinalizeWrap", test_finalize_wrap),
    DECLARE_NODE_API_PROPERTY("finalizeWasCalled", finalize_was_called),
    DECLARE_NODE_API_PROPERTY("derefItemWasCalled", deref_item_was_called),
    DECLARE_NODE_API_PROPERTY("testAdjustExternalMemory", testAdjustExternalMemory)
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  return exports;
}
EXTERN_C_END
