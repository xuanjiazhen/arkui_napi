#define JSVM_VERSION 9
#include <assert.h>
#include <jsvm.h>
#include <stdlib.h>
#include "../common.h"
#include "../entry_point.h"

static int test_value = 1;
static int finalize_count = 0;
static JSVM_Ref test_reference = NULL;

static JSVM_Value GetFinalizeCount(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_CreateInt32(env, finalize_count, &result));
  return result;
}

static void FinalizeExternal(JSVM_Env env, void* data, void* hint) {
  int *actual_value = data;
  NODE_API_ASSERT_RETURN_VOID(env, actual_value == &test_value,
      "The correct pointer was passed to the finalizer");
  finalize_count++;
}

static JSVM_Value CreateExternal(JSVM_Env env, JSVM_CallbackInfo info) {
  int* data = &test_value;

  JSVM_Value result;
  NODE_API_CALL(env,
      OH_JSVM_CreateExternal(env,
                           data,
                           NULL, /* finalize_cb */
                           NULL, /* finalize_hint */
                           &result));

  finalize_count = 0;
  return result;
}

static JSVM_Value CreateSymbol(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));
  NODE_API_ASSERT(
      env, argc == 1, "Expect one argument only (symbol description)");

  JSVM_Value result_symbol;

  NODE_API_CALL(env, OH_JSVM_CreateSymbol(env, args[0], &result_symbol));
  return result_symbol;
}

static JSVM_Value CreateSymbolFor(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];

  char description[256];
  size_t description_length;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));
  NODE_API_ASSERT(
      env, argc == 1, "Expect one argument only (symbol description)");

  NODE_API_CALL(
      env,
      OH_JSVM_GetValueStringUtf8(
          env, args[0], description, sizeof(description), &description_length));
  NODE_API_ASSERT(env,
                  description_length <= 255,
                  "Cannot accommodate descriptions longer than 255 bytes");

  JSVM_Value result_symbol;

  NODE_API_CALL(env,
                OH_JSVM_SymbolFor(
                    env, description, description_length, &result_symbol));
  return result_symbol;
}

static JSVM_Value CreateSymbolForEmptyString(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result_symbol;
  NODE_API_CALL(env, OH_JSVM_SymbolFor(env, NULL, 0, &result_symbol));
  return result_symbol;
}

static JSVM_Value CreateSymbolForIncorrectLength(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result_symbol;
  NODE_API_CALL(env, OH_JSVM_SymbolFor(env, NULL, 5, &result_symbol));
  return result_symbol;
}

static JSVM_Value
CreateExternalWithFinalize(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  NODE_API_CALL(env,
      OH_JSVM_CreateExternal(env,
                           &test_value,
                           FinalizeExternal,
                           NULL, /* finalize_hint */
                           &result));

  finalize_count = 0;
  return result;
}

static JSVM_Value CheckExternal(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value arg;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &arg, NULL, NULL));

  NODE_API_ASSERT(env, argc == 1, "Expected one argument.");

  JSVM_ValueType argtype;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, arg, &argtype));

  NODE_API_ASSERT(env, argtype == jsvm_external, "Expected an external value.");

  void* data;
  NODE_API_CALL(env, OH_JSVM_GetValueExternal(env, arg, &data));

  NODE_API_ASSERT(env, data != NULL && *(int*)data == test_value,
      "An external data value of 1 was expected.");

  return NULL;
}

static JSVM_Value CreateReference(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_ASSERT(env, test_reference == NULL,
      "The test allows only one reference at a time.");

  size_t argc = 2;
  JSVM_Value args[2];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));
  NODE_API_ASSERT(env, argc == 2, "Expected two arguments.");

  uint32_t initial_refcount;
  NODE_API_CALL(env, OH_JSVM_GetValueUint32(env, args[1], &initial_refcount));

  NODE_API_CALL(env,
      OH_JSVM_CreateReference(env, args[0], initial_refcount, &test_reference));

  NODE_API_ASSERT(env, test_reference != NULL,
      "A reference should have been created.");

  return NULL;
}

