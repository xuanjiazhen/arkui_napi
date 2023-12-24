#include <jsvm.h>
#include <string.h>
#include "../common.h"
#include "../entry_point.h"
#include "test_null.h"

static int test_value = 3;

static JSVM_Value Get(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 2, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_object,
      "Wrong type of arguments. Expects an object as first argument.");

  JSVM_ValueType valuetype1;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &valuetype1));

  NODE_API_ASSERT(env, valuetype1 == jsvm_string || valuetype1 == jsvm_symbol,
      "Wrong type of arguments. Expects a string or symbol as second.");

  JSVM_Value object = args[0];
  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_GetProperty(env, object, args[1], &output));

  return output;
}

static JSVM_Value GetNamed(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  char key[256] = "";
  size_t key_length;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 2, "Wrong number of arguments");

  JSVM_ValueType value_type0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &value_type0));

  NODE_API_ASSERT(env, value_type0 == jsvm_object,
      "Wrong type of arguments. Expects an object as first argument.");

  JSVM_ValueType value_type1;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &value_type1));

  NODE_API_ASSERT(env, value_type1 == jsvm_string,
      "Wrong type of arguments. Expects a string as second.");

  JSVM_Value object = args[0];
  NODE_API_CALL(env,
      OH_JSVM_GetValueStringUtf8(env, args[1], key, 255, &key_length));
  key[255] = 0;
  NODE_API_ASSERT(env, key_length <= 255,
      "Cannot accommodate keys longer than 255 bytes");
  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_GetNamedProperty(env, object, key, &output));

  return output;
}

static JSVM_Value GetPropertyNames(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType value_type0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &value_type0));

  NODE_API_ASSERT(env, value_type0 == jsvm_object,
      "Wrong type of arguments. Expects an object as first argument.");

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_GetPropertyNames(env, args[0], &output));

  return output;
}

static JSVM_Value GetSymbolNames(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType value_type0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &value_type0));

  NODE_API_ASSERT(env,
              value_type0 == jsvm_object,
              "Wrong type of arguments. Expects an object as first argument.");

  JSVM_Value output;
  NODE_API_CALL(env,
      OH_JSVM_GetAllPropertyNames(
          env, args[0], JSVM_KEY_INCLUDE_PROTOTYPES, JSVM_KEY_SKIP_STRINGS,
          JSVM_KEY_NUMBERS_TO_STRINGS, &output));

  return output;
}

static JSVM_Value GetEnumerableWritableNames(JSVM_Env env,
                                             JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType value_type0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &value_type0));

  NODE_API_ASSERT(
      env,
      value_type0 == jsvm_object,
      "Wrong type of arguments. Expects an object as first argument.");

  JSVM_Value output;
  NODE_API_CALL(
      env,
      OH_JSVM_GetAllPropertyNames(env,
                                  args[0],
                                  JSVM_KEY_INCLUDE_PROTOTYPES,
                                  JSVM_KEY_ENUMERABLE | JSVM_KEY_WRITABLE,
                                  JSVM_KEY_NUMBERS_TO_STRINGS,
                                  &output));

  return output;
}

static JSVM_Value GetOwnWritableNames(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType value_type0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &value_type0));

  NODE_API_ASSERT(
      env,
      value_type0 == jsvm_object,
      "Wrong type of arguments. Expects an object as first argument.");

  JSVM_Value output;
  NODE_API_CALL(env,
                OH_JSVM_GetAllPropertyNames(env,
                                            args[0],
                                            JSVM_KEY_OWN_ONLY,
                                            JSVM_KEY_WRITABLE,
                                            JSVM_KEY_NUMBERS_TO_STRINGS,
                                            &output));

  return output;
}

