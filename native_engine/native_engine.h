/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_ACE_NAPI_NATIVE_ENGINE_NATIVE_ENGINE_H
#define FOUNDATION_ACE_NAPI_NATIVE_ENGINE_NATIVE_ENGINE_H

#include <functional>
#include <list>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#ifdef LINUX_PLATFORM
#include<atomic>
#endif
#include <iostream>
#include <regex>
#include <memory>
#include <mutex>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#if !defined(PREVIEW) && !defined(IOS_PLATFORM)
#include <sys/wait.h>
#endif

#include "callback_scope_manager/native_callback_scope_manager.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "module_manager/native_module_manager.h"
#include "native_engine/native_async_work.h"
#include "native_engine/native_deferred.h"
#include "native_engine/native_reference.h"
#include "native_engine/native_safe_async_work.h"
#include "native_engine/native_value.h"
#include "native_property.h"
#include "reference_manager/native_reference_manager.h"
#include "utils/macros.h"

namespace panda::ecmascript {
    class EcmaVM;
}

typedef int32_t (*GetContainerScopeIdCallback)(void);
typedef void (*ContainerScopeCallback)(int32_t);
typedef struct uv_loop_s uv_loop_t;

struct NativeErrorExtendedInfo {
    const char* message = nullptr;
    void* reserved = nullptr;
    uint32_t engineErrorCode = 0;
    int errorCode = 0;
};

struct ExceptionInfo {
    const char* message_ = nullptr;
    int32_t lineno_ = 0;
    int32_t colno_ = 0;

    ~ExceptionInfo()
    {
        if (message_ != nullptr) {
            delete[] message_;
        }
    }
};

enum LoopMode {
    LOOP_DEFAULT, LOOP_ONCE, LOOP_NOWAIT
};

enum class DumpFormat {
    JSON, BINARY, OTHER
};

enum class WorkerVersion {
    NONE, OLD, NEW
};

class CleanupHookCallback {
public:
    using Callback = void (*)(void*);

    CleanupHookCallback(Callback fn, void* arg, uint64_t insertion_order_counter)
        : fn_(fn), arg_(arg), insertion_order_counter_(insertion_order_counter)
    {}

    struct Hash {
        inline size_t operator()(const CleanupHookCallback& cb) const
        {
            return std::hash<void*>()(cb.arg_);
        }
    };
    struct Equal {
        inline bool operator()(const CleanupHookCallback& a, const CleanupHookCallback& b) const
        {
            return a.fn_ == b.fn_ && a.arg_ == b.arg_;
        };
    };

private:
    friend class NativeEngine;
    Callback fn_;
    void* arg_;
    uint64_t insertion_order_counter_;
};

using PostTask = std::function<void(bool needSync)>;
using CleanEnv = std::function<void()>;
using InitWorkerFunc = std::function<void(NativeEngine* engine)>;
using GetAssetFunc = std::function<void(const std::string& uri, std::vector<uint8_t>& content, std::string& ami)>;
using OffWorkerFunc = std::function<void(NativeEngine* engine)>;
using DebuggerPostTask = std::function<void(std::function<void()>&&)>;
using NapiUncaughtExceptionCallback = std::function<void(napi_value value)>;
using PermissionCheckCallback = std::function<bool()>;
using NapiConcurrentCallback = void (*)(napi_env env, napi_value result, bool success, void* data);
using SourceMapCallback = std::function<std::string(const std::string& rawStack)>;
using SourceMapTranslateCallback = std::function<bool(std::string& url, int& line, int& column)>;
using EcmaVM = panda::ecmascript::EcmaVM;

using panda::Local;
using panda::LocalScope;
using panda::JSValueRef;
using panda::JsiRuntimeCallInfo;

class NativeChunk {
public:
    static constexpr size_t CHUNK_PAGE_SIZE = 8 * 1024;

    NativeChunk() {};
    ~NativeChunk()
    {
        for (auto iter = usedPage_.begin(); iter != usedPage_.end(); iter++) {
            free(*iter);
        }
        usedPage_.clear();
    }

