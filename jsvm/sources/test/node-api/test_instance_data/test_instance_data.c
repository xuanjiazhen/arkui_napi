#include <stdlib.h>
#include <uv.h>
#include <node_api.h>
#include "../../js-native-api/common.h"

typedef struct {
  JSVM_Ref js_cb_ref;
  JSVM_Ref js_tsfn_finalizer_ref;
  jsvm_threadsafe_function tsfn;
  uv_thread_t thread;
} AddonData;

static void AsyncWorkCbExecute(JSVM_Env env, void* data) {
  (void) env;
  (void) data;
}

static void call_cb_and_delete_ref(JSVM_Env env, JSVM_Ref* optional_ref) {
  JSVM_Value js_cb, undefined;

  if (optional_ref == NULL) {
    AddonData* data;
    NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetInstanceData(env, (void**)&data));
    optional_ref = &data->js_cb_ref;
  }

  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_GetReferenceValue(env, *optional_ref, &js_cb));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetUndefined(env, &undefined));
  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_CallFunction(env, undefined, js_cb, 0, NULL, NULL));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, *optional_ref));

  *optional_ref = NULL;
}

static void AsyncWorkCbComplete(JSVM_Env env,
                                   JSVM_Status status,
                                   void* data) {
  (void) status;
  (void) data;
  call_cb_and_delete_ref(env, NULL);
}

static bool establish_callback_ref(JSVM_Env env, JSVM_CallbackInfo info) {
  AddonData* data;
  size_t argc = 1;
  JSVM_Value js_cb;

  NODE_API_CALL_BASE(env, OH_JSVM_GetInstanceData(env, (void**)&data), false);
  NODE_API_ASSERT_BASE(
      env, data->js_cb_ref == NULL, "reference must be NULL", false);
  NODE_API_CALL_BASE(
      env, OH_JSVM_GetCbInfo(env, info, &argc, &js_cb, NULL, NULL), false);
  NODE_API_CALL_BASE(
      env, OH_JSVM_CreateReference(env, js_cb, 1, &data->js_cb_ref), false);

  return true;
}

static JSVM_Value AsyncWorkCallback(JSVM_Env env, JSVM_CallbackInfo info) {
  if (establish_callback_ref(env, info)) {
    JSVM_Value resource_name;
    jsvm_async_work work;

    NODE_API_CALL(env,
        OH_JSVM_CreateStringUtf8(
            env, "AsyncIncrement", JSVM_AUTO_LENGTH, &resource_name));
    NODE_API_CALL(env,
        jsvm_create_async_work(
            env, NULL, resource_name, AsyncWorkCbExecute, AsyncWorkCbComplete,
            NULL, &work));
    NODE_API_CALL(env, jsvm_queue_async_work(env, work));
  }

  return NULL;
}

static void TestBufferFinalizerCallback(JSVM_Env env, void* data, void* hint) {
  (void) data;
  (void) hint;
  call_cb_and_delete_ref(env, NULL);
}

static JSVM_Value TestBufferFinalizer(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value buffer = NULL;
  if (establish_callback_ref(env, info)) {
    NODE_API_CALL(env,
        OH_JSVM_CreateExternal_buffer(
            env, sizeof(JSVM_Callback), TestBufferFinalizer,
            TestBufferFinalizerCallback, NULL, &buffer));
  }
  return buffer;
}

static void ThreadsafeFunctionCallJS(JSVM_Env env,
                                     JSVM_Value tsfn_cb,
                                     void* context,
                                     void* data) {
  (void) tsfn_cb;
  (void) context;
  (void) data;
  call_cb_and_delete_ref(env, NULL);
}

