// For the purpose of this test we use libuv's threading library. When deciding
// on a threading library for a new project it bears remembering that in the
// future libuv may introduce API changes which may render it non-ABI-stable,
// which, in turn, may affect the ABI stability of the project despite its use
// of N-API.
#include <uv.h>
#include <node_api.h>
#include "../../js-native-api/common.h"

static uv_thread_t uv_thread;

static void work_thread(void* data) {
  jsvm_fatal_error("work_thread", JSVM_AUTO_LENGTH,
      "foobar", JSVM_AUTO_LENGTH);
}

static JSVM_Value Test(JSVM_Env env, JSVM_CallbackInfo info) {
  jsvm_fatal_error("test_fatal::Test", JSVM_AUTO_LENGTH,
                   "fatal message", JSVM_AUTO_LENGTH);
  return NULL;
}

static JSVM_Value TestThread(JSVM_Env env, JSVM_CallbackInfo info) {
  NODE_API_ASSERT(env,
      (uv_thread_create(&uv_thread, work_thread, NULL) == 0),
      "Thread creation");
  return NULL;
}

static JSVM_Value TestStringLength(JSVM_Env env, JSVM_CallbackInfo info) {
  jsvm_fatal_error("test_fatal::TestStringLength", 16, "fatal message", 13);
  return NULL;
}

static JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor properties[] = {
    DECLARE_NODE_API_PROPERTY("Test", Test),
    DECLARE_NODE_API_PROPERTY("TestStringLength", TestStringLength),
    DECLARE_NODE_API_PROPERTY("TestThread", TestThread),
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(properties) / sizeof(*properties), properties));

  return exports;
}

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
