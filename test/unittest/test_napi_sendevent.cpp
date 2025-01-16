#include "test.h"
#include "js_native_api_types.h"
#include "native_engine.h"
#include "napi/native_node_api.h"
#include "native_engine/native_event.h"
#include "napi/native_api.h"


class NapiSendEventTest:public NativeEngineTest{
public:
    static void SetUpTestSuite()
    {
    }
    static void TearDownTestSuite()
    {

    }
    void SetUp() override {}
    void TearDown() override {}
}


/**
 * @tc.name: SendEventTest001
 * @tc.desc: Test napi_send_event invalidParams
 * @tc.type:FUNC
 */
HWTEST_F(NapiSendEventTest, SendEventTest001, testing::ext::TestSize.Level1){
    ASSERT_NE(engine_,nullptr);
    napi_env env = (napi_env)engine_;
    const std::function<void()> cb = [] -> void {};
    auto status = napi_send_event(nullptr,cb,napi_eprio_high);
    EXPECT_EQ(status, napi_status::napi_invalid_arg);
    status = napi_send_event(env,nullptr,napi_eprio_high);
    EXPECT_EQ(status, napi_status::napi_invalid_arg);
    status = napi_send_event(env,cb,static_cast<napi_event_priority>(-1));
    EXPECT_EQ(status, napi_status::napi_invalid_arg);
    status = napi_send_event(env,cb,static_cast<napi_event_priority>(5));
    EXPECT_EQ(status, napi_status::napi_invalid_arg);
}

/**
 * @tc.name: SendEventTest002
 * @tc.desc: Test napi_send_event sendEventByHandler
 * @tc.type:FUNC
 */
HWTEST_F(NapiSendEventTest, SendEventTest002, testing::ext::TestSize.Level1){
    ASSERT_NE(engine_,nullptr);
    napi_env env = (napi_env)engine_;
    const std::function<void()> cb = [] -> void {};
    if(eventHandler_ == nullptr) {
        auto runner = OHOS::AppExecFwk::EventRunner::Create(false);
        EXPECT_NE(runner, nullptr);
        eventHandler_ = std::make_shared<OHOS::AppExecFwk::EventHandler>(runner);
    }
    EXPECT_NE(eventHandler_, nullptr);
    auto status = napi_send_event(env,cb,napi_eprio_high);
    EXPECT_EQ(status, napi_status::napi_ok);
}

/**
 * @tc.name: SendEventTest003
 * @tc.desc: Test napi_send_event sendEventByUv
 * @tc.type:FUNC
 */
HWTEST_F(NapiSendEventTest, SendEventTest003, testing::ext::TestSize.Level1){
    ASSERT_NE(engine_,nullptr);
    napi_env env = (napi_env)engine_;
    const std::function<void()> cb = [] -> void {};
    eventHandler_ = nullptr;
    auto status = napi_send_event(env,cb,napi_eprio_high);
    EXPECT_EQ(status, napi_status::napi_ok);
}

/**
 * @tc.name: SendCancelableEent001
 * @tc.desc: Test napi_send_cancelable_event invalidParams
 * @tc.type:FUNC
 */
HWTEST_F(NapiSendEventTest, SendCancelableEent001, testing::ext::TestSize.Level1){
    ASSERT_NE(engine_,nullptr);
    napi_env env = reinterpret_cast<napi_env>(engine_);
    uint64_t handleId = 0;
    char data[] = "test data";
    const std::function<void()> cb = [] -> void {
        HILOG_INFO("function called");
    };
    auto status = napi_send_cancelable_event(nullptr, cb, data, napi_eprio_high, &handleId, "default");
    EXPECT_EQ(status, napi_status::napi_invalid_arg);
    status = napi_send_cancelable_event(env, nullptr, data, napi_eprio_high, &handleId, "default");
    EXPECT_EQ(status, napi_status::napi_invalid_arg);
    status = napi_send_cancelable_event(env, cb, nullptr, napi_eprio_high,&handleId, "default"); 
    EXPECT_EQ(status, napi_status::napi_invalid_arg);
    status = napi_send_cancelable_event(env, cb, data, napi_event_priority(napi_eprio_idle + 1), &handleId, "default");
    EXPECT_EQ(status, napi_status::napi_invalid_arg);
    status = napi_send_cancelable_event(env, cb, data, napi_eprio_high , nullptr, "default");
    EXPECT_EQ(status, napi_status::napi_invalid_arg);
}

/**
 * @tc.name: SendCancelableEent002
 * @tc.desc: Test napi_send_cancelable_event sendEventByHandler
 * @tc.type:FUNC
 */
