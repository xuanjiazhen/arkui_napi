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

#include "native_engine/native_engine.h"

#include <uv.h>
#if !defined(WINDOWS_PLATFORM) && !defined(MAC_PLATFORM) && !defined(IOS_PLATFORM) && !defined(LINUX_PLATFORM)
#include <sys/epoll.h>
#endif
#ifdef IOS_PLATFORM
#include <sys/event.h>
#endif
#ifdef ENABLE_HITRACE
#include <sys/prctl.h>
#endif

#include "ecmascript/napi/include/dfx_jsnapi.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "native_engine/native_utils.h"
#include "unicode/ucnv.h"
#include "utils/log.h"
#include "native_deferred.h"
#include "native_reference.h"
#include "native_engine/native_property.h"
#include "securec.h"
#include "utils/log.h"
#if !defined(PREVIEW) && !defined(ANDROID_PLATFORM) && !defined(IOS_PLATFORM)
#include "parameters.h"
#endif
#ifdef ENABLE_CONTAINER_SCOPE
#include "core/common/container_scope.h"
#endif
#ifdef ENABLE_HITRACE
#include "hitrace_meter.h"
#include "parameter.h"
#endif

constexpr size_t NAME_BUFFER_SIZE = 64;
static constexpr size_t DESTRUCTION_TIMEOUT = 3000;

using panda::DFXJSNApi;
using panda::JSNApi;
using panda::LocalScope;
using panda::JsiRuntimeCallInfo;
using panda::BooleanRef;
using panda::ObjectRef;
using panda::StringRef;
using panda::Global;
using panda::FunctionRef;
using panda::PrimitiveRef;
using panda::ArrayBufferRef;
using panda::TypedArrayRef;
using panda::PromiseCapabilityRef;
using panda::NativePointerRef;
using panda::SymbolRef;
using panda::IntegerRef;
using panda::DateRef;
using panda::BigIntRef;
using panda::PropertyAttribute;

static constexpr auto PANDA_MAIN_FUNCTION = "_GLOBAL::func_main_0";
static constexpr auto PANDA_MODULE_NAME = "_GLOBAL_MODULE_NAME";
static constexpr auto PANDA_MODULE_NAME_LEN = 32;
static constexpr uint32_t MAX_CHUNK_ARRAY_SIZE = 10;
static std::unordered_set<std::string> NATIVE_MODULE = {"system.app", "ohos.app", "system.router", 
    "system.curves", "ohos.curves", "system.matrix4", "ohos.matrix4"};
static constexpr auto NATIVE_MODULE_PREFIX = "@native:";
static constexpr auto OHOS_MODULE_PREFIX = "@ohos:";
#if !defined(PREVIEW) && !defined(IOS_PLATFORM) && !defined(ANDROID_PLATFORM)
static constexpr uint32_t DEC_TO_INT = 100;
static size_t g_threshold = OHOS::system::GetUintParameter<size_t>("persist.ark.leak.threshold", 85);
static constexpr int TIME_OUT = 20;
static size_t g_checkInterval = OHOS::system::GetUintParameter<size_t>("persist.ark.leak.checkinterval", 60);
static constexpr int DEFAULT_SLEEP_TIME = 100000; //poll every 0.1s
static bool g_enableProperty = OHOS::system::GetBoolParameter("persist.ark.leak.enableproperty", true);
static uint64_t g_lastHeapDumpTime = 0;
static bool g_debugLeak = OHOS::system::GetBoolParameter("debug.arkengine.tags.enableleak", false);
static constexpr uint64_t HEAP_DUMP_REPORT_INTERVAL = 24 * 3600 * 1000;
static constexpr uint64_t SEC_TO_MILSEC = 1000;
#endif
#ifdef ENABLE_HITRACE
constexpr auto NAPI_PROFILER_PARAM_SIZE = 10;
#endif

std::string NativeEngine::tempModuleName_ {""};
bool NativeEngine::napiProfilerEnabled {false};
bool NativeEngine::napiProfilerParamReaded {false};
PermissionCheckCallback NativeEngine::permissionCheckCallback_ {nullptr};

// To be delete
napi_value LocalValueToLocalNapiValue(panda::Local<panda::JSValueRef> local)
{
    return JsValueFromLocalValue(local);
}

// To be delete
panda::Local<panda::JSValueRef> NapiValueToLocalValue(napi_value v)
{
    return LocalValueFromJsValue(v);
}

void FunctionSetContainerId(const EcmaVM *vm, panda::Local<panda::JSValueRef> &value)
{
    panda::Local<panda::FunctionRef> funcValue(value);
    if (funcValue->IsNative(vm)) {
        return;
    }

    auto extraInfo = funcValue->GetData(vm);
    if (extraInfo != nullptr) {
        return;
    }

    NapiFunctionInfo *funcInfo = NapiFunctionInfo::CreateNewInstance();
#ifdef ENABLE_CONTAINER_SCOPE
    funcInfo->scopeId = OHOS::Ace::ContainerScope::CurrentId();
#endif
    funcValue->SetData(vm, reinterpret_cast<void*>(funcInfo),
        [](void *externalPointer, void *data) {
            auto info = reinterpret_cast<NapiFunctionInfo*>(data);
            if (info != nullptr) {
                delete info;
                info = nullptr;
            }
        }, true);
}

panda::Local<panda::JSValueRef> NapiDefineClass(napi_env env, const char* name, NapiNativeCallback callback,
    void* data, const NapiPropertyDescriptor* properties, size_t length)
{
    auto vm = const_cast<EcmaVM*>(reinterpret_cast<NativeEngine*>(env)->GetEcmaVm());
    std::string className(name);
    if (NativeEngine::napiProfilerEnabled) {
        className = NativeEngine::tempModuleName_ + "." + name;
    }

    NapiFunctionInfo* funcInfo = NapiFunctionInfo::CreateNewInstance();
    if (funcInfo == nullptr) {
        HILOG_ERROR("funcInfo is nullptr");
        return panda::JSValueRef::Undefined(vm);
    }
    funcInfo->env = env;
    funcInfo->callback = callback;
    funcInfo->data = data;
#ifdef ENABLE_CONTAINER_SCOPE
    funcInfo->scopeId = OHOS::Ace::ContainerScope::CurrentId();
#endif

    Local<panda::FunctionRef> fn = panda::FunctionRef::NewClassFunction(vm, NativeFunctionCallBack,
        [](void* externalPointer, void* data) {
            auto info = reinterpret_cast<NapiFunctionInfo*>(data);
                if (info != nullptr) {
                    delete info;
                }
            },
        reinterpret_cast<void*>(funcInfo), true);
    Local<panda::ObjectRef> classPrototype = fn->GetFunctionPrototype(vm);
    Local<panda::ObjectRef> fnObj = fn->ToObject(vm);
    reinterpret_cast<NativeEngine*>(env)->SetModuleName(fnObj, className);
    for (size_t i = 0; i < length; i++) {
        if (properties[i].attributes & NATIVE_STATIC) {
            NapiDefineProperty(env, fnObj, properties[i]);
        } else {
            if (classPrototype->IsUndefined()) {
                HILOG_ERROR("NativeEngineImpl::Class's prototype is null");
                continue;
            }
            reinterpret_cast<NativeEngine*>(env)->SetModuleName(classPrototype, className);
            NapiDefineProperty(env, classPrototype, properties[i]);
        }
    }

    return fn;
}

struct MoudleNameLocker {
    explicit MoudleNameLocker(std::string moduleName)
    {
        NativeEngine::tempModuleName_ = moduleName;
    }
    ~MoudleNameLocker()
    {
        NativeEngine::tempModuleName_ = "";
    }
};

namespace {
const char* g_errorMessages[] = {
    nullptr,
    "Invalid parameter",
    "Need object",
    "Need string",
    "Need string or symbol",
    "Need function",
    "Need number",
    "Need boolean",
    "Need array",
    "Generic failure",
    "An exception is blocking",
    "Asynchronous work cancelled",
    "Escape called twice",
    "Handle scope mismatch",
    "Callback scope mismatch",
    "Asynchronous work queue is full",
    "Asynchronous work handle is closing",
    "Need bigint",
    "Need date",
    "Need arraybuffer",
    "Need detachable arraybuffer",
};
} // namespace

static GetContainerScopeIdCallback getContainerScopeIdFunc_;
static ContainerScopeCallback initContainerScopeFunc_;
static ContainerScopeCallback finishContainerScopeFunc_;

