#include "test.h"
#include "gtest/gtest.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "securec.h"
#include "utils/log.h"
#include "common/common.h"

napi_deferred deferred = NULL;

static napi_value createPromise(napi_env env, napi_callback_info info) {
  napi_value promise;

  // We do not overwrite an existing deferred.
  if (deferred != NULL) {
    return NULL;
  }

  NODE_API_CALL(env, napi_create_promise(env, &deferred, &promise));

  return promise;
}

static napi_value concludeCurrentPromise(napi_env env, napi_callback_info info) {
  napi_value argv[2];
  size_t argc = 2;
  bool resolution;

  NODE_API_CALL(env, napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
  NODE_API_CALL(env, napi_get_value_bool(env, argv[1], &resolution));
  if (resolution) {
    NODE_API_CALL(env, napi_resolve_deferred(env, deferred, argv[0]));
  } else {
    NODE_API_CALL(env, napi_reject_deferred(env, deferred, argv[0]));
  }

  deferred = NULL;

  return NULL;
}

static napi_value isPromise(napi_env env, napi_callback_info info) {
  napi_value promise, result;
  size_t argc = 1;
  bool is_promise;

  NODE_API_CALL(env, napi_get_cb_info(env, info, &argc, &promise, NULL, NULL));
  NODE_API_CALL(env, napi_is_promise(env, promise, &is_promise));
  NODE_API_CALL(env, napi_get_boolean(env, is_promise, &result));

  return result;
}


EXTERN_C_START
napi_value Init(napi_env env, napi_value exports) {
  napi_property_descriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("createPromise", createPromise),
    DECLARE_NODE_API_PROPERTY("concludeCurrentPromise", concludeCurrentPromise),
    DECLARE_NODE_API_PROPERTY("isPromise", isPromise),
  };

  NODE_API_CALL(env, napi_define_properties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  return exports;
}
EXTERN_C_END


class NapiPromiseTest : public NativeEngineTest {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "NapiPromiseTest SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "NapiPromiseTest TearDownTestCase";
    }

    void SetUp() override {}
    void TearDown() override {}
};

/**
 * @tc.name: PromiseTest001
 * @tc.desc: 
 * @tc.type: FUNC
 */

 // A resolution
 
HWTEST_F(NapiPromiseTest, PromiseTest001, testing::ext::TestSize.Level1)
{
    HILOG_INFO("Promise_Test_0100 start");
    napi_env env = (napi_env)engine_;
    const int expected_result = 42;
    
    //auto promise = createPromise(env,nullptr);
    createPromise(env,nullptr).then([&](int result) {
        EXPECT_EQ(result, expected_result);
    }, []() {
        SUCCEED();
    });
    /*
    promise.then([&](int result) {
        EXPECT_EQ(result, expected_result);
    }, []() {
        SUCCEED();
    });*/
    
    concludeCurrentPromise(expected_result, true);
    HILOG_INFO("Promise_Test_0100 end");
}

/**
 * @tc.name: PromiseTest002
 * @tc.desc: 
 * @tc.type: FUNC
 */

 // A rejection
 /*
HWTEST_F(NapiPromiseTest, PromiseTest002, testing::ext::TestSize.Level1)
{
    HILOG_INFO("Promise_Test_0200 start");
    napi_env env = (napi_env)engine_;
    std::string expected_result = "It\'s not you, it\'s me.";
    auto promise = createPromise(env,nullptr);
    promise->then([]() {
        SUCCEED();
    },[&](int result) {
        EXPECT_EQ(result, expected_result);
    });
    
    concludeCurrentPromise(expected_result, false);

    HILOG_INFO("Promise_Test_0200 end");
}*/