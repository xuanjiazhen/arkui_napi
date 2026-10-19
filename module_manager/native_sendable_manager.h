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

#ifndef FOUNDATION_ACE_NAPI_MODULE_MANAGER_NATIVE_SENDABLE_MANAGER_H
#define FOUNDATION_ACE_NAPI_MODULE_MANAGER_NATIVE_SENDABLE_MANAGER_H

#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>

#include "ecmascript/napi/include/jsnapi.h"

struct NativeModule;

enum class SharedRootState : uint8_t {
    NOT_INITIALIZED,
    INITIALIZING,
    INITIALIZED,
    ERRORED,
};

class NativeSendableManager {
public:
    enum class SharedRootResult : uint8_t {
        READY,
        START_INIT,
        FAILED,
    };

    NativeSendableManager() = default;
    ~NativeSendableManager();
    NativeSendableManager(const NativeSendableManager&) = delete;
    NativeSendableManager& operator=(const NativeSendableManager&) = delete;
    NativeSendableManager(NativeSendableManager&&) = delete;
    NativeSendableManager& operator=(NativeSendableManager&&) = delete;

    SharedRootResult GetOrWaitSharedRoot(const panda::EcmaVM* vm, NativeModule* module,
                                         panda::Local<panda::JSValueRef>* outRoot);

    void PublishSharedRoot(NativeModule* module, const panda::EcmaVM* vm,
                           panda::Local<panda::JSValueRef> root);

    void MarkInitFailed(NativeModule* module);

private:
    struct SharedRootEntry {
        std::unique_ptr<panda::SendableGlobal<panda::JSValueRef>> root;
        SharedRootState state = SharedRootState::NOT_INITIALIZED;
        std::mutex entryMutex;
        std::condition_variable entryCondvar;
    };

    SharedRootEntry* FindOrCreateEntry(NativeModule* module);

    std::mutex cacheMutex_;
    std::unordered_map<NativeModule*, std::unique_ptr<SharedRootEntry>> sharedRoots_;
};

#endif /* FOUNDATION_ACE_NAPI_MODULE_MANAGER_NATIVE_SENDABLE_MANAGER_H */