NativeEngine::NativeEngine(EcmaVM* vm, void* jsEngine, bool isLimitedWorker) : jsEngine_(jsEngine),
                                                                               vm_(vm),
                                                                               isLimitedWorker_(isLimitedWorker)
{
    HILOG_DEBUG("NativeEngine::NativeEngine");
#if !defined(PREVIEW) && !defined(IOS_PLATFORM) && !defined(ANDROID_PLATFORM)
    if (g_enableProperty) {
        std::call_once(flag_, [this] {
            if (threadJsHeap_ == nullptr) {
                threadJsHeap_ = std::make_unique<std::thread>(&NativeEngine::JsHeapStart, this);
                HILOG_ERROR("JsHeapStart is OK");
            }
        });
    }
#endif
#ifdef ENABLE_HITRACE
    if (!NativeEngine::napiProfilerParamReaded) {
        char napiProfilerParam[NAPI_PROFILER_PARAM_SIZE] = {0};
        int ret = GetParameter("persist.hiviewdfx.napiprofiler.enabled", "false",
            napiProfilerParam, sizeof(napiProfilerParam));
        if (ret > 0 && strcmp(napiProfilerParam, "true") == 0) {
            NativeEngine::napiProfilerEnabled = true;
        }
        NativeEngine::napiProfilerParamReaded = true;
    }
#endif
    LocalScope scope(vm_);
    Local<StringRef> requireInternalName = StringRef::NewFromUtf8(vm, "requireInternal");
    void* requireData = static_cast<void*>(this);

    Local<FunctionRef> requireNapi =
        FunctionRef::New(
            vm,
            [](JsiRuntimeCallInfo *info) -> Local<JSValueRef> {
                EcmaVM *ecmaVm = info->GetVM();
                panda::EscapeLocalScope scope(ecmaVm);
                NativeModuleManager* moduleManager = NativeModuleManager::GetInstance();
                NativeEngine* nativeEngine = static_cast<NativeEngine*>(info->GetData());
                Local<StringRef> moduleName(info->GetCallArgRef(0));
                NativeModule* module = nullptr;
                bool isAppModule = false;
#ifdef IOS_PLATFORM
                module = moduleManager->LoadNativeModule(moduleName->ToString().c_str(), nullptr, false, false,
                                                         "", nativeEngine->isLimitedWorker_);
#else
                const uint32_t lengthMax = 2;
                if (info->GetArgsNumber() >= lengthMax) {
                    Local<BooleanRef> ret(info->GetCallArgRef(1));
                    isAppModule = ret->Value();
                }
                if (info->GetArgsNumber() == 3) { // 3:Determine if the number of parameters is equal to 3
                    Local<StringRef> path(info->GetCallArgRef(2)); // 2:Take the second parameter
                    module =
                        moduleManager->LoadNativeModule(moduleName->ToString().c_str(), path->ToString().c_str(),
                            isAppModule, false, "", nativeEngine->isLimitedWorker_);
                } else if (info->GetArgsNumber() == 4) { // 4:Determine if the number of parameters is equal to 4
                    Local<StringRef> path(info->GetCallArgRef(2)); // 2:Take the second parameter
                    Local<StringRef> relativePath(info->GetCallArgRef(3)); // 3:Take the second parameter
                    module =
                        moduleManager->LoadNativeModule(moduleName->ToString().c_str(), nullptr, isAppModule, false,
                            relativePath->ToString().c_str(), nativeEngine->isLimitedWorker_);
                } else {
                    module =
                        moduleManager->LoadNativeModule(moduleName->ToString().c_str(), nullptr, isAppModule, false,
                                                        "", nativeEngine->isLimitedWorker_);
                }
#endif
                Local<JSValueRef> exports(JSValueRef::Undefined(ecmaVm));
                if (module != nullptr) {
                    auto it = nativeEngine->loadedModules_.find(module);
                    if (it != nativeEngine->loadedModules_.end()) {
                        return scope.Escape(it->second.ToLocal(ecmaVm));
                    }
                    std::string strModuleName = moduleName->ToString();
                    moduleManager->SetNativeEngine(strModuleName, nativeEngine);
                    MoudleNameLocker nameLocker(strModuleName);

                    if (module->jsCode == nullptr && module->getABCCode != nullptr) {
                        module->getABCCode(&module->jsCode, &module->jsCodeLen);
                    }
                    if (module->jsABCCode != nullptr || module->jsCode != nullptr) {
                        char fileName[NAPI_PATH_MAX] = { 0 };
                        const char* name = module->name;
                        if (sprintf_s(fileName, sizeof(fileName), "lib%s.z.so/%s.js", name, name) == -1) {
                            HILOG_ERROR("sprintf_s file name failed");
                            return scope.Escape(exports);
                        }
                        HILOG_DEBUG("load js code from %{public}s", fileName);
                        const void *buffer = nullptr;
                        if (module->jsABCCode) {
                            buffer = static_cast<const void *>(module->jsABCCode);
                        } else {
                            buffer = static_cast<const void *>(module->jsCode);
                        }
                        auto exportObject = nativeEngine->LoadArkModule(buffer, module->jsCodeLen, fileName);
                        if (exportObject->IsUndefined()) {
                            HILOG_ERROR("load module failed");
                            return scope.Escape(exports);
                        } else {
                            exports = exportObject;
                            nativeEngine->loadedModules_[module] = Global<JSValueRef>(ecmaVm, exports);
                        }
                    } else if (module->registerCallback != nullptr) {
                        Local<ObjectRef> exportObj = ObjectRef::New(ecmaVm);
#ifdef ENABLE_HITRACE
                        StartTrace(HITRACE_TAG_ACE, "NAPI module init, name = " + std::string(module->name));
#endif
                        nativeEngine->SetModuleName(exportObj, module->name);
                        module->registerCallback(reinterpret_cast<napi_env>(nativeEngine),
                                                 JsValueFromLocalValue(exportObj));
#ifdef ENABLE_HITRACE
                        FinishTrace(HITRACE_TAG_ACE);
#endif
                        exports = exportObj;
                        nativeEngine->loadedModules_[module] = Global<JSValueRef>(ecmaVm, exports);
                    } else {
                        HILOG_ERROR("init module failed");
                        return scope.Escape(exports);
                    }
                }
                return scope.Escape(exports);
            },
            nullptr,
            requireData);

    Local<FunctionRef> requireInternal =
        FunctionRef::New(
            vm,
            [](JsiRuntimeCallInfo *info) -> Local<JSValueRef> {
                EcmaVM *ecmaVm = info->GetVM();
                panda::EscapeLocalScope scope(ecmaVm);
                NativeModuleManager* moduleManager = NativeModuleManager::GetInstance();
                NativeEngine* nativeEngine = static_cast<NativeEngine*>(info->GetData());
                Local<StringRef> moduleName(info->GetCallArgRef(0));
                NativeModule* module = moduleManager->LoadNativeModule(moduleName->ToString().c_str(), nullptr, false,
                                                                       false, "", nativeEngine->isLimitedWorker_);
                Local<JSValueRef> exports(JSValueRef::Undefined(ecmaVm));
                MoudleNameLocker nameLocker(moduleName->ToString());
                if (module != nullptr && nativeEngine) {
                    auto it = nativeEngine->loadedModules_.find(module);
                    if (it != nativeEngine->loadedModules_.end()) {
                        return scope.Escape(it->second.ToLocal(ecmaVm));
                    }
                    std::string strModuleName = moduleName->ToString();
                    moduleManager->SetNativeEngine(strModuleName, nativeEngine);
                    Local<ObjectRef> exportObj = ObjectRef::New(ecmaVm);
                    if (exportObj->IsObject()) {
                        nativeEngine->SetModuleName(exportObj, module->name);
                        module->registerCallback(reinterpret_cast<napi_env>(nativeEngine),
                                                 JsValueFromLocalValue(exportObj));
                        exports = exportObj;
                        nativeEngine->loadedModules_[module] = Global<JSValueRef>(ecmaVm, exports);
                    } else {
                        HILOG_ERROR("exportObject is nullptr");
                        return scope.Escape(exports);
                    }
                }
                return scope.Escape(exports);
            },
            nullptr,
            requireData);

    Local<ObjectRef> global = panda::JSNApi::GetGlobalObject(vm);
#if !defined(PREVIEW)
    Local<StringRef> requireName = StringRef::NewFromUtf8(vm, "requireNapi");
    global->Set(vm, requireName, requireNapi);
#else
    Local<StringRef> requireNapiPreview = StringRef::NewFromUtf8(vm, "requireNapiPreview");
    global->Set(vm, requireNapiPreview, requireNapi);
#endif
    global->Set(vm, requireInternalName, requireInternal);
    JSNApi::SetNativePtrGetter(vm, reinterpret_cast<void*>(NativeEngine::GetNativePtrCallBack));
    // need to call init of base class.
    NativeModuleManager* moduleManager = NativeModuleManager::GetInstance();
    JSNApi::SetUnloadNativeModuleCallback(vm, std::bind(&NativeModuleManager::UnloadNativeModule,
        moduleManager, std::placeholders::_1));
    Init();
    panda::JSNApi::SetLoop(vm, loop_);
}

NativeEngine::~NativeEngine()
{
    HILOG_INFO("NativeEngine::~NativeEngine");
    Deinit();
    // Free cached objects
    for (auto&& [module, exportObj] : loadedModules_) {
        exportObj.FreeGlobalHandleAddr();
    }
    // Free callbackRef
    if (promiseRejectCallbackRef_ != nullptr) {
        delete promiseRejectCallbackRef_;
    }
    if (checkCallbackRef_ != nullptr) {
        delete checkCallbackRef_;
    }
#if !defined(PREVIEW) && !defined(IOS_PLATFORM)
    // Free threadJsHeap_
    needStop_ = true;
    condition_.notify_all();
    if (threadJsHeap_->joinable()) {
        threadJsHeap_->join();
    }
#endif
    if (cleanEnv_ != nullptr) {
        cleanEnv_();
    }
    std::lock_guard<std::mutex> insLock(instanceDataLock_);
    FinalizerInstanceData();
}

void NativeEngine::Init()
{
    HILOG_INFO("NativeEngine::Init");
    moduleManager_ = NativeModuleManager::GetInstance();
    referenceManager_ = new NativeReferenceManager();
    callbackScopeManager_ = new NativeCallbackScopeManager();
    loop_ = uv_loop_new();
    if (loop_ == nullptr) {
        return;
    }
    tid_ = pthread_self();
    uv_async_init(loop_, &uvAsync_, nullptr);
    uv_sem_init(&uvSem_, 0);
}

void NativeEngine::Deinit()
{
    HILOG_INFO("NativeEngine::Deinit");
    uv_sem_destroy(&uvSem_);
    uv_close((uv_handle_t*)&uvAsync_, nullptr);
    RunCleanup();
    if (referenceManager_ != nullptr) {
        delete referenceManager_;
        referenceManager_ = nullptr;
    }

    SetStopping(true);
    uv_loop_delete(loop_);
    loop_ = nullptr;
}

NativeReferenceManager* NativeEngine::GetReferenceManager()
{
    return referenceManager_;
}

NativeModuleManager* NativeEngine::GetModuleManager()
{
    return moduleManager_;
}

NativeCallbackScopeManager* NativeEngine::GetCallbackScopeManager()
{
    return callbackScopeManager_;
}

uv_loop_t* NativeEngine::GetUVLoop() const
{
    return loop_;
}

pthread_t NativeEngine::GetTid() const
{
    return tid_;
}

bool NativeEngine::ReinitUVLoop()
{
    if (loop_ != nullptr) {
        uv_sem_destroy(&uvSem_);
        uv_close((uv_handle_t*)&uvAsync_, nullptr);
        uv_run(loop_, UV_RUN_ONCE);
        uv_loop_delete(loop_);
    }

    loop_ = uv_loop_new();
    if (loop_ == nullptr) {
        return false;
    }
    tid_ = pthread_self();
    uv_async_init(loop_, &uvAsync_, nullptr);
    uv_sem_init(&uvSem_, 0);
    return true;
}

void NativeEngine::Loop(LoopMode mode, bool needSync)
{
    LocalScope scope(vm_);
    bool more = true;
    switch (mode) {
        case LoopMode::LOOP_DEFAULT:
            more = uv_run(loop_, UV_RUN_DEFAULT);
            break;
        case LoopMode::LOOP_ONCE:
            more = uv_run(loop_, UV_RUN_ONCE);
            break;
        case LoopMode::LOOP_NOWAIT:
            more = uv_run(loop_, UV_RUN_NOWAIT);
            break;
        default:
            return;
    }
    if (more == false) {
        uv_loop_alive(loop_);
    }

    if (needSync) {
        uv_sem_post(&uvSem_);
    }
    panda::JSNApi::ExecutePendingJob(vm_);
}

NativeAsyncWork* NativeEngine::CreateAsyncWork(napi_value asyncResource, napi_value asyncResourceName,
    NativeAsyncExecuteCallback execute, NativeAsyncCompleteCallback complete, void* data)
{
    (void)asyncResource;
    (void)asyncResourceName;
    char name[NAME_BUFFER_SIZE] = {0};
    if (asyncResourceName != nullptr) {
        auto val = LocalValueFromJsValue(asyncResourceName);
        size_t strLength = 0;
        auto vm = GetEcmaVm();
        LocalScope scope(vm);
        auto str = val->ToString(vm);
        char* buffer = name;
        if (buffer == nullptr) {
            strLength = str->Utf8Length(vm) - 1;
        } else if (NAME_BUFFER_SIZE != 0) {
            int copied = str->WriteUtf8(buffer, NAME_BUFFER_SIZE - 1, true) - 1;
            buffer[copied] = '\0';
            strLength = copied;
        } else {
            strLength = 0;
        }
    }
    return new NativeAsyncWork(this, execute, complete, name, data);
}

NativeAsyncWork* NativeEngine::CreateAsyncWork(const std::string& asyncResourceName, NativeAsyncExecuteCallback execute,
    NativeAsyncCompleteCallback complete, void* data)
{
    return new NativeAsyncWork(this, execute, complete, asyncResourceName, data);
}

NativeSafeAsyncWork* NativeEngine::CreateSafeAsyncWork(napi_value func, napi_value asyncResource,
    napi_value asyncResourceName, size_t maxQueueSize, size_t threadCount, void* finalizeData,
    NativeFinalize finalizeCallback, void* context, NativeThreadSafeFunctionCallJs callJsCallback)
{
    return new NativeSafeAsyncWork(this, func, asyncResource, asyncResourceName, maxQueueSize, threadCount,
        finalizeData, finalizeCallback, context, callJsCallback);
}

NativeErrorExtendedInfo* NativeEngine::GetLastError()
{
    return &lastError_;
}

void NativeEngine::SetLastError(int errorCode, uint32_t engineErrorCode, void* engineReserved)
{
    lastError_.errorCode = errorCode;
    lastError_.engineErrorCode = engineErrorCode;
    lastError_.message = g_errorMessages[lastError_.errorCode];
    lastError_.reserved = engineReserved;
}

void NativeEngine::ClearLastError()
{
    lastError_.errorCode = 0;
    lastError_.engineErrorCode = 0;
    lastError_.message = nullptr;
    lastError_.reserved = nullptr;
}

void SubEncodeToUtf8(const EcmaVM* vm,
                     Local<JSValueRef>& nativeValue,
                     Local<StringRef>& nativeString,
                     char* buffer,
                     int32_t* written,
                     size_t bufferSize,
                     int32_t* nchars)
{
    int32_t length = nativeString->Length();
    int32_t pos = 0;
    int32_t writableSize = static_cast<int32_t>(bufferSize);
    int32_t i = 0;
    panda::Local<ObjectRef> strObj = nativeValue->ToObject(vm);
    for (; i < length; i++) {
        panda::Local<StringRef> str = strObj->Get(vm, i)->ToString(vm);
        int32_t len = str->Utf8Length(vm) - 1;
        if (len > writableSize) {
            break;
        }
        str->WriteUtf8((buffer + pos), writableSize);
        writableSize -= len;
        pos += len;
    }
    *nchars = i;
    HILOG_DEBUG("EncodeWriteUtf8 the result of buffer: %{public}s", buffer);
    *written = pos;
}

void NativeEngine::EncodeToUtf8(napi_value value, char* buffer, int32_t* written, size_t bufferSize, int32_t* nchars)
{
    auto nativeValue = LocalValueFromJsValue(value);
    if (nativeValue->IsNull() || nchars == nullptr || written == nullptr) {
        HILOG_ERROR("NativeEngine EncodeToUtf8 args is nullptr");
        return;
    }

    auto vm = GetEcmaVm();
    LocalScope scope(vm);
    auto nativeString = nativeValue->ToString(vm);
    if (!nativeString->IsString()) {
        HILOG_ERROR("nativeValue not is string");
        return;
    }

    if (buffer == nullptr) {
        HILOG_ERROR("buffer is null");
    }

    SubEncodeToUtf8(vm, nativeValue, nativeString, buffer, written, bufferSize, nchars);
}

void SubEncodeToChinese(const EcmaVM* vm,
                        Local<JSValueRef>& nativeValue,
                        Local<StringRef>& nativeString,
                        std::string& buffer,
                        const char* encode)
{
    int32_t length = nativeString->Length();
    int32_t pos = 0;
    const int32_t writableSize = 22; // 22 : encode max bytes of the ucnv_convent function;
    std::string tempBuf = "";
    tempBuf.resize(writableSize + 1);
    UErrorCode ErrorCode = U_ZERO_ERROR;
    const char* encFrom = "utf8";
    panda::Local<ObjectRef> strObj = nativeValue->ToObject(vm);
    for (int32_t i = 0; i < length; i++) {
        panda::Local<StringRef> str = strObj->Get(vm, i)->ToString(vm);
        int32_t len = str->Utf8Length(vm) - 1;
        if ((pos + len) >= writableSize) {
            char outBuf[writableSize] = {0};
            ucnv_convert(encode, encFrom, outBuf, writableSize, tempBuf.c_str(), pos, &ErrorCode);
            if (ErrorCode != U_ZERO_ERROR) {
                HILOG_ERROR("ucnv_convert is failed : ErrorCode = %{public}d", static_cast<int32_t>(ErrorCode));
                return;
            }
            buffer += outBuf;
            tempBuf.clear();
            pos = 0;
        }
        str->WriteUtf8((tempBuf.data() + pos), pos + len + 1);
        pos += len;
    }
    if (pos > 0) {
        char outBuf[writableSize] = {0};
        ucnv_convert(encode, encFrom, outBuf, writableSize, tempBuf.c_str(), pos, &ErrorCode);
        if (ErrorCode != U_ZERO_ERROR) {
            HILOG_ERROR("ucnv_convert is failed : ErrorCode = %{public}d", static_cast<int32_t>(ErrorCode));
            return;
        }
        buffer += outBuf;
    }
}

void NativeEngine::EncodeToChinese(napi_value value, std::string& buffer, const std::string& encoding)
{
    if (value == nullptr) {
        HILOG_ERROR("nativeValue GetInterface is nullptr");
        return;
    }

    auto nativeValue = LocalValueFromJsValue(value);
    auto vm = GetEcmaVm();
    LocalScope scope(vm);
    auto nativeString = nativeValue->ToString(vm);
    if (!nativeString->IsString()) {
        HILOG_ERROR("nativeValue not is string");
        return;
    }

    auto encode = encoding.c_str();
    if (encode == nullptr) {
        HILOG_ERROR("encoding is nullptr");
        return;
    }

    SubEncodeToChinese(vm, nativeValue, nativeString, buffer, encode);
}

#if !defined(PREVIEW)
void NativeEngine::CheckUVLoop()
{
    checkUVLoop_ = true;
    uv_thread_create(&uvThread_, NativeEngine::UVThreadRunner, this);
}

void NativeEngine::UVThreadRunner(void* nativeEngine)
{
    std::string name("UVLoop");
#ifdef IOS_PLATFORM
    pthread_setname_np(name.c_str());
#else
    pthread_setname_np(pthread_self(), name.c_str());
#endif
    auto engine = static_cast<NativeEngine*>(nativeEngine);
    engine->PostLoopTask();
    while (engine->checkUVLoop_) {
        int32_t fd = uv_backend_fd(engine->loop_);
        int32_t timeout = uv_backend_timeout(engine->loop_);
        int32_t result = -1;
#ifdef IOS_PLATFORM
        struct kevent events[1];
        struct timespec spec;
        static const int32_t mSec = 1000;
        static const int32_t uSec = 1000000;
        if (timeout != -1) {
            spec.tv_sec = timeout / mSec;
            spec.tv_nsec = (timeout % mSec) * uSec;
        }
        result = kevent(fd, NULL, 0, events, 1, timeout == -1 ? NULL : &spec);

#else
        struct epoll_event ev;
        result = epoll_wait(fd, &ev, 1, timeout);
#endif

        if (!engine->checkUVLoop_) {
            HILOG_INFO("break thread after epoll wait");
            break;
        }
        if (result >= 0) {
            engine->PostLoopTask();
        } else {
            HILOG_ERROR("epoll wait fail: result: %{public}d, errno: %{public}d", result, errno);
        }
        if (!engine->checkUVLoop_) {
            HILOG_INFO("break thread after post loop task");
            break;
        }
    }
}

void NativeEngine::CancelCheckUVLoop()
{
    checkUVLoop_ = false;
    uv_async_send(&uvAsync_);
    uv_sem_post(&uvSem_);
    uv_thread_join(&uvThread_);
}

void NativeEngine::PostLoopTask()
{
    postTask_(true);
    uv_sem_wait(&uvSem_);
}
#endif

void NativeEngine::SetPostTask(PostTask postTask)
{
    postTask_ = postTask;
}

void NativeEngine::TriggerPostTask()
{
    if (postTask_ == nullptr) {
        HILOG_ERROR("postTask_ is nullptr");
        return;
    }
    postTask_(false);
}

void* NativeEngine::GetJsEngine()
{
    return jsEngine_;
}

// register init worker func
void NativeEngine::SetInitWorkerFunc(InitWorkerFunc func)
{
    initWorkerFunc_ = func;
}
InitWorkerFunc NativeEngine::GetInitWorkerFunc() const
{
    return initWorkerFunc_;
}
void NativeEngine::SetGetAssetFunc(GetAssetFunc func)
{
    getAssetFunc_ = func;
}
GetAssetFunc NativeEngine::GetGetAssetFunc() const
{
    return getAssetFunc_;
}
void NativeEngine::SetOffWorkerFunc(OffWorkerFunc func)
{
    offWorkerFunc_ = func;
}
OffWorkerFunc NativeEngine::GetOffWorkerFunc() const
{
    return offWorkerFunc_;
}

// call init worker func
bool NativeEngine::CallInitWorkerFunc(NativeEngine* engine)
{
    if (initWorkerFunc_ != nullptr) {
        initWorkerFunc_(engine);
        return true;
    }
    return false;
}
bool NativeEngine::CallGetAssetFunc(const std::string& uri, std::vector<uint8_t>& content, std::string& ami)
{
    if (getAssetFunc_ != nullptr) {
        getAssetFunc_(uri, content, ami);
        return true;
    }
    return false;
}
bool NativeEngine::CallOffWorkerFunc(NativeEngine* engine)
{
    if (offWorkerFunc_ != nullptr) {
        offWorkerFunc_(engine);
        return true;
    }
    return false;
}

// adapt worker to ace container
void NativeEngine::SetGetContainerScopeIdFunc(GetContainerScopeIdCallback func)
{
    getContainerScopeIdFunc_ = func;
}
void NativeEngine::SetInitContainerScopeFunc(ContainerScopeCallback func)
{
    initContainerScopeFunc_ = func;
}
void NativeEngine::SetFinishContainerScopeFunc(ContainerScopeCallback func)
{
    finishContainerScopeFunc_ = func;
}
int32_t NativeEngine::GetContainerScopeIdFunc()
{
    int32_t scopeId = -1;
    if (getContainerScopeIdFunc_ != nullptr) {
        scopeId = getContainerScopeIdFunc_();
    }
    return scopeId;
}
bool NativeEngine::InitContainerScopeFunc(int32_t id)
{
    if (initContainerScopeFunc_ != nullptr) {
        initContainerScopeFunc_(id);
        return true;
    }
    return false;
}
bool NativeEngine::FinishContainerScopeFunc(int32_t id)
{
    if (finishContainerScopeFunc_ != nullptr) {
        finishContainerScopeFunc_(id);
        return true;
    }
    return false;
}

#if !defined(PREVIEW)
void NativeEngine::CallDebuggerPostTaskFunc(std::function<void()>&& task)
{
    if (debuggerPostTaskFunc_ != nullptr) {
        debuggerPostTaskFunc_(std::move(task));
    }
}

void NativeEngine::SetDebuggerPostTaskFunc(DebuggerPostTask func)
{
    debuggerPostTaskFunc_ = func;
}
#endif

void NativeEngine::SetHostEngine(NativeEngine* engine)
{
    hostEngine_ = engine;
}

NativeEngine* NativeEngine::GetHostEngine() const
{
    return hostEngine_;
}

void NativeEngine::AddCleanupHook(CleanupCallback fun, void* arg)
{
    HILOG_INFO("%{public}s, start.", __func__);
    auto insertion_info = cleanup_hooks_.emplace(CleanupHookCallback { fun, arg, cleanup_hook_counter_++ });
    if (insertion_info.second != true) {
        HILOG_ERROR("AddCleanupHook Failed.");
    }
    HILOG_INFO("%{public}s, end.", __func__);
}

void NativeEngine::RemoveCleanupHook(CleanupCallback fun, void* arg)
{
    HILOG_INFO("%{public}s, start.", __func__);
    CleanupHookCallback hook { fun, arg, 0 };
    cleanup_hooks_.erase(hook);
    HILOG_INFO("%{public}s, end.", __func__);
}

void NativeEngine::StartCleanupTimer()
{
    uv_timer_init(loop_, &timer_);
    timer_.data = this;
    uv_timer_start(&timer_, [](uv_timer_t* handle) {
        HILOG_DEBUG("NativeEngine:: timer is end with timeout.");
        reinterpret_cast<NativeEngine*>(handle->data)->cleanupTimeout_ = true;
    }, DESTRUCTION_TIMEOUT, 0);
    uv_unref(reinterpret_cast<uv_handle_t*>(&timer_));
}

void NativeEngine::RunCleanup()
{
    HILOG_DEBUG("%{public}s, start.", __func__);
    StartCleanupTimer();
    CleanupHandles();
    // sync clean up
    while (!cleanup_hooks_.empty()) {
        HILOG_DEBUG("NativeEngine::RunCleanup cleanup_hooks is not empty");
        // Copy into a vector, since we can't sort an unordered_set in-place.
        std::vector<CleanupHookCallback> callbacks(cleanup_hooks_.begin(), cleanup_hooks_.end());
        // We can't erase the copied elements from `cleanup_hooks_` yet, because we
        // need to be able to check whether they were un-scheduled by another hook.

        std::sort(callbacks.begin(), callbacks.end(), [](const CleanupHookCallback& a, const CleanupHookCallback& b) {
            // Sort in descending order so that the most recently inserted callbacks are run first.
            return a.insertion_order_counter_ > b.insertion_order_counter_;
        });
        HILOG_DEBUG(
            "NativeEngine::RunCleanup cleanup_hooks callbacks size:%{public}d", (int32_t)callbacks.size());
        for (const CleanupHookCallback& cb : callbacks) {
            if (cleanup_hooks_.count(cb) == 0) {
                // This hook was removed from the `cleanup_hooks_` set during another
                // hook that was run earlier. Nothing to do here.
                continue;
            }
            cb.fn_(cb.arg_);
            cleanup_hooks_.erase(cb);
        }
        CleanupHandles();
    }

    while (uv_run(loop_, UV_RUN_NOWAIT) != 0 && !cleanupTimeout_) {}
    uv_timer_stop(&timer_);
    uv_close(reinterpret_cast<uv_handle_t*>(&timer_), nullptr);
    uv_run(loop_, UV_RUN_ONCE);

    if (cleanupTimeout_) {
        HILOG_ERROR("RunCleanup timeout");
    }
    HILOG_DEBUG("%{public}s, end.", __func__);
}

void NativeEngine::CleanupHandles()
{
    while (request_waiting_.load() > 0 && !cleanupTimeout_) {
        HILOG_INFO("%{public}s, request waiting:%{public}d.", __func__,
            request_waiting_.load(std::memory_order_relaxed));
        uv_run(loop_, UV_RUN_ONCE);
    }
}

void NativeEngine::IncreaseWaitingRequestCounter()
{
    request_waiting_++;
}

void NativeEngine::DecreaseWaitingRequestCounter()
{
    request_waiting_--;
}

bool NativeEngine::HasWaitingRequest()
{
    return request_waiting_.load() != 0;
}

void NativeEngine::RegisterWorkerFunction(const NativeEngine* engine)
{
    if (engine == nullptr) {
        return;
    }
    SetInitWorkerFunc(engine->GetInitWorkerFunc());
    SetGetAssetFunc(engine->GetGetAssetFunc());
    SetOffWorkerFunc(engine->GetOffWorkerFunc());
}

napi_value NativeEngine::RunScript(const char* path)
{
    std::vector<uint8_t> scriptContent;
    std::string pathStr(path);
    std::string ami;
    if (!CallGetAssetFunc(pathStr, scriptContent, ami)) {
        HILOG_ERROR("Get asset error");
        return nullptr;
    }
    HILOG_INFO("asset size is %{public}zu", scriptContent.size());
    return RunActor(scriptContent, ami.c_str());
}

void NativeEngine::SetInstanceData(void* data, NativeFinalize finalize_cb, void* hint)
{
    HILOG_INFO("NativeEngineWraper::%{public}s, start.", __func__);
    std::lock_guard<std::mutex> insLock(instanceDataLock_);
    FinalizerInstanceData();
    instanceDataInfo_.engine = this;
    instanceDataInfo_.callback = finalize_cb;
    instanceDataInfo_.nativeObject = data;
    instanceDataInfo_.hint = hint;
}

void NativeEngine::GetInstanceData(void** data)
{
    HILOG_INFO("NativeEngineWraper::%{public}s, start.", __func__);
    std::lock_guard<std::mutex> insLock(instanceDataLock_);
    if (data) {
        *data = instanceDataInfo_.nativeObject;
    }
}

void NativeEngine::FinalizerInstanceData(void)
{
    if (instanceDataInfo_.engine != nullptr && instanceDataInfo_.callback != nullptr) {
        instanceDataInfo_.callback(instanceDataInfo_.engine, instanceDataInfo_.nativeObject, instanceDataInfo_.hint);
    }
    instanceDataInfo_.engine = nullptr;
    instanceDataInfo_.callback = nullptr;
    instanceDataInfo_.nativeObject = nullptr;
    instanceDataInfo_.hint = nullptr;
}

const char* NativeEngine::GetModuleFileName()
{
    HILOG_INFO("%{public}s, start.", __func__);
    NativeModuleManager* moduleManager = GetModuleManager();
    HILOG_INFO("NativeEngineWraper::GetFileName GetModuleManager");
    if (moduleManager != nullptr) {
        const char* moduleFileName = moduleManager->GetModuleFileName(moduleName_.c_str(), false);
        HILOG_INFO("NativeEngineWraper::GetFileName end filename:%{public}s", moduleFileName);
        return moduleFileName;
    }
    return nullptr;
}

void NativeEngine::SetModuleFileName(std::string& moduleName)
{
    moduleName_ = moduleName;
}

void NativeEngine::SetExtensionInfos(std::unordered_map<std::string, int32_t>&& extensionInfos)
{
    extensionInfos_ = extensionInfos;
}

const std::unordered_map<std::string, int32_t>& NativeEngine::GetExtensionInfos()
{
    return extensionInfos_;
}

void NativeEngine::SetModuleLoadChecker(const std::shared_ptr<ModuleCheckerDelegate>& moduleCheckerDelegate)
{
    NativeModuleManager* moduleManager = GetModuleManager();
    if (!moduleManager) {
        HILOG_ERROR("SetModuleLoadChecker failed, moduleManager is nullptr");
        return;
    }
    moduleManager->SetModuleLoadChecker(moduleCheckerDelegate);
}

void* NativeEngine::GetNativePtrCallBack(void* data)
{
    if (data == nullptr) {
        HILOG_ERROR("data is nullptr");
        return nullptr;
    }
    auto info = reinterpret_cast<NapiFunctionInfo*>(data);
    auto cb = reinterpret_cast<void*>(info->callback);
    return cb;
}

const EcmaVM* NativeEngine::GetEcmaVm() const
{
    return vm_;
}

static inline void StartNapiProfilerTrace(panda::JsiRuntimeCallInfo *runtimeInfo)
{
#ifdef ENABLE_HITRACE
    if (NativeEngine::napiProfilerEnabled) {
        EcmaVM *vm = runtimeInfo->GetVM();
        LocalScope scope(vm);
        Local<panda::FunctionRef> fn = runtimeInfo->GetFunctionRef();
        Local<panda::StringRef> nameRef = fn->GetName(vm);
        char threadName[128];
        prctl(PR_GET_NAME, threadName);
        StartTraceArgs(HITRACE_TAG_ACE, "Napi called:%s, tname:%s", nameRef->ToString().c_str(), threadName);
    }
#endif
}

static inline void FinishNapiProfilerTrace()
{
#ifdef ENABLE_HITRACE
    if (NativeEngine::napiProfilerEnabled) {
        FinishTrace(HITRACE_TAG_ACE);
    }
#endif
}

panda::Local<panda::JSValueRef> NativeFunctionCallBack(JsiRuntimeCallInfo *runtimeInfo)
{
    EcmaVM *vm = runtimeInfo->GetVM();
    panda::EscapeLocalScope scope(vm);
    auto info = reinterpret_cast<NapiFunctionInfo*>(runtimeInfo->GetData());
    auto engine = reinterpret_cast<NativeEngine*>(info->env);
    auto cb = info->callback;
    if (engine == nullptr) {
        HILOG_ERROR("native engine is null");
        return JSValueRef::Undefined(vm);
    }

    NapiNativeCallbackInfo cbInfo = { 0 };
    StartNapiProfilerTrace(runtimeInfo);
    cbInfo.thisVar = JsValueFromLocalValue(runtimeInfo->GetThisRef());
    panda::Local<panda::JSValueRef> newValue = runtimeInfo->GetNewTargetRef();
    if (newValue->IsFunction()) {
        FunctionSetContainerId(vm, newValue);
    }
    cbInfo.function = JsValueFromLocalValue(newValue);
    cbInfo.argc = static_cast<size_t>(runtimeInfo->GetArgsNumber());
    cbInfo.argv = nullptr;
    cbInfo.functionInfo = info;
    if (cbInfo.argc > 0) {
        if (cbInfo.argc > MAX_CHUNK_ARRAY_SIZE) {
            cbInfo.argv = new napi_value [cbInfo.argc];
        } else {
            cbInfo.argv = engine->GetNativeChunk().NewArray<napi_value>(cbInfo.argc);
        }
        for (size_t i = 0; i < cbInfo.argc; i++) {
            panda::Local<panda::JSValueRef> value = runtimeInfo->GetCallArgRef(i);
            if (value->IsFunction()) {
                FunctionSetContainerId(vm, value);
            }
            cbInfo.argv[i] = JsValueFromLocalValue(value);
        }
    }

    if (JSNApi::IsMixedDebugEnabled(vm)) {
        JSNApi::NotifyNativeCalling(vm, reinterpret_cast<void *>(cb));
    }

    napi_value result = nullptr;
    if (cb != nullptr) {
        result = cb(info->env, &cbInfo);
    }

    if (JSNApi::IsMixedDebugEnabled(vm)) {
        JSNApi::NotifyNativeReturnJS(vm);
    }

    if (cbInfo.argv != nullptr) {
        if (cbInfo.argc > MAX_CHUNK_ARRAY_SIZE) {
            delete[] cbInfo.argv;
        } else {
            engine->GetNativeChunk().Delete(cbInfo.argv);
        }
        cbInfo.argv = nullptr;
    }

    Local<panda::JSValueRef> localRet = panda::JSValueRef::Undefined(vm);
    if (result != nullptr) {
        localRet = LocalValueFromJsValue(result);
    }

    FinishNapiProfilerTrace();
    if (localRet.IsEmpty()) {
        return JSValueRef::Undefined(vm);
    }
    return scope.Escape(localRet);
}

