#include "node_api.h"
#include "assert.h"
#include "uv.h"
#include <stdlib.h>
#include "../../js-native-api/common.h"

static int cleanup_hook_count = 0;
static void MustNotCall(jsvm_async_cleanup_hook_handle hook, void* arg) {
  assert(0);
}

struct AsyncData {
  uv_async_t async;
  JSVM_Env env;
  jsvm_async_cleanup_hook_handle handle;
};

static struct AsyncData* CreateAsyncData() {
  struct AsyncData* data = (struct AsyncData*) malloc(sizeof(struct AsyncData));
  data->handle = NULL;
  return data;
}

static void AfterCleanupHookTwo(uv_handle_t* handle) {
  cleanup_hook_count++;
  struct AsyncData* data = (struct AsyncData*) handle->data;
  JSVM_Status status = jsvm_remove_async_cleanup_hook(data->handle);
  assert(status == JSVM_OK);
  free(data);
}

static void AfterCleanupHookOne(uv_async_t* async) {
  cleanup_hook_count++;
  uv_close((uv_handle_t*) async, AfterCleanupHookTwo);
}

static void AsyncCleanupHook(jsvm_async_cleanup_hook_handle handle, void* arg) {
  cleanup_hook_count++;
  struct AsyncData* data = (struct AsyncData*) arg;
  uv_loop_t* loop;
  JSVM_Status status = jsvm_get_uv_event_loop(data->env, &loop);
  assert(status == JSVM_OK);
  int err = uv_async_init(loop, &data->async, AfterCleanupHookOne);
  assert(err == 0);

  data->async.data = data;
  data->handle = handle;
  uv_async_send(&data->async);
}

static void ObjectFinalizer(JSVM_Env env, void* data, void* hint) {
  // AsyncCleanupHook and its subsequent callbacks are called twice.
  assert(cleanup_hook_count == 6);

  JSVM_Ref* ref = data;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, *ref));
  free(ref);
}

static void CreateObjectWrap(JSVM_Env env) {
  JSVM_Value js_obj;
  JSVM_Ref* ref = malloc(sizeof(*ref));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_CreateObject(env, &js_obj));
  NODE_API_CALL_RETURN_VOID(
      env, OH_JSVM_Wrap(env, js_obj, ref, ObjectFinalizer, NULL, ref));
  // create a strong reference so that the finalizer is called at shutdown.
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_ReferenceRef(env, *ref, NULL));
}

static JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  // Reinitialize the static variable to be compatible with musl libc.
  cleanup_hook_count = 0;
  // Create object wrap before cleanup hooks.
  CreateObjectWrap(env);

  {
    struct AsyncData* data = CreateAsyncData();
    data->env = env;
    jsvm_add_async_cleanup_hook(env, AsyncCleanupHook, data, &data->handle);
  }

  {
    struct AsyncData* data = CreateAsyncData();
    data->env = env;
    jsvm_add_async_cleanup_hook(env, AsyncCleanupHook, data, NULL);
  }

  {
    jsvm_async_cleanup_hook_handle must_not_call_handle;
    jsvm_add_async_cleanup_hook(
        env, MustNotCall, NULL, &must_not_call_handle);
    jsvm_remove_async_cleanup_hook(must_not_call_handle);
  }

  // Create object wrap after cleanup hooks.
  CreateObjectWrap(env);

  return NULL;
}

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