    template<class T>
    [[nodiscard]] T *NewArray(size_t size)
    {
        return static_cast<T *>(Allocate(size * sizeof(T)));
    }

    template<typename T, typename... Args>
    [[nodiscard]] T *New(Args &&... args)
    {
        auto p = reinterpret_cast<void *>(Allocate(sizeof(T)));
        new (p) T(std::forward<Args>(args)...);
        return reinterpret_cast<T *>(p);
    }

    template<class T>
    void Delete(T *ptr)
    {
        if (!useChunk_) {
            delete ptr;
            return;
        }

        if (std::is_class<T>::value) {
            ptr->~T();
        }
    }

private:
    void *Allocate(size_t size)
    {
        uintptr_t result = ptr_;
        if (size > end_ - ptr_) {
            result = Expand();
        }
        ptr_ += size;
        return reinterpret_cast<void *>(result);
    }

    uintptr_t Expand()
    {
        void *ptr = malloc(CHUNK_PAGE_SIZE);
        if (ptr == nullptr) {
            std::abort();
        }
        usedPage_.emplace_back(ptr);
        useChunk_ = true;
        ptr_ = reinterpret_cast<uintptr_t>(ptr);
        end_ = ptr_ + CHUNK_PAGE_SIZE;
        return ptr_;
    }

    uintptr_t ptr_ {0};
    uintptr_t end_ {0};
    bool useChunk_ {false};
    std::vector<void *> usedPage_ {};
};

namespace panda::ecmascript {
struct JsFrameInfo {
    std::string functionName;
    std::string fileName;
    std::string pos;
    uintptr_t* nativePointer = nullptr;
};
}
using ArkJsFrameInfo = panda::ecmascript::JsFrameInfo;

Local<panda::JSValueRef> NativeFunctionCallBack(JsiRuntimeCallInfo *runtimeInfo);
bool NapiDefineProperty(napi_env env, Local<panda::ObjectRef> &obj, NapiPropertyDescriptor propertyDescriptor);

enum class ForceExpandState : int32_t {
    FINISH_COLD_START = 0,
    START_HIGH_SENSITIVE,
    FINISH_HIGH_SENSITIVE,
};

NAPI_EXPORT Local<JSValueRef> NapiValueToLocalValue(napi_value v);
NAPI_EXPORT napi_value LocalValueToLocalNapiValue(Local<JSValueRef> local);
void FunctionSetContainerId(const EcmaVM *vm, panda::Local<panda::JSValueRef> &local);
panda::Local<panda::JSValueRef> NapiDefineClass(napi_env env, const char* name, NapiNativeCallback callback,
    void* data, const NapiPropertyDescriptor* properties, size_t length);

class SerializationData {
public:
    SerializationData() : data_(nullptr), size_(0) {}
    ~SerializationData() = default;
 
    uint8_t* GetData() const
    {
        return data_.get();
    }
    size_t GetSize() const
    {
        return size_;
    }

private:
    struct DataDeleter {
        void operator()(uint8_t* p) const
        {
            free(p);
        }
    };
    std::unique_ptr<uint8_t, DataDeleter> data_;
    size_t size_;
};

class NAPI_EXPORT NativeEngine {
friend struct MoudleNameLocker;
public:
    NativeEngine(EcmaVM* vm, void* jsEngine, bool isLimitedWorker = false);
    ~NativeEngine();
    NativeModuleManager* GetModuleManager();
    NativeReferenceManager* GetReferenceManager();
    NativeCallbackScopeManager* GetCallbackScopeManager();
    uv_loop_t* GetUVLoop() const;
    pthread_t GetTid() const;

    bool ReinitUVLoop();
    void Loop(LoopMode mode, bool needSync = false);
    void SetPostTask(PostTask postTask);
    void TriggerPostTask();
#if !defined(PREVIEW)
    void CheckUVLoop();
    void CancelCheckUVLoop();
#endif
    void* GetJsEngine();

