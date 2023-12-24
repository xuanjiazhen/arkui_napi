#include <limits.h>  // INT_MAX
#include <stdlib.h>
#include <string.h>
#define JSVM_EXPERIMENTAL
#include <jsvm.h>
#include "../common.h"
#include "../entry_point.h"
#include "test_null.h"

enum length_type { actual_length, auto_length };

static JSVM_Status validate_and_retrieve_single_string_arg(
    JSVM_Env env, JSVM_CallbackInfo info, JSVM_Value* arg) {
  size_t argc = 1;
  NODE_API_CHECK_STATUS(OH_JSVM_GetCbInfo(env, info, &argc, arg, NULL, NULL));

  NODE_API_ASSERT_STATUS(env, argc >= 1, "Wrong number of arguments");

  JSVM_ValueType valuetype;
  NODE_API_CHECK_STATUS(OH_JSVM_Typeof(env, *arg, &valuetype));

  NODE_API_ASSERT_STATUS(env,
                         valuetype == jsvm_string,
                         "Wrong type of argment. Expects a string.");

  return JSVM_OK;
}

// These help us factor out code that is common between the bindings.
typedef JSVM_Status (*OneByteCreateAPI)(JSVM_Env,
                                        const char*,
                                        size_t,
                                        JSVM_Value*);
typedef JSVM_Status (*OneByteGetAPI)(
    JSVM_Env, JSVM_Value, char*, size_t, size_t*);
typedef JSVM_Status (*TwoByteCreateAPI)(JSVM_Env,
                                        const char16_t*,
                                        size_t,
                                        JSVM_Value*);
typedef JSVM_Status (*TwoByteGetAPI)(
    JSVM_Env, JSVM_Value, char16_t*, size_t, size_t*);

// Test passing back the one-byte string we got from JS.
static JSVM_Value TestOneByteImpl(JSVM_Env env,
                                  JSVM_CallbackInfo info,
                                  OneByteGetAPI get_api,
                                  OneByteCreateAPI create_api,
                                  enum length_type length_mode) {
  JSVM_Value args[1];
  NODE_API_CALL(env, validate_and_retrieve_single_string_arg(env, info, args));

  char buffer[128];
  size_t buffer_size = 128;
  size_t copied;

  NODE_API_CALL(env, get_api(env, args[0], buffer, buffer_size, &copied));

  JSVM_Value output;
  if (length_mode == auto_length) {
    copied = JSVM_AUTO_LENGTH;
  }
  NODE_API_CALL(env, create_api(env, buffer, copied, &output));

  return output;
}

// Test passing back the two-byte string we got from JS.
static JSVM_Value TestTwoByteImpl(JSVM_Env env,
                                  JSVM_CallbackInfo info,
                                  TwoByteGetAPI get_api,
                                  TwoByteCreateAPI create_api,
                                  enum length_type length_mode) {
  JSVM_Value args[1];
  NODE_API_CALL(env, validate_and_retrieve_single_string_arg(env, info, args));

  char16_t buffer[128];
  size_t buffer_size = 128;
  size_t copied;

  NODE_API_CALL(env, get_api(env, args[0], buffer, buffer_size, &copied));

  JSVM_Value output;
  if (length_mode == auto_length) {
    copied = JSVM_AUTO_LENGTH;
  }
  NODE_API_CALL(env, create_api(env, buffer, copied, &output));

  return output;
}

static void free_string(JSVM_Env env, void* data, void* hint) {
  free(data);
}

static JSVM_Status create_external_latin1(JSVM_Env env,
                                          const char* string,
                                          size_t length,
                                          JSVM_Value* result) {
  JSVM_Status status;
  // Initialize to true, because that is the value we don't want.
  bool copied = true;
  char* string_copy;
  const size_t actual_length =
      (length == JSVM_AUTO_LENGTH ? strlen(string) : length);
  const size_t length_bytes = (actual_length + 1) * sizeof(*string_copy);
  string_copy = malloc(length_bytes);
  memcpy(string_copy, string, length_bytes);
  string_copy[actual_length] = 0;

  status = OH_JSVM_CreateExternalStringLatin1(
      env, string_copy, length, free_string, NULL, result, &copied);
  // We do not want the string to be copied.
  if (copied) {
    return JSVM_GENERIC_FAILURE;
  }
  if (status != JSVM_OK) {
    free(string_copy);
    return status;
  }
  return JSVM_OK;
}

// strlen for char16_t. Needed in case we're copying a string of length
// JSVM_AUTO_LENGTH.
static size_t strlen16(const char16_t* string) {
  for (const char16_t* iter = string;; iter++) {
    if (*iter == 0) {
      return iter - string;
    }
  }
  // We should never get here.
  abort();
}

