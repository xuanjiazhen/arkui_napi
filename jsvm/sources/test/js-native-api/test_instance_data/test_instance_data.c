#include <jsvm.h>
#include <stdio.h>
#include <stdlib.h>
#include "../common.h"
#include "../entry_point.h"

typedef struct {
  size_t value;
  bool print;
  JSVM_Ref js_cb_ref;
} AddonData;

static JSVM_Value Increment(JSVM_Env env, JSVM_CallbackInfo info) {
  AddonData* data;
  JSVM_Value result;

  NODE_API_CALL(env, OH_JSVM_GetInstanceData(env, (void**)&data));
  NODE_API_CALL(env, OH_JSVM_CreateUint32(env, ++data->value, &result));

  return result;
}

static void DeleteAddonData(JSVM_Env env, void* raw_data, void* hint) {
  AddonData* data = raw_data;
  if (data->print) {
    printf("deleting addon data\n");
  }
  if (data->js_cb_ref != NULL) {
    NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, data->js_cb_ref));
  }
  free(data);
}

static JSVM_Value SetPrintOnDelete(JSVM_Env env, JSVM_CallbackInfo info) {
  AddonData* data;

  NODE_API_CALL(env, OH_JSVM_GetInstanceData(env, (void**)&data));
  data->print = true;

  return NULL;
}

static void TestFinalizer(JSVM_Env env, void* raw_data, void* hint) {
  (void) raw_data;
  (void) hint;

  AddonData* data;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetInstanceData(env, (void**)&data));
  JSVM_Value js_cb, undefined;
  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_GetReferenceValue(env, data->js_cb_ref, &js_cb));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetUndefined(env, &undefined));
  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_CallFunction(env, undefined, js_cb, 0, NULL, NULL));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, data->js_cb_ref));
  data->js_cb_ref = NULL;
}

static JSVM_Value ObjectWithFinalizer(JSVM_Env env, JSVM_CallbackInfo info) {
  AddonData* data;
  JSVM_Value result, js_cb;
  size_t argc = 1;

  NODE_API_CALL(env, OH_JSVM_GetInstanceData(env, (void**)&data));
  NODE_API_ASSERT(env, data->js_cb_ref == NULL, "reference must be NULL");
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &js_cb, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &result));
  NODE_API_CALL(env,
      OH_JSVM_AddFinalizer(env, result, NULL, TestFinalizer, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_CreateReference(env, js_cb, 1, &data->js_cb_ref));

  return result;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  AddonData* data = malloc(sizeof(*data));
  data->value = 41;
  data->print = false;
  data->js_cb_ref = NULL;

  NODE_API_CALL(env, OH_JSVM_SetInstanceData(env, data, DeleteAddonData, NULL));

  JSVM_PropertyDescriptor props[] = {
    DECLARE_NODE_API_PROPERTY("increment", Increment),
    DECLARE_NODE_API_PROPERTY("setPrintOnDelete", SetPrintOnDelete),
    DECLARE_NODE_API_PROPERTY("objectWithFinalizer", ObjectWithFinalizer),
  };

  NODE_API_CALL(env,
      OH_JSVM_DefineProperties(
          env, exports, sizeof(props) / sizeof(*props), props));

  return exports;
}
EXTERN_C_END