    const EcmaVM* GetEcmaVm() const;
    void SetPromiseRejectCallback(NativeReference* rejectCallbackRef, NativeReference* checkCallbackRef);
    bool InitTaskPoolThread(NativeEngine* engine, NapiConcurrentCallback callback);
    bool InitTaskPoolThread(napi_env env, NapiConcurrentCallback callback);
    bool InitTaskPoolFunc(napi_env env, napi_value func, void* taskInfo);
    bool HasPendingJob();
    bool IsProfiling();
    bool IsExecutingPendingJob() const;
    void TerminateExecution() const;
    void* GetCurrentTaskInfo() const;

    napi_value CallFunction(napi_value thisVar,
                            napi_value function,
                            napi_value const *argv,
                            size_t argc);
    bool RunScriptPath(const char* path);
    napi_value RunScriptBuffer(const char* path, std::vector<uint8_t>& buffer, bool isBundle);
    bool RunScriptBuffer(const std::string &path, uint8_t* buffer, size_t size, bool isBundle);
    napi_value RunBufferScript(std::vector<uint8_t>& buffer);
    napi_value RunActor(std::vector<uint8_t>& buffer, const char* descriptor);

    // Set lib path
    void SetPackagePath(const std::string appLinPathKey, const std::vector<std::string>& packagePath);

    napi_value CreateInstance(napi_value constructor, napi_value const *argv, size_t argc);
    NativeReference* CreateReference(napi_value value, uint32_t initialRefcount,
        bool flag = false, NapiNativeFinalize callback = nullptr, void* data = nullptr, void* hint = nullptr);

    NativeAsyncWork* CreateAsyncWork(napi_value asyncResource,
                                     napi_value asyncResourceName,
                                     NativeAsyncExecuteCallback execute,
                                     NativeAsyncCompleteCallback complete,
                                     void* data);

    NativeAsyncWork* CreateAsyncWork(const std::string &asyncResourceName,
                                     NativeAsyncExecuteCallback execute,
                                     NativeAsyncCompleteCallback complete,
                                     void* data);
    NativeSafeAsyncWork* CreateSafeAsyncWork(napi_value func, napi_value asyncResource,
        napi_value asyncResourceName, size_t maxQueueSize, size_t threadCount, void* finalizeData,
        NativeFinalize finalizeCallback, void* context, NativeThreadSafeFunctionCallJs callJsCallback);
    void* CreateRuntime(bool isLimitedWorker = false);
    napi_value CreatePromise(NativeDeferred** deferred);

    std::string GetOhmurl(const char* str);
    napi_value NapiLoadModule(const char* str);
    void StartCpuProfiler(const std::string& fileName = "");
    void StopCpuProfiler();

    void ResumeVM();
    bool SuspendVM();
    bool IsSuspended();
    bool CheckSafepoint();
    bool SuspendVMById(uint32_t tid);
    void ResumeVMById(uint32_t tid);

    void DumpHeapSnapshot(const std::string &path, bool isVmMode = true, DumpFormat dumpFormat = DumpFormat::JSON);
    void DumpHeapSnapshot(bool isVmMode = true, DumpFormat dumpFormat = DumpFormat::JSON,
        bool isPrivate = false, bool isFullGC = true);
    bool BuildNativeAndJsStackTrace(std::string &stackTraceStr);
    bool BuildJsStackTrace(std::string &stackTraceStr);
    bool BuildJsStackInfoList(uint32_t tid, std::vector<JsFrameInfo>& jsFrames);
    bool DeleteWorker(NativeEngine* workerEngine);
    bool StartHeapTracking(double timeInterval, bool isVmMode = true);
    bool StopHeapTracking(const std::string &filePath);

    void AllowCrossThreadExecution() const;
    static void PromiseRejectCallback(void* values);