static JSVM_Status create_external_utf16(JSVM_Env env,
                                         const char16_t* string,
                                         size_t length,
                                         JSVM_Value* result) {
  JSVM_Status status;
  // Initialize to true, because that is the value we don't want.
  bool copied = true;
  char16_t* string_copy;
  const size_t actual_length =
      (length == JSVM_AUTO_LENGTH ? strlen16(string) : length);
  const size_t length_bytes = (actual_length + 1) * sizeof(*string_copy);
  string_copy = malloc(length_bytes);
  memcpy(string_copy, string, length_bytes);
  string_copy[actual_length] = 0;

  status = OH_JSVM_CreateExternalStringUtf16(
      env, string_copy, length, free_string, NULL, result, &copied);
  if (status != JSVM_OK) {
    free(string_copy);
    return status;
  }

  return JSVM_OK;
}

static JSVM_Value TestLatin1(JSVM_Env env, JSVM_CallbackInfo info) {
  return TestOneByteImpl(env,
                         info,
                         OH_JSVM_GetValueStringLatin1,
                         OH_JSVM_CreateStringLatin1,
                         actual_length);
}

static JSVM_Value TestUtf8(JSVM_Env env, JSVM_CallbackInfo info) {
  return TestOneByteImpl(env,
                         info,
                         OH_JSVM_GetValueStringUtf8,
                         OH_JSVM_CreateStringUtf8,
                         actual_length);
}

static JSVM_Value TestUtf16(JSVM_Env env, JSVM_CallbackInfo info) {
  return TestTwoByteImpl(env,
                         info,
                         OH_JSVM_GetValueStringUtf16,
                         OH_JSVM_CreateStringUtf16,
                         actual_length);
}

static JSVM_Value TestLatin1AutoLength(JSVM_Env env, JSVM_CallbackInfo info) {
  return TestOneByteImpl(env,
                         info,
                         OH_JSVM_GetValueStringLatin1,
                         OH_JSVM_CreateStringLatin1,
                         auto_length);
}

static JSVM_Value TestUtf8AutoLength(JSVM_Env env, JSVM_CallbackInfo info) {
  return TestOneByteImpl(env,
                         info,
                         OH_JSVM_GetValueStringUtf8,
                         OH_JSVM_CreateStringUtf8,
                         auto_length);
}

static JSVM_Value TestUtf16AutoLength(JSVM_Env env, JSVM_CallbackInfo info) {
  return TestTwoByteImpl(env,
                         info,
                         OH_JSVM_GetValueStringUtf16,
                         OH_JSVM_CreateStringUtf16,
                         auto_length);
}

static JSVM_Value TestLatin1External(JSVM_Env env, JSVM_CallbackInfo info) {
  return TestOneByteImpl(env,
                         info,
                         OH_JSVM_GetValueStringLatin1,
                         create_external_latin1,
                         actual_length);
}

static JSVM_Value TestUtf16External(JSVM_Env env, JSVM_CallbackInfo info) {
  return TestTwoByteImpl(env,
                         info,
                         OH_JSVM_GetValueStringUtf16,
                         create_external_utf16,
                         actual_length);
}

static JSVM_Value TestLatin1ExternalAutoLength(JSVM_Env env,
                                               JSVM_CallbackInfo info) {
  return TestOneByteImpl(env,
                         info,
                         OH_JSVM_GetValueStringLatin1,
                         create_external_latin1,
                         auto_length);
}

static JSVM_Value TestUtf16ExternalAutoLength(JSVM_Env env,
                                              JSVM_CallbackInfo info) {
  return TestTwoByteImpl(env,
                         info,
                         OH_JSVM_GetValueStringUtf16,
                         create_external_utf16,
                         auto_length);
}

static JSVM_Value TestLatin1Insufficient(JSVM_Env env,
                                         JSVM_CallbackInfo info) {
  JSVM_Value args[1];
  NODE_API_CALL(env, validate_and_retrieve_single_string_arg(env, info, args));

  char buffer[4];
  size_t buffer_size = 4;
  size_t copied;

  NODE_API_CALL(
      env,
      OH_JSVM_GetValueStringLatin1(env, args[0], buffer, buffer_size, &copied));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateStringLatin1(env, buffer, copied, &output));

  return output;
}

static JSVM_Value TestUtf8Insufficient(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value args[1];
  NODE_API_CALL(env, validate_and_retrieve_single_string_arg(env, info, args));

  char buffer[4];
  size_t buffer_size = 4;
  size_t copied;

  NODE_API_CALL(
      env,
      OH_JSVM_GetValueStringUtf8(env, args[0], buffer, buffer_size, &copied));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf8(env, buffer, copied, &output));

  return output;
}

static JSVM_Value TestUtf16Insufficient(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value args[1];
  NODE_API_CALL(env, validate_and_retrieve_single_string_arg(env, info, args));

  char16_t buffer[4];
  size_t buffer_size = 4;
  size_t copied;

  NODE_API_CALL(
      env,
      OH_JSVM_GetValueStringUtf16(env, args[0], buffer, buffer_size, &copied));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateStringUtf16(env, buffer, copied, &output));

  return output;
}

