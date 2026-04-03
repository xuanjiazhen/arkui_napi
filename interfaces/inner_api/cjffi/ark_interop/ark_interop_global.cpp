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

#include "ark_interop_internal.h"
#include "ark_interop_napi.h"
#include "ark_interop_log.h"

#include <atomic>
#include <unordered_map>
#include <vector>

using namespace panda;
using namespace panda::ecmascript;

/**
 * @file ark_interop_global.cpp
 * @brief `ARKTS_Global` (persistent VM Global handles) and thread-safe deferred disposal via `GlobalManager` on the Ark thread.
 */

struct ARKTS_Global_ {
    ARKTS_Global_(EcmaVM *vm, const Local<JSValueRef>& value): ref(vm, value), isDisposed(false) {}

    ~ARKTS_Global_()
    {
        if (!isDisposed) {
            ref.FreeGlobalHandleAddr();
        }
    }

    /**
     * @brief Turns the underlying `Global` weak with a callback: after GC collects the object, frees the handle and sets `isDisposed`.
     */
    void SetWeak()
    {
        if (isDisposed) {
            return;
        }
        if (ref.IsWeak()) {
            return;
        }
        // First callback: VM runs this when the weak target dies. Second: optional extra teardown (unused).
        ref.SetWeakCallback(this, [](void* handle) {
            auto global = P_CAST(handle, ARKTS_Global);
            if (!global) {
                return;
            }
            global->isDisposed = true;
            global->ref.FreeGlobalHandleAddr();
        }, [](void*) {});
    }

    /**
     * @brief If weak, promotes back to strong; no-op if empty or not weak.
     */
    void ClearWeak()
    {
        if (isDisposed) {
            return;
        }
        if (ref.IsEmpty() || !ref.IsWeak()) {
            return;
        }
        ref.ClearWeak();
    }

    /**
     * @brief Encodes the referenced JS value as `ARKTS_Value`; returns `Undefined` when disposed.
     */
    ARKTS_Value GetValue() const
    {
        if (isDisposed) {
            return ARKTS_CreateUndefined();
        }
        auto result = BIT_CAST(ref, const Local<JSValueRef>);
        return ARKTS_FromHandle(result);
    }

    /** @brief True if not disposed and the VM handle is non-empty. */
    bool IsAlive() const
    {
        return !isDisposed && !ref.IsEmpty();
    }

    /**
     * @brief Marks the wrapper as invalid (e.g. runtime torn down) so `FreeGlobalHandleAddr` is skipped.
     */
    void SetDisposed()
    {
        isDisposed = true;
    }

private:
    Global<JSValueRef> ref;
    bool isDisposed;
};

namespace {
/** Lightweight spin mutex with Clang thread-safety annotations for `GlobalManager`. */
class __attribute__((capability("mutex"))) SpinLock final {
public:
    void Acquire() __attribute__((acquire_capability()))
    {
        bool locked = false;
        while (!isLocked_.compare_exchange_strong(locked, true)) {
            locked = false;
        }
    }

    /** @brief Releases the spin lock (release semantics via annotation). */
    void Release() __attribute__((release_capability()))
    {
        isLocked_ = false;
    }
private:
    std::atomic<bool> isLocked_ {false};
};

class GlobalManager {
public:
    static void Dispose(EcmaVM* vm, uintptr_t handle);
    static void AsyncDisposer(ARKTS_Env env, int64_t data);
    static void AddManager(ARKTS_Env env);
    static void RemoveManager(ARKTS_Env env);

