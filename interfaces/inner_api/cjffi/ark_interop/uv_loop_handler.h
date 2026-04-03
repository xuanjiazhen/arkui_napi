/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef NAPI_UV_LOOP_HANDLER_H
#define NAPI_UV_LOOP_HANDLER_H

#include "ark_interop_macro.h"
#include "event_handler.h"
#include "uv.h"

/**
 * @brief Attaches `uv_loop_t`'s backend fd to OHOS `EventHandler`; on read/write runs `uv_run(NOWAIT)` and schedules the next wake via `uv_backend_timeout`.
 *
 * Bridges libuv timer resolution to `EventHandler` when the native engine thread runs both ArkTS and libuv outside a pure poll mode.
 */
class COV_EXPORT UVLoopHandler : public OHOS::AppExecFwk::FileDescriptorListener,
                                 public std::enable_shared_from_this<UVLoopHandler> {
public:
    /** @param uvLoop uv loop tied to `ArkNativeEngine` (must be non-null). */
    explicit UVLoopHandler(uv_loop_t* uvLoop) : uvLoop_(uvLoop) {}
    /** Readable fd: forwards to `OnTriggered`. */
    void OnReadable(int32_t) override;
    /** Writable fd: forwards to `OnTriggered`. */
    void OnWritable(int32_t) override;

private:
    /** Runs `UV_RUN_NOWAIT` once and installs/adjusts TIMER_TASK from `uv_backend_timeout`. */
    void OnTriggered();

    uv_loop_t* uvLoop_ = nullptr;
    int64_t lastTimeStamp_ = 0;
    bool haveTimerTask_ = false;
};

#endif // NAPI_UV_LOOP_HANDLER_H
