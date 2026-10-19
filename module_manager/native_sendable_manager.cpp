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

#include "native_sendable_manager.h"

#include "ecmascript/napi/include/jsnapi_expo.h"
#include "utils/log.h"

// Handles are NOT freed here on purpose: releasing a SendableGlobal requires a live VM on
// the owning thread (FreeSendableGlobalHandleAddr runs a cross-thread check), which is not
// guaranteed at teardown. The cache is process-lifetime by design — the shared GC keeps the
// roots alive and the Runtime reclaims the whole storage at process exit.
NativeSendableManager::~NativeSendableManager()
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    sharedRoots_.clear();
}

NativeSendableManager::SharedRootEntry* NativeSendableManager::FindOrCreateEntry(NativeModule* module)
{
    std::lock_guard<std::mutex> lock(cacheMutex_);
    auto result = sharedRoots_.try_emplace(module, std::make_unique<SharedRootEntry>());
    return result.first->second.get();
}

NativeSendableManager::SharedRootResult NativeSendableManager::GetOrWaitSharedRoot(
    const panda::EcmaVM* vm, NativeModule* module, panda::Local<panda::JSValueRef>* outRoot)
{
    SharedRootEntry* entry = FindOrCreateEntry(module);
    std::unique_lock<std::mutex> lock(entry->entryMutex);
    if (entry->state == SharedRootState::INITIALIZING) {
        // Another VM is running the register callback; native module init is short, so
        // blocking beats returning a half-initialized module.
        entry->entryCondvar.wait(lock, [entry] {
            return entry->state != SharedRootState::INITIALIZING;
        });
    }
    switch (entry->state) {
        case SharedRootState::INITIALIZED:
            *outRoot = entry->root->ToLocal(vm);
            return SharedRootResult::READY;
        case SharedRootState::ERRORED:
            return SharedRootResult::FAILED;
        case SharedRootState::NOT_INITIALIZED:
            entry->state = SharedRootState::INITIALIZING;
            return SharedRootResult::START_INIT;
        case SharedRootState::INITIALIZING:
        default:
            // Unreachable: the wait above only returns on INITIALIZED/ERRORED.
            MODULEMNG_HILOG_ERROR("unexpected shared root state");
            return SharedRootResult::FAILED;
    }
}

void NativeSendableManager::PublishSharedRoot(NativeModule* module, const panda::EcmaVM* vm,
                                               panda::Local<panda::JSValueRef> root)
{
    SharedRootEntry* entry = FindOrCreateEntry(module);
    std::lock_guard<std::mutex> lock(entry->entryMutex);
    if (entry->state != SharedRootState::INITIALIZING) {
        MODULEMNG_HILOG_ERROR("publish on a non-initializing entry, ignore");
        return;
    }
    // SendableGlobal stores the root in the process-level storage (shared-GC rooted);
    // the engine validated root->IsSendable(vm) before calling, so the construction
    // cannot hit the non-sendable rejection inside GetSendableGlobalHandleAddr.
    entry->root = std::make_unique<panda::SendableGlobal<panda::JSValueRef>>(vm, root);
    entry->state = SharedRootState::INITIALIZED;
    entry->entryCondvar.notify_all();
}

void NativeSendableManager::MarkInitFailed(NativeModule* module)
{
    SharedRootEntry* entry = FindOrCreateEntry(module);
    std::lock_guard<std::mutex> lock(entry->entryMutex);
    if (entry->state != SharedRootState::INITIALIZING) {
        MODULEMNG_HILOG_ERROR("mark failure on a non-initializing entry, ignore");
        return;
    }
    entry->state = SharedRootState::ERRORED;
    entry->entryCondvar.notify_all();
}
