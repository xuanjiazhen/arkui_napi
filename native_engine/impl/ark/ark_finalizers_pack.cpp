/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ark_finalizers_pack.h"

#include "ark_native_engine.h"
#include "utils/log.h"
#include "utils/trace.h"

using AsyncNativeCallbacksPack = panda::AsyncNativeCallbacksPack;
using TriggerGCData = panda::TriggerGCData;

void ArkFinalizersPack::PostFinalizeTasks(ArkNativeEngine* engine)
{
    if (engine->IsInDestructor()) {
        return;
    }
    auto pendingAsyncFinalizers = engine->GetPendingAsyncFinalizers();
    if (!pendingAsyncFinalizers.empty()) {
        uv_work_t* asynWork = new uv_work_t;
        std::vector<RefAsyncFinalizer>* asyncFinalizers = new std::vector<RefAsyncFinalizer>();
        asyncFinalizers->swap(pendingAsyncFinalizers);
        asynWork->data = reinterpret_cast<void*>(asyncFinalizers);

        int ret = uv_queue_work_with_qos(
            engine->GetUVLoop(), asynWork,
            [](uv_work_t* asynWork) {
                std::vector<RefAsyncFinalizer>* finalizers =
                    reinterpret_cast<std::vector<RefAsyncFinalizer>*>(asynWork->data);
                RunAsyncCallbacks(finalizers);
                HILOG_DEBUG("uv_queue_work async running ");
                delete finalizers;
            },
            [](uv_work_t* asynWork, int32_t) { delete asynWork; }, uv_qos_t(napi_qos_background));
        if (ret != 0) {
            HILOG_ERROR("uv_queue_work fail ret '%{public}d'", ret);
            RunAsyncCallbacks(asyncFinalizers);
            delete asynWork;
            delete asyncFinalizers;
        }
    }
    auto arkFinalizersPack = engine->GetArkFinalizersPack();
    if (arkFinalizersPack->Empty()) {
        return;
    }
    ArkFinalizersPack* finalizersPack = new ArkFinalizersPack();
    std::swap(*arkFinalizersPack, *finalizersPack);
    EcmaVM* vm = const_cast<EcmaVM*>(engine->GetEcmaVm());
    if (!engine->IsMainThread()) {
        panda::JsiNativeScope nativeScope(vm);
        RunCallbacks(finalizersPack);
        delete finalizersPack;
        return;
    }
    size_t bindingSize = finalizersPack->GetTotalNativeBindingSize();
    if (engine->GetPendingFinalizersPackNativeBindingSize() > FINALIZERS_PACK_PENDING_NATIVE_BINDING_SIZE_THRESHOLD &&
        bindingSize > 0) {
        HILOG_DEBUG("Pending Finalizers NativeBindingSize '%{public}zu' large than '%{public}zu', process sync.",
                    engine->GetPendingFinalizersPackNativeBindingSize(),
                    FINALIZERS_PACK_PENDING_NATIVE_BINDING_SIZE_THRESHOLD);
        panda::JsiNativeScope nativeScope(vm);
        RunCallbacks(finalizersPack);
        delete finalizersPack;
        return;
    }
    uv_work_t* syncWork = new uv_work_t;
    finalizersPack->RegisterFinishNotify([engine](size_t totalNativeBindingSize) {
        engine->DecreasePendingFinalizersPackNativeBindingSize(totalNativeBindingSize);
    });
    engine->IncreasePendingFinalizersPackNativeBindingSize(bindingSize);

    syncWork->data = reinterpret_cast<void*>(finalizersPack);
    int ret = uv_queue_work_with_qos(
        engine->GetUVLoop(), syncWork, [](uv_work_t*) {},
        [](uv_work_t* syncWork, int32_t) {
            ArkFinalizersPack* finalizersPack = reinterpret_cast<ArkFinalizersPack*>(syncWork->data);
            RunCallbacks(finalizersPack);
            HILOG_DEBUG("uv_queue_work running");
            delete syncWork;
            delete finalizersPack;
        },
        uv_qos_t(napi_qos_background));
    if (ret != 0) {
        HILOG_ERROR("uv_queue_work fail ret '%{public}d'", ret);
        panda::JsiNativeScope nativeScope(vm);
        RunCallbacks(finalizersPack);
        delete syncWork;
        delete finalizersPack;
    }
}

__attribute__((optnone)) void ArkFinalizersPack::RunCallbacks(TriggerGCData* triggerGCData)
{
    ArkHiTrace trace("RunTriggerGCTaskCallback");
    std::pair<void*, uint8_t>& param = *triggerGCData;
    JSNApi::TriggerIdleGC(reinterpret_cast<panda::ecmascript::EcmaVM*>(param.first),
                          static_cast<panda::JSNApi::TRIGGER_IDLE_GC_TYPE>(param.second));
}

__attribute__((optnone)) void ArkFinalizersPack::RunCallbacks(ArkFinalizersPack* finalizersPack)
{
    ArkHiTrace trace("RunFinalizeCallbacks:" + std::to_string(finalizersPack->GetNumFinalizers()));
    finalizersPack->ProcessAll();
}

__attribute__((optnone)) void ArkFinalizersPack::RunAsyncCallbacks(std::vector<RefAsyncFinalizer>* finalizers)
{
    ArkHiTrace trace("RunAsyncFinalizeCallbacks:" + std::to_string(finalizers->size()));
    INIT_CRASH_HOLDER(holder, "NAPI");
    for (auto iter : (*finalizers)) {
        NapiNativeFinalize callback = iter.first;
        std::pair<void*, void*>& param = iter.second;
        holder.UpdateCallbackPtr(reinterpret_cast<uintptr_t>(callback));
        callback(nullptr, std::get<0>(param), std::get<1>(param)); // 1 is the param.
    }
}

__attribute__((optnone)) void ArkFinalizersPack::RunCallbacks(AsyncNativeCallbacksPack* callbacksPack)
{
    ArkHiTrace trace("RunNativeCallbacks:" + std::to_string(callbacksPack->GetNumCallBacks()));
    callbacksPack->ProcessAll("NAPI");
}

void ArkFinalizersPack::PostAsyncTask(ArkNativeEngine* engine, panda::AsyncNativeCallbacksPack* callBacksPack)
{
    if (engine->IsInDestructor()) {
        delete callBacksPack;
        return;
    }
    uv_work_t* syncWork = new uv_work_t;
    syncWork->data = reinterpret_cast<void*>(callBacksPack);

    int ret = uv_queue_work_with_qos(
        engine->GetUVLoop(), syncWork, [](uv_work_t*) {},
        [](uv_work_t* syncWork, int32_t) {
            auto* finalizers = reinterpret_cast<panda::AsyncNativeCallbacksPack*>(syncWork->data);
            RunCallbacks(finalizers);
            delete syncWork;
            delete finalizers;
        },
        uv_qos_t(napi_qos_background));
    if (ret != 0) {
        HILOG_ERROR("uv_queue_work fail ret '%{public}d'", ret);
        panda::JsiNativeScope nativeScope(engine->GetEcmaVm());
        RunCallbacks(callBacksPack);
        delete callBacksPack;
        delete syncWork;
    }
}