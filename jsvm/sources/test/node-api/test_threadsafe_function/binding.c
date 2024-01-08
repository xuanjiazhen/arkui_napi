// For the purpose of this test we use libuv's threading library. When deciding
// on a threading library for a new project it bears remembering that in the
// future libuv may introduce API changes which may render it non-ABI-stable,
// which, in turn, may affect the ABI stability of the project despite its use
// of N-API.
#include <uv.h>
#include <node_api.h>
#include "../../js-native-api/common.h"

#define ARRAY_LENGTH 10000
#define MAX_QUEUE_SIZE 2

static uv_thread_t uv_threads[2];
static jsvm_threadsafe_function ts_fn;

typedef struct {
  jsvm_threadsafe_function_call_mode block_on_full;
  jsvm_threadsafe_function_release_mode abort;
  bool start_secondary;
  JSVM_Ref js_finalize_cb;
  uint32_t max_queue_size;
} ts_fn_hint;

static ts_fn_hint ts_info;

// Thread data to transmit to JS
static int ints[ARRAY_LENGTH];

static void secondary_thread(void* data) {
  jsvm_threadsafe_function ts_fn = data;

  if (jsvm_release_threadsafe_function(ts_fn, jsvm_tsfn_release) != JSVM_OK) {
    jsvm_fatal_error("secondary_thread", JSVM_AUTO_LENGTH,
        "jsvm_release_threadsafe_function failed", JSVM_AUTO_LENGTH);
  }
}

// Source thread producing the data
static void data_source_thread(void* data) {
  jsvm_threadsafe_function ts_fn = data;
  int index;
  void* hint;
  ts_fn_hint *ts_fn_info;
  JSVM_Status status;
  bool queue_was_full = false;
  bool queue_was_closing = false;

  if (jsvm_get_threadsafe_function_context(ts_fn, &hint) != JSVM_OK) {
    jsvm_fatal_error("data_source_thread", JSVM_AUTO_LENGTH,
        "jsvm_get_threadsafe_function_context failed", JSVM_AUTO_LENGTH);
  }

  ts_fn_info = (ts_fn_hint *)hint;

  if (ts_fn_info != &ts_info) {
    jsvm_fatal_error("data_source_thread", JSVM_AUTO_LENGTH,
      "thread-safe function hint is not as expected", JSVM_AUTO_LENGTH);
  }

  if (ts_fn_info->start_secondary) {
    if (jsvm_acquire_threadsafe_function(ts_fn) != JSVM_OK) {
      jsvm_fatal_error("data_source_thread", JSVM_AUTO_LENGTH,
        "jsvm_acquire_threadsafe_function failed", JSVM_AUTO_LENGTH);
    }

    if (uv_thread_create(&uv_threads[1], secondary_thread, ts_fn) != 0) {
      jsvm_fatal_error("data_source_thread", JSVM_AUTO_LENGTH,
        "failed to start secondary thread", JSVM_AUTO_LENGTH);
    }
  }

  for (index = ARRAY_LENGTH - 1; index > -1 && !queue_was_closing; index--) {
    status = jsvm_call_threadsafe_function(ts_fn, &ints[index],
        ts_fn_info->block_on_full);
    if (ts_fn_info->max_queue_size == 0 && (index % 1000 == 0)) {
      // Let's make this thread really busy for 200 ms to give the main thread a
      // chance to abort.
      uint64_t start = uv_hrtime();
      for (; uv_hrtime() - start < 200000000;);
    }
    switch (status) {
      case JSVM_QUEUE_FULL:
        queue_was_full = true;
        index++;
        // fall through

      case JSVM_OK:
        continue;

      case JSVM_CLOSING:
        queue_was_closing = true;
        break;

      default:
        jsvm_fatal_error("data_source_thread", JSVM_AUTO_LENGTH,
            "jsvm_call_threadsafe_function failed", JSVM_AUTO_LENGTH);
    }
  }

  // Assert that the enqueuing of a value was refused at least once, if this is
  // a non-blocking test run.
  if (!ts_fn_info->block_on_full && !queue_was_full) {
    jsvm_fatal_error("data_source_thread", JSVM_AUTO_LENGTH,
        "queue was never full", JSVM_AUTO_LENGTH);
  }

  // Assert that the queue was marked as closing at least once, if this is an
  // aborting test run.
  if (ts_fn_info->abort == jsvm_tsfn_abort && !queue_was_closing) {
    jsvm_fatal_error("data_source_thread", JSVM_AUTO_LENGTH,
      "queue was never closing", JSVM_AUTO_LENGTH);
  }

  if (!queue_was_closing &&
      jsvm_release_threadsafe_function(ts_fn, jsvm_tsfn_release) != JSVM_OK) {
    jsvm_fatal_error("data_source_thread", JSVM_AUTO_LENGTH,
        "jsvm_release_threadsafe_function failed", JSVM_AUTO_LENGTH);
  }
}