    NativeErrorExtendedInfo* GetLastError();
    void SetLastError(int errorCode, uint32_t engineErrorCode = 0, void* engineReserved = nullptr);
    void ClearLastError();
    bool IsExceptionPending() const;
    void EncodeToUtf8(napi_value value, char* buffer, int32_t* written, size_t bufferSize, int32_t* nchars);
    void EncodeToChinese(napi_value value, std::string& buffer, const std::string& encoding);
    NativeEngine(NativeEngine&) = delete;
    NativeEngine& operator=(NativeEngine&) = delete;

    panda::Local<panda::ObjectRef> LoadArkModule(const void *buffer, int32_t len, const std::string& fileName);
    napi_value ValueToNapiValue(JSValueWrapper& value);
    static napi_value ArkValueToNapiValue(napi_env env, Local<JSValueRef> value);
    std::string GetSourceCodeInfo(napi_value value, ErrorPos pos);
 
    bool ExecuteJsBin(const std::string& fileName);
    panda::Local<panda::ObjectRef> LoadModuleByName(const std::string& moduleName, bool isAppModule,
        const std::string& param, const std::string& instanceName, void* instance, const std::string& path = "");

    bool TriggerFatalException(napi_value error);
    bool AdjustExternalMemory(int64_t ChangeInBytes, int64_t* AdjustedValue);

    void MarkWorkerThread()
    {
        jsThreadType_ = JSThreadType::WORKER_THREAD;
    }
    void MarkTaskPoolThread()
    {
        jsThreadType_ = JSThreadType::TASKPOOL_THREAD;
    }
    bool IsWorkerThread() const
    {
        return jsThreadType_ == JSThreadType::WORKER_THREAD;
    }
    bool IsTaskPoolThread() const
    {
        return jsThreadType_ == JSThreadType::TASKPOOL_THREAD;
    }
    bool IsMainThread() const
    {
        return jsThreadType_ == JSThreadType::MAIN_THREAD;
    }

    bool CheckAndSetWorkerVersion(WorkerVersion expected, WorkerVersion desired)
    {
        return workerVersion_.compare_exchange_strong(expected, desired);
    }
    bool IsTargetWorkerVersion(WorkerVersion target) const
    {
        return workerVersion_.load() == target;
    }

    void IncreaseSubEnvCounter()
    {
        subEnvCounter_++;
    }
    void DecreaseSubEnvCounter()
    {
        subEnvCounter_--;
    }
    bool HasSubEnv()
    {
        return subEnvCounter_.load() != 0;
    }

    void SetCleanEnv(CleanEnv cleanEnv)
    {
        cleanEnv_ = cleanEnv;
    }

    void SetPromiseRejectCallBackRef(NativeReference* rejectCallbackRef)
    {
        promiseRejectCallbackRef_ = rejectCallbackRef;
    }

    // register init worker func
    void SetInitWorkerFunc(InitWorkerFunc func);
    InitWorkerFunc GetInitWorkerFunc() const;
    void SetGetAssetFunc(GetAssetFunc func);
    GetAssetFunc GetGetAssetFunc() const;
    void SetOffWorkerFunc(OffWorkerFunc func);
    OffWorkerFunc GetOffWorkerFunc() const;

    // call init worker func
    bool CallInitWorkerFunc(NativeEngine* engine);
    bool CallGetAssetFunc(const std::string& uri, std::vector<uint8_t>& content, std::string& ami);
    bool CallOffWorkerFunc(NativeEngine* engine);

    // adapt worker to ace container
    void SetGetContainerScopeIdFunc(GetContainerScopeIdCallback func);
    void SetInitContainerScopeFunc(ContainerScopeCallback func);
    void SetFinishContainerScopeFunc(ContainerScopeCallback func);
    int32_t GetContainerScopeIdFunc();
    bool InitContainerScopeFunc(int32_t id);
    bool FinishContainerScopeFunc(int32_t id);

#if !defined(PREVIEW)
    void SetDebuggerPostTaskFunc(DebuggerPostTask func);
    void CallDebuggerPostTaskFunc(std::function<void()>&& task);
#endif

    void SetHostEngine(NativeEngine* engine);
    NativeEngine* GetHostEngine() const;