static void ThreadsafeFunctionTestThread(void* raw_data) {
  AddonData* data = raw_data;
  JSVM_Status status;

  // No need to call `jsvm_acquire_threadsafe_function()` because the main
  // thread has set the refcount to 1 and there is only this one secondary
  // thread.
  status = jsvm_call_threadsafe_function(data->tsfn,
                                         ThreadsafeFunctionCallJS,
                                         jsvm_tsfn_nonblocking);
  if (status != JSVM_OK) {
    jsvm_fatal_error("ThreadSafeFunctionTestThread",
                     JSVM_AUTO_LENGTH,
                     "Failed to call TSFN",
                     JSVM_AUTO_LENGTH);
  }

  status = jsvm_release_threadsafe_function(data->tsfn, jsvm_tsfn_release);
  if (status != JSVM_OK) {
    jsvm_fatal_error("ThreadSafeFunctionTestThread",
                     JSVM_AUTO_LENGTH,
                     "Failed to release TSFN",
                     JSVM_AUTO_LENGTH);
  }

}

static void FinalizeThreadsafeFunction(JSVM_Env env, void* raw, void* hint) {
  AddonData* data;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetInstanceData(env, (void**)&data));
  NODE_API_ASSERT_RETURN_VOID(env,
      uv_thread_join(&data->thread) == 0, "Failed to join the thread");
  call_cb_and_delete_ref(env, &data->js_tsfn_finalizer_ref);
  data->tsfn = NULL;
}

// Ths function accepts two arguments: the JS callback, and the finalize
// callback. The latter moves the test forward.
static JSVM_Value
TestThreadsafeFunction(JSVM_Env env, JSVM_CallbackInfo info) {
  AddonData* data;
  size_t argc = 2;
  JSVM_Value argv[2], resource_name;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_GetInstanceData(env, (void**)&data));
  NODE_API_ASSERT(env, data->js_cb_ref == NULL, "reference must be NULL");
  NODE_API_ASSERT(
      env, data->js_tsfn_finalizer_ref == NULL,
      "tsfn finalizer reference must be NULL");
  NODE_API_CALL(env, OH_JSVM_CreateReference(env, argv[0], 1, &data->js_cb_ref));
  NODE_API_CALL(env,
      OH_JSVM_CreateReference(env, argv[1], 1, &data->js_tsfn_finalizer_ref));
  NODE_API_CALL(env,
      OH_JSVM_CreateStringUtf8(
          env, "TSFN instance data test", JSVM_AUTO_LENGTH, &resource_name));
  NODE_API_CALL(env,
      jsvm_create_threadsafe_function(
          env, NULL, NULL, resource_name, 0, 1, NULL,
          FinalizeThreadsafeFunction, NULL, ThreadsafeFunctionCallJS,
          &data->tsfn));
  NODE_API_ASSERT(env,
      uv_thread_create(&data->thread, ThreadsafeFunctionTestThread, data) == 0,
      "uv_thread_create failed");

  return NULL;
}

static void DeleteAddonData(JSVM_Env env, void* raw_data, void* hint) {
  AddonData* data = raw_data;
  if (data->js_cb_ref) {
    NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, data->js_cb_ref));
  }
  if (data->js_tsfn_finalizer_ref) {
    NODE_API_CALL_RETURN_VOID(env,
        OH_JSVM_DeleteReference(env, data->js_tsfn_finalizer_ref));
  }
  free(data);
}

static JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  AddonData* data = malloc(sizeof(*data));
  data->js_cb_ref = NULL;
  data->js_tsfn_finalizer_ref = NULL;

  NODE_API_CALL(env, OH_JSVM_SetInstanceData(env, data, DeleteAddonData, NULL));

  JSVM_PropertyDescriptor props[] = {
    DECLARE_NODE_API_PROPERTY("asyncWorkCallback", AsyncWorkCallback),
    DECLARE_NODE_API_PROPERTY("testBufferFinalizer", TestBufferFinalizer),
    DECLARE_NODE_API_PROPERTY("testThreadsafeFunction", TestThreadsafeFunction),
  };

  NODE_API_CALL(env,
      OH_JSVM_DefineProperties(
          env, exports, sizeof(props) / sizeof(*props), props));

  return exports;
}

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