    explicit GlobalManager(EcmaVM* vm);
    ~GlobalManager();

private:
    static SpinLock managersMutex_;
    static std::unordered_map<EcmaVM*, GlobalManager*> managers_ __attribute__((guarded_by(managersMutex_)));

private:
    EcmaVM* vm_;
    bool onSchedule_;
    SpinLock handleMutex_ {};
    /** Queue of `ARKTS_Global_*` pointers to `delete` on the VM thread (consumed by `AsyncDisposer`). */
    std::vector<uintptr_t> handlesToDispose_ __attribute__((guarded_by(handleMutex_))) {};
};

std::unordered_map<EcmaVM*, GlobalManager*> GlobalManager::managers_;
SpinLock GlobalManager::managersMutex_;

GlobalManager::GlobalManager(EcmaVM* vm)
{
    onSchedule_ = false;
    vm_ = vm;
}

/**
 * @brief Runs on the JS thread: validates `data` matches the VM's manager, then deletes queued global wrappers.
 */
void GlobalManager::AsyncDisposer(ARKTS_Env env, int64_t data)
{
    GlobalManager* current = nullptr;
    managersMutex_.Acquire();
    auto exist = managers_.find(reinterpret_cast<EcmaVM*>(env));
    if (exist != managers_.end()) {
        current = exist->second;
    }
    managersMutex_.Release();
    // Stale task: manager was removed/replaced, or wrong pointer — do not delete another VM's queue.
    if (!current || data != reinterpret_cast<uintptr_t>(current)) {
        return;
    }
    auto manager = reinterpret_cast<GlobalManager*>(data);
    std::vector<uintptr_t> toDispose;
    manager->handleMutex_.Acquire();
    manager->onSchedule_ = false;
    std::swap(toDispose, manager->handlesToDispose_);
    manager->handleMutex_.Release();

    for (auto handle : toDispose) {
        auto global = P_CAST(handle, ARKTS_Global);
        delete global;
    }
}

/**
 * @brief Requests disposal of a global (`handle` is `ARKTS_Global_*`).
 *        If the manager still exists, enqueues and schedules `AsyncDisposer` at most once; if the VM is gone, synchronously `SetDisposed` then `delete`s the wrapper.
 */
void GlobalManager::Dispose(EcmaVM* vm, uintptr_t handle)
{
    GlobalManager* manager = nullptr;
    managersMutex_.Acquire();
    auto itor = managers_.find(vm);
    if (itor != managers_.end()) {
        manager = itor->second;
    }
    managersMutex_.Release();
    if (!manager) {
        // JSRuntime is disposed, still need to delete c++ object.
        auto global = reinterpret_cast<ARKTS_Global_*>(handle);
        // JSRuntime is disposed, can not FreeGlobal by now, set disposed to skip FreeGlobal.
        global->SetDisposed();
        delete global;
        return;
    }
    bool needSchedule = false;
    manager->handleMutex_.Acquire();
    manager->handlesToDispose_.push_back(handle);
    // Coalesce: one AsyncDisposer per burst of Dispose calls; it will drain the whole vector.
    if (!manager->onSchedule_) {
        manager->onSchedule_ = needSchedule = true;
    }
    manager->handleMutex_.Release();
    if (needSchedule) {
        ARKTSInner_CreateAsyncTask(P_CAST(vm, ARKTS_Env), AsyncDisposer, reinterpret_cast<uintptr_t>(manager));
    }
}

/** @brief Registers one `GlobalManager` per VM for `env` (duplicate registration deletes the new instance). */
void GlobalManager::AddManager(ARKTS_Env env)
{
    auto vm = reinterpret_cast<EcmaVM*>(env);
    auto manager = new GlobalManager(vm);
    managersMutex_.Acquire();
    auto result = managers_.try_emplace(vm, manager);
    managersMutex_.Release();
    if (!result.second) {
        delete manager;
    }
}

/** @brief Removes and deletes the manager on engine shutdown; destructor drains any queued globals. */
void GlobalManager::RemoveManager(ARKTS_Env env)
{
    auto vm = reinterpret_cast<EcmaVM*>(env);
    GlobalManager* current = nullptr;
    managersMutex_.Acquire();
    auto itor = managers_.find(vm);
    if (itor != managers_.end()) {
        current = itor->second;
        managers_.erase(itor);
    }
    managersMutex_.Release();
    if (current) {
        delete current;
    }
}

GlobalManager::~GlobalManager()
{
    std::vector<uintptr_t> toDispose;
    handleMutex_.Acquire();
    std::swap(toDispose, handlesToDispose_);
    handleMutex_.Release();
    // Flush pending handles; SetDisposed ensures ~ARKTS_Global_ skips FreeGlobalHandleAddr when the VM is already invalid.
    for (auto p : toDispose) {
        auto global = reinterpret_cast<ARKTS_Global_*>(p);
        global->SetDisposed();
        delete global;
    }
}
}

/**
 * @brief Creates a persistent Global for heap object `value` (must run on the Ark thread / appropriate native scope).
 * @pre `value` must be a heap object.
 */
ARKTS_Global ARKTS_CreateGlobal(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_P(env, "env is null");
    auto vm = P_CAST(env, EcmaVM*);
    panda::JsiFastNativeScope fastNativeScope(vm);
    ARKTS_ASSERT_P(ARKTS_IsHeapObject(value), "value is not heap object");

    auto handle = BIT_CAST(value, Local<JSValueRef>);
    auto result = new ARKTS_Global_(vm, handle);

    return P_CAST(result, ARKTS_Global);
}