    using CleanupCallback = CleanupHookCallback::Callback;
    void AddCleanupHook(CleanupCallback fun, void* arg);
    void RemoveCleanupHook(CleanupCallback fun, void* arg);

    void CleanupHandles();
    void IncreaseWaitingRequestCounter();
    void DecreaseWaitingRequestCounter();
    bool HasWaitingRequest();

    void RunCleanup();

    bool IsStopping() const
    {
        return isStopping_.load();
    }

    void SetStopping(bool value)
    {
        isStopping_.store(value);
    }

    void PrintStatisticResult();
    void StartRuntimeStat();
    void StopRuntimeStat();
    size_t GetArrayBufferSize();
    size_t GetHeapTotalSize();
    size_t GetHeapUsedSize();
    size_t GetHeapLimitSize();
    void NotifyApplicationState(bool inBackground);
    void NotifyIdleStatusControl(std::function<void(bool)> callback);
    void NotifyIdleTime(int idleMicroSec);
    void NotifyMemoryPressure(bool inHighMemoryPressure = false);
    void NotifyForceExpandState(int32_t value);
    void SetMockModuleList(const std::map<std::string, std::string> &list);

    // debugger
    bool IsMixedDebugEnabled();
    void JsHeapStart();
    uint64_t GetCurrentTickMillseconds();
    void JudgmentDump(size_t limitSize);
    void NotifyNativeCalling(const void *nativeAddress);

    void RegisterWorkerFunction(const NativeEngine* engine);
    void RegisterNapiUncaughtExceptionHandler(NapiUncaughtExceptionCallback callback);
    void HandleUncaughtException();
    bool HasPendingException();
    void RegisterPermissionCheck(PermissionCheckCallback callback);
    bool ExecutePermissionCheck();
    void RegisterTranslateBySourceMap(SourceMapCallback callback);
    std::string ExecuteTranslateBySourceMap(const std::string& rawStack);
    void RegisterSourceMapTranslateCallback(SourceMapTranslateCallback callback);
    inline void SetModuleName(panda::Local<panda::ObjectRef> &nativeObj, std::string moduleName);
    panda::Local<panda::ObjectRef> GetModuleFromName(
        const std::string& moduleName, bool isAppModule, const std::string& id, const std::string& param,
        const std::string& instanceName, void** instance);
    NativeReference* GetPromiseRejectCallBackRef()
    {
        return promiseRejectCallbackRef_;
    }

    NapiConcurrentCallback GetConcurrentCallbackFunc()
    {
        return concurrentCallbackFunc_;
    }

    NativeReference* GetCheckCallbackRef()
    {
        return checkCallbackRef_;
    }

    void SetCheckCallbackRef(NativeReference* checkCallbackRef)
    {
        checkCallbackRef_ = checkCallbackRef;
    }

    NapiUncaughtExceptionCallback GetNapiUncaughtExceptionCallback()
    {
        return napiUncaughtExceptionCallback_;
    }
    void* GetPromiseRejectCallback()
    {
        return reinterpret_cast<void*>(PromiseRejectCallback);
    }

    static bool napiProfilerEnabled;

    static void* GetNativePtrCallBack(void* data);

    // run script by path
    napi_value RunScript(const char* path);

    const char* GetModuleFileName();

    void SetModuleFileName(std::string &moduleName);

    void SetInstanceData(void* data, NativeFinalize finalize_cb, void* hint);
    void GetInstanceData(void** data);

    /**
     * @brief Set the Extension Infos
     *
     * @param extensionInfos extension infos to set
     */
    void SetExtensionInfos(std::unordered_map<std::string, int32_t>&& extensionInfos);

    /**
     * @brief Get the Extension Infos
     *
     * @return extension infos
     */
    const std::unordered_map<std::string, int32_t>& GetExtensionInfos();

    /**
     * @brief Set the Module Blocklist
     *
     * @param blocklist the blocklist set to native engine
     */
    void SetModuleBlocklist(std::unordered_map<int32_t, std::unordered_set<std::string>>&& blocklist);

    /**
     * @brief Set the Module Load Checker
     *
     * @param moduleCheckerDelegate the module checker delegate will intercept the module loading
     */
    void SetModuleLoadChecker(const std::shared_ptr<ModuleCheckerDelegate>& moduleCheckerDelegate);

    NativeChunk& GetNativeChunk()
    {
        return nativeChunk_;
    }

private:
    static NativeEngine* CreateRuntimeFunc(NativeEngine* engine, void* jsEngine, bool isLimitedWorker = false);
    void StartCleanupTimer();
protected:
    void *jsEngine_;
    void* jsEngineInterface_;

    void Init();
    void Deinit();

    NativeModuleManager* moduleManager_ = nullptr;
    NativeReferenceManager* referenceManager_ = nullptr;
    NativeCallbackScopeManager* callbackScopeManager_ = nullptr;

    uv_loop_t* loop_ = nullptr;

    NativeErrorExtendedInfo lastError_;

    // register for worker
    InitWorkerFunc initWorkerFunc_ {nullptr};
    GetAssetFunc getAssetFunc_ {nullptr};
    OffWorkerFunc offWorkerFunc_ {nullptr};
#if !defined(PREVIEW)
    DebuggerPostTask debuggerPostTaskFunc_ {nullptr};
#endif
    NativeEngine* hostEngine_ {nullptr};

public:
    uint64_t openHandleScopes_ = 0;
    static std::string tempModuleName_;

private:
    std::string moduleName_;
    std::mutex instanceDataLock_;
    NativeObjectInfo instanceDataInfo_;
    void FinalizerInstanceData(void);
    pthread_t tid_ = 0;
    std::unordered_map<std::string, int32_t> extensionInfos_;
    uv_sem_t uvSem_;

    // the old worker api use before api9, the new worker api start with api9
    enum JSThreadType { MAIN_THREAD, WORKER_THREAD, TASKPOOL_THREAD };
    JSThreadType jsThreadType_ = JSThreadType::MAIN_THREAD;
    // current is hostengine, can create old worker, new worker, or no workers on hostengine
    std::atomic<WorkerVersion> workerVersion_ { WorkerVersion::NONE };

#if !defined(PREVIEW)
    static void UVThreadRunner(void* nativeEngine);
    void PostLoopTask();

    bool checkUVLoop_ = false;
    uv_thread_t uvThread_;
#endif

    PostTask postTask_ = nullptr;
    CleanEnv cleanEnv_ = nullptr;
    uv_async_t uvAsync_;
    std::unordered_set<CleanupHookCallback, CleanupHookCallback::Hash, CleanupHookCallback::Equal> cleanup_hooks_;
    uint64_t cleanup_hook_counter_ = 0;
    std::atomic_int request_waiting_ { 0 };
    std::atomic_int subEnvCounter_ { 0 };
    std::atomic_bool isStopping_ { false };
    bool cleanupTimeout_ = false;
    uv_timer_t timer_;
    NativeChunk nativeChunk_;
    EcmaVM* vm_ = nullptr;
    bool needStop_ = false;
    NapiConcurrentCallback concurrentCallbackFunc_ { nullptr };
    NativeReference* promiseRejectCallbackRef_ { nullptr };
    NativeReference* checkCallbackRef_ { nullptr };
    std::unordered_map<NativeModule*, panda::Global<panda::JSValueRef>> loadedModules_;
    static PermissionCheckCallback permissionCheckCallback_;
    NapiUncaughtExceptionCallback napiUncaughtExceptionCallback_ { nullptr };
    SourceMapCallback SourceMapCallback_ { nullptr };
    static bool napiProfilerParamReaded;
    std::once_flag flag_;
    std::unique_ptr<std::thread> threadJsHeap_;
    std::mutex lock_;
    std::condition_variable condition_;
    bool isLimitedWorker_ = false;
};

#endif /* FOUNDATION_ACE_NAPI_NATIVE_ENGINE_NATIVE_ENGINE_H */