// Getting the data into JS
static void call_js(JSVM_Env env, JSVM_Value cb, void* hint, void* data) {
  if (!(env == NULL || cb == NULL)) {
    JSVM_Value argv, undefined;
    NODE_API_CALL_RETURN_VOID(env, OH_JSVM_CreateInt32(env, *(int*)data, &argv));
    NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetUndefined(env, &undefined));
    NODE_API_CALL_RETURN_VOID(env, OH_JSVM_CallFunction(env, undefined, cb, 1, &argv,
        NULL));
  }
}

static JSVM_Ref alt_ref;
// Getting the data into JS with the alternative reference
static void call_ref(JSVM_Env env, JSVM_Value _, void* hint, void* data) {
  if (!(env == NULL || alt_ref == NULL)) {
    JSVM_Value fn, argv, undefined;
    NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetReferenceValue(env, alt_ref, &fn));
    NODE_API_CALL_RETURN_VOID(env, OH_JSVM_CreateInt32(env, *(int*)data, &argv));
    NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetUndefined(env, &undefined));
    NODE_API_CALL_RETURN_VOID(env, OH_JSVM_CallFunction(env, undefined, fn, 1, &argv,
        NULL));
  }
}

// Cleanup
static JSVM_Value StopThread(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value argv[2];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));
  JSVM_ValueType value_type;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, argv[0], &value_type));
  NODE_API_ASSERT(env, value_type == jsvm_function,
      "StopThread argument is a function");
  NODE_API_ASSERT(env, (ts_fn != NULL), "Existing threadsafe function");
  NODE_API_CALL(env,
      OH_JSVM_CreateReference(env, argv[0], 1, &(ts_info.js_finalize_cb)));
  bool abort;
  NODE_API_CALL(env, OH_JSVM_GetValueBool(env, argv[1], &abort));
  NODE_API_CALL(env,
      jsvm_release_threadsafe_function(ts_fn,
          abort ? jsvm_tsfn_abort : jsvm_tsfn_release));
  ts_fn = NULL;
  return NULL;
}

// Join the thread and inform JS that we're done.
static void join_the_threads(JSVM_Env env, void *data, void *hint) {
  uv_thread_t *the_threads = data;
  ts_fn_hint *the_hint = hint;
  JSVM_Value js_cb, undefined;

  uv_thread_join(&the_threads[0]);
  if (the_hint->start_secondary) {
    uv_thread_join(&the_threads[1]);
  }

  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_GetReferenceValue(env, the_hint->js_finalize_cb, &js_cb));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetUndefined(env, &undefined));
  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_CallFunction(env, undefined, js_cb, 0, NULL, NULL));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env,
      the_hint->js_finalize_cb));
  if (alt_ref != NULL) {
    NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, alt_ref));
    alt_ref = NULL;
  }
}

static JSVM_Value StartThreadInternal(JSVM_Env env,
                                      JSVM_CallbackInfo info,
                                      jsvm_threadsafe_function_call_js cb,
                                      bool block_on_full,
                                      bool alt_ref_js_cb) {

  size_t argc = 4;
  JSVM_Value argv[4];

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));
  if (alt_ref_js_cb) {
    NODE_API_CALL(env, OH_JSVM_CreateReference(env, argv[0], 1, &alt_ref));
    argv[0] = NULL;
  }

  ts_info.block_on_full =
      (block_on_full ? jsvm_tsfn_blocking : jsvm_tsfn_nonblocking);

  NODE_API_ASSERT(env, (ts_fn == NULL), "Existing thread-safe function");
  JSVM_Value async_name;
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(env,
      "N-API Thread-safe Function Test", JSVM_AUTO_LENGTH, &async_name));
  NODE_API_CALL(env,
      OH_JSVM_GetValueUint32(env, argv[3], &ts_info.max_queue_size));
  NODE_API_CALL(env, jsvm_create_threadsafe_function(env,
                                                     argv[0],
                                                     NULL,
                                                     async_name,
                                                     ts_info.max_queue_size,
                                                     2,
                                                     uv_threads,
                                                     join_the_threads,
                                                     &ts_info,
                                                     cb,
                                                     &ts_fn));
  bool abort;
  NODE_API_CALL(env, OH_JSVM_GetValueBool(env, argv[1], &abort));
  ts_info.abort = abort ? jsvm_tsfn_abort : jsvm_tsfn_release;
  NODE_API_CALL(env,
      OH_JSVM_GetValueBool(env, argv[2], &(ts_info.start_secondary)));

  NODE_API_ASSERT(env,
      (uv_thread_create(&uv_threads[0], data_source_thread, ts_fn) == 0),
      "Thread creation");

  return NULL;
}

