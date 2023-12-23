#include <jsvm.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common.h"
#include "../entry_point.h"

typedef struct {
  int32_t finalize_count;
  JSVM_Ref js_func;
} FinalizerData;

static void finalizerOnlyCallback(JSVM_Env env,
                                  void* finalize_data,
                                  void* finalize_hint) {
  FinalizerData* data = (FinalizerData*)finalize_data;
  int32_t count = ++data->finalize_count;

  // It is safe to access instance data
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetInstanceData(env, (void**)&data));
  NODE_API_ASSERT_RETURN_VOID(env,
                              count = data->finalize_count,
                              "Expected to be the same FinalizerData");
}

static void finalizerCallingJSCallback(JSVM_Env env,
                                       void* finalize_data,
                                       void* finalize_hint) {
  JSVM_Value js_func, undefined;
  FinalizerData* data = (FinalizerData*)finalize_data;
  NODE_API_CALL_RETURN_VOID(
      env, OH_JSVM_GetReferenceValue(env, data->js_func, &js_func));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetUndefined(env, &undefined));
  NODE_API_CALL_RETURN_VOID(
      env, OH_JSVM_CallFunction(env, undefined, js_func, 0, NULL, NULL));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, data->js_func));
  data->js_func = NULL;
  ++data->finalize_count;
}

// Schedule async finalizer to run JavaScript-touching code.
static void finalizerWithJSCallback(JSVM_Env env,
                                    void* finalize_data,
                                    void* finalize_hint) {
  NODE_API_CALL_RETURN_VOID(
      env,
      OH_JSVM_PostFinalizer(
          env, finalizerCallingJSCallback, finalize_data, finalize_hint));
}

static void finalizerWithFailedJSCallback(JSVM_Env env,
                                          void* finalize_data,
                                          void* finalize_hint) {
  JSVM_Value obj;
  FinalizerData* data = (FinalizerData*)finalize_data;
  ++data->finalize_count;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_CreateObject(env, &obj));
}

static JSVM_Value addFinalizer(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value argv[1] = {0};
  FinalizerData* data;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_GetInstanceData(env, (void**)&data));
  NODE_API_CALL(env,
                OH_JSVM_AddFinalizer(
                    env, argv[0], data, finalizerOnlyCallback, NULL, NULL));
  return NULL;
}

// This finalizer is going to call JavaScript from finalizer and succeed.
static JSVM_Value addFinalizerWithJS(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value argv[2] = {0};
  JSVM_ValueType arg_type;
  FinalizerData* data;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_GetInstanceData(env, (void**)&data));
  NODE_API_CALL(env, OH_JSVM_Typeof(env, argv[1], &arg_type));
  NODE_API_ASSERT(
      env, arg_type == jsvm_function, "Expected function as the second arg");
  NODE_API_CALL(env, OH_JSVM_CreateReference(env, argv[1], 1, &data->js_func));
  NODE_API_CALL(env,
                OH_JSVM_AddFinalizer(
                    env, argv[0], data, finalizerWithJSCallback, NULL, NULL));
  return NULL;
}

// This finalizer is going to call JavaScript from finalizer and fail.
static JSVM_Value addFinalizerFailOnJS(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value argv[1] = {0};
  FinalizerData* data;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_GetInstanceData(env, (void**)&data));
  NODE_API_CALL(
      env,
      OH_JSVM_AddFinalizer(
          env, argv[0], data, finalizerWithFailedJSCallback, NULL, NULL));
  return NULL;
}

static JSVM_Value getFinalizerCallCount(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value argv[1];
  FinalizerData* data;
  JSVM_Value result;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_GetInstanceData(env, (void**)&data));
  NODE_API_CALL(env, OH_JSVM_CreateInt32(env, data->finalize_count, &result));
  return result;
}

static void finalizeData(JSVM_Env env, void* data, void* hint) {
  free(data);
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  FinalizerData* data = (FinalizerData*)malloc(sizeof(FinalizerData));
  NODE_API_ASSERT(env, data != NULL, "Failed to allocate memory");
  memset(data, 0, sizeof(FinalizerData));
  NODE_API_CALL(env, OH_JSVM_SetInstanceData(env, data, finalizeData, NULL));
  JSVM_PropertyDescriptor descriptors[] = {
      DECLARE_NODE_API_PROPERTY("addFinalizer", addFinalizer),
      DECLARE_NODE_API_PROPERTY("addFinalizerWithJS", addFinalizerWithJS),
      DECLARE_NODE_API_PROPERTY("addFinalizerFailOnJS", addFinalizerFailOnJS),
      DECLARE_NODE_API_PROPERTY("getFinalizerCallCount",
                                getFinalizerCallCount)};

  NODE_API_CALL(
      env,
      OH_JSVM_DefineProperties(env,
                             exports,
                             sizeof(descriptors) / sizeof(*descriptors),
                             descriptors));

  return exports;
}
EXTERN_C_END