static JSVM_Value GetEnumerableConfigurableNames(JSVM_Env env,
                                                 JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType value_type0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &value_type0));

  NODE_API_ASSERT(
      env,
      value_type0 == jsvm_object,
      "Wrong type of arguments. Expects an object as first argument.");

  JSVM_Value output;
  NODE_API_CALL(
      env,
      OH_JSVM_GetAllPropertyNames(env,
                                  args[0],
                                  JSVM_KEY_INCLUDE_PROTOTYPES,
                                  JSVM_KEY_ENUMERABLE | JSVM_KEY_CONFIGURABLE,
                                  JSVM_KEY_NUMBERS_TO_STRINGS,
                                  &output));

  return output;
}

static JSVM_Value GetOwnConfigurableNames(JSVM_Env env,
                                          JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType value_type0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &value_type0));

  NODE_API_ASSERT(
      env,
      value_type0 == jsvm_object,
      "Wrong type of arguments. Expects an object as first argument.");

  JSVM_Value output;
  NODE_API_CALL(env,
                OH_JSVM_GetAllPropertyNames(env,
                                            args[0],
                                            JSVM_KEY_OWN_ONLY,
                                            JSVM_KEY_CONFIGURABLE,
                                            JSVM_KEY_NUMBERS_TO_STRINGS,
                                            &output));

  return output;
}

static JSVM_Value Set(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 3;
  JSVM_Value args[3];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 3, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_object,
      "Wrong type of arguments. Expects an object as first argument.");

  JSVM_ValueType valuetype1;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &valuetype1));

  NODE_API_ASSERT(env, valuetype1 == jsvm_string || valuetype1 == jsvm_symbol,
      "Wrong type of arguments. Expects a string or symbol as second.");

  NODE_API_CALL(env, OH_JSVM_SetProperty(env, args[0], args[1], args[2]));

  JSVM_Value valuetrue;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, true, &valuetrue));

  return valuetrue;
}

static JSVM_Value SetNamed(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 3;
  JSVM_Value args[3];
  char key[256] = "";
  size_t key_length;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 3, "Wrong number of arguments");

  JSVM_ValueType value_type0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &value_type0));

  NODE_API_ASSERT(env, value_type0 == jsvm_object,
      "Wrong type of arguments. Expects an object as first argument.");

  JSVM_ValueType value_type1;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &value_type1));

  NODE_API_ASSERT(env, value_type1 == jsvm_string,
      "Wrong type of arguments. Expects a string as second.");

  NODE_API_CALL(env,
      OH_JSVM_GetValueStringUtf8(env, args[1], key, 255, &key_length));
  key[255] = 0;
  NODE_API_ASSERT(env, key_length <= 255,
      "Cannot accommodate keys longer than 255 bytes");

  NODE_API_CALL(env, OH_JSVM_SetNamedProperty(env, args[0], key, args[2]));

  JSVM_Value value_true;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, true, &value_true));

  return value_true;
}

static JSVM_Value Has(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 2, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_object,
      "Wrong type of arguments. Expects an object as first argument.");

  JSVM_ValueType valuetype1;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &valuetype1));

  NODE_API_ASSERT(env, valuetype1 == jsvm_string || valuetype1 == jsvm_symbol,
      "Wrong type of arguments. Expects a string or symbol as second.");

  bool has_property;
  NODE_API_CALL(env, OH_JSVM_HasProperty(env, args[0], args[1], &has_property));

  JSVM_Value ret;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, has_property, &ret));

  return ret;
}

static JSVM_Value HasNamed(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  char key[256] = "";
  size_t key_length;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 2, "Wrong number of arguments");

  JSVM_ValueType value_type0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &value_type0));

  NODE_API_ASSERT(env, value_type0 == jsvm_object,
      "Wrong type of arguments. Expects an object as first argument.");

  JSVM_ValueType value_type1;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &value_type1));

  NODE_API_ASSERT(env, value_type1 == jsvm_string || value_type1 == jsvm_symbol,
      "Wrong type of arguments. Expects a string as second.");

  NODE_API_CALL(env,
      OH_JSVM_GetValueStringUtf8(env, args[1], key, 255, &key_length));
  key[255] = 0;
  NODE_API_ASSERT(env, key_length <= 255,
      "Cannot accommodate keys longer than 255 bytes");

  bool has_property;
  NODE_API_CALL(env, OH_JSVM_HasNamedProperty(env, args[0], key, &has_property));

  JSVM_Value ret;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, has_property, &ret));

  return ret;
}

static JSVM_Value HasOwn(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc == 2, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_object,
      "Wrong type of arguments. Expects an object as first argument.");

  // JSVM_ValueType valuetype1;
  // NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &valuetype1));
  //
  // NODE_API_ASSERT(env, valuetype1 == jsvm_string || valuetype1 == jsvm_symbol,
  //   "Wrong type of arguments. Expects a string or symbol as second.");

  bool has_property;
  NODE_API_CALL(env, OH_JSVM_HasOwnProperty(env, args[0], args[1], &has_property));

  JSVM_Value ret;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, has_property, &ret));

  return ret;
}

static JSVM_Value Delete(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));
  NODE_API_ASSERT(env, argc == 2, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));
  NODE_API_ASSERT(env, valuetype0 == jsvm_object,
      "Wrong type of arguments. Expects an object as first argument.");

  JSVM_ValueType valuetype1;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &valuetype1));
  NODE_API_ASSERT(env, valuetype1 == jsvm_string || valuetype1 == jsvm_symbol,
      "Wrong type of arguments. Expects a string or symbol as second.");

  bool result;
  JSVM_Value ret;
  NODE_API_CALL(env, OH_JSVM_DeleteProperty(env, args[0], args[1], &result));
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, result, &ret));

  return ret;
}

static JSVM_Value New(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value ret;
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &ret));

  JSVM_Value num;
  NODE_API_CALL(env, OH_JSVM_CreateInt32(env, 987654321, &num));

  NODE_API_CALL(env, OH_JSVM_SetNamedProperty(env, ret, "test_number", num));

  JSVM_Value str;
  const char* str_val = "test string";
  size_t str_len = strlen(str_val);
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(env, str_val, str_len, &str));

  NODE_API_CALL(env, OH_JSVM_SetNamedProperty(env, ret, "test_string", str));

  return ret;
}

static JSVM_Value Inflate(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_object,
      "Wrong type of arguments. Expects an object as first argument.");

  JSVM_Value obj = args[0];
  JSVM_Value propertynames;
  NODE_API_CALL(env, OH_JSVM_GetPropertyNames(env, obj, &propertynames));

  uint32_t i, length;
  NODE_API_CALL(env, OH_JSVM_GetArrayLength(env, propertynames, &length));

  for (i = 0; i < length; i++) {
    JSVM_Value property_str;
    NODE_API_CALL(env, OH_JSVM_GetElement(env, propertynames, i, &property_str));

    JSVM_Value value;
    NODE_API_CALL(env, OH_JSVM_GetProperty(env, obj, property_str, &value));

    double double_val;
    NODE_API_CALL(env, OH_JSVM_GetValueDouble(env, value, &double_val));
    NODE_API_CALL(env, OH_JSVM_CreateDouble(env, double_val + 1, &value));
    NODE_API_CALL(env, OH_JSVM_SetProperty(env, obj, property_str, value));
  }

  return obj;
}

static JSVM_Value Wrap(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value arg;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &arg, NULL, NULL));

  NODE_API_CALL(env, OH_JSVM_Wrap(env, arg, &test_value, NULL, NULL, NULL));
  return NULL;
}

static JSVM_Value Unwrap(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value arg;
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &arg, NULL, NULL));

  void* data;
  NODE_API_CALL(env, OH_JSVM_Unwrap(env, arg, &data));

  bool is_expected = (data != NULL && *(int*)data == 3);
  JSVM_Value result;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, is_expected, &result));
  return result;
}