static JSVM_Value DeleteReference(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_ASSERT(env, test_reference != NULL,
      "A reference must have been created.");

  NODE_API_CALL(env, OH_JSVM_DeleteReference(env, test_reference));
  test_reference = NULL;
  return NULL;
}

static JSVM_Value IncrementRefcount(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_ASSERT(env, test_reference != NULL,
      "A reference must have been created.");

  uint32_t refcount;
  NODE_API_CALL(env, OH_JSVM_ReferenceRef(env, test_reference, &refcount));

  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_CreateUint32(env, refcount, &result));
  return result;
}

static JSVM_Value DecrementRefcount(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_ASSERT(env, test_reference != NULL,
      "A reference must have been created.");

  uint32_t refcount;
  NODE_API_CALL(env, OH_JSVM_ReferenceUnref(env, test_reference, &refcount));

  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_CreateUint32(env, refcount, &result));
  return result;
}

static JSVM_Value GetReferenceValue(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_ASSERT(env, test_reference != NULL,
      "A reference must have been created.");

  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_GetReferenceValue(env, test_reference, &result));
  return result;
}

static void DeleteBeforeFinalizeFinalizer(
    JSVM_Env env, void* finalize_data, void* finalize_hint) {
  JSVM_Ref* ref = (JSVM_Ref*)finalize_data;
  JSVM_Value value;
  assert(OH_JSVM_GetReferenceValue(env, *ref, &value) == JSVM_OK);
  assert(value == NULL);
  OH_JSVM_DeleteReference(env, *ref);
  free(ref);
}

static JSVM_Value ValidateDeleteBeforeFinalize(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value wrapObject;
  size_t argc = 1;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &wrapObject, NULL, NULL));

  JSVM_Ref* ref_t = malloc(sizeof(JSVM_Ref));
  NODE_API_CALL(env,
      OH_JSVM_Wrap(
          env, wrapObject, ref_t, DeleteBeforeFinalizeFinalizer, NULL, NULL));

  // Create a reference that will be eligible for collection at the same
  // time as the wrapped object by passing in the same wrapObject.
  // This means that the FinalizeOrderValidation callback may be run
  // before the finalizer for the newly created reference (there is a finalizer
  // behind the scenes even though it cannot be passed to OH_JSVM_CreateReference)
  // The Finalizer for the wrap (which is different than the finalizer
  // for the reference) calls OH_JSVM_DeleteReference validating that
  // OH_JSVM_DeleteReference can be called before the finalizer for the
  // reference runs.
  NODE_API_CALL(env, OH_JSVM_CreateReference(env, wrapObject, 0, ref_t));
  return wrapObject;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
      DECLARE_NODE_API_GETTER("finalizeCount", GetFinalizeCount),
      DECLARE_NODE_API_PROPERTY("createExternal", CreateExternal),
      DECLARE_NODE_API_PROPERTY("createExternalWithFinalize",
                                CreateExternalWithFinalize),
      DECLARE_NODE_API_PROPERTY("checkExternal", CheckExternal),
      DECLARE_NODE_API_PROPERTY("createReference", CreateReference),
      DECLARE_NODE_API_PROPERTY("createSymbol", CreateSymbol),
      DECLARE_NODE_API_PROPERTY("createSymbolFor", CreateSymbolFor),
      DECLARE_NODE_API_PROPERTY("createSymbolForEmptyString",
                                CreateSymbolForEmptyString),
      DECLARE_NODE_API_PROPERTY("createSymbolForIncorrectLength",
                                CreateSymbolForIncorrectLength),
      DECLARE_NODE_API_PROPERTY("deleteReference", DeleteReference),
      DECLARE_NODE_API_PROPERTY("incrementRefcount", IncrementRefcount),
      DECLARE_NODE_API_PROPERTY("decrementRefcount", DecrementRefcount),
      DECLARE_NODE_API_GETTER("referenceValue", GetReferenceValue),
      DECLARE_NODE_API_PROPERTY("validateDeleteBeforeFinalize",
                                ValidateDeleteBeforeFinalize),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  return exports;
}
EXTERN_C_END
