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

// Init state machine of one shared-sendable module root (process-wide). The register
// callback runs exactly once per process: the first engine to load the module claims
// START_INIT, every other engine blocks until the root is published or the init fails.
enum class SharedRootState : uint8_t {
    NOT_INITIALIZED, // no root yet; the next claimer becomes the initializer
    INITIALIZING,    // one engine is running the native register callback
    INITIALIZED,     // root published in the process-level cache; shared by every VM
    ERRORED,         // init failed once; later loads of this module fail fast
};

// Process-level shared-root cache for modules registered via napi_shared_module_register.
//
// The cached root is held by a SendableGlobal handle, which lives in the process-wide
// EcmaGlobalStorage rooted by the shared GC: the root object stays alive for the whole
// process regardless of which VM created it or which VMs are destroyed. Entries are
// never removed (shared modules are system modules loaded for the process lifetime),
// so entry pointers handed out by FindOrCreateEntry stay stable.
class NativeSendableManager {
public:
    // Outcome of GetOrWaitSharedRoot.
    enum class SharedRootResult : uint8_t {
        READY,      // shared root available; *outRoot is the root as a Local in the CALLER's vm
        START_INIT, // caller won the init race: run the register callback, then PublishSharedRoot
                    // or MarkInitFailed; never leave the entry in INITIALIZING
        FAILED,     // a previous init failed permanently; the module is unusable in this process
    };

    NativeSendableManager() = default;
    ~NativeSendableManager();
    NativeSendableManager(const NativeSendableManager&) = delete;
    NativeSendableManager& operator=(const NativeSendableManager&) = delete;
    NativeSendableManager(NativeSendableManager&&) = delete;
    NativeSendableManager& operator=(NativeSendableManager&&) = delete;

    // Single entry point of the shared loading branch. Fast paths: cached root (READY)
    // or previously failed (FAILED). Contended path: blocks on the entry condvar while
    // another VM is initializing, then returns READY or FAILED. Uncontended path:
    // flips NOT_INITIALIZED to INITIALIZING and returns START_INIT.
    // vm must belong to the calling thread (ToLocal/CROSS_THREAD_CHECK requirement).
    SharedRootResult GetOrWaitSharedRoot(const panda::EcmaVM* vm, NativeModule* module,
                                         panda::Local<panda::JSValueRef>* outRoot);

    // Publishes the sendable root produced by the initializer engine: wraps it into a
    // process-level SendableGlobal and wakes all waiters. Only the engine that received
    // START_INIT may call this, and exactly once.
    void PublishSharedRoot(NativeModule* module, const panda::EcmaVM* vm,
                           panda::Local<panda::JSValueRef> root);

    // Marks the module init as permanently failed and wakes all waiters. Only the engine
    // that received START_INIT may call this (when the callback returns null or a
    // non-sendable value).
    void MarkInitFailed(NativeModule* module);

private:
    // State + root of one module. Address-stable (held by unique_ptr in the map) so
    // waiters can block on it after dropping cacheMutex_.
    struct SharedRootEntry {
        std::unique_ptr<panda::SendableGlobal<panda::JSValueRef>> root;
        SharedRootState state = SharedRootState::NOT_INITIALIZED;
        std::mutex entryMutex;
        std::condition_variable entryCondvar;
    };

    // Returns the entry of `module`, creating a NOT_INITIALIZED one on first touch.
    // Entry pointers stay valid for the manager's lifetime (entries are never erased).
    SharedRootEntry* FindOrCreateEntry(NativeModule* module);

    std::mutex cacheMutex_;
    std::unordered_map<NativeModule*, std::unique_ptr<SharedRootEntry>> sharedRoots_;
};

#endif /* FOUNDATION_ACE_NAPI_MODULE_MANAGER_NATIVE_SENDABLE_MANAGER_H */
