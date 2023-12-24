#define JSVM_VERSION 9
#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"

static double value_ = 1;

static JSVM_Value GetValue(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 0;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, NULL, NULL, NULL));

  NODE_API_ASSERT(env, argc == 0, "Wrong number of arguments");

  JSVM_Value number;
  NODE_API_CALL(env, OH_JSVM_CreateDouble(env, value_, &number));

  return number;
}

static JSVM_Value SetValue(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc == 1, "Wrong number of arguments");

  NODE_API_CALL(env, OH_JSVM_GetValueDouble(env, args[0], &value_));

  return NULL;
}

static JSVM_Value Echo(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc == 1, "Wrong number of arguments");

  return args[0];
}

static JSVM_Value HasNamedProperty(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc == 2, "Wrong number of arguments");

  // Extract the name of the property to check
  char buffer[128];
  size_t copied;
  NODE_API_CALL(env,
      OH_JSVM_GetValueStringUtf8(env, args[1], buffer, sizeof(buffer), &copied));

  // do the check and create the boolean return value
  bool value;
  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_HasNamedProperty(env, args[0], buffer, &value));
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, value, &result));

  return result;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_Value number;
  NODE_API_CALL(env, OH_JSVM_CreateDouble(env, value_, &number));

  JSVM_Value name_value;
  NODE_API_CALL(env,
      OH_JSVM_CreateStringUtf8(
          env, "NameKeyValue", JSVM_AUTO_LENGTH, &name_value));

  JSVM_Value symbol_description;
  JSVM_Value name_symbol;
  NODE_API_CALL(env,
      OH_JSVM_CreateStringUtf8(
          env, "NameKeySymbol", JSVM_AUTO_LENGTH, &symbol_description));
  NODE_API_CALL(env,
      OH_JSVM_CreateSymbol(env, symbol_description, &name_symbol));

  JSVM_Value name_symbol_descriptionless;
  NODE_API_CALL(env,
      OH_JSVM_CreateSymbol(env, NULL, &name_symbol_descriptionless));

  JSVM_Value name_symbol_for;
  NODE_API_CALL(env, OH_JSVM_SymbolFor(env,
                                         "NameKeySymbolFor",
                                         JSVM_AUTO_LENGTH,
                                         &name_symbol_for));

  JSVM_PropertyDescriptor properties[] = {
    { "echo", 0, Echo, 0, 0, 0, JSVM_ENUMERABLE, 0 },
    { "readwriteValue", 0, 0, 0, 0, number, JSVM_ENUMERABLE | JSVM_WRITABLE, 0 },
    { "readonlyValue", 0, 0, 0, 0, number, JSVM_ENUMERABLE, 0},
    { "hiddenValue", 0, 0, 0, 0, number, JSVM_DEFAULT, 0},
    { NULL, name_value, 0, 0, 0, number, JSVM_ENUMERABLE, 0},
    { NULL, name_symbol, 0, 0, 0, number, JSVM_ENUMERABLE, 0},
    { NULL, name_symbol_descriptionless, 0, 0, 0, number, JSVM_ENUMERABLE, 0},
    { NULL, name_symbol_for, 0, 0, 0, number, JSVM_ENUMERABLE, 0},
    { "readwriteAccessor1", 0, 0, GetValue, SetValue, 0, JSVM_DEFAULT, 0},
    { "readwriteAccessor2", 0, 0, GetValue, SetValue, 0, JSVM_WRITABLE, 0},
    { "readonlyAccessor1", 0, 0, GetValue, NULL, 0, JSVM_DEFAULT, 0},
    { "readonlyAccessor2", 0, 0, GetValue, NULL, 0, JSVM_WRITABLE, 0},
    { "hasNamedProperty", 0, HasNamedProperty, 0, 0, 0, JSVM_DEFAULT, 0 },
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(properties) / sizeof(*properties), properties));

  return exports;
}
EXTERN_C_END
