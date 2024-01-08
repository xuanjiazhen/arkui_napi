#include <assert.h>
#include <stdio.h>
#include <node_api.h>
#include <uv.h>
#include "../../js-native-api/common.h"

// this needs to be greater than the thread pool size
#define MAX_CANCEL_THREADS 6

typedef struct {
  int32_t _input;
  int32_t _output;
  JSVM_Ref _callback;
  jsvm_async_work _request;
} carrier;

static carrier the_carrier;
static carrier async_carrier[MAX_CANCEL_THREADS];

static void Execute(JSVM_Env env, void* data) {
  uv_sleep(1000);
  carrier* c = (carrier*)(data);

  assert(c == &the_carrier);

  c->_output = c->_input * 2;
}

static void Complete(JSVM_Env env, JSVM_Status status, void* data) {
  carrier* c = (carrier*)(data);

  if (c != &the_carrier) {
    OH_JSVM_ThrowTypeError(env, NULL, "Wrong data parameter to Complete.");
    return;
  }

  if (status != JSVM_OK) {
    OH_JSVM_ThrowTypeError(env, NULL, "Execute callback failed.");
    return;
  }

  JSVM_Value argv[2];

  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetNull(env, &argv[0]));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_CreateInt32(env, c->_output, &argv[1]));
  JSVM_Value callback;
  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_GetReferenceValue(env, c->_callback, &callback));
  JSVM_Value global;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetGlobal(env, &global));

  JSVM_Value result;
  NODE_API_CALL_RETURN_VOID(env,
    OH_JSVM_CallFunction(env, global, callback, 2, argv, &result));

  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, c->_callback));
  NODE_API_CALL_RETURN_VOID(env, jsvm_delete_async_work(env, c->_request));
}

static JSVM_Value Test(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 3;
  JSVM_Value argv[3];
  JSVM_Value _this;
  JSVM_Value resource_name;
  void* data;
  NODE_API_CALL(env,
    OH_JSVM_GetCbInfo(env, info, &argc, argv, &_this, &data));
  NODE_API_ASSERT(env, argc >= 3, "Not enough arguments, expected 3.");

  JSVM_ValueType t;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, argv[0], &t));
  NODE_API_ASSERT(env, t == jsvm_number,
      "Wrong first argument, integer expected.");
  NODE_API_CALL(env, OH_JSVM_Typeof(env, argv[1], &t));
  NODE_API_ASSERT(env, t == jsvm_object,
    "Wrong second argument, object expected.");
  NODE_API_CALL(env, OH_JSVM_Typeof(env, argv[2], &t));
  NODE_API_ASSERT(env, t == jsvm_function,
    "Wrong third argument, function expected.");

  the_carrier._output = 0;

  NODE_API_CALL(env,
      OH_JSVM_GetValueInt32(env, argv[0], &the_carrier._input));
  NODE_API_CALL(env,
    OH_JSVM_CreateReference(env, argv[2], 1, &the_carrier._callback));

  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
      env, "TestResource", JSVM_AUTO_LENGTH, &resource_name));
  NODE_API_CALL(env, jsvm_create_async_work(env, argv[1], resource_name,
    Execute, Complete, &the_carrier, &the_carrier._request));
  NODE_API_CALL(env,
      jsvm_queue_async_work(env, the_carrier._request));

  return NULL;
}

static void BusyCancelComplete(JSVM_Env env, JSVM_Status status, void* data) {
  carrier* c = (carrier*)(data);
  NODE_API_CALL_RETURN_VOID(env, jsvm_delete_async_work(env, c->_request));
}

static void CancelComplete(JSVM_Env env, JSVM_Status status, void* data) {
  carrier* c = (carrier*)(data);

  if (status == JSVM_CANCELLED) {
    // ok we got the status we expected so make the callback to
    // indicate the cancel succeeded.
    JSVM_Value callback;
    NODE_API_CALL_RETURN_VOID(env,
        OH_JSVM_GetReferenceValue(env, c->_callback, &callback));
    JSVM_Value global;
    NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetGlobal(env, &global));
    JSVM_Value result;
    NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_CallFunction(env, global, callback, 0, NULL, &result));
  }

  NODE_API_CALL_RETURN_VOID(env, jsvm_delete_async_work(env, c->_request));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, c->_callback));
}

