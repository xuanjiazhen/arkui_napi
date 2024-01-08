#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"

static JSVM_Value BaseClass(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value newTargetArg;
  NODE_API_CALL(env, OH_JSVM_GetNewTarget(env, info, &newTargetArg));
  JSVM_Value thisArg;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, NULL, NULL, &thisArg, NULL));
  JSVM_Value undefined;
  NODE_API_CALL(env, OH_JSVM_GetUndefined(env, &undefined));

  // this !== new.target since we are being invoked through super()
  bool result;
  NODE_API_CALL(env, OH_JSVM_StrictEquals(env, newTargetArg, thisArg, &result));
  NODE_API_ASSERT(env, !result, "this !== new.target");

  // new.target !== undefined because we should be called as a new expression
  NODE_API_ASSERT(env, newTargetArg != NULL, "newTargetArg != NULL");
  NODE_API_CALL(env, OH_JSVM_StrictEquals(env, newTargetArg, undefined, &result));
  NODE_API_ASSERT(env, !result, "new.target !== undefined");

  return thisArg;
}

static JSVM_Value Constructor(JSVM_Env env, JSVM_CallbackInfo info) {
  bool result;
  JSVM_Value newTargetArg;
  NODE_API_CALL(env, OH_JSVM_GetNewTarget(env, info, &newTargetArg));
  size_t argc = 1;
  JSVM_Value argv;
  JSVM_Value thisArg;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &argv, &thisArg, NULL));
  JSVM_Value undefined;
  NODE_API_CALL(env, OH_JSVM_GetUndefined(env, &undefined));

  // new.target !== undefined because we should be called as a new expression
  NODE_API_ASSERT(env, newTargetArg != NULL, "newTargetArg != NULL");
  NODE_API_CALL(env, OH_JSVM_StrictEquals(env, newTargetArg, undefined, &result));
  NODE_API_ASSERT(env, !result, "new.target !== undefined");

  // arguments[0] should be Constructor itself (test harness passed it)
  NODE_API_CALL(env, OH_JSVM_StrictEquals(env, newTargetArg, argv, &result));
  NODE_API_ASSERT(env, result, "new.target === Constructor");

  return thisArg;
}

static JSVM_Value OrdinaryFunction(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value newTargetArg;
  NODE_API_CALL(env, OH_JSVM_GetNewTarget(env, info, &newTargetArg));

  NODE_API_ASSERT(env, newTargetArg == NULL, "newTargetArg == NULL");

  JSVM_Value _true;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, true, &_true));
  return _true;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  const JSVM_PropertyDescriptor desc[] = {
    DECLARE_NODE_API_PROPERTY("BaseClass", BaseClass),
    DECLARE_NODE_API_PROPERTY("OrdinaryFunction", OrdinaryFunction),
    DECLARE_NODE_API_PROPERTY("Constructor", Constructor)
  };
  NODE_API_CALL(env, OH_JSVM_DefineProperties(env, exports, 3, desc));
  return exports;
}
EXTERN_C_END