Local<panda::JSValueRef> NapiNativeCreateFunction(napi_env env, const char* name, NapiNativeCallback cb, void* value)
{
    auto engine = reinterpret_cast<NativeEngine*>(env);
    auto vm = const_cast<EcmaVM*>(engine->GetEcmaVm());
    NapiFunctionInfo* funcInfo = NapiFunctionInfo::CreateNewInstance();
    if (funcInfo == nullptr) {
        HILOG_ERROR("funcInfo is nullptr");
        return JSValueRef::Undefined(vm);
    }
    funcInfo->env = env;
    funcInfo->callback = cb;
    funcInfo->data = value;
#ifdef ENABLE_CONTAINER_SCOPE
    funcInfo->scopeId = OHOS::Ace::ContainerScope::CurrentId();
#endif

    Local<panda::FunctionRef> fn = panda::FunctionRef::New(vm, NativeFunctionCallBack,
                                                           [](void* externalPointer, void* data) {
                                                                auto info = reinterpret_cast<NapiFunctionInfo*>(data);
                                                                if (info != nullptr) {
                                                                    delete info;
                                                                }
                                                           },
                                                           reinterpret_cast<void*>(funcInfo), true);
    Local<panda::StringRef> fnName = panda::StringRef::NewFromUtf8(vm, name);
    fn->SetName(vm, fnName);
    return fn;
}

Local<JSValueRef> GetProperty(EcmaVM* vm, Local<panda::ObjectRef> &obj, const char* name)
{
    Local<StringRef> key = StringRef::NewFromUtf8(vm, name);
    Local<JSValueRef> val = obj->Get(vm, key);
    return val;
}

void GetCString(EcmaVM* vm, Local<StringRef> str, char* buffer, size_t size, size_t* length)
{
    if (length == nullptr) {
        return;
    }
    if (buffer == nullptr) {
        *length = str->Utf8Length(vm) - 1;
    } else if (size != 0) {
        int copied = str->WriteUtf8(buffer, size - 1, true) - 1;
        buffer[copied] = '\0';
        *length = copied;
    } else {
        *length = 0;
    }
}

std::string NapiGetModuleName(napi_env env, Local<panda::ObjectRef> &obj)
{
    auto engine = reinterpret_cast<NativeEngine*>(env);
    auto vm = const_cast<EcmaVM*>(engine->GetEcmaVm());
    std::string moduleName("");
    auto nativeModuleName = GetProperty(vm, obj, PANDA_MODULE_NAME);
    if (nativeModuleName->IsString()) {
        char arrayName[PANDA_MODULE_NAME_LEN] = {0};
        size_t len = 0;
        GetCString(vm, nativeModuleName, arrayName, PANDA_MODULE_NAME_LEN, &len);
        moduleName += arrayName;
        moduleName += ".";
    }
    return moduleName;
}

bool NapiDefineProperty(napi_env env, Local<panda::ObjectRef> &obj, NapiPropertyDescriptor propertyDescriptor)
{
    auto engine = reinterpret_cast<NativeEngine*>(env);
    auto vm = engine->GetEcmaVm();
    bool result = false;
    Local<panda::StringRef> propertyName = panda::StringRef::NewFromUtf8(vm, propertyDescriptor.utf8name);

    bool writable = (propertyDescriptor.attributes & NATIVE_WRITABLE) != 0;
    bool enumable = (propertyDescriptor.attributes & NATIVE_ENUMERABLE) != 0;
    bool configable = (propertyDescriptor.attributes & NATIVE_CONFIGURABLE) != 0;

    std::string fullName("");
#ifdef ENABLE_HITRACE
    fullName += NapiGetModuleName(env, obj);
#endif
    if (propertyDescriptor.getter != nullptr || propertyDescriptor.setter != nullptr) {
        Local<panda::JSValueRef> localGetter = panda::JSValueRef::Undefined(vm);
        Local<panda::JSValueRef> localSetter = panda::JSValueRef::Undefined(vm);

        if (propertyDescriptor.getter != nullptr) {
            fullName += "getter";
            localGetter = NapiNativeCreateFunction(env, fullName.c_str(),
                                                   propertyDescriptor.getter, propertyDescriptor.data);
        }
        if (propertyDescriptor.setter != nullptr) {
            fullName += "setter";
            localSetter = NapiNativeCreateFunction(env, fullName.c_str(),
                                                   propertyDescriptor.setter, propertyDescriptor.data);
        }

        PropertyAttribute attr(panda::JSValueRef::Undefined(vm), false, enumable, configable);
        result = obj->SetAccessorProperty(vm, propertyName, localGetter, localSetter, attr);
    } else if (propertyDescriptor.method != nullptr) {
        fullName += propertyDescriptor.utf8name;
        Local<panda::JSValueRef> cbObj = NapiNativeCreateFunction(env, fullName.c_str(),
                                                                  propertyDescriptor.method, propertyDescriptor.data);
        PropertyAttribute attr(cbObj, writable, enumable, configable);
        result = obj->DefineProperty(vm, propertyName, attr);
    } else {
        Local<panda::JSValueRef> val = LocalValueFromJsValue(propertyDescriptor.value);

        PropertyAttribute attr(val, writable, enumable, configable);
        result = obj->DefineProperty(vm, propertyName, attr);
    }
    Local<panda::ObjectRef> excep = panda::JSNApi::GetUncaughtException(vm);
    if (!excep.IsNull()) {
        HILOG_ERROR("DefineProperty occur Exception");
        panda::JSNApi::GetAndClearUncaughtException(vm);
    }
    return result;
}

panda::Local<panda::ObjectRef> NativeEngine::GetModuleFromName(
    const std::string& moduleName, bool isAppModule, const std::string& id, const std::string& param,
    const std::string& instanceName, void** instance)
{
    panda::EscapeLocalScope scope(vm_);
    Local<ObjectRef> exports(JSValueRef::Undefined(vm_));
    NativeModuleManager* moduleManager = NativeModuleManager::GetInstance();
    NativeModule* module = moduleManager->LoadNativeModule(moduleName.c_str(), nullptr, isAppModule);
    if (module != nullptr) {
        Local<StringRef> idStr = StringRef::NewFromUtf8(vm_, id.c_str(), id.size());
        napi_value idValue = JsValueFromLocalValue(idStr);
        Local<StringRef> paramStr = StringRef::NewFromUtf8(vm_, param.c_str(), param.size());
        napi_value paramValue = JsValueFromLocalValue(paramStr);
        Local<ObjectRef> exportObj = ObjectRef::New(vm_);
        NapiPropertyDescriptor idProperty, paramProperty;
        idProperty.utf8name = "id";
        idProperty.value = idValue;
        paramProperty.utf8name = "param";
        paramProperty.value = paramValue;
        SetModuleName(exportObj, module->name);
        NapiDefineProperty(reinterpret_cast<napi_env>(this), exportObj, idProperty);
        NapiDefineProperty(reinterpret_cast<napi_env>(this), exportObj, paramProperty);
        MoudleNameLocker nameLocker(module->name);
        module->registerCallback(reinterpret_cast<napi_env>(this), JsValueFromLocalValue(exportObj));
        napi_value nExport = JsValueFromLocalValue(exportObj);
        napi_value exportInstance = nullptr;
        napi_status status = napi_get_named_property(
            reinterpret_cast<napi_env>(this), nExport, instanceName.c_str(), &exportInstance);
        if (status != napi_ok) {
            HILOG_ERROR("GetModuleFromName napi_get_named_property status != napi_ok");
        }

        status = napi_unwrap(reinterpret_cast<napi_env>(this), exportInstance, reinterpret_cast<void**>(instance));
        if (status != napi_ok) {
            HILOG_ERROR("GetModuleFromName napi_unwrap status != napi_ok");
        }
        exports = exportObj;
    }
    return scope.Escape(exports);
}

napi_value NativeEngine::CreatePromise(NativeDeferred** deferred)
{
    panda::EscapeLocalScope scope(vm_);
    Local<PromiseCapabilityRef> capability = PromiseCapabilityRef::New(vm_);
    *deferred = new NativeDeferred(this, capability);
    Local<JSValueRef> promiseValue = capability->GetPromise(vm_);
    return JsValueFromLocalValue(scope.Escape(promiseValue));
}

panda::Local<panda::ObjectRef> NativeEngine::LoadModuleByName(const std::string& moduleName, bool isAppModule,
    const std::string& param, const std::string& instanceName, void* instance, const std::string& path)
{
    panda::EscapeLocalScope scope(vm_);
    Local<ObjectRef> exports(JSValueRef::Undefined(vm_));
    NativeModuleManager* moduleManager = NativeModuleManager::GetInstance();
    NativeModule* module =
        moduleManager->LoadNativeModule(moduleName.c_str(), path.empty() ? nullptr : path.c_str(), isAppModule);
    if (module != nullptr) {
        Local<ObjectRef> exportObj = ObjectRef::New(vm_);
        NapiPropertyDescriptor paramProperty, instanceProperty;
        Local<StringRef> paramStr = StringRef::NewFromUtf8(vm_, param.c_str(), param.size());
        napi_value paramValue = JsValueFromLocalValue(paramStr);
        paramProperty.utf8name = "param";
        paramProperty.value = paramValue;
        Local<ObjectRef> instanceValue = ObjectRef::New(vm_);
        Local<StringRef> key = StringRef::GetNapiWrapperString(vm_);
        if (instance == nullptr && instanceValue->Has(vm_, key)) {
            Local<ObjectRef> wrapper = instanceValue->Get(vm_, key);
            auto ref = reinterpret_cast<NativeReference*>(wrapper->GetNativePointerField(0));
            wrapper->SetNativePointerField(0, nullptr, nullptr, nullptr, 0);
            instanceValue->Delete(vm_, key);
            delete ref;
        } else {
            Local<ObjectRef> object = ObjectRef::New(vm_);
            NativeReference* ref = nullptr;
            Local<JSValueRef> value(instanceValue);
            ref = new NativeReference(this, value, 0, true, nullptr, instance, nullptr);

            object->SetNativePointerFieldCount(1);
            object->SetNativePointerField(0, ref, nullptr, nullptr, 0);
            PropertyAttribute attr(object, true, false, true);
            instanceValue->DefineProperty(vm_, key, attr);
        }
        instanceProperty.utf8name = instanceName.c_str();
        instanceProperty.value = JsValueFromLocalValue(instanceValue);
        SetModuleName(exportObj, module->name);
        NapiDefineProperty(reinterpret_cast<napi_env>(this), exportObj, paramProperty);
        NapiDefineProperty(reinterpret_cast<napi_env>(this), exportObj, instanceProperty);

        MoudleNameLocker nameLocker(module->name);
        module->registerCallback(reinterpret_cast<napi_env>(this), JsValueFromLocalValue(exportObj));
        exports = exportObj;
    }
    return scope.Escape(exports);
}

