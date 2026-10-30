/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cinttypes>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <string>

#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "gtest/gtest.h"
#include "multi_thread_error.h"
#include "native_api_internal.h"
#include "test.h"

namespace {
constexpr size_t MAX_CAPTURED_MSG_LEN = 256;

#if defined(OHOS_PLATFORM)
struct test_debug_msg {
    uint64_t timestamp;
    const char *msg;
};

volatile sig_atomic_t g_signalCount = 0;
uint64_t g_capturedTimestamp = 0;
char g_capturedMsg[MAX_CAPTURED_MSG_LEN] = {0};

void ResetCapture()
{
    g_signalCount = 0;
    g_capturedTimestamp = 0;
    g_capturedMsg[0] = '\0';
}

void TestSignalHandler(int, siginfo_t *si, void *)
{
    if (si == nullptr || si->si_value.sival_ptr == nullptr) {
        return;
    }
    auto *debugMessage = static_cast<test_debug_msg *>(si->si_value.sival_ptr);
    g_capturedTimestamp = debugMessage->timestamp;
    size_t i = 0;
    while (i < sizeof(g_capturedMsg) - 1 && debugMessage->msg[i] != '\0') {
        g_capturedMsg[i] = debugMessage->msg[i];
        ++i;
    }
    g_capturedMsg[i] = '\0';
    g_signalCount++;
}

class SignalHandlerGuard {
public:
    SignalHandlerGuard()
    {
        struct sigaction action = {};
        action.sa_sigaction = TestSignalHandler;
        action.sa_flags = SA_SIGINFO;
        constexpr int multiThreadErrorSigno = 42;
        sigaction(multiThreadErrorSigno, &action, &oldAction_);
    }

    ~SignalHandlerGuard()
    {
        constexpr int multiThreadErrorSigno = 42;
        sigaction(multiThreadErrorSigno, &oldAction_, nullptr);
    }

private:
    struct sigaction oldAction_ {};
};

class DefaultHandlerGuard {
public:
    DefaultHandlerGuard()
    {
        constexpr int multiThreadErrorSigno = 42;
        struct sigaction action = {};
        action.sa_handler = SIG_DFL;
        sigemptyset(&action.sa_mask);
        sigaction(multiThreadErrorSigno, &action, &oldAction_);
    }

    ~DefaultHandlerGuard()
    {
        constexpr int multiThreadErrorSigno = 42;
        sigaction(multiThreadErrorSigno, &oldAction_, nullptr);
    }

private:
    struct sigaction oldAction_ {};
};

void SetDetectionConfigForTest(int64_t frequency, int64_t interval)
{
    auto &config = const_cast<panda::DFXJSNApi::MultithreadingDetectionOptions &>(
        panda::DFXJSNApi::GetDetectionConfig());
    config.frequency.store(frequency);
    config.interval.store(interval);
}

class DetectionConfigGuard {
public:
    DetectionConfigGuard()
    {
        const auto &config = panda::DFXJSNApi::GetDetectionConfig();
        frequency_ = config.frequency.load();
        interval_ = config.interval.load();
    }

    ~DetectionConfigGuard()
    {
        SetDetectionConfigForTest(frequency_, interval_);
    }

private:
    int64_t frequency_ = 100;
    int64_t interval_ = 5;
};
#endif // OHOS_PLATFORM
} // namespace

class MultiThreadErrorTest : public NativeEngineTest {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "MultiThreadErrorTest SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "MultiThreadErrorTest TearDownTestCase";
    }

    void SetUp() override {}

    void TearDown() override {}
};

/**
 * @tc.name: ConvertHilongToTmtTest001
 * @tc.desc: Test ConvertHilongToTmt strips HILOG privacy markers (%{public},
 *           %{private}, %{limit}) while keeping printf conversion specifiers,
 *           and tolerates empty / null / escaped inputs
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, ConvertHilongToTmtTest001, testing::ext::TestSize.Level1)
{
    ASSERT_EQ(ConvertHilongToTmt("%{public}s at %{public}s:%{public}d"), "%s at %s:%d");
    ASSERT_EQ(ConvertHilongToTmt("%{private}llu / %{limit}x"), "%llu / %x");
    ASSERT_EQ(ConvertHilongToTmt("no privacy marker"), "no privacy marker");
    ASSERT_EQ(ConvertHilongToTmt(""), "");
    ASSERT_EQ(ConvertHilongToTmt(nullptr), "");
    ASSERT_EQ(ConvertHilongToTmt("100%% and %{public}d"), "100%% and %d");
}

/**
 * @tc.name: FormatHilogMsgTest001
 * @tc.desc: Test FormatHilogMsg formats HILOG-style format strings with the
 *           real LOG_IF_SPECIAL call-site signatures, including PRIu64 and
 *           the zero-argument case
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, FormatHilogMsgTest001, testing::ext::TestSize.Level1)
{
    ASSERT_EQ(FormatHilogMsg("param env not equal to its owner. %{public}s at %{public}s:%{public}d",
                             "Func", "File.cpp", 12),
              "param env not equal to its owner. Func at File.cpp:12");
    uint64_t ownerId = 7;
    ASSERT_EQ(FormatHilogMsg("owner id: %{public}" PRIu64, ownerId), "owner id: 7");
    ASSERT_EQ(FormatHilogMsg("no args at all"), "no args at all");
}

#if defined(OHOS_PLATFORM)
/**
 * @tc.name: NotifyMultiThreadErrorTest001
 * @tc.desc: Test NotifyMultiThreadError reports through signal 42 with the
 *           message and millisecond timestamp carried in the debug_msg
 *           protocol when no gate throttles the report
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorTest001, testing::ext::TestSize.Level1)
{
    DetectionConfigGuard configGuard;
    SignalHandlerGuard handlerGuard;
    SetDetectionConfigForTest(1, 0);

    ResetCapture();
    NotifyMultiThreadError(engine_, "multi-thread violation detected");

    ASSERT_EQ(g_signalCount, 1);
    ASSERT_STREQ(g_capturedMsg, "multi-thread violation detected");
    uint64_t before = static_cast<uint64_t>(time(nullptr)) * 1000 - 1000;
    uint64_t after = static_cast<uint64_t>(time(nullptr)) * 1000 + 1000;
    ASSERT_GE(g_capturedTimestamp, before);
    ASSERT_LE(g_capturedTimestamp, after);
}

/**
 * @tc.name: NotifyMultiThreadErrorTest002
 * @tc.desc: Test the frequency gate: with frequency=2 the report is allowed
 *           at every second detection, and the counter restart after a pass
 *           makes the pass / block / pass sequence deterministic whatever
 *           the counter state left by earlier tests
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorTest002, testing::ext::TestSize.Level1)
{
    DetectionConfigGuard configGuard;
    SignalHandlerGuard handlerGuard;
    SetDetectionConfigForTest(2, 0);

    constexpr int loopLimit = 100;
    int loops = 0;
    ResetCapture();
    while (g_signalCount == 0 && loops < loopLimit) {
        NotifyMultiThreadError(engine_, "frequency gate");
        ++loops;
    }
    ASSERT_GT(g_signalCount, 0);

    ResetCapture();
    NotifyMultiThreadError(engine_, "frequency gate");
    ASSERT_EQ(g_signalCount, 0);

    ResetCapture();
    NotifyMultiThreadError(engine_, "frequency gate");
    ASSERT_EQ(g_signalCount, 1);
}

/**
 * @tc.name: NotifyMultiThreadErrorTest003
 * @tc.desc: Test the interval gate: after a report an interval of 1440
 *           minutes blocks further reports while interval=0 lets them
 *           through again
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorTest003, testing::ext::TestSize.Level1)
{
    DetectionConfigGuard configGuard;
    SignalHandlerGuard handlerGuard;

    SetDetectionConfigForTest(1, 0);
    ResetCapture();
    NotifyMultiThreadError(engine_, "interval gate");
    ASSERT_EQ(g_signalCount, 1);

    SetDetectionConfigForTest(1, 1440);
    ResetCapture();
    NotifyMultiThreadError(engine_, "interval gate");
    ASSERT_EQ(g_signalCount, 0);

    SetDetectionConfigForTest(1, 0);
    ResetCapture();
    NotifyMultiThreadError(engine_, "interval gate");
    ASSERT_EQ(g_signalCount, 1);
}

/**
 * @tc.name: NotifyMultiThreadErrorNoHandlerTest001
 * @tc.desc: Test NotifyMultiThreadError skips signal 42 and falls back to
 *           plain hilog when no handler is installed (SIG_DFL), so the
 *           real-time signal default action never terminates the process
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, NotifyMultiThreadErrorNoHandlerTest001, testing::ext::TestSize.Level1)
{
    ASSERT_NE(engine_, nullptr);
    DetectionConfigGuard configGuard;
    DefaultHandlerGuard defaultGuard;
    SetDetectionConfigForTest(1, 0);

    ResetCapture();
    NotifyMultiThreadError(engine_, "report without handler");

    ASSERT_EQ(g_signalCount, 0);
}

/**
 * @tc.name: LogIfSpecialIntegrationTest001
 * @tc.desc: Test the LOG_IF_SPECIAL else-branch full chain on a valid
 *           engine: the formatted message reaches the faultlogger signal
 *           together with the appended hybrid stack
 * @tc.type: FUNC
 */
HWTEST_F(MultiThreadErrorTest, LogIfSpecialIntegrationTest001, testing::ext::TestSize.Level1)
{
    ASSERT_NE(engine_, nullptr);
    DetectionConfigGuard configGuard;
    SignalHandlerGuard handlerGuard;
    SetDetectionConfigForTest(1, 0);

    ResetCapture();
    LOG_IF_SPECIAL(engine_, false, "current napi interface cannot run in multi-thread, id: %{public}d", 42);

    ASSERT_EQ(g_signalCount, 1);
    const std::string expectedPrefix = "current napi interface cannot run in multi-thread, id: 42";
    ASSERT_EQ(std::string(g_capturedMsg).substr(0, expectedPrefix.size()), expectedPrefix);
    ASSERT_NE(std::string(g_capturedMsg).find("\n"), std::string::npos);
}
#endif // OHOS_PLATFORM