static JSVM_Value TestSetProperty(JSVM_Env env,
                                  JSVM_CallbackInfo info) {
  JSVM_Status status;
  JSVM_Value object, key, value;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &object));

  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(env, "", JSVM_AUTO_LENGTH, &key));

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &value));

  status = OH_JSVM_SetProperty(NULL, object, key, value);

  add_returned_status(env,
                      "envIsNull",
                      object,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      status);

  OH_JSVM_SetProperty(env, NULL, key, value);

  add_last_status(env, "objectIsNull", object);

  OH_JSVM_SetProperty(env, object, NULL, value);

  add_last_status(env, "keyIsNull", object);

  OH_JSVM_SetProperty(env, object, key, NULL);

  add_last_status(env, "valueIsNull", object);

  return object;
}

static JSVM_Value TestHasProperty(JSVM_Env env,
                                  JSVM_CallbackInfo info) {
  JSVM_Status status;
  JSVM_Value object, key;
  bool result;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &object));

  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(env, "", JSVM_AUTO_LENGTH, &key));

  status = OH_JSVM_HasProperty(NULL, object, key, &result);

  add_returned_status(env,
                      "envIsNull",
                      object,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      status);

  OH_JSVM_HasProperty(env, NULL, key, &result);

  add_last_status(env, "objectIsNull", object);

  OH_JSVM_HasProperty(env, object, NULL, &result);

  add_last_status(env, "keyIsNull", object);

  OH_JSVM_HasProperty(env, object, key, NULL);

  add_last_status(env, "resultIsNull", object);

  return object;
}

static JSVM_Value TestGetProperty(JSVM_Env env,
                                  JSVM_CallbackInfo info) {
  JSVM_Status status;
  JSVM_Value object, key, result;

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &object));

  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(env, "", JSVM_AUTO_LENGTH, &key));

  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &result));

  status = OH_JSVM_GetProperty(NULL, object, key, &result);

  add_returned_status(env,
                      "envIsNull",
                      object,
                      "Invalid argument",
                      JSVM_INVALID_ARG,
                      status);

  OH_JSVM_GetProperty(env, NULL, key, &result);

  add_last_status(env, "objectIsNull", object);

  OH_JSVM_GetProperty(env, object, NULL, &result);

  add_last_status(env, "keyIsNull", object);

  OH_JSVM_GetProperty(env, object, key, NULL);

  add_last_status(env, "resultIsNull", object);

  return object;
}

static JSVM_Value TestFreeze(JSVM_Env env,
                             JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value object = args[0];
  NODE_API_CALL(env, OH_JSVM_ObjectFreeze(env, object));

  return object;
}

static JSVM_Value TestSeal(JSVM_Env env,
                           JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  JSVM_Value object = args[0];
  NODE_API_CALL(env, OH_JSVM_ObjectSeal(env, object));

  return object;
}

// We create two type tags. They are basically 128-bit UUIDs.
#define TYPE_TAG_COUNT 5
static const JSVM_TypeTag type_tags[TYPE_TAG_COUNT] = {
    {0xdaf987b3cc62481a, 0xb745b0497f299531},
    {0xbb7936c374084d9b, 0xa9548d0762eeedb9},
    {0xa5ed9ce2e4c00c38, 0},
    {0, 0},
    {0xa5ed9ce2e4c00c38, 0xdaf987b3cc62481a},
};
#define VALIDATE_TYPE_INDEX(env, type_index)                                   \
  do {                                                                         \
    if ((type_index) >= TYPE_TAG_COUNT) {                                      \
      NODE_API_CALL((env),                                                     \
                    OH_JSVM_ThrowRangeError((env),                              \
                                           "NODE_API_TEST_INVALID_TYPE_INDEX", \
                                           "Invalid type index"));             \
    }                                                                          \
  } while (0)

static JSVM_Value
TypeTaggedInstance(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  uint32_t type_index;
  JSVM_Value instance, which_type;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &which_type, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_GetValueUint32(env, which_type, &type_index));
  VALIDATE_TYPE_INDEX(env, type_index);
  NODE_API_CALL(env, OH_JSVM_CreateObject(env, &instance));
  NODE_API_CALL(env, OH_JSVM_TypeTagObject(env, instance, &type_tags[type_index]));

  return instance;
}

// V8 will not allowe us to construct an external with a NULL data value.
#define IN_LIEU_OF_NULL ((void*)0x1)