HWTEST_F(NapiSendEventTest, SendCancelableEent002, testing::ext::TestSize.Level1){
    ASSERT_NE(engine_,nullptr);
    napi_env env = (napi_env)engine_;
    const std::function<void()> cb = [] -> void {
        HILOG_INFO("function called");
    };
    char data[] = "test data";
    uint64_t handleId = 0;
    if(eventHandler_ == nullptr) {
        auto runner = OHOS::AppExecFwk::EventRunner::Create(false);
        ASSERT_NE(runner, nullptr);
        eventHandler_ = std::make_shared<OHOS::AppExecFwk::EventHandler>(runner);
        ASSERT_NE(eventHandler_, nullptr);
    }
    auto status = napi_send_cancelable_event(env, cb, data, napi_eprio_high , &handleId, "default");
    ASSERT_EQ(status,napi_status::napi_ok);
}

/**
 * @tc.name: SendCancelableEent003
 * @tc.desc: Test napi_send_cancelable_event sendEventByUv
 * @tc.type:FUNC
 */
HWTEST_F(NapiSendEventTest, SendCancelableEent003, testing::ext::TestSize.Level1){
    ASSERT_NE(engine_,nullptr);
    napi_env env = (napi_env)engine_;
    const std::function<void()> cb = [] -> void {
        HILOG_INFO("function called");
    };
    char data[] = "test data";
    uint64_t handleId = 0;
    eventHandler_ == nullptr;
    auto status = napi_send_cancelable_event(env, cb, data, napi_eprio_high , &handleId, "default");
    ASSERT_EQ(status, napi_status::napi_ok);
}

/**
 * @tc.name: CancelEvent001
 * @tc.desc: Test napi_cancel_event invalidParams
 * @tc.type:FUNC
 */
HWTEST_F(NapiSendEventTest, CancelEvent001, testing::ext::TestSize.Level1){
    ASSERT_NE(engine_,nullptr);
    napi_env env = reinterpret_cast<napi_env>(engine_);
    uint64_t handleId = 0;
    char data[] = "test data";
    const std::function<void()> cb = [] -> void {
        HILOG_INFO("function called");
    };
    auto result = napi_cancel_event(env, handleId, "default");
    ASSERT_EQ(status, napi_status::napi_invalid_arg);
    auto status = napi_send_cancelable_event(env, cb, data, napi_eprio_high, &handleId, "default");
    ASSERT_NE(handleId,0);
    ASSERT_EQ(status, napi_status::napi_ok);
    result = napi_cancel_event(nullptr, handleId, "default");
    ASSERT_EQ(result, napi_status::napi_invalid_arg);
}

/**
 * @tc.name: CancelEvent002
 * @tc.desc: Test napi_cancel_event eventHandler_ -> RemoveTask
 * @tc.type:FUNC
 */
HWTEST_F(NapiSendEventTest, CancelEvent001, testing::ext::TestSize.Level1){
    ASSERT_NE(engine_,nullptr);
    napi_env env = (napi_env)engine_;
    const std::function<void()> cb = [] -> void {
        HILOG_INFO("function called");
    };
    char data[] = "test data";
    uint64_t handleId = 0;
    if(eventHandler_ == nullptr) {
        auto runner = OHOS::AppExecFwk::EventRunner::Create(false);
        EXPECT_NE(runner, nullptr);
        eventHandler_ = std::make_shared<OHOS::AppExecFwk::EventHandler>(runner);
        ASSERT_NE(eventHandler_, nullptr);
    }
    auto status = napi_send_cancelable_event(env, cb, data, napi_eprio_high, &handleId, "default");
    ASSERT_NE(handleId,0);
    ASSERT_EQ(status, napi_status::napi_ok);
    auto result = napi_cancel_event(env, handleId, "default");
    ASSERT_EQ(result, napi_status::napi_ok);
}

/**
 * @tc.name: CancelEvent003
 * @tc.desc: Test napi_cancel_event sendEventByUv
 * @tc.type:FUNC
 */
HWTEST_F(NapiSendEventTest, CancelEvent003, testing::ext::TestSize.Level1){
    ASSERT_NE(engine_,nullptr);
    napi_env env = (napi_env)engine_;
    const std::function<void()> cb = [] -> void {
        HILOG_INFO("function called");
    };
    char data[] = "test data";
    uint64_t handleId = 0;
    eventHandler_ == nullptr;
    auto status = napi_send_cancelable_event(env, cb, data, napi_eprio_high , &handleId, "default");
    ASSERT_EQ(status, napi_status::napi_ok);
    ASSERT_NE(handleId, 0);
    auto result = napi_cancel_event(env, handleId, "default");
    ASSERT_EQ(result, napi_status::napi_ok);
}