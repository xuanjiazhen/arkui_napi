/*
 * Copyright (c) 2024 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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

#ifndef FOUNDATION_ACE_NAPI_NATIVE_ENGINE_IMPL_ARK_ARK_NATIVE_TASK_RUNNER_H
#define FOUNDATION_ACE_NAPI_NATIVE_ENGINE_IMPL_ARK_ARK_NATIVE_TASK_RUNNER_H

#include <memory>
#include <mutex>
#include <queue>
#include <unistd.h>

#include "ecmascript/napi/include/jsnapi.h"
#include "uv.h"

using TaskRunner = panda::TaskRunner;
using Task = panda::Task;

class ArkNativeTaskRunner;

template<typename Inner, typename Outer>
constexpr uintptr_t OffsetOf(Inner Outer::*field)
{
    return reinterpret_cast<uintptr_t>(&(static_cast<Outer*>(nullptr)->*field));
}

// The helper is for doing safe downcasts from base types to derived types.
template<typename Inner, typename Outer>
class ContainerOfHelper {
public:
    inline ContainerOfHelper(Inner Outer::*field, Inner* pointer);
    template<typename TypeName>
    inline operator TypeName*() const;

private:
    Outer* const pointer_;
};

template<class T>
class TaskQueue {
public:
    TaskQueue();
    ~TaskQueue() = default;

    void Push(std::unique_ptr<T> task);
    std::unique_ptr<T> Pop();
    std::queue<std::unique_ptr<T>> PopAll();
    void Stop();

private:
    std::mutex lock_;
    bool stopped_;
    std::queue<std::unique_ptr<T>> taskQueue_;
};

struct DelayedTask {
    std::unique_ptr<Task> task_;
    uv_timer_t timer_;
    uint64_t timeout_;
    std::shared_ptr<ArkNativeTaskRunner> arkNativeTaskRunner_;
};

class ArkNativeTaskRunner : public TaskRunner, public std::enable_shared_from_this<ArkNativeTaskRunner> {
public:
    explicit ArkNativeTaskRunner(uv_loop_t* loop);
    ~ArkNativeTaskRunner() override;

    void PostTask(std::unique_ptr<Task> task) override;
    void PostDelayedTask(std::unique_ptr<Task> task, uint64_t delay) override;
    bool DelayedTasksEnabled() override
    {
        return true;
    }

    void AddShutdownCallback(void (*callback)(void*), void* data);
    void Shutdown();

    // Returns true if work was dispatched or executed. New tasks that are
    // posted during flushing of the queue are postponed until the next
    // flushing.
    bool FlushForegroundTasksInternal();

    const uv_loop_t* EventLoop() const
    {
        return loop_;
    }

private:
    void DeleteFromScheduledTasks(DelayedTask* task);
    void DecreaseHandleCount();

    static void FlushTasks(uv_async_t* handle);
    void RunForegroundTask(std::unique_ptr<Task> task);
    static void RunForegroundTask(uv_timer_t* timer);

    struct ShutdownCallback {
        void (*cb)(void*);
        void* data;
    };
    typedef std::vector<ShutdownCallback> ShutdownCbList;
    ShutdownCbList shutdownCallbacks_;
    // shared_ptr to self to keep this object alive during shutdown.
    std::shared_ptr<ArkNativeTaskRunner> selfReference_;
    uint32_t uvHandleCount_ = 1; // 1 = flush_tasks_

    uv_loop_t* const loop_;
    uv_async_t* flushTasks_ = nullptr;
    TaskQueue<Task> foregroundTasks_;
    TaskQueue<DelayedTask> foregroundDelayedTasks_;

    // Use a custom deleter because libuv needs to close the handle first.
    typedef std::unique_ptr<DelayedTask, void (*)(DelayedTask*)> DelayedTaskPointer;
    std::vector<DelayedTaskPointer> scheduledDelayedTasks_;
};
#endif /* FOUNDATION_ACE_NAPI_NATIVE_ENGINE_IMPL_ARK_ARK_NATIVE_TASK_RUNNER_H */