inline void NativeEngine::SetModuleName(Local<ObjectRef> &nativeObj, std::string moduleName)
{
#ifdef ENABLE_HITRACE
    if (NativeEngine::napiProfilerEnabled) {
        Local<StringRef> moduleNameStr = StringRef::NewFromUtf8(vm_, moduleName.c_str(), moduleName.size());
        Local<StringRef> key = StringRef::NewFromUtf8(vm_, PANDA_MODULE_NAME);
        nativeObj->Set(vm_, key, moduleNameStr);
    }
#endif
}

static void ConcurrentCallbackFunc(Local<JSValueRef> result, bool success, void *taskInfo, void *data)
{
    if (data == nullptr) {
        return;
    }
    auto engine = static_cast<NativeEngine *>(data);
    auto concurrentCallbackFunc = engine->GetConcurrentCallbackFunc();
    if (concurrentCallbackFunc == nullptr) {
        return;
    }
    napi_env env = reinterpret_cast<napi_env>(engine);
    concurrentCallbackFunc(env, NativeEngine::ArkValueToNapiValue(env, result), success, taskInfo);
}

bool NativeEngine::InitTaskPoolThread(NativeEngine* engine, NapiConcurrentCallback callback)
{
    concurrentCallbackFunc_ = callback;
    return JSNApi::InitForConcurrentThread(vm_, ConcurrentCallbackFunc, static_cast<void *>(this));
}

bool NativeEngine::InitTaskPoolThread(napi_env env, NapiConcurrentCallback callback)
{
    concurrentCallbackFunc_ = callback;
    return JSNApi::InitForConcurrentThread(vm_, ConcurrentCallbackFunc, static_cast<void *>(this));
}

bool NativeEngine::InitTaskPoolFunc(napi_env env, napi_value func, void* taskInfo)
{
    LocalScope scope(vm_);
    Local<JSValueRef> function = LocalValueFromJsValue(func);
    return JSNApi::InitForConcurrentFunction(vm_, function, taskInfo);
}

bool NativeEngine::HasPendingJob()
{
    return JSNApi::HasPendingJob(vm_);
}

bool NativeEngine::IsProfiling()
{
    return JSNApi::IsProfiling(vm_);
}

bool NativeEngine::IsExecutingPendingJob() const
{
    return panda::JSNApi::IsExecutingPendingJob(vm_);
}

void NativeEngine::TerminateExecution() const
{
    DFXJSNApi::TerminateExecution(vm_);
}

void* NativeEngine::GetCurrentTaskInfo() const
{
    return JSNApi::GetCurrentTaskInfo(vm_);
}

napi_value NativeEngine::CallFunction(
    napi_value thisVar, napi_value function, napi_value const* argv, size_t argc)
{
    if (function == nullptr) {
        return nullptr;
    }
    panda::EscapeLocalScope scope(vm_);
    Local<JSValueRef> thisObj = JSValueRef::Undefined(vm_);
    if (thisVar != nullptr) {
        thisObj = LocalValueFromJsValue(thisVar);
    }
    Local<FunctionRef> funcObj = LocalValueFromJsValue(function);
    std::vector<Local<JSValueRef>> args;
    args.reserve(argc);
    for (size_t i = 0; i < argc; i++) {
        if (argv[i] != nullptr) {
            args.emplace_back(LocalValueFromJsValue(argv[i]));
        } else {
            args.emplace_back(JSValueRef::Undefined(vm_));
        }
    }

    Local<JSValueRef> value = funcObj->Call(vm_, thisObj, args.data(), argc);
    if (panda::JSNApi::HasPendingException(vm_)) {
        HILOG_ERROR("pending exception when js function called");
        HILOG_ERROR("print exception info: ");
        panda::JSNApi::PrintExceptionInfo(vm_);
        return nullptr;
    }

    return JsValueFromLocalValue(scope.Escape(value));
}

bool NativeEngine::RunScriptPath(const char* path)
{
    panda::JSExecutionScope executionScope(vm_);
    LocalScope scope(vm_);
    [[maybe_unused]] bool ret = panda::JSNApi::Execute(vm_, path, PANDA_MAIN_FUNCTION);
    if (panda::JSNApi::HasPendingException(vm_)) {
        HandleUncaughtException();
        return false;
    }
    return true;
}

/*
 * Before: input: 1. @ohos.hilog
                  2. @system.app (NATIVE_MODULE contains this name)
 * After: return: 1.@ohos:hilog
 *                2.@native:system.app 
 */
std::string NativeEngine::GetOhmurl(const char* str)
{
    std::string path(str);
    const std::regex reg("@(ohos|system)\\.(\\S+)");
    path.erase(0, path.find_first_not_of(" "));
    path.erase(path.find_last_not_of(" ") + 1);
    bool ret = std::regex_match(path, reg);
    if (!ret) {
        HILOG_ERROR("NativeEngine:The module name doesn't comply with the naming rules");
        return "";
    }
    std::string systemModule = path.substr(1);
    if (NATIVE_MODULE.count(systemModule)) {
        return NATIVE_MODULE_PREFIX + systemModule;
    } else {
        int pos = path.find('.');
        std::string systemKey = path.substr(pos + 1, systemModule.size());
        return OHOS_MODULE_PREFIX + systemKey;
    }
}

napi_value NativeEngine::NapiLoadModule(const char* str)
{
    if (str == nullptr) {
        HILOG_ERROR("NativeEngine:The module name is empty");
        return nullptr;
    }
    std::string key = GetOhmurl(str);
    if (key.size() == 0) {
        return nullptr;
    }
    panda::EscapeLocalScope scope(vm_);
    Local<ObjectRef> exportObj = panda::JSNApi::ExecuteNativeModule(vm_, key);
    if (exportObj->IsNull()) {
        HILOG_ERROR("NativeEngine:napi load module failed");
        return nullptr;
    }
    return JsValueFromLocalValue(scope.Escape(exportObj));
}

bool NativeEngine::SuspendVMById(uint32_t tid)
{
#if !defined(PREVIEW) && !defined(IOS_PLATFORM)
    return DFXJSNApi::SuspendVMById(vm_, tid);
#else
    HILOG_WARN("ARK does not support dfx on windows");
    return false;
#endif
}

void NativeEngine::ResumeVMById(uint32_t tid)
{
#if !defined(PREVIEW) && !defined(IOS_PLATFORM)
    DFXJSNApi::ResumeVMById(vm_, tid);
#else
    HILOG_WARN("ARK does not support dfx on windows");
    return;
#endif
}

// The security interface needs to be modified accordingly.
napi_value NativeEngine::RunScriptBuffer(const char* path, std::vector<uint8_t>& buffer, bool isBundle)
{
    panda::EscapeLocalScope scope(vm_);
    [[maybe_unused]] bool ret = false;
    if (isBundle) {
        ret = panda::JSNApi::Execute(vm_, buffer.data(), buffer.size(), PANDA_MAIN_FUNCTION, path);
    } else {
        ret = panda::JSNApi::ExecuteModuleBuffer(vm_, buffer.data(), buffer.size(), path);
    }

    if (panda::JSNApi::HasPendingException(vm_)) {
        HandleUncaughtException();
        return nullptr;
    }
    Local<JSValueRef> undefObj = JSValueRef::Undefined(vm_);
    return JsValueFromLocalValue(scope.Escape(undefObj));
}

bool NativeEngine::RunScriptBuffer(const std::string& path, uint8_t* buffer, size_t size, bool isBundle)
{
    panda::JSExecutionScope executionScope(vm_);
    LocalScope scope(vm_);
    bool ret = false;
    if (isBundle) {
        ret = panda::JSNApi::ExecuteSecure(vm_, buffer, size, PANDA_MAIN_FUNCTION, path);
    } else {
        ret = panda::JSNApi::ExecuteModuleBufferSecure(vm_, buffer, size, path);
    }

    if (panda::JSNApi::HasPendingException(vm_)) {
        HandleUncaughtException();
        return false;
    }
    return ret;
}

void NativeEngine::SetPackagePath(const std::string appLibPathKey, const std::vector<std::string>& packagePath)
{
    auto moduleManager = NativeModuleManager::GetInstance();
    if (moduleManager && !packagePath.empty()) {
        moduleManager->SetAppLibPath(appLibPathKey, packagePath);
    }
}

napi_value NativeEngine::CreateInstance(napi_value constructor, napi_value const *argv, size_t argc)
{
    if (constructor == nullptr) {
        return nullptr;
    }
    panda::EscapeLocalScope scope(vm_);
    Local<FunctionRef> value = LocalValueFromJsValue(constructor);
    std::vector<Local<JSValueRef>> args;
    args.reserve(argc);
    for (size_t i = 0; i < argc; i++) {
        if (argv[i] != nullptr) {
            args.emplace_back(LocalValueFromJsValue(argv[i]));
        } else {
            args.emplace_back(JSValueRef::Undefined(vm_));
        }
    }
    Local<JSValueRef> instance = value->Constructor(vm_, args.data(), argc);
    Local<ObjectRef> excep = panda::JSNApi::GetUncaughtException(vm_);
    if (!excep.IsNull()) {
        HILOG_ERROR("CreateInstance occur Exception");
        return nullptr;
    }
    return JsValueFromLocalValue(scope.Escape(instance));
}

NativeReference* NativeEngine::CreateReference(napi_value value, uint32_t initialRefcount,
    bool flag, NapiNativeFinalize callback, void* data, void* hint)
{
    return new NativeReference(this, value, initialRefcount, flag, callback, data, hint);
}

bool NativeEngine::IsExceptionPending() const
{
    return panda::JSNApi::HasPendingException(vm_);
}