static JSVM_Value Unref(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_ASSERT(env, ts_fn != NULL, "No existing thread-safe function");
  NODE_API_CALL(env, jsvm_unref_threadsafe_function(env, ts_fn));
  return NULL;
}

static JSVM_Value Release(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_ASSERT(env, ts_fn != NULL, "No existing thread-safe function");
  NODE_API_CALL(env, jsvm_release_threadsafe_function(ts_fn, jsvm_tsfn_release));
  return NULL;
}

// Startup
static JSVM_Value StartThread(JSVM_Env env, JSVM_CallbackInfo info) {
  return StartThreadInternal(env, info, call_js,
    /** block_on_full */true, /** alt_ref_js_cb */false);
}

static JSVM_Value StartThreadNonblocking(JSVM_Env env,
                                         JSVM_CallbackInfo info) {
  return StartThreadInternal(env, info, call_js,
    /** block_on_full */false, /** alt_ref_js_cb */false);
}

static JSVM_Value StartThreadNoNative(JSVM_Env env, JSVM_CallbackInfo info) {
  return StartThreadInternal(env, info, NULL,
    /** block_on_full */true, /** alt_ref_js_cb */false);
}

static JSVM_Value StartThreadNoJsFunc(JSVM_Env env, JSVM_CallbackInfo info) {
  return StartThreadInternal(env, info, call_ref,
    /** block_on_full */true, /** alt_ref_js_cb */true);
}

// Testing calling into JavaScript
static void ThreadSafeFunctionFinalize(JSVM_Env env,
                              void* finalize_data,
                              void* finalize_hint) {
  JSVM_Ref js_func_ref = (JSVM_Ref) finalize_data;
  JSVM_Value js_func;
  JSVM_Value recv;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetReferenceValue(env, js_func_ref, &js_func));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetGlobal(env, &recv));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_CallFunction(env, recv, js_func, 0, NULL, NULL));
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
  NODE_API_CALL(env, jsvm_create_threadsafe_function(env, argv[0], argv[1], argv[2], 0, 1, finalize_func, ThreadSafeFunctionFinalize, NULL, NULL, &tsfn));
  NODE_API_CALL(env, jsvm_call_threadsafe_function(tsfn, NULL, jsvm_tsfn_blocking));
  NODE_API_CALL(env, jsvm_release_threadsafe_function(tsfn, jsvm_tsfn_release));
  return NULL;
}

// Module init
static JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  size_t index;
  for (index = 0; index < ARRAY_LENGTH; index++) {
    ints[index] = index;
  }
  JSVM_Value js_array_length, js_max_queue_size;
  OH_JSVM_CreateUint32(env, ARRAY_LENGTH, &js_array_length);
  OH_JSVM_CreateUint32(env, MAX_QUEUE_SIZE, &js_max_queue_size);

  JSVM_PropertyDescriptor properties[] = {
    {
      "ARRAY_LENGTH",
      NULL,
      NULL,
      NULL,
      NULL,
      js_array_length,
      JSVM_ENUMERABLE,
      NULL
    },
    {
      "MAX_QUEUE_SIZE",
      NULL,
      NULL,
      NULL,
      NULL,
      js_max_queue_size,
      JSVM_ENUMERABLE,
      NULL
    },
    DECLARE_NODE_API_PROPERTY("StartThread", StartThread),
    DECLARE_NODE_API_PROPERTY("StartThreadNoNative", StartThreadNoNative),
    DECLARE_NODE_API_PROPERTY("StartThreadNonblocking", StartThreadNonblocking),
    DECLARE_NODE_API_PROPERTY("StartThreadNoJsFunc", StartThreadNoJsFunc),
    DECLARE_NODE_API_PROPERTY("StopThread", StopThread),
    DECLARE_NODE_API_PROPERTY("Unref", Unref),
    DECLARE_NODE_API_PROPERTY("Release", Release),
    DECLARE_NODE_API_PROPERTY("CallIntoModule", CallIntoModule),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(env, exports,
    sizeof(properties)/sizeof(properties[0]), properties));

  return exports;
}
JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
