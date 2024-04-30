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
#include "ark_native_task_runner.h"

#include "utils/log.h"

template<typename Inner, typename Outer>
ContainerOfHelper<Inner, Outer>::ContainerOfHelper(Inner Outer::*field, Inner* pointer)
    : pointer_(reinterpret_cast<Outer*>(reinterpret_cast<uintptr_t>(pointer) - OffsetOf(field)))
{}

template<typename Inner, typename Outer>
template<typename TypeName>
ContainerOfHelper<Inner, Outer>::operator TypeName*() const
{
    return static_cast<TypeName*>(pointer_);
}

template<typename Inner, typename Outer>
constexpr ContainerOfHelper<Inner, Outer> ContainerOf(Inner Outer::*field, Inner* pointer)
{
    return ContainerOfHelper<Inner, Outer>(field, pointer);
}

ArkNativeTaskRunner::ArkNativeTaskRunner(uv_loop_t* loop) : loop_(loop)
{
    HILOG_DEBUG("ArkNativeTaskRunner::ArkNativeTaskRunner");
    flushTasks_ = new uv_async_t();
    uv_async_init(loop, flushTasks_, FlushTasks);
    flushTasks_->data = static_cast<void*>(this);
    uv_unref(reinterpret_cast<uv_handle_t*>(flushTasks_));
}

void ArkNativeTaskRunner::FlushTasks(uv_async_t* handle)
{
    auto arkNativeTaskRunner = static_cast<ArkNativeTaskRunner*>(handle->data);
    arkNativeTaskRunner->FlushForegroundTasksInternal();
}

void ArkNativeTaskRunner::PostTask(std::unique_ptr<Task> task)
{
    if (flushTasks_ == nullptr) {
        // Tasks may be posted during EcmaVM disposal. In that case, the only
        // sensible path forward is to discard the task.
        return;
    }
    foregroundTasks_.Push(std::move(task));
    auto ret = uv_async_send(flushTasks_);
    if (ret != 0) {
        HILOG_ERROR("uv async send failed %d", ret);
        return;
    }
}

void ArkNativeTaskRunner::PostDelayedTask(std::unique_ptr<Task> task, uint64_t delay)
{
    if (flushTasks_ == nullptr) {
        // Tasks may be posted during EcmaVM disposal. In that case, the only
        // sensible path forward is to discard the task.
        return;
    }
    std::unique_ptr<DelayedTask> delayedTask = std::make_unique<DelayedTask>();
    delayedTask->task_ = std::move(task);
    delayedTask->arkNativeTaskRunner_ = shared_from_this();
    delayedTask->timeout_ = delay;
    foregroundDelayedTasks_.Push(std::move(delayedTask));
    auto ret = uv_async_send(flushTasks_);
    if (ret != 0) {
        HILOG_ERROR("uv async send failed %d", ret);
        return;
    }
}

ArkNativeTaskRunner::~ArkNativeTaskRunner()
{
    HILOG_DEBUG("~ArkNativeTaskRunner %{public}d", !flushTasks_);
}

void ArkNativeTaskRunner::AddShutdownCallback(void (*callback)(void*), void* data)
{
    shutdownCallbacks_.emplace_back(ShutdownCallback { callback, data });
}

void ArkNativeTaskRunner::Shutdown()
{
    HILOG_DEBUG("ArkNativeTaskRunner::Shutdown()");

    if (flushTasks_ == nullptr) {
        return;
    }

    foregroundDelayedTasks_.PopAll();
    foregroundTasks_.PopAll();
    scheduledDelayedTasks_.clear();

    // Both destroying the scheduledDelayedTasks_ lists and closing
    // flushTasks_ handle add tasks to the event loop. We keep a count of all
    // non-closed handles, and when that reaches zero, we inform any shutdown
    // callbacks that the platform is done as far as this Isolate is concerned.
    selfReference_ = shared_from_this();
    uv_close(reinterpret_cast<uv_handle_t*>(flushTasks_), [](uv_handle_t* handle) {
        std::unique_ptr<uv_async_t> flushTasks { reinterpret_cast<uv_async_t*>(handle) };
        ArkNativeTaskRunner* arkNativeTaskRunner = static_cast<ArkNativeTaskRunner*>(flushTasks->data);
        arkNativeTaskRunner->DecreaseHandleCount();
        arkNativeTaskRunner->selfReference_.reset();
    });
    flushTasks_ = nullptr;
}