NativeEngine* NativeEngine::CreateRuntimeFunc(NativeEngine* engine, void* jsEngine, bool isLimitedWorker)
{
    panda::RuntimeOption option;
#if defined(OHOS_PLATFORM) && !defined(IOS_PLATFORM) && !defined(ANDROID_PLATFORM)
    int arkProperties = OHOS::system::GetIntParameter<int>("persist.ark.properties", -1);
    std::string bundleName = OHOS::system::GetParameter("persist.ark.arkbundlename", "");
    size_t gcThreadNum = OHOS::system::GetUintParameter<size_t>("persist.ark.gcthreads", 7);
    size_t longPauseTime = OHOS::system::GetUintParameter<size_t>("persist.ark.longpausetime", 40);
    bool asmInterpreterEnabled = OHOS::system::GetBoolParameter("persist.ark.asminterpreter", true);
    std::string asmOpcodeDisableRange = OHOS::system::GetParameter("persist.ark.asmopcodedisablerange", "");
    bool builtinsLazyEnabled = OHOS::system::GetBoolParameter("persist.ark.enablebuiltinslazy", true);
    option.SetEnableBuiltinsLazy(builtinsLazyEnabled);
    option.SetArkProperties(arkProperties);
    option.SetArkBundleName(bundleName);
    option.SetGcThreadNum(gcThreadNum);
    option.SetLongPauseTime(longPauseTime);
    option.SetEnableAsmInterpreter(asmInterpreterEnabled);
    option.SetAsmOpcodeDisableRange(asmOpcodeDisableRange);
    option.SetIsWorker();
    HILOG_INFO("CreateRuntimeFunc ark properties = %{public}d, bundlename = %{public}s",
        arkProperties, bundleName.c_str());
#endif
    option.SetGcType(panda::RuntimeOption::GC_TYPE::GEN_GC);
    const int64_t poolSize = 0x1000000;
    option.SetGcPoolSize(poolSize);
#if !defined(PREVIEW) && !defined(IOS_PLATFORM)
    option.SetLogLevel(panda::RuntimeOption::LOG_LEVEL::INFO);
#endif
    option.SetDebuggerLibraryPath("");
    EcmaVM* vm = panda::JSNApi::CreateJSVM(option);
    if (vm == nullptr) {
        return nullptr;
    }
    // worker adaptation mergeabc
    const panda::ecmascript::EcmaVM* hostVM = reinterpret_cast<NativeEngine*>(engine)->GetEcmaVm();
    panda::JSNApi::SynchronizVMInfo(vm, hostVM);
    NativeEngine* arkEngine = new NativeEngine(vm, jsEngine, isLimitedWorker);
    // init callback
    arkEngine->RegisterWorkerFunction(engine);
    arkEngine->SetHostEngine(engine);

    auto cleanEnv = [vm]() {
        if (vm != nullptr) {
            HILOG_INFO("cleanEnv is called");
            panda::JSNApi::DestroyJSVM(vm);
        }
    };
    arkEngine->SetCleanEnv(cleanEnv);

    if (hostVM != nullptr) {
        panda::JSNApi::addWorker(const_cast<EcmaVM*>(hostVM), vm);
    }
    return arkEngine;
}

void* NativeEngine::CreateRuntime(bool isLimitedWorker)
{
    return NativeEngine::CreateRuntimeFunc(this, jsEngine_, isLimitedWorker);
}

void NativeEngine::StartCpuProfiler(const std::string& fileName)
{
    JSNApi::SetNativePtrGetter(vm_, reinterpret_cast<void*>(NativeEngine::GetNativePtrCallBack));
    DFXJSNApi::StartCpuProfilerForFile(vm_, fileName);
}

void NativeEngine::StopCpuProfiler()
{
    DFXJSNApi::StopCpuProfilerForFile(vm_);
    JSNApi::SetNativePtrGetter(vm_, nullptr);
}

void NativeEngine::ResumeVM()
{
    DFXJSNApi::ResumeVM(vm_);
}

bool NativeEngine::SuspendVM()
{
    return DFXJSNApi::SuspendVM(vm_);
}

bool NativeEngine::IsSuspended()
{
    return DFXJSNApi::IsSuspended(vm_);
}

bool NativeEngine::CheckSafepoint()
{
    return DFXJSNApi::CheckSafepoint(vm_);
}

napi_value NativeEngine::RunBufferScript(std::vector<uint8_t>& buffer)
{
    panda::EscapeLocalScope scope(vm_);
    [[maybe_unused]] bool ret = panda::JSNApi::Execute(vm_, buffer.data(), buffer.size(), PANDA_MAIN_FUNCTION);

    if (panda::JSNApi::HasPendingException(vm_)) {
        HandleUncaughtException();
        return nullptr;
    }
    Local<JSValueRef> undefObj = JSValueRef::Undefined(vm_);
    return JsValueFromLocalValue(scope.Escape(undefObj));
}

napi_value NativeEngine::RunActor(std::vector<uint8_t>& buffer, const char* descriptor)
{
    panda::EscapeLocalScope scope(vm_);
    std::string desc(descriptor);
    [[maybe_unused]] bool ret = false;
    if (panda::JSNApi::IsBundle(vm_) || !buffer.empty()) {
        ret = panda::JSNApi::Execute(vm_, buffer.data(), buffer.size(), PANDA_MAIN_FUNCTION, desc);
    } else {
        ret = panda::JSNApi::Execute(vm_, desc, PANDA_MAIN_FUNCTION);
    }

    if (panda::JSNApi::HasPendingException(vm_)) {
        HandleUncaughtException();
        return nullptr;
    }
    Local<JSValueRef> undefObj = JSValueRef::Undefined(vm_);
    return JsValueFromLocalValue(scope.Escape(undefObj));
}

panda::Local<panda::ObjectRef> NativeEngine::LoadArkModule(const void* buffer,
    int32_t len, const std::string& fileName)
{
    panda::EscapeLocalScope scope(vm_);
    Local<ObjectRef> undefObj(JSValueRef::Undefined(vm_));
    if (buffer == nullptr || len <= 0 || fileName.empty()) {
        HILOG_ERROR("fileName is nullptr or source code is nullptr");
        return scope.Escape(undefObj);
    }

    bool res = JSNApi::ExecuteModuleFromBuffer(vm_, buffer, len, fileName);
    if (!res) {
        HILOG_ERROR("Execute module failed");
        return scope.Escape(undefObj);
    }

    Local<ObjectRef> exportObj = JSNApi::GetExportObjectFromBuffer(vm_, fileName, "default");
    if (exportObj->IsNull()) {
        HILOG_ERROR("Get export object failed");
        return scope.Escape(undefObj);
    }

    HILOG_DEBUG("NativeEngineImpl::LoadModule end");
    return scope.Escape(exportObj);
}

napi_value NativeEngine::ValueToNapiValue(JSValueWrapper& value)
{
    Global<JSValueRef> arkValue = value;
    return JsValueFromLocalValue(arkValue.ToLocal(vm_));
}

napi_value NativeEngine::ArkValueToNapiValue(napi_env env, Local<JSValueRef> value)
{
    return JsValueFromLocalValue(value);
}

std::string NativeEngine::GetSourceCodeInfo(napi_value value, ErrorPos pos)
{
    if (value == nullptr || pos.first == 0) {
        return "";
    }

    LocalScope scope(vm_);
    Local<panda::FunctionRef> func = LocalValueFromJsValue(value);
    uint32_t line = pos.first;
    uint32_t column = pos.second;
    Local<panda::StringRef> sourceCode = func->GetSourceCode(vm_, line);
    std::string sourceCodeStr = sourceCode->ToString();
    if (sourceCodeStr.empty()) {
        return "";
    }
    std::string sourceCodeInfo = "SourceCode:\n";
    sourceCodeInfo.append(sourceCodeStr).append("\n");
    for (uint32_t k = 0; k < column - 1; k++) {
        sourceCodeInfo.push_back(' ');
    }
    sourceCodeInfo.append("^\n");
    return sourceCodeInfo;
}

bool NativeEngine::ExecuteJsBin(const std::string& fileName)
{
    panda::JSExecutionScope executionScope(vm_);
    LocalScope scope(vm_);
    bool ret = JSNApi::Execute(vm_, fileName, PANDA_MAIN_FUNCTION);
    return ret;
}

bool NativeEngine::TriggerFatalException(napi_value error)
{
    return true;
}

bool NativeEngine::AdjustExternalMemory(int64_t ChangeInBytes, int64_t* AdjustedValue)
{
    return true;
}

void NativeEngine::SetPromiseRejectCallback(NativeReference* rejectCallbackRef, NativeReference* checkCallbackRef)
{
    if (rejectCallbackRef == nullptr || checkCallbackRef == nullptr) {
        HILOG_ERROR("rejectCallbackRef or checkCallbackRef is nullptr");
        return;
    }
    promiseRejectCallbackRef_ = rejectCallbackRef;
    checkCallbackRef_ = checkCallbackRef;
    JSNApi::SetHostPromiseRejectionTracker(vm_, reinterpret_cast<void*>(PromiseRejectCallback),
                                           reinterpret_cast<void*>(this));
}

void NativeEngine::PromiseRejectCallback(void* info)
{
    panda::PromiseRejectInfo* promiseRejectInfo = reinterpret_cast<panda::PromiseRejectInfo*>(info);
    NativeEngine* env = reinterpret_cast<NativeEngine*>(promiseRejectInfo->GetData());

    if (env == nullptr) {
        HILOG_ERROR("engine is nullptr");
        return;
    }

    if (env->promiseRejectCallbackRef_ == nullptr || env->checkCallbackRef_ == nullptr) {
        HILOG_ERROR("promiseRejectCallbackRef or checkCallbackRef is nullptr");
        return;
    }
    panda::ecmascript::EcmaVM* vm = const_cast<EcmaVM*>(env->GetEcmaVm());
    LocalScope scope(vm);
    Local<JSValueRef> promise = promiseRejectInfo->GetPromise();
    Local<JSValueRef> reason = promiseRejectInfo->GetReason();
    panda::PromiseRejectInfo::PROMISE_REJECTION_EVENT operation = promiseRejectInfo->GetOperation();

    Local<JSValueRef> type(IntegerRef::New(vm, static_cast<int32_t>(operation)));

    Local<JSValueRef> args[] = {type, promise, reason};

    napi_value promiseNapiRejectCallback = env->promiseRejectCallbackRef_->Get();
    Local<FunctionRef> promiseRejectCallback = LocalValueFromJsValue(promiseNapiRejectCallback);
    if (!promiseRejectCallback.IsEmpty()) {
        promiseRejectCallback->Call(vm, JSValueRef::Undefined(vm), args, 3); // 3 args size
    }

    if (operation == panda::PromiseRejectInfo::PROMISE_REJECTION_EVENT::REJECT) {
        Local<JSValueRef> checkCallback = LocalValueFromJsValue(env->checkCallbackRef_->Get());
        if (!checkCallback.IsEmpty()) {
            JSNApi::SetHostEnqueueJob(vm, checkCallback);
        }
    }
}

void NativeEngine::DumpHeapSnapshot(const std::string& path, bool isVmMode, DumpFormat dumpFormat)
{
    if (dumpFormat == DumpFormat::JSON) {
        DFXJSNApi::DumpHeapSnapshot(vm_, 0, path, isVmMode);
    }
    if (dumpFormat == DumpFormat::BINARY) {
        DFXJSNApi::DumpHeapSnapshot(vm_, 1, path, isVmMode);
    }
    if (dumpFormat == DumpFormat::OTHER) {
        DFXJSNApi::DumpHeapSnapshot(vm_, 2, path, isVmMode); // 2:enum is 2
    }
}

void NativeEngine::DumpHeapSnapshot(bool isVmMode, DumpFormat dumpFormat, bool isPrivate, bool isFullGC)
{
    if (dumpFormat == DumpFormat::JSON) {
        DFXJSNApi::DumpHeapSnapshot(vm_, 0, isVmMode, isPrivate, false, isFullGC);
    }
    if (dumpFormat == DumpFormat::BINARY) {
        DFXJSNApi::DumpHeapSnapshot(vm_, 1, isVmMode, isPrivate);
    }
    if (dumpFormat == DumpFormat::OTHER) {
        DFXJSNApi::DumpHeapSnapshot(vm_, 2, isVmMode, isPrivate); // 2:enum is 2
    }
}

bool NativeEngine::BuildNativeAndJsStackTrace(std::string& stackTraceStr)
{
#if !defined(PREVIEW) && !defined(IOS_PLATFORM)
    return DFXJSNApi::BuildNativeAndJsStackTrace(vm_, stackTraceStr);
#else
    HILOG_WARN("ARK does not support dfx on windows");
    return false;
#endif
}