static void CancelExecute(JSVM_Env env, void* data) {
  uv_sleep(1000);
}

static JSVM_Value TestCancel(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value argv[1];
  JSVM_Value _this;
  JSVM_Value resource_name;
  void* data;

  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(
      env, "TestResource", JSVM_AUTO_LENGTH, &resource_name));

  // make sure the work we are going to cancel will not be
  // able to start by using all the threads in the pool
  for (int i = 1; i < MAX_CANCEL_THREADS; i++) {
    NODE_API_CALL(env, jsvm_create_async_work(env, NULL, resource_name,
      CancelExecute, BusyCancelComplete,
      &async_carrier[i], &async_carrier[i]._request));
    NODE_API_CALL(env, jsvm_queue_async_work(env, async_carrier[i]._request));
  }

  // now queue the work we are going to cancel and then cancel it.
  // cancel will fail if the work has already started, but
  // we have prevented it from starting by consuming all of the
  // workers above.
  NODE_API_CALL(env,
    OH_JSVM_GetCbInfo(env, info, &argc, argv, &_this, &data));
  NODE_API_CALL(env, jsvm_create_async_work(env, NULL, resource_name,
    CancelExecute, CancelComplete,
    &async_carrier[0], &async_carrier[0]._request));
  NODE_API_CALL(env,
      OH_JSVM_CreateReference(env, argv[0], 1, &async_carrier[0]._callback));
  NODE_API_CALL(env, jsvm_queue_async_work(env, async_carrier[0]._request));
  NODE_API_CALL(env, jsvm_cancel_async_work(env, async_carrier[0]._request));
  return NULL;
}

struct {
  JSVM_Ref ref;
  jsvm_async_work work;
} repeated_work_info = { NULL, NULL };

static void RepeatedWorkerThread(JSVM_Env env, void* data) {}

static void RepeatedWorkComplete(JSVM_Env env, JSVM_Status status, void* data) {
  JSVM_Value cb, js_status;
  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_GetReferenceValue(env, repeated_work_info.ref, &cb));
  NODE_API_CALL_RETURN_VOID(env,
      jsvm_delete_async_work(env, repeated_work_info.work));
  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_DeleteReference(env, repeated_work_info.ref));
  repeated_work_info.work = NULL;
  repeated_work_info.ref = NULL;
  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_CreateUint32(env, (uint32_t)status, &js_status));
  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_CallFunction(env, cb, cb, 1, &js_status, NULL));
}

static JSVM_Value DoRepeatedWork(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value cb, name;
  NODE_API_ASSERT(env, repeated_work_info.ref == NULL,
      "Reference left over from previous work");
  NODE_API_ASSERT(env, repeated_work_info.work == NULL,
      "Work pointer left over from previous work");
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &cb, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_CreateReference(env, cb, 1, &repeated_work_info.ref));
  NODE_API_CALL(env,
      OH_JSVM_CreateStringUtf8(env, "Repeated Work", JSVM_AUTO_LENGTH, &name));
  NODE_API_CALL(env,
      jsvm_create_async_work(env, NULL, name, RepeatedWorkerThread,
          RepeatedWorkComplete, &repeated_work_info, &repeated_work_info.work));
  NODE_API_CALL(env, jsvm_queue_async_work(env, repeated_work_info.work));
  return NULL;
}

static JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor properties[] = {
    DECLARE_NODE_API_PROPERTY("Test", Test),
    DECLARE_NODE_API_PROPERTY("TestCancel", TestCancel),
    DECLARE_NODE_API_PROPERTY("DoRepeatedWork", DoRepeatedWork),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(properties) / sizeof(*properties), properties));

  return exports;
}

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