static JSVM_Value Utf16Length(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value args[1];
  NODE_API_CALL(env, validate_and_retrieve_single_string_arg(env, info, args));

  size_t length;
  NODE_API_CALL(env,
                OH_JSVM_GetValueStringUtf16(env, args[0], NULL, 0, &length));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateUint32(env, (uint32_t)length, &output));

  return output;
}

static JSVM_Value Utf8Length(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value args[1];
  NODE_API_CALL(env, validate_and_retrieve_single_string_arg(env, info, args));

  size_t length;
  NODE_API_CALL(env,
                OH_JSVM_GetValueStringUtf8(env, args[0], NULL, 0, &length));

  JSVM_Value output;
  NODE_API_CALL(env, OH_JSVM_CreateUint32(env, (uint32_t)length, &output));

  return output;
}

static JSVM_Value TestLargeUtf8(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value output;
  if (SIZE_MAX > INT_MAX) {
    NODE_API_CALL(
        env, OH_JSVM_CreateStringUtf8(env, "", ((size_t)INT_MAX) + 1, &output));
  } else {
    // just throw the expected error as there is nothing to test
    // in this case since we can't overflow
    NODE_API_CALL(env, OH_JSVM_ThrowError(env, NULL, "Invalid argument"));
  }

  return output;
}

static JSVM_Value TestLargeLatin1(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value output;
  if (SIZE_MAX > INT_MAX) {
    NODE_API_CALL(
        env,
        OH_JSVM_CreateStringLatin1(env, "", ((size_t)INT_MAX) + 1, &output));
  } else {
    // just throw the expected error as there is nothing to test
    // in this case since we can't overflow
    NODE_API_CALL(env, OH_JSVM_ThrowError(env, NULL, "Invalid argument"));
  }

  return output;
}

static JSVM_Value TestLargeUtf16(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value output;
  if (SIZE_MAX > INT_MAX) {
    NODE_API_CALL(
        env,
        OH_JSVM_CreateStringUtf16(
            env, ((const char16_t*)""), ((size_t)INT_MAX) + 1, &output));
  } else {
    // just throw the expected error as there is nothing to test
    // in this case since we can't overflow
    NODE_API_CALL(env, OH_JSVM_ThrowError(env, NULL, "Invalid argument"));
  }

  return output;
}

static JSVM_Value TestMemoryCorruption(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args[1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc == 1, "Wrong number of arguments");

  char buf[10] = {0};
  NODE_API_CALL(env, OH_JSVM_GetValueStringUtf8(env, args[0], buf, 0, NULL));

  char zero[10] = {0};
  if (memcmp(buf, zero, sizeof(buf)) != 0) {
    NODE_API_CALL(env, OH_JSVM_ThrowError(env, NULL, "Buffer overwritten"));
  }

  return NULL;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor properties[] = {
      DECLARE_NODE_API_PROPERTY("TestLatin1", TestLatin1),
      DECLARE_NODE_API_PROPERTY("TestLatin1AutoLength", TestLatin1AutoLength),
      DECLARE_NODE_API_PROPERTY("TestLatin1External", TestLatin1External),
      DECLARE_NODE_API_PROPERTY("TestLatin1ExternalAutoLength",
                                TestLatin1ExternalAutoLength),
      DECLARE_NODE_API_PROPERTY("TestLatin1Insufficient",
                                TestLatin1Insufficient),
      DECLARE_NODE_API_PROPERTY("TestUtf8", TestUtf8),
      DECLARE_NODE_API_PROPERTY("TestUtf8AutoLength", TestUtf8AutoLength),
      DECLARE_NODE_API_PROPERTY("TestUtf8Insufficient", TestUtf8Insufficient),
      DECLARE_NODE_API_PROPERTY("TestUtf16", TestUtf16),
      DECLARE_NODE_API_PROPERTY("TestUtf16AutoLength", TestUtf16AutoLength),
      DECLARE_NODE_API_PROPERTY("TestUtf16External", TestUtf16External),
      DECLARE_NODE_API_PROPERTY("TestUtf16ExternalAutoLength",
                                TestUtf16ExternalAutoLength),
      DECLARE_NODE_API_PROPERTY("TestUtf16Insufficient", TestUtf16Insufficient),
      DECLARE_NODE_API_PROPERTY("Utf16Length", Utf16Length),
      DECLARE_NODE_API_PROPERTY("Utf8Length", Utf8Length),
      DECLARE_NODE_API_PROPERTY("TestLargeUtf8", TestLargeUtf8),
      DECLARE_NODE_API_PROPERTY("TestLargeLatin1", TestLargeLatin1),
      DECLARE_NODE_API_PROPERTY("TestLargeUtf16", TestLargeUtf16),
      DECLARE_NODE_API_PROPERTY("TestMemoryCorruption", TestMemoryCorruption),
  };

  init_test_null(env, exports);

  NODE_API_CALL(
      env,
      OH_JSVM_DefineProperties(
          env, exports, sizeof(properties) / sizeof(*properties), properties));

  return exports;
}
EXTERN_C_END