static JSVM_Value PlainExternal(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value instance;

  NODE_API_CALL(
      env, OH_JSVM_CreateExternal(env, IN_LIEU_OF_NULL, NULL, NULL, &instance));

  return instance;
}

static JSVM_Value TypeTaggedExternal(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  uint32_t type_index;
  JSVM_Value instance, which_type;

  NODE_API_CALL(env,
                OH_JSVM_GetCbInfo(env, info, &argc, &which_type, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_GetValueUint32(env, which_type, &type_index));
  VALIDATE_TYPE_INDEX(env, type_index);
  NODE_API_CALL(
      env, OH_JSVM_CreateExternal(env, IN_LIEU_OF_NULL, NULL, NULL, &instance));
  NODE_API_CALL(env,
                OH_JSVM_TypeTagObject(env, instance, &type_tags[type_index]));

  return instance;
}

static JSVM_Value
CheckTypeTag(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  bool result;
  JSVM_Value argv[2], js_result;
  uint32_t type_index;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, argv, NULL, NULL));
  NODE_API_CALL(env, OH_JSVM_GetValueUint32(env, argv[0], &type_index));
  VALIDATE_TYPE_INDEX(env, type_index);
  NODE_API_CALL(env, OH_JSVM_CheckObjectTypeTag(env,
                                            argv[1],
                                            &type_tags[type_index],
                                            &result));
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, result, &js_result));

  return js_result;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
      DECLARE_NODE_API_PROPERTY("Get", Get),
      DECLARE_NODE_API_PROPERTY("GetNamed", GetNamed),
      DECLARE_NODE_API_PROPERTY("GetPropertyNames", GetPropertyNames),
      DECLARE_NODE_API_PROPERTY("GetSymbolNames", GetSymbolNames),
      DECLARE_NODE_API_PROPERTY("GetEnumerableWritableNames",
                                GetEnumerableWritableNames),
      DECLARE_NODE_API_PROPERTY("GetOwnWritableNames", GetOwnWritableNames),
      DECLARE_NODE_API_PROPERTY("GetEnumerableConfigurableNames",
                                GetEnumerableConfigurableNames),
      DECLARE_NODE_API_PROPERTY("GetOwnConfigurableNames",
                                GetOwnConfigurableNames),
      DECLARE_NODE_API_PROPERTY("Set", Set),
      DECLARE_NODE_API_PROPERTY("SetNamed", SetNamed),
      DECLARE_NODE_API_PROPERTY("Has", Has),
      DECLARE_NODE_API_PROPERTY("HasNamed", HasNamed),
      DECLARE_NODE_API_PROPERTY("HasOwn", HasOwn),
      DECLARE_NODE_API_PROPERTY("Delete", Delete),
      DECLARE_NODE_API_PROPERTY("New", New),
      DECLARE_NODE_API_PROPERTY("Inflate", Inflate),
      DECLARE_NODE_API_PROPERTY("Wrap", Wrap),
      DECLARE_NODE_API_PROPERTY("Unwrap", Unwrap),
      DECLARE_NODE_API_PROPERTY("TestSetProperty", TestSetProperty),
      DECLARE_NODE_API_PROPERTY("TestHasProperty", TestHasProperty),
      DECLARE_NODE_API_PROPERTY("TypeTaggedInstance", TypeTaggedInstance),
      DECLARE_NODE_API_PROPERTY("TypeTaggedExternal", TypeTaggedExternal),
      DECLARE_NODE_API_PROPERTY("PlainExternal", PlainExternal),
      DECLARE_NODE_API_PROPERTY("CheckTypeTag", CheckTypeTag),
      DECLARE_NODE_API_PROPERTY("TestGetProperty", TestGetProperty),
      DECLARE_NODE_API_PROPERTY("TestFreeze", TestFreeze),
      DECLARE_NODE_API_PROPERTY("TestSeal", TestSeal),
  };

  init_test_null(env, exports);

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  return exports;
}
EXTERN_C_END