void ArkNativeTaskRunner::DecreaseHandleCount()
{
    if (uvHandleCount_ < 1) {
        HILOG_ERROR("uvHandleCount_ < 1");
    }
    if (--uvHandleCount_ == 0) {
        HILOG_DEBUG("--uvHandleCount_ == 0");
        for (const auto& callback : shutdownCallbacks_) {
            callback.cb(callback.data);
        }
    }
}

void ArkNativeTaskRunner::RunForegroundTask(std::unique_ptr<Task> task)
{
    task->Run(0);
}

void ArkNativeTaskRunner::DeleteFromScheduledTasks(DelayedTask* task)
{
    auto it = std::find_if(scheduledDelayedTasks_.begin(), scheduledDelayedTasks_.end(),
        [task](const DelayedTaskPointer& delayed) -> bool { return delayed.get() == task; });
    if (it != scheduledDelayedTasks_.end()) {
        scheduledDelayedTasks_.erase(it);
    }
}

void ArkNativeTaskRunner::RunForegroundTask(uv_timer_t* handle)
{
    DelayedTask* delayed = ContainerOf(&DelayedTask::timer_, handle);
    delayed->arkNativeTaskRunner_->RunForegroundTask(std::move(delayed->task_));
    delayed->arkNativeTaskRunner_->DeleteFromScheduledTasks(delayed);
}

bool ArkNativeTaskRunner::FlushForegroundTasksInternal()
{
    HILOG_DEBUG("ArkNativeTaskRunner::FlushForegroundTasksInternal");
    bool didWork = false;

    while (std::unique_ptr<DelayedTask> delayedTask = foregroundDelayedTasks_.Pop()) {
        didWork = true;
        delayedTask->timer_.data = static_cast<void*>(delayedTask.get());
        uv_timer_init(loop_, &delayedTask->timer_);
        uv_update_time(loop_);
        // Timers may not guarantee queue ordering of events with the same delay if
        // the delay is non-zero. This should not be a problem in practice.
        uv_timer_start(&delayedTask->timer_, RunForegroundTask, delayedTask->timeout_, 0);
        uv_unref(reinterpret_cast<uv_handle_t*>(&delayedTask->timer_));
        uvHandleCount_++;

        scheduledDelayedTasks_.emplace_back(delayedTask.release(), [](DelayedTask* delayedTask) {
            uv_close(reinterpret_cast<uv_handle_t*>(&delayedTask->timer_), [](uv_handle_t* handle) {
                std::unique_ptr<DelayedTask> task { static_cast<DelayedTask*>(handle->data) };
                task->arkNativeTaskRunner_->DecreaseHandleCount();
            });
        });
    }
    // Move all foreground tasks into a separate queue and flush that queue.
    // This way tasks that are posted while flushing the queue will be run on the
    // next call of FlushForegroundTasksInternal.
    std::queue<std::unique_ptr<Task>> tasks = foregroundTasks_.PopAll();
    while (!tasks.empty()) {
        std::unique_ptr<Task> task = std::move(tasks.front());
        tasks.pop();
        didWork = true;
        RunForegroundTask(std::move(task));
    }
    return didWork;
}

template<class T>
TaskQueue<T>::TaskQueue() : lock_(), stopped_(false), taskQueue_()
{}

template<class T>
void TaskQueue<T>::Push(std::unique_ptr<T> task)
{
    std::lock_guard<std::mutex> lock(lock_);
    taskQueue_.push(std::move(task));
}

template<class T>
std::unique_ptr<T> TaskQueue<T>::Pop()
{
    std::lock_guard<std::mutex> lock(lock_);
    if (taskQueue_.empty()) {
        return std::unique_ptr<T>(nullptr);
    }
    std::unique_ptr<T> task = std::move(taskQueue_.front());
    taskQueue_.pop();
    return task;
}

template<class T>
void TaskQueue<T>::Stop()
{
    std::lock_guard<std::mutex> lock(lock_);
    stopped_ = true;
}

template<class T>
std::queue<std::unique_ptr<T>> TaskQueue<T>::PopAll()
{
    std::lock_guard<std::mutex> lock(lock_);
    std::queue<std::unique_ptr<T>> taskQueue;
    taskQueue.swap(taskQueue_);
    return taskQueue;
}