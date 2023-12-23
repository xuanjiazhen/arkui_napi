#include <node_api.h>
#include "../../js-native-api/common.h"
#include "stdlib.h"

static uint32_t finalizeCount = 0;

static void FreeData(JSVM_Env env, void* data, void* hint) {
  NODE_API_ASSERT_RETURN_VOID(env, data != NULL, "Expects non-NULL data.");
  free(data);
}

static void Finalize(JSVM_Env env, void* data, void* hint) {
  ++finalizeCount;
}

static JSVM_Status GetArgValue(JSVM_Env env,
                               JSVM_CallbackInfo info,
                               JSVM_Value* argValue) {
  size_t argc = 1;
  NODE_API_CHECK_STATUS(
      OH_JSVM_GetCbInfo(env, info, &argc, argValue, NULL, NULL));

  NODE_API_ASSERT_STATUS(env, argc == 1, "Expects one arg.");
  return JSVM_OK;
}

static JSVM_Status GetArgValueAsIndex(JSVM_Env env,
                                      JSVM_CallbackInfo info,
                                      uint32_t* index) {
  JSVM_Value argValue;
  NODE_API_CHECK_STATUS(GetArgValue(env, info, &argValue));

  JSVM_ValueType valueType;
  NODE_API_CHECK_STATUS(OH_JSVM_Typeof(env, argValue, &valueType));
  NODE_API_ASSERT_STATUS(
      env, valueType == jsvm_number, "Argument must be a number.");

  return OH_JSVM_GetValueUint32(env, argValue, index);
}

static JSVM_Status GetRef(JSVM_Env env,
                          JSVM_CallbackInfo info,
                          JSVM_Ref* ref) {
  uint32_t index;
  NODE_API_CHECK_STATUS(GetArgValueAsIndex(env, info, &index));

  JSVM_Ref* refValues;
  NODE_API_CHECK_STATUS(OH_JSVM_GetInstanceData(env, (void**)&refValues));
  NODE_API_ASSERT_STATUS(env, refValues != NULL, "Cannot get instance data.");

  *ref = refValues[index];
  return JSVM_OK;
}

static JSVM_Value ToUInt32Value(JSVM_Env env, uint32_t value) {
  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_CreateUint32(env, value, &result));
  return result;
}

static JSVM_Status InitRefArray(JSVM_Env env) {
  // valueRefs array has one entry per JSVM_ValueType
  JSVM_Ref* valueRefs = malloc(sizeof(JSVM_Ref) * ((int)jsvm_bigint + 1));
  return OH_JSVM_SetInstanceData(env, valueRefs, &FreeData, NULL);
}

static JSVM_Value CreateExternal(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  int* data = (int*)malloc(sizeof(int));
  *data = 42;
  NODE_API_CALL(env, OH_JSVM_CreateExternal(env, data, &FreeData, NULL, &result));
  return result;
}

static JSVM_Value CreateRef(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value argValue;
  NODE_API_CALL(env, GetArgValue(env, info, &argValue));

  JSVM_ValueType valueType;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, argValue, &valueType));
  uint32_t index = (uint32_t)valueType;

  JSVM_Ref* valueRefs;
  NODE_API_CALL(env, OH_JSVM_GetInstanceData(env, (void**)&valueRefs));
  NODE_API_CALL(env,
                OH_JSVM_CreateReference(env, argValue, 1, valueRefs + index));

  return ToUInt32Value(env, index);
}

static JSVM_Value GetRefValue(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Ref refValue;
  NODE_API_CALL(env, GetRef(env, info, &refValue));
  JSVM_Value value;
  NODE_API_CALL(env, OH_JSVM_GetReferenceValue(env, refValue, &value));
  return value;
}

static JSVM_Value Ref(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Ref refValue;
  NODE_API_CALL(env, GetRef(env, info, &refValue));
  uint32_t refCount;
  NODE_API_CALL(env, OH_JSVM_ReferenceRef(env, refValue, &refCount));
  return ToUInt32Value(env, refCount);
}

static JSVM_Value Unref(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Ref refValue;
  NODE_API_CALL(env, GetRef(env, info, &refValue));
  uint32_t refCount;
  NODE_API_CALL(env, OH_JSVM_ReferenceUnref(env, refValue, &refCount));
  return ToUInt32Value(env, refCount);
}

static JSVM_Value DeleteRef(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Ref refValue;
  NODE_API_CALL(env, GetRef(env, info, &refValue));
  NODE_API_CALL(env, OH_JSVM_DeleteReference(env, refValue));
  return NULL;
}

static JSVM_Value AddFinalizer(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value obj;
  NODE_API_CALL(env, GetArgValue(env, info, &obj));

  JSVM_ValueType valueType;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, obj, &valueType));
  NODE_API_ASSERT(env, valueType == jsvm_object, "Argument must be an object.");

  NODE_API_CALL(env, OH_JSVM_AddFinalizer(env, obj, NULL, &Finalize, NULL, NULL));
  return NULL;
}

static JSVM_Value GetFinalizeCount(JSVM_Env env, JSVM_CallbackInfo info) {
  return ToUInt32Value(env, finalizeCount);
}

static JSVM_Value InitFinalizeCount(JSVM_Env env, JSVM_CallbackInfo info) {
  finalizeCount = 0;
  return NULL;
}

EXTERN_C_START

JSVM_MODULE_INIT() {
  finalizeCount = 0;
  NODE_API_CALL(env, InitRefArray(env));

  JSVM_PropertyDescriptor properties[] = {
      DECLARE_NODE_API_PROPERTY("createExternal", CreateExternal),
      DECLARE_NODE_API_PROPERTY("createRef", CreateRef),
      DECLARE_NODE_API_PROPERTY("getRefValue", GetRefValue),
      DECLARE_NODE_API_PROPERTY("ref", Ref),
      DECLARE_NODE_API_PROPERTY("unref", Unref),
      DECLARE_NODE_API_PROPERTY("deleteRef", DeleteRef),
      DECLARE_NODE_API_PROPERTY("addFinalizer", AddFinalizer),
      DECLARE_NODE_API_PROPERTY("getFinalizeCount", GetFinalizeCount),
      DECLARE_NODE_API_PROPERTY("initFinalizeCount", InitFinalizeCount),
  };

  NODE_API_CALL(
      env,
      OH_JSVM_DefineProperties(
          env, exports, sizeof(properties) / sizeof(*properties), properties));

  return exports;
}

EXTERN_C_END
