#include <inttypes.h>
#include <jsvm.h>
#include <limits.h>
#include <stdio.h>
#include "../common.h"
#include "../entry_point.h"

static JSVM_Value IsLossless(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 2;
  JSVM_Value args[2];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  bool is_signed;
  NODE_API_CALL(env, OH_JSVM_GetValueBool(env, args[1], &is_signed));

  bool lossless;

  if (is_signed) {
    int64_t input;
    NODE_API_CALL(env, OH_JSVM_GetValueBigintInt64(env, args[0], &input, &lossless));
  } else {
    uint64_t input;
    NODE_API_CALL(env, OH_JSVM_GetValueBigintUint64(env, args[0], &input, &lossless));
  }

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_GetBoolean(env, lossless, &output));

  return output;
}

static JSVM_Value TestInt64(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_bigint,
      "Wrong type of arguments. Expects a bigint as first argument.");

  int64_t input;
  bool lossless;
  NODE_API_CALL(env, OH_JSVM_GetValueBigintInt64(env, args[0], &input, &lossless));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateBigintInt64(env, input, &output));

  return output;
}

static JSVM_Value TestUint64(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_bigint,
      "Wrong type of arguments. Expects a bigint as first argument.");

  uint64_t input;
  bool lossless;
  NODE_API_CALL(env, OH_JSVM_GetValueBigintUint64(
        env, args[0], &input, &lossless));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateBigintUint64(env, input, &output));

  return output;
}

static JSVM_Value TestWords(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[0], &valuetype0));

  NODE_API_ASSERT(env, valuetype0 == jsvm_bigint,
      "Wrong type of arguments. Expects a bigint as first argument.");

  size_t expected_word_count;
  NODE_API_CALL(env, OH_JSVM_GetValueBigintWords(
        env, args[0], NULL, &expected_word_count, NULL));

  int sign_bit;
  size_t word_count = 10;
  uint64_t words[10];

  NODE_API_CALL(env, OH_JSVM_GetValueBigintWords(
        env, args[0], &sign_bit, &word_count, words));

  NODE_API_ASSERT(env, word_count == expected_word_count,
      "word counts do not match");

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateBigintWords(
        env, sign_bit, word_count, words, &output));

  return output;
}

// throws RangeError
static JSVM_Value CreateTooBigBigInt(JSVM_Env env, JSVM_CallbackInfo info) {
  int sign_bit = 0;
  size_t word_count = SIZE_MAX;
  uint64_t words[10] = {0};

  JSVM_Value output;

  NODE_API_CALL(env, OH_JSVM_CreateBigintWords(
        env, sign_bit, word_count, words, &output));

  return output;
}

// Test that we correctly forward exceptions from the engine.
static JSVM_Value MakeBigIntWordsThrow(JSVM_Env env, JSVM_CallbackInfo info) {
  uint64_t words[10] = {0};
  JSVM_Value output;

  JSVM_Status status = OH_JSVM_CreateBigintWords(env,
                                                0,
                                                INT_MAX,
                                                words,
                                                &output);
  if (status != JSVM_PENDING_EXCEPTION)
    OH_JSVM_ThrowError(env, NULL, "Expected status `JSVM_PENDING_EXCEPTION`");

  return NULL;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("IsLossless", IsLossless),
    DECLARE_NODE_API_PROPERTY("TestInt64", TestInt64),
    DECLARE_NODE_API_PROPERTY("TestUint64", TestUint64),
    DECLARE_NODE_API_PROPERTY("TestWords", TestWords),
    DECLARE_NODE_API_PROPERTY("CreateTooBigBigInt", CreateTooBigBigInt),
    DECLARE_NODE_API_PROPERTY("MakeBigIntWordsThrow", MakeBigIntWordsThrow),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  return exports;
}
EXTERN_C_END
