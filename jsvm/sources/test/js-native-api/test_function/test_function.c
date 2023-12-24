#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"

static JSVM_Value TestCreateFunctionParameters(JSVM_Env env,
                                               JSVM_CallbackInfo info) {
  JSVM_Status status;
  JSVM_Value result, return_value;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &return_value));

  status = OH_JSVM_CreateFunction(NULL,
                                "TrackedFunction",
                                JSVM_AUTO_LENGTH,
                                TestCreateFunctionParameters,
                                NULL,
                                &result);

  add_returned_status(env,
                      "envIsNull",
                      return_value,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      status);

  OH_JSVM_CreateFunction(env,
                       NULL,
                       JSVM_AUTO_LENGTH,
                       TestCreateFunctionParameters,
                       NULL,
                       &result);

  add_last_status(env, "nameIsNull", return_value);

  OH_JSVM_CreateFunction(env,
                       "TrackedFunction",
                       JSVM_AUTO_LENGTH,
                       NULL,
                       NULL,
                       &result);

  add_last_status(env, "cbIsNull", return_value);

  OH_JSVM_CreateFunction(env,
                       "TrackedFunction",
                       JSVM_AUTO_LENGTH,
                       TestCreateFunctionParameters,
                       NULL,
                       NULL);

  add_last_status(env, "resultIsNull", return_value);

  return return_value;
}

static JSVM_Value TestCallFunction(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 10;
  JSVM_Value args[10];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc > 0, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_function,
      "Wrong type of arguments. Expects a function as first argument.");

  JSVM_Value* argv = args + 1;
  argc = argc - 1;

  JSVM_Value global;
  NODE_API_CALL(env, OH_JSVM_GetGlobal(env, &global));

  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_CallFunction(env, global, args[0], argc, argv, &result));

  return result;
}

static JSVM_Value TestFunctionName(JSVM_Env env, JSVM_CallbackInfo info) {
  return NULL;
}

static void finalize_function(JSVM_Env env, void* data, void* hint) {
  JSVM_Ref ref = data;

  // Retrieve the JavaScript undefined value.
  JSVM_Value undefined;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetUndefined(env, &undefined));

  // Retrieve the JavaScript function we must call.
  JSVM_Value js_function;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_GetReferenceValue(env, ref, &js_function));

  // Call the JavaScript function to indicate that the generated JavaScript
  // function is about to be gc-ed.
  NODE_API_CALL_RETURN_VOID(env,
      OH_JSVM_CallFunction(env, undefined, js_function, 0, NULL, NULL));

  // Destroy the persistent reference to the function we just called so as to
  // properly clean up.
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, ref));
}

static JSVM_Value MakeTrackedFunction(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value js_finalize_cb;
  JSVM_ValueType arg_type;

  // Retrieve and validate from the arguments the function we will use to
  // indicate to JavaScript that the function we are about to create is about to
  // be gc-ed.
  NODE_API_CALL(env,
      OH_JSVM_GetCbInfo(env, info, &argc, &js_finalize_cb, NULL, NULL));
  NODE_API_ASSERT(env, argc == 1, "Wrong number of arguments");
  NODE_API_CALL(env, OH_JSVM_Typeof(env, js_finalize_cb, &arg_type));
  NODE_API_ASSERT(env, arg_type == jsvm_function, "Argument must be a function");

  // Dynamically create a function.
  JSVM_Value result;
  NODE_API_CALL(env,
      OH_JSVM_CreateFunction(
          env, "TrackedFunction", JSVM_AUTO_LENGTH, TestFunctionName, NULL,
          &result));

  // Create a strong reference to the function we will call when the tracked
  // function is about to be gc-ed.
  JSVM_Ref js_finalize_cb_ref;
  NODE_API_CALL(env,
      OH_JSVM_CreateReference(env, js_finalize_cb, 1, &js_finalize_cb_ref));

  // Attach a finalizer to the dynamically created function and pass it the
  // strong reference we created in the previous step.
  NODE_API_CALL(env,
      OH_JSVM_Wrap(
          env, result, js_finalize_cb_ref, finalize_function, NULL, NULL));

  return result;
}

static JSVM_Value TestBadReturnExceptionPending(JSVM_Env env, JSVM_CallbackInfo info) {
  OH_JSVM_ThrowError(env, "throwing exception", "throwing exception");

  // addons should only ever return a valid JSVM_Value even if an
  // exception occurs, but we have seen that the C++ wrapper
  // with exceptions enabled sometimes returns an invalid value
  // when an exception is thrown. Test that we ignore the return
  // value then an exeption is pending. We use 0xFFFFFFFF as a value
  // that should never be a valid JSVM_Value and node seems to
  // crash if it is not ignored indicating that it is indeed invalid.
  return (JSVM_Value)(0xFFFFFFFFF);
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_Value fn1;
  NODE_API_CALL(env, OH_JSVM_CreateFunction(
      env, NULL, JSVM_AUTO_LENGTH, TestCallFunction, NULL, &fn1));

  JSVM_Value fn2;
  NODE_API_CALL(env, OH_JSVM_CreateFunction(
      env, "Name", JSVM_AUTO_LENGTH, TestFunctionName, NULL, &fn2));

  JSVM_Value fn3;
  NODE_API_CALL(env, OH_JSVM_CreateFunction(
      env, "Name_extra", 5, TestFunctionName, NULL, &fn3));

  JSVM_Value fn4;
  NODE_API_CALL(env,
      OH_JSVM_CreateFunction(
          env, "MakeTrackedFunction", JSVM_AUTO_LENGTH, MakeTrackedFunction,
          NULL, &fn4));

  JSVM_Value fn5;
  NODE_API_CALL(env,
      OH_JSVM_CreateFunction(
          env, "TestCreateFunctionParameters", JSVM_AUTO_LENGTH,
          TestCreateFunctionParameters, NULL, &fn5));

  JSVM_Value fn6;
  NODE_API_CALL(env,
      OH_JSVM_CreateFunction(
          env, "TestBadReturnExceptionPending", JSVM_AUTO_LENGTH,
          TestBadReturnExceptionPending, NULL, &fn6));

  NODE_API_CALL(env, OH_JSVM_SetNamedProperty(env, exports, "TestCall", fn1));
  NODE_API_CALL(env, OH_JSVM_SetNamedProperty(env, exports, "TestName", fn2));
  NODE_API_CALL(env,
      OH_JSVM_SetNamedProperty(env, exports, "TestNameShort", fn3));
  NODE_API_CALL(env,
      OH_JSVM_SetNamedProperty(env, exports, "MakeTrackedFunction", fn4));

  NODE_API_CALL(env,
      OH_JSVM_SetNamedProperty(
          env, exports, "TestCreateFunctionParameters", fn5));

  NODE_API_CALL(env,
      OH_JSVM_SetNamedProperty(
          env, exports, "TestBadReturnExceptionPending", fn6));

  return exports;
}
EXTERN_C_END