bool NativeEngine::BuildJsStackTrace(std::string& stackTraceStr)
{
#if !defined(PREVIEW) && !defined(IOS_PLATFORM)
    return DFXJSNApi::BuildJsStackTrace(vm_, stackTraceStr);
#else
    HILOG_WARN("ARK does not support dfx on windows");
    return false;
#endif
}

bool NativeEngine::BuildJsStackInfoList(uint32_t tid, std::vector<JsFrameInfo>& jsFrames)
{
#if !defined(PREVIEW) && !defined(IOS_PLATFORM)
    std::vector<ArkJsFrameInfo> arkJsFrames;
    bool sign = DFXJSNApi::BuildJsStackInfoList(vm_, tid, arkJsFrames);
    for (auto jf : arkJsFrames) {
        struct JsFrameInfo jsframe;
        jsframe.fileName = jf.fileName;
        jsframe.functionName = jf.functionName;
        jsframe.pos = jf.pos;
        jsframe.nativePointer = jf.nativePointer;
        jsFrames.emplace_back(jsframe);
    }
    return sign;
#else
    HILOG_WARN("ARK does not support dfx on windows");
    return false;
#endif
}

bool NativeEngine::DeleteWorker(NativeEngine* workerEngine)
{
    if (workerEngine != nullptr) {
#if !defined(PREVIEW)
        const panda::ecmascript::EcmaVM* workerVM = reinterpret_cast<NativeEngine*>(workerEngine)->GetEcmaVm();
        if (workerVM != nullptr) {
            return panda::JSNApi::DeleteWorker(vm_, const_cast<EcmaVM*>(workerVM));
        }
#else
        HILOG_WARN("ARK does not support dfx on windows");
#endif
        return false;
        }
    return false;
}

bool NativeEngine::StartHeapTracking(double timeInterval, bool isVmMode)
{
    return DFXJSNApi::StartHeapTracking(vm_, timeInterval, isVmMode);
}

bool NativeEngine::StopHeapTracking(const std::string &filePath)
{
    return DFXJSNApi::StopHeapTracking(vm_, filePath);
}

#if !defined(PREVIEW)
void NativeEngine::PrintStatisticResult()
{
    DFXJSNApi::PrintStatisticResult(vm_);
}

void NativeEngine::StartRuntimeStat()
{
    DFXJSNApi::StartRuntimeStat(vm_);
}

void NativeEngine::StopRuntimeStat()
{
    DFXJSNApi::StopRuntimeStat(vm_);
}

size_t NativeEngine::GetArrayBufferSize()
{
    return DFXJSNApi::GetArrayBufferSize(vm_);
}

size_t NativeEngine::GetHeapTotalSize()
{
    return DFXJSNApi::GetHeapTotalSize(vm_);
}

size_t NativeEngine::GetHeapUsedSize()
{
    return DFXJSNApi::GetHeapUsedSize(vm_);
}

size_t NativeEngine::GetHeapLimitSize()
{
    return DFXJSNApi::GetHeapLimitSize(vm_);
}

void NativeEngine::NotifyApplicationState(bool inBackground)
{
    DFXJSNApi::NotifyApplicationState(vm_, inBackground);
}

void NativeEngine::NotifyIdleStatusControl(std::function<void(bool)> callback)
{
    DFXJSNApi::NotifyIdleStatusControl(vm_, callback);
}

void NativeEngine::NotifyIdleTime(int idleMicroSec)
{
    DFXJSNApi::NotifyIdleTime(vm_, idleMicroSec);
}

void NativeEngine::NotifyMemoryPressure(bool inHighMemoryPressure)
{
    DFXJSNApi::NotifyMemoryPressure(vm_, inHighMemoryPressure);
}

void NativeEngine::NotifyForceExpandState(int32_t value)
{
    switch (ForceExpandState(value)) {
        case ForceExpandState::FINISH_COLD_START:
            DFXJSNApi::NotifyFinishColdStart(vm_, true);
            break;
        case ForceExpandState::START_HIGH_SENSITIVE:
            DFXJSNApi::NotifyHighSensitive(vm_, true);
            break;
        case ForceExpandState::FINISH_HIGH_SENSITIVE:
            DFXJSNApi::NotifyHighSensitive(vm_, false);
            break;
        default:
            HILOG_ERROR("Invalid Force Expand State: %{public}d.", value);
            break;
    }
}
#else
void NativeEngine::PrintStatisticResult()
{
    HILOG_WARN("ARK does not support dfx on windows");
}

void NativeEngine::StartRuntimeStat()
{
    HILOG_WARN("ARK does not support dfx on windows");
}

void NativeEngine::StopRuntimeStat()
{
    HILOG_WARN("ARK does not support dfx on windows");
}

size_t NativeEngine::GetArrayBufferSize()
{
    HILOG_WARN("ARK does not support dfx on windows");
    return 0;
}

size_t NativeEngine::GetHeapTotalSize()
{
    HILOG_WARN("ARK does not support dfx on windows");
    return 0;
}

size_t NativeEngine::GetHeapUsedSize()
{
    HILOG_WARN("ARK does not support dfx on windows");
    return 0;
}

size_t NativeEngine::GetHeapLimitSize()
{
    HILOG_WARN("ARK does not support dfx on windows");
    return 0;
}

void NativeEngine::NotifyApplicationState([[maybe_unused]] bool inBackground)
{
    HILOG_WARN("ARK does not support dfx on windows");
}

void NativeEngine::NotifyIdleStatusControl([[maybe_unused]] std::function<void(bool)> callback)
{
    HILOG_WARN("ARK does not support dfx on windows");
}

void NativeEngine::NotifyIdleTime([[maybe_unused]] int idleMicroSec)
{
    HILOG_WARN("ARK does not support dfx on windows");
}

void NativeEngine::NotifyMemoryPressure([[maybe_unused]] bool inHighMemoryPressure)
{
    HILOG_WARN("ARK does not support dfx on windows");
}

void NativeEngine::NotifyForceExpandState([[maybe_unused]] int32_t value)
{
    HILOG_WARN("ARK does not support dfx on windows");
}
#endif

void NativeEngine::SetMockModuleList(const std::map<std::string, std::string> &list)
{
    JSNApi::SetMockModuleList(vm_, list);
}

void NativeEngine::RegisterNapiUncaughtExceptionHandler(NapiUncaughtExceptionCallback callback)
{
    JSNApi::EnableUserUncaughtErrorHandler(vm_);
    napiUncaughtExceptionCallback_ = callback;
}

void NativeEngine::HandleUncaughtException()
{
    if (napiUncaughtExceptionCallback_ == nullptr) {
        return;
    }
    LocalScope scope(vm_);
    Local<ObjectRef> exception = JSNApi::GetAndClearUncaughtException(vm_);
    if (!exception.IsEmpty() && !exception->IsHole()) {
        if (napiUncaughtExceptionCallback_ != nullptr) {
            napiUncaughtExceptionCallback_(ArkValueToNapiValue(reinterpret_cast<napi_env>(this), exception));
        }
    }
}

bool NativeEngine::HasPendingException()
{
    return panda::JSNApi::HasPendingException(vm_);
}

void NativeEngine::RegisterPermissionCheck(PermissionCheckCallback callback)
{
    if (permissionCheckCallback_ == nullptr) {
        permissionCheckCallback_ = callback;
    }
}

bool NativeEngine::ExecutePermissionCheck()
{
    if (permissionCheckCallback_ != nullptr) {
        return permissionCheckCallback_();
    } else {
        HILOG_INFO("permissionCheckCallback_ is still nullptr when executing permission check!");
        return true;
    }
}

void NativeEngine::RegisterTranslateBySourceMap(SourceMapCallback callback)
{
    if (SourceMapCallback_ == nullptr) {
        SourceMapCallback_ = callback;
    }
}

void NativeEngine::RegisterSourceMapTranslateCallback(SourceMapTranslateCallback callback)
{
    panda::JSNApi::SetSourceMapTranslateCallback(vm_, callback);
}

std::string NativeEngine::ExecuteTranslateBySourceMap(const std::string& rawStack)
{
    if (SourceMapCallback_ != nullptr) {
        return SourceMapCallback_(rawStack);
    } else {
        HILOG_WARN("SourceMapCallback_ is nullptr.");
        return "";
    }
}

bool NativeEngine::IsMixedDebugEnabled()
{
    return JSNApi::IsMixedDebugEnabled(vm_);
}

void NativeEngine::NotifyNativeCalling(const void *nativeAddress)
{
    JSNApi::NotifyNativeCalling(vm_, nativeAddress);
}

void NativeEngine::AllowCrossThreadExecution() const
{
    JSNApi::AllowCrossThreadExecution(vm_);
}

#if !defined(PREVIEW) && !defined(IOS_PLATFORM) && !defined(ANDROID_PLATFORM)
uint64_t NativeEngine::GetCurrentTickMillseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

void NativeEngine::JudgmentDump(size_t limitSize)
{
    if (!limitSize) {
        return;
    }
    size_t nowPrecent = GetHeapUsedSize() * DEC_TO_INT / limitSize;
    if (g_debugLeak || (nowPrecent >= g_threshold && (g_lastHeapDumpTime == 0 ||
        GetCurrentTickMillseconds() - g_lastHeapDumpTime > HEAP_DUMP_REPORT_INTERVAL))) {
        HILOG_INFO("dumpheapSnapshot ready");
        int pid = -1;
        DFXJSNApi::GetHeapPrepare(vm_);
        time_t startTime = time(nullptr);
        g_lastHeapDumpTime = GetCurrentTickMillseconds();
        if((pid = fork()) < 0) {
            HILOG_ERROR("ready dumpheapSnapshot Fork error, err:%{public}d", errno);
            sleep(g_checkInterval);
            return;
        }
        if (pid == 0) {
            AllowCrossThreadExecution();
            DumpHeapSnapshot(true, DumpFormat::JSON, false, false);
            HILOG_INFO("dumpheapSnapshot successful, now you can check some file");
            _exit(0);
        }
        while (true) {
            int status = 0;
            pid_t p = waitpid(pid, &status, 0);
            if (p < 0) {
                HILOG_ERROR("dumpheapSnapshot Waitpid return p=%{public}d, err:%{public}d", p, errno);
                break;
            }
            if (p == pid) {
                HILOG_ERROR("dumpheapSnapshot dump process exited status is %{public}d", status);
                break;
            }
            if (time(nullptr) > startTime + TIME_OUT) {
                HILOG_ERROR("time out to wait child process, killing forkpid %{public}d", pid);
                kill(pid, SIGKILL);
                break;
            }
            usleep(DEFAULT_SLEEP_TIME);
        }
    }
}

void NativeEngine::JsHeapStart()
{
    if (pthread_setname_np(pthread_self(), "JsHeapThread") != 0) {
        HILOG_ERROR("Failed to set threadName for JsHeap, errno:%d", errno);
    }
    while (!needStop_) {
        std::unique_lock<std::mutex> lock(lock_);
        condition_.wait_for(lock, std::chrono::milliseconds(g_checkInterval * SEC_TO_MILSEC));
        if (needStop_) {
            return;
        }
        size_t limitSize = GetHeapLimitSize();
        JudgmentDump(limitSize);
    }
}
#endif
