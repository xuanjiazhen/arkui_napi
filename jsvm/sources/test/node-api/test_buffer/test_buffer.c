#include <stdlib.h>
#include <string.h>
#include <node_api.h>
#include "../../js-native-api/common.h"

static const char theText[] =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit.";

static int deleterCallCount = 0;
static void deleteTheText(JSVM_Env env, void* data, void* finalize_hint) {
  NODE_API_ASSERT_RETURN_VOID(
      env, data != NULL && strcmp(data, theText) == 0, "invalid data");
  (void)finalize_hint;
  free(data);
  deleterCallCount++;
}

static void noopDeleter(JSVM_Env env, void* data, void* finalize_hint) {
  NODE_API_ASSERT_RETURN_VOID(
      env, data != NULL && strcmp(data, theText) == 0, "invalid data");
  (void)finalize_hint;
  deleterCallCount++;
}

static JSVM_Value newBuffer(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value theBuffer;
  char* theCopy;
  const unsigned int kBufferSize = sizeof(theText);

  NODE_API_CALL(env,
      jsvm_create_buffer(
          env, sizeof(theText), (void**)(&theCopy), &theBuffer));
  NODE_API_ASSERT(env, theCopy, "Failed to copy static text for newBuffer");
  memcpy(theCopy, theText, kBufferSize);

  return theBuffer;
}

static JSVM_Value newExternalBuffer(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value theBuffer;
  char* theCopy = strdup(theText);
  NODE_API_ASSERT(
      env, theCopy, "Failed to copy static text for newExternalBuffer");
  NODE_API_CALL(env,
      OH_JSVM_CreateExternal_buffer(
          env, sizeof(theText), theCopy, deleteTheText,
          NULL /* finalize_hint */, &theBuffer));

  return theBuffer;
}

static JSVM_Value getDeleterCallCount(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value callCount;
  NODE_API_CALL(env, OH_JSVM_CreateInt32(env, deleterCallCount, &callCount));
  return callCount;
}

static JSVM_Value copyBuffer(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value theBuffer;
  NODE_API_CALL(env, jsvm_create_buffer_copy(
      env, sizeof(theText), theText, NULL, &theBuffer));
  return theBuffer;
}

static JSVM_Value bufferHasInstance(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));
  NODE_API_ASSERT(env, argc == 1, "Wrong number of arguments");
  JSVM_Value theBuffer = args[0];
  bool hasInstance;
  JSVM_ValueType theType;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, theBuffer, &theType));
  NODE_API_ASSERT(env,
      theType == jsvm_object, "bufferHasInstance: instance is not an object");
  NODE_API_CALL(env, jsvm_is_buffer(env, theBuffer, &hasInstance));
  NODE_API_ASSERT(env, hasInstance, "bufferHasInstance: instance is not a buffer");
  JSVM_Value returnValue;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, hasInstance, &returnValue));
  return returnValue;
}

static JSVM_Value bufferInfo(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));
  NODE_API_ASSERT(env, argc == 1, "Wrong number of arguments");
  JSVM_Value theBuffer = args[0];
  char *bufferData;
  JSVM_Value returnValue;
  size_t bufferLength;
  NODE_API_CALL(env,
      jsvm_get_buffer_info(
          env, theBuffer, (void**)(&bufferData), &bufferLength));
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env,
      !strcmp(bufferData, theText) && bufferLength == sizeof(theText),
      &returnValue));
  return returnValue;
}

static JSVM_Value staticBuffer(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value theBuffer;
  NODE_API_CALL(env,
      OH_JSVM_CreateExternal_buffer(
          env, sizeof(theText), (void*)theText, noopDeleter,
          NULL /* finalize_hint */, &theBuffer));
  return theBuffer;
}

static JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_Value theValue;

  NODE_API_CALL(env,
      OH_JSVM_CreateStringUtf8(env, theText, sizeof(theText), &theValue));
  NODE_API_CALL(env,
      OH_JSVM_SetNamedProperty(env, exports, "theText", theValue));

  JSVM_PropertyDescriptor methods[] = {
      DECLARE_NODE_API_PROPERTY("newBuffer", newBuffer),
      DECLARE_NODE_API_PROPERTY("newExternalBuffer", newExternalBuffer),
      DECLARE_NODE_API_PROPERTY("getDeleterCallCount", getDeleterCallCount),
      DECLARE_NODE_API_PROPERTY("copyBuffer", copyBuffer),
      DECLARE_NODE_API_PROPERTY("bufferHasInstance", bufferHasInstance),
      DECLARE_NODE_API_PROPERTY("bufferInfo", bufferInfo),
      DECLARE_NODE_API_PROPERTY("staticBuffer", staticBuffer),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(methods) / sizeof(methods[0]), methods));

  return exports;
}

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