/**
 * @brief Current projected value; `Undefined` if disposed.
 */
ARKTS_Value ARKTS_GetGlobalValue(ARKTS_Global global)
{
    ARKTS_ASSERT_U(global, "global is null");

    return global->GetValue();
}

/**
 * Thread-affinity constraints for global-handle disposal.
 *
 * Cangjie object finalizers may run on threads other than the ArkTS runtime thread.
 * However, freeing VM global handles typically must happen on the VM/JS thread; doing it elsewhere can crash GC
 * or trigger undefined behavior.
 *
 * Therefore this function enqueues the handle into `GlobalManager` and schedules an async task to dispose it on
 * the ArkTS thread at a safe point.
 */
void ARKTS_DisposeGlobal(ARKTS_Env env, ARKTS_Global global)
{
    ARKTS_ASSERT_V(env, "env is null");
    ARKTS_ASSERT_V(global, "global is null");
    GlobalManager::Dispose(P_CAST(env, EcmaVM*), P_CAST(global, uintptr_t));
}

/**
 * @brief **Unsafe**: assumes correct thread/context and `delete`s the wrapper without the async queue.
 * @note Caller must guarantee this cannot race the GC incorrectly.
 */
void ARKTS_DisposeGlobalSync(ARKTS_Env env, ARKTS_Global global)
{
    ARKTS_ASSERT_V(env, "env is null");
    ARKTS_ASSERT_V(global, "handle is null");

    delete global;
}

/** @brief See `ARKTS_Global_::SetWeak`. */
void ARKTS_GlobalSetWeak(ARKTS_Env env, ARKTS_Global global)
{
    ARKTS_ASSERT_V(env, "env is null");
    ARKTS_ASSERT_V(global, "global is null");
    global->SetWeak();
}

/** @brief See `ARKTS_Global_::ClearWeak`. */
void ARKTS_GlobalClearWeak(ARKTS_Env env, ARKTS_Global global)
{
    ARKTS_ASSERT_V(env, "env is null");
    ARKTS_ASSERT_V(global, "global is null");
    global->ClearWeak();
}

constexpr uint64_t GLOBAL_TAG = 0b11111ULL << 48;
constexpr uint64_t GLOBAL_MASK = 0x0000'FFFF'FFFF'FFFF;

/**
 * @brief Packs raw `global` into double payload (top 5 bits `GLOBAL_TAG`) for passing as an immediate across some FFI boundaries.
 */
ARKTS_Value ARKTS_GlobalToValue(ARKTS_Env env, ARKTS_Global global)
{
    ARKTS_ASSERT_U(env, "env is null");
    ARKTS_ASSERT_U(global, "global is null");

    auto value = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(global)) & GLOBAL_MASK;
    value = value | GLOBAL_TAG;
    auto dValue = static_cast<double>(value);
    return ARKTS_CreateF64(dValue);
}

/**
 * @brief Inverse of `GlobalToValue`: checks tag then recovers the pointer.
 */
ARKTS_Global ARKTS_GlobalFromValue(ARKTS_Env env, ARKTS_Value value)
{
    ARKTS_ASSERT_P(env, "env is null");
    ARKTS_ASSERT_P(ARKTS_IsNumber(value), "value is a number");

    auto dValue = ARKTS_GetValueNumber(value);
    auto iValue = static_cast<uint64_t>(dValue);
    ARKTS_ASSERT_P((iValue & GLOBAL_TAG) == GLOBAL_TAG, "invalid tag value");
    iValue = iValue & GLOBAL_MASK;
    return BIT_CAST(iValue, ARKTS_Global);
}

/**
 * @brief True if the wrapper is not disposed and the handle is non-empty.
 */
bool ARKTS_GlobalIsAlive(ARKTS_Env env, ARKTS_Global global)
{
    ARKTS_ASSERT_F(env, "env is null");
    ARKTS_ASSERT_F(global, "global is null");

    return global->IsAlive();
}

/** @brief Paired with `ARKTSInner_InitLoop` to register per-VM GlobalManager on the uv path. */
void ARKTS_InitGlobalManager(ARKTS_Env env)
{
    GlobalManager::AddManager(env);
}

/** @brief Invoked from env cleanup hooks to drop the manager. */
void ARKTS_RemoveGlobalManager(ARKTS_Env env)
{
    GlobalManager::RemoveManager(env);
}