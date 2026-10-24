# Sendable 架构总览（napi + ets_runtime 双仓）

本文梳理 Sendable（ArkTS 跨 Worker 共享对象）的底层实现机制，覆盖 `foundation/arkui/napi` 与 `arkcompiler/ets_runtime` 两个仓库的交互边界。所有引用均为 `file_path:line_number` 格式。

> 相关文档：
> - [02-isjsshared-bit-setting.md](./02-isjsshared-bit-setting.md)：`IsJSShared` bit 如何设置
> - [03-sendable-class-instantiation-flow.md](./03-sendable-class-instantiation-flow.md)：实例化端到端流程
> - [04-constructor-vs-newtarget.md](./04-constructor-vs-newtarget.md)：constructor vs newTarget 语义

## 1. 总体架构

Sendable 的本质是 **进程级共享堆 + 进程级共享 GC 根存储**。对象本身在共享内存里，从不复制，多个 Worker（多个 EcmaVM 实例）通过进程级单例直接访问同一块内存。

```
进程级单例（所有 Worker 共享）
┌──────────────────────────────────────────────────────────────────────────┐
│ Runtime 单例 (ets_runtime/ecmascript/runtime.h:59)                       │
│  ├─ threads_ : 所有 Worker 的 JSThread (runtime.h:516)                   │
│  ├─ sendableGlobalStorage_ : EcmaGlobalStorage (runtime.h:587)          │
│  │    (thread_ = nullptr，跨线程；持 sendableGlobalStorageLock_)          │
│  └─ 上限 MAX_SENDABLE_GLOBAL_HANDLE_COUNT = 51200 (runtime.h:592)       │
│                                                                          │
│ SharedHeap 单例 (ecmascript/mem/heap.h:525) ← 所有 sendable 对象在此分配 │
└──────────────────────────────────────────────────────────────────────────┘
              ▲ SharedGC 追踪              ▲ 共享内存直接访问（无复制）
              │                              │
   ┌──────────┴──────────┐        ┌──────────┴──────────┐
   │ Worker A / EcmaVM_A │        │ Worker B / EcmaVM_B │
   │  本地堆 + 本地 GC    │        │  本地堆 + 本地 GC    │
   │  globalStorage_     │        │  globalStorage_     │
   │  (per-VM 普通 Global)│       │  (per-VM 普通 Global)│
   └─────────────────────┘        └─────────────────────┘
```

关键判断：普通 `Global<T>` 是 per-thread/per-VM 的本地句柄；`SendableGlobal<T>` 是进程级共享持久句柄，脱离任何具体 VM 存在。

## 2. SendableGlobal 句柄机制

### 2.1 类定义与 Global 的本质区别

`SendableGlobal<T>` 定义在 `ecmascript/napi/include/jsnapi_expo.h:553`：

```cpp
template<typename T>
class PUBLIC_API SendableGlobal {
public:
    SendableGlobal(const EcmaVM *vm, const Local<S> &current);  // jsnapi_expo.h:561
    Local<T> ToLocal(const EcmaVM *vm) const;                    // jsnapi_expo.h:565  ← 需要 vm
    void FreeSendableGlobalHandleAddr(const EcmaVM *vm);          // jsnapi_expo.h:593
private:
    uintptr_t address_ = 0U;  // jsnapi_expo.h:600  ← 只有 address_，没有 vm_
};
```

对比 `Global<T>`（`jsnapi_expo.h:460` 起）：

| 维度 | `Global<T>` | `SendableGlobal<T>` |
|---|---|---|
| 成员字段 | `address_` + `vm_` (jsnapi_expo.h:548-549) | **只有 `address_`** (jsnapi_expo.h:600) |
| 句柄存储位置 | `JSThread::globalStorage_`（per-VM，js_thread.h:2472） | **`Runtime::sendableGlobalStorage_`**（进程级单例，runtime.h:587） |
| 跨线程 | 否（绑定单个 VM） | **是**（进程级共享） |
| 锁 | 无（单线程访问） | **`sendableGlobalStorageLock_`**（runtime.h:586） |
| `ToLocal` | `Local<T>(vm_, *this)`（自带 vm_） | `Local<T>(vm, *this)`（**必须传入 vm**） |
| GC 追踪 | 本地 Heap GC | SharedGC |
| 对象域 | 本地堆 | SharedHeap |

本质区别：`Global<T>` 是"某个 VM 拥有的本地持久句柄"；`SendableGlobal<T>` 是"进程级共享持久句柄"——不存 `vm_`，存的对象必须在共享堆上（`IsSendable` 校验通过）。

### 2.2 句柄创建/释放调用链

构造（`jsnapi_expo.h:2387`）：
```
SendableGlobal<T>::SendableGlobal(vm, local)
  └─ JSNApi::GetSendableGlobalHandleAddr(vm, localAddress)         // jsnapi_expo.h:2390
     └─ (jsnapi_expo.cpp:7317)
        ├─ CROSS_THREAD_CHECK(vm)                                   // jsnapi_expo.cpp:7322
        ├─ ThreadManagedScope scope(thread)
        ├─ if (!handle->IsSendable()) THROW_TYPE_ERROR               // jsnapi_expo.cpp:7325  强校验
        └─ Runtime::GetInstance()->NewSendableGlobalHandle(value)    // jsnapi_expo.cpp:7329
           └─ (runtime.cpp:813)
              ├─ LockHolder(sendableGlobalStorageLock_)              // runtime.cpp:815  互斥
              ├─ ASSERT(JSTaggedValue(value).IsSendable())          // runtime.cpp:816
              ├─ ++aliveSendableGlobalHandleCount_ > 51200 → FATAL   // :817
              └─ newSendableGlobalHandle_(value)                    // :821
                 └─ sendableGlobalStorage_->NewGlobalHandle<NORMAL_NODE>(value)  // runtime.cpp:798
```

释放（`jsnapi_expo.h:2395` → `jsnapi_expo.cpp:7332`）：
```
SendableGlobal<T>::FreeSendableGlobalHandleAddr(vm)
  └─ JSNApi::DisposeSendableGlobalHandleAddr(vm, address_)          // jsnapi_expo.h:2400
     └─ (jsnapi_expo.cpp:7332)
        ├─ CROSS_THREAD_CHECK(vm)                                   // jsnapi_expo.cpp:7337
        └─ Runtime::GetInstance()->DisposeSendableGlobalHandle(addr) // :7342
           └─ (runtime.cpp:824)
              ├─ LockHolder(sendableGlobalStorageLock_)              // 互斥
              ├─ --aliveSendableGlobalHandleCount_ < 0 → FATAL
              └─ disposeSendableGlobalHandle_(addr)
                 └─ sendableGlobalStorage_->DisposeGlobalHandle<NORMAL_NODE>(addr)  // runtime.cpp:801
```

### 2.3 SendableGlobal → 某 VM 的 Local 转换

`Local<T>::Local(vm, SendableGlobal<T>&)`（`jsnapi_expo.h:2422`）调用 `JSNApi::GetHandleAddr`（普通版，`jsnapi_expo.cpp:6319`）：
- 从进程级 Node 读出 JSTaggedValue（共享堆对象指针）
- 在**当前 VM 的本地 handle scope** 里建瞬态本地句柄（`EcmaHandleScope::NewHandle`）

即：`address_` 指向 `Runtime::sendableGlobalStorage_` 中某 Node 的 `obj_` 字段；访问时读出其中的 JSTaggedValue，再在目标 VM 的本地 scope 建瞬态句柄。瞬态句柄由本地 GC 跟踪，SendableGlobal 本身由 SharedGC 跟踪。

## 3. Sendable 句柄域/存储区

### 3.1 存储结构

Sendable global handle 存放在 **`Runtime::sendableGlobalStorage_`**，类型 `EcmaGlobalStorage<Node>`（`runtime.h:587`），在 `Runtime::InitSendableGlobalStorage()`（`runtime.cpp:794`）创建：

```cpp
sendableGlobalStorage_ = new EcmaGlobalStorage<Node>(nullptr, nativeAreaAllocator_.get());  // runtime.cpp:796
```

注意第一个参数 `JSThread*` 传 **`nullptr`**（`ecma_global_storage.h:712` 的 `thread_`），因为该存储跨所有线程，不归属任何单线程。

`EcmaGlobalStorage`（`ecma_global_storage.h:411`）是分块链表：
- 每个 `NodeList<T>`（`ecma_global_storage.h:237`）含 `GLOBAL_BLOCK_SIZE=256` 个 `Node`（`:394`）
- `Node`（`:30`）字段：`obj_`（JSTaggedType，`:120`）、`next_/prev_`、`index_`、`isUsing_`、`isWeak_`、`isInYoungList_`
- 句柄地址 = `Node::GetObjectAddress()`（`:102`），即 `&obj_`

### 3.2 普通 Global 与 SendableGlobal 在存储上的区分

| | 普通 `Global<T>` | `SendableGlobal<T>` |
|---|---|---|
| 实例归属 | 每个 `JSThread` 一个 `EcmaGlobalStorage` | 进程级一个 `EcmaGlobalStorage`（在 `Runtime`） |
| NodeKind | `NORMAL_NODE` + 可选 `UNIFIED_NODE`(XRef/hybrid) | `NORMAL_NODE` |
| `thread_` 字段 | 非 nullptr（绑定本线程） | **nullptr**（跨线程） |
| 是否进 young list | 是（`CheckAndAddToYoungGlobalNodes`，`:532`） | 否（sendable 对象在共享堆，无 young space） |
| 并发访问 | 单线程，无需锁 | **必须 `sendableGlobalStorageLock_`** |

### 3.3 为什么 SendableGlobal 需要 mutex

- **普通 `Global`**：归属单个 `JSThread`，只有该线程创建/释放/遍历节点（本地 GC 也只在该线程挂起后跑），访问天然单线程，无需锁。
- **`SendableGlobal`**：`sendableGlobalStorage_` 是进程级共享，**任意 Worker 线程**都可能在同一时刻 `NewSendableGlobalHandle`/`DisposeSendableGlobalHandle`。链表节点的分配/归还若不互斥会损坏链表。因此 `Runtime::NewSendableGlobalHandle`/`DisposeSendableGlobalHandle` 都用 `LockHolder(sendableGlobalStorageLock_)`（`runtime.cpp:815,826`），并用 `aliveSendableGlobalHandleCount_` 计数 + 上限 51200 做保护。

> 注：`ecma_global_storage_hybrid.h` 的 `topXRefGlobalNodes_`/`xRefFreeListNodes_`（`UNIFIED_NODE`/XRef）是 hybrid JS/ETS 模式下的交叉引用节点，**不是** sendable 存储——sendable 走独立的 `Runtime::sendableGlobalStorage_`。

## 4. 共享堆分配

所有 sendable 对象分配在 `SharedHeap` 单例（`mem/heap.h:525`）。`ObjectFactory::sHeap_`（`object_factory.h:1020`）即 `SharedHeap*`。工厂方法最终走 `NewSharedOldSpaceObject`（`shared_object_factory.cpp:349`）→ `sHeap_->AllocateOldOrHugeObject`（`SHARED_NON_MOVABLE`/`SHARED_OLD_SPACE`）。

HClass 的 `IsInSharedHeap()` 直接返回 `IsJSShared()`（`js_hclass.h:1155`）——"是 JSShared 类型"就等于"在共享堆"。

## 5. Sendable 对象类型清单

所有 sendable 对象的 HClass 都置 `IsJSSharedBit`（`js_hclass.h:406`），分配走共享堆。JSType 枚举见 `js_hclass.h:363`。

| 类型 | JSType | 关键文件:行 | 创建入口 |
|---|---|---|---|
| JSSharedObject（@Sendable 通用对象） | `JS_SHARED_OBJECT` (`js_hclass.h:99`) | `shared_object_factory.cpp:370` `NewSharedOldSpaceJSObject` | `ObjectRef::NewS` (`jsnapi_expo.cpp:2802`) |
| JSSharedFunction（sendable function） | `JS_SHARED_FUNCTION`(`:104`)/`JS_SHARED_ASYNC_FUNCTION`(`:120`) | `shared_object_factory.cpp:332` `NewSFunctionByHClass` | `FunctionRef::NewSendable` (`jsnapi_expo.cpp:3904`) |
| JSSendableArrayBuffer | `JS_SENDABLE_ARRAY_BUFFER` (`:188`) | `shared_objects/js_sendable_arraybuffer.h:22` | `SendableArrayBufferRef::New` (`jsnapi_expo.cpp:3645`) |
| JSSharedArray | `JS_SHARED_ARRAY` (`:198`) | `jsnapi_expo.cpp:4513` | `SendableArrayRef::New` |
| JSSharedTypedArray（SharedInt8Array 等） | `JS_SHARED_INT8_ARRAY`…`JS_SHARED_BIGUINT64_ARRAY` (`:228-238`) | `jsnapi_expo.h:1402` | `SendableTypedArrayRef` 子类 |
| JSSharedMap / JSSharedSet | `JS_SHARED_MAP`(`:140`)/`JS_SHARED_SET`(`:138`) | `jsnapi_expo.cpp:1607/1772` | `SendableMapRef::New`/`SendableSetRef::New` |
| Shared 迭代器 | `JS_SHARED_MAP_ITERATOR`/`JS_SHARED_SET_ITERATOR`/`JS_SHARED_ARRAY_ITERATOR` (:151,153,171) | — | 对应容器 `GetEntries/GetKeys/GetValues` 产生 |
| JSSharedArrayBuffer（与 SendableArrayBuffer 区分） | `JS_SHARED_ARRAY_BUFFER` (`:187`) | — | 内置/Atomics 路径 |
| SendableEnv（可序列化词法环境） | TaggedArray 子类 | `ecmascript/sendable_env.h:23` | 序列化器使用，`base_serializer.h:80,107` |

`NewSendableClassFunction`（`jsnapi_expo.cpp:4120`）流程：`JSNapiSendable` 装配属性描述符 → `CreateSPrototypeHClass` 建 prototype → `CreateSConstructorHClass` 建 constructor → `SetSProperties` → `SetExtensible(false)` → `CreateSHClass` 建实例 HClass。

sendable 函数与普通函数的关键差异：**词法环境为 `Undefined`**（`jsnapi_expo.cpp:3919,4163`，避免跨 Worker 引用本地对象）、不可扩展、走共享堆、HClass 置 `IsJSSharedBit`。

`SharedFieldType` 枚举（`property_attributes.h:40-50`）描述 sendable 属性字段类型，由 `JSNapiSendable::GetSharedFieldType`（`jsnapi_sendable.cpp:113`）根据值类型推断，用于共享对象上做类型特化属性布局。

## 6. GC：独立的 SharedGC

Sendable 对象不走本地 GC，由独立的 SharedGC 回收（`ecmascript/mem/shared_heap/`）：

- `SharedPartialGC`（跑在 `DaemonThread`）/`SharedFullGC`（可移动）/`SharedCC`（并发拷贝）
- 标记器 `SharedGCMarkerBase`（`shared_gc_marker.h`）及子类 `SharedGCMarker`/`SharedGCMovableMarker`（`shared_gc_marker.cpp:184,187`）

### 6.1 sendable 根的标记链

`SharedGCMarkerBase::MarkGlobalRoots`（`shared_gc_marker.cpp:32`）依次：
```
MarkGlobalRoots(visitor)
 ├─ MarkSendableGlobalStorage(visitor)   // shared_gc_marker.cpp:34,102-106
 │    └─ Runtime::GetInstance()->IterateSendableGlobalStorage(visitor)  // runtime.cpp:834
 │         └─ 遍历 sendableGlobalStorage_ 的所有 Node，作为 ROOT_GLOBAL_HANDLE 访问
 ├─ MarkSerializeRoots
 ├─ MarkSharedModule
 └─ MarkStringCache
```

随后 `MarkAllLocalRoots`（`shared_gc_marker.cpp:40`）通过 `Runtime::GCIterateThreadList`（`runtime.h:101`，持 `threadsLock_`）遍历**所有 Worker 线程**，对每个 VM 调 `MarkLocalVMRoots` + `CollectLocalVMRSet`（收集本地→共享堆的 RSet/remembered set，`shared_gc_marker.cpp:74`）。

### 6.2 分配在共享堆上

- `ObjectFactory::sHeap_`（`object_factory.h:1020`）即 `SharedHeap*`（构造 `object_factory.h:214`）
- 所有 sendable 工厂方法最终走 `NewSharedOldSpaceObject`（`shared_object_factory.cpp:349`）→ `sHeap_->AllocateOldOrHugeObject`（:352），分配在 `SHARED_NON_MOVABLE`/`SHARED_OLD_SPACE`
- HClass 的 `IsInSharedHeap()` 直接返回 `IsJSShared()`（`js_hclass.h:1155-1158`）
- 本地 GC 通过 RSet 通知共享 GC：`ProcessThenMergeBackRSetFromBoundJSThread`（`shared_gc_marker.cpp:114`）处理本地对 `IsInSharedSweepableSpace()` 的引用（:124）

### 6.3 跨 Worker GC 协调

SharedGC 需要 STW 所有 Worker：`GCIterateThreadList`（`runtime.h:101`）遍历 `threads_`，断言 `thread->IsSuspended() || HasLaunchedSuspendAll()`（`shared_gc_marker.cpp:47`）。RSet 机制（`RSetWorkListHandler`，`shared_gc_marker.cpp:74-88`）处理"本地堆对象引用共享堆对象"的跨堆引用追踪。

## 7. 跨 Worker 共享机制

**机制：纯共享内存，无复制。** 三个进程级单例共同支撑：

1. **`Runtime`** 单例（`Runtime::GetInstance()`）：持有 `threads_`（所有 Worker 的 `JSThread*`，`runtime.h:516`）。每个 Worker 创建 `JSThread` 时 `JSThread::RegisterThread`（`js_thread.cpp:109-111`）→ `Runtime::RegisterThread`（`runtime.cpp:280`）→ `threads_.emplace_back`（:285）。Worker 退出 `UnregisterThread`（`runtime.cpp:306`）。

2. **`SharedHeap`** 单例（`SharedHeap::GetInstance()`，`mem/heap.h:525`）：所有 sendable 对象在此分配，所有 Worker 直接访问同一块共享堆内存。

3. **`Runtime::sendableGlobalStorage_`**（`runtime.h:587`）：进程级共享的持久句柄根集。任意 Worker `NewSendableGlobalHandle` 写入的 Node，对其他 Worker 可见（受 `sendableGlobalStorageLock_` 保护）。

**数据流示例**（Worker A 创建 sendable 对象 + SendableGlobal，Worker B 读取）：
```
Worker A:
  factory->NewSharedOldSpaceJSObject(hclass)
    └─ sHeap_->AllocateOldOrHugeObject  ← 分配在共享堆
  SendableGlobal<T>(vm_A, local)
    └─ Runtime::NewSendableGlobalHandle(value)  ← Node 写入进程级 storage（持锁）
       └─ Node.obj_ = 共享堆对象指针

Worker B（拿到同一个 SendableGlobal 对象引用）:
  localB = sendableGlobalRef.ToLocal(vm_B)
    └─ Local<T>(vm_B, sendableGlobal)
       └─ JSNApi::GetHandleAddr(vm_B, address_)  // jsnapi_expo.cpp:6319
          ├─ value = *(JSTaggedType*)address_  ← 直接读进程级 Node.obj_（同一份内存）
          └─ EcmaHandleScope::NewHandle(thread_B, value)  ← 在 B 的本地 scope 建瞬态句柄
```
B 通过本地句柄访问共享堆对象，对象本身从未被复制。GC 由 SharedGC 跨所有 Worker 协调回收。

## 8. IsSendable 判定

声明：`ecmascript/js_tagged_value.h:760`
实现：`ecmascript/js_tagged_value-inl.h:1494`

```cpp
inline bool JSTaggedValue::IsSendable() const
{
    return IsJSShared() || IsString() || IsNumber() || IsBoolean() ||
           IsUndefined() || IsNull() || IsBigInt();
}
```

即一个值被判为 sendable 当且仅当它是：
- **`IsJSShared()`**（`js_tagged_value-inl.h:1375`）：`IsHeapObject() && GetTaggedObject()->GetClass()->IsJSShared()`（HClass 的 `IsJSSharedBit` 置位，`js_hclass.h:1160`）—— 即所有 `JS_SHARED_*`/`JS_SENDABLE_ARRAY_BUFFER` 等共享堆对象；或
- **原始值**：String / Number / Boolean / Undefined / Null / BigInt（这些原始值天然跨 Worker 安全，无归属）。

相关谓词的区别：
- `IsJSSharedObject()`（`js_tagged_value-inl.h:1365`）：仅 `JS_SHARED_OBJECT` 类型。
- `IsJSSharedFunction()`（`:1370`）：仅 `JS_SHARED_FUNCTION`。
- `IsSharedType()`（`:1380`）：实现未含 `IsString`/`IsBigInt`（与 IsSendable 略有差异，不可互换）。

`IsSendable` 是 `Runtime::NewSendableGlobalHandle` 的准入校验（`runtime.cpp:816` `ASSERT`，及 `jsnapi_expo.cpp:7325` 的 `THROW_TYPE_ERROR`）——非 sendable 值无法包装成 SendableGlobal，从源头保证共享句柄只引用跨 Worker 安全的对象。

## 9. napi↔ets_runtime 交互边界

`foundation/arkui/napi` 通过 `ecmascript/napi/include/jsnapi.h`/`jsnapi_expo.h`（`panda::` 命名空间）调用引擎层。已确认的调用点：

| napi 侧文件:行 | 调用的 ets_runtime sendable API | 用途 |
|---|---|---|
| `native_engine/impl/ark/ark_sendable_native_reference.h:35` | 持有 `panda::SendableGlobal<JSValueRef> value_` | napi_sendable_ref 的底层实现 |
| `native_engine/impl/ark/ark_sendable_native_reference.cpp:21` | `SendableGlobal(vm, value)` 构造 | 创建跨 Worker 引用 |
| `native_engine/impl/ark/ark_sendable_native_reference.cpp:31` | `value_.FreeSendableGlobalHandleAddr(vm)` | 释放（→ `Runtime::DisposeSendableGlobalHandle`，持锁） |
| `native_engine/impl/ark/ark_sendable_native_reference.cpp:42` | `value_.ToLocal(vm)` | 取值（→ `GetHandleAddr` 在当前 VM 建本地句柄） |
| `native_engine/native_sendable.cpp:26` | `FunctionRef::SendablePropertiesInfos` + `CreateSendablePropertiesInfo` | 把 `NapiPropertyDescriptor` 转成 sendable 属性描述信息 |
| `native_engine/native_sendable.cpp:85` | `ObjectRef::CreateSendableAccessorData(vm, getter, setter)` | 建 sendable accessor |
| `native_engine/native_sendable.cpp:124` | `FunctionRef::NewSendable(vm, ArkNativeFunctionCallBack, CommonDeleter, data, true)` | 建 sendable 函数（napi 回调包装） |
| `native_engine/impl/ark/ark_native_engine.cpp:400-402` | `CreateSendablePropertiesInfos` + `FunctionRef::NewSendableClassFunction(vm, cb, deleter, data, name, infos, parent, true)` | `napi_define_sendable_class` 的引擎实现入口 |
| `native_engine/impl/ark/ark_native_engine.cpp:1505-1509` | `InitSendablePropertiesInfo` + `ObjectRef::NewSWithProperties(vm, info)` | 建 sendable 对象 |
| `native_engine/native_api.cpp:294,311` | `panda::SendableArrayRef::New(engine->GetEcmaVm(), ...)` | `napi_create_sendable_array` 等 |
| `native_engine/native_api.cpp:1770` | `panda::SendableMapRef::New(engine->GetEcmaVm())` | `napi_create_sendable_map` |
| `native_engine/native_api.cpp:2873` | `panda::SendableArrayBufferRef::New(vm, byte_length)` | `napi_create_sendable_arraybuffer` |

### 边界关键约束（与 napi 仓 AGENTS.md 一致）

1. **跨 Worker 引用必须用 Sendable 变体**：`ark_sendable_native_reference.cpp` 用 `SendableGlobal<JSValueRef>` 而非 `Global`。底层 `GetSendableGlobalHandleAddr` 会 `IsSendable` 校验，非 sendable 值直接 `THROW_TYPE_ERROR`（`jsnapi_expo.cpp:7326`）。

2. **Sendable 无自动清理**：`ArkSendableNativeReference` 必须显式 `DeleteSendableRef`（`ark_sendable_native_reference.cpp:24`），否则 `Runtime::sendableGlobalStorage_` 中的 Node 不会归还，`aliveSendableGlobalHandleCount_` 不递减。SendableGlobal 生命周期脱离任何 VM，VM 销毁不会触发它的析构。

3. **CROSS_THREAD_CHECK**：`GetSendableGlobalHandleAddr`/`DisposeSendableGlobalHandleAddr` 都有 `CROSS_THREAD_CHECK(vm)`（`jsnapi_expo.cpp:7322,7337`）——创建/释放必须在 vm 所属线程，但**句柄本身可跨线程传递使用**（这正是 SendableGlobal 的目的，与普通 Global 完全禁止跨线程相反）。

4. **上限保护**：进程级 `MAX_SENDABLE_GLOBAL_HANDLE_COUNT = 51200`（`runtime.h:592`），超限 FATAL。napi 侧需注意不要无限累积 sendable ref 而不释放。

5. **多 context 限制**：`napi_create_strong_sendable_reference` 仅主 env context 支持（`IsMainEnvContext()`，`native_api.cpp:4911`），多 context 不支持 sendable feature。

## 10. 关键文件清单

### 句柄与 JSNApi 接口层
- `ecmascript/napi/include/jsnapi_expo.h:553` — `SendableGlobal<T>` 类定义
- `ecmascript/napi/include/jsnapi_expo.h:460` — `Global<T>` 类（对比）
- `ecmascript/napi/include/jsnapi_expo.h:2387` — `SendableGlobal` 构造
- `ecmascript/napi/include/jsnapi_expo.h:2395` — `FreeSendableGlobalHandleAddr`
- `ecmascript/napi/include/jsnapi_expo.h:2422` — `Local(vm, SendableGlobal)`
- `ecmascript/napi/include/jsnapi_expo.h:2165-2166` — `JSNApi::GetSendableGlobalHandleAddr`/`DisposeSendableGlobalHandleAddr` 声明
- `ecmascript/napi/jsnapi_expo.cpp:7317` — `GetSendableGlobalHandleAddr` 实现
- `ecmascript/napi/jsnapi_expo.cpp:7332` — `DisposeSendableGlobalHandleAddr` 实现
- `ecmascript/napi/jsnapi_expo.cpp:6319` — `GetHandleAddr`（SendableGlobal→某 VM Local 的值读取）
- `ecmascript/napi/include/jsnapi_expo.h:917-926` — `SendableType` 枚举 / `SendablePropertiesInfo`
- `ecmascript/napi/include/jsnapi_expo.h:997-1001` — `SendablePropertiesInfos`
- `ecmascript/napi/include/jsnapi_expo.h:1041` — `NewSendable` 声明
- `ecmascript/napi/include/jsnapi_expo.h:1088` — `NewSendableClassFunction` 声明
- `ecmascript/napi/jsnapi_expo.cpp:3904` — `NewSendable` 实现
- `ecmascript/napi/jsnapi_expo.cpp:4120` — `NewSendableClassFunction` 实现
- `ecmascript/jsnapi_sendable.cpp:25` — `JSNapiSendable` 构造（属性描述符装配）

### 存储与 Runtime 层
- `ecmascript/ecma_global_storage.h:411` — `EcmaGlobalStorage<T>` 类
- `ecmascript/ecma_global_storage.h:30` — `Node` 类（`obj_` 在 :120）
- `ecmascript/runtime.h:585-593` — sendable 存储成员声明（锁/storage/count/MAX）
- `ecmascript/runtime.cpp:794` — `InitSendableGlobalStorage`
- `ecmascript/runtime.cpp:813` — `NewSendableGlobalHandle`
- `ecmascript/runtime.cpp:824` — `DisposeSendableGlobalHandle`
- `ecmascript/runtime.cpp:834` — `IterateSendableGlobalStorage`（GC root 遍历）
- `ecmascript/js_thread.h:2472` — `JSThread::globalStorage_`（普通 Global 存储，per-VM）
- `ecmascript/runtime.h:516` — `threads_` 列表
- `ecmascript/runtime.cpp:280` — `RegisterThread`

### 类型与 HClass
- `ecmascript/js_tagged_value.h:760` — `IsSendable` 声明
- `ecmascript/js_tagged_value-inl.h:1494` — `IsSendable` 实现
- `ecmascript/js_tagged_value-inl.h:1365-1384` — `IsJSSharedObject`/`IsJSSharedFunction`/`IsJSShared`/`IsSharedType`
- `ecmascript/js_hclass.h:363` — `JSType` 枚举（共享类型 :99-238）
- `ecmascript/js_hclass.h:406` — `IsJSSharedBit`
- `ecmascript/js_hclass.h:1155-1168` — `IsInSharedHeap`/`IsJSShared`/`SetIsJSShared`
- `ecmascript/property_attributes.h:40` — `SharedFieldType` 枚举

### 对象分配（共享堆）
- `ecmascript/mem/heap.h:519-525` — `SharedHeap` 单例
- `ecmascript/object_factory.h:1020` — `sHeap_`（`SharedHeap*`）
- `ecmascript/shared_object_factory.cpp:349` — `NewSharedOldSpaceObject`
- `ecmascript/shared_object_factory.cpp:370` — `NewSharedOldSpaceJSObject`
- `ecmascript/shared_object_factory.cpp:332` — `NewSFunctionByHClass`

### GC（共享堆回收）
- `ecmascript/mem/shared_heap/shared_gc.h:30` — `SharedGC` 基类
- `ecmascript/mem/shared_heap/shared_partial_gc.h:22` — `SharedPartialGC`
- `ecmascript/mem/shared_heap/shared_full_gc.h:30` — `SharedFullGC`
- `ecmascript/mem/shared_heap/shared_cc.h:64` — `SharedCC`
- `ecmascript/mem/shared_heap/shared_gc_marker.cpp:32` — `MarkGlobalRoots`
- `ecmascript/mem/shared_heap/shared_gc_marker.cpp:102` — `MarkSendableGlobalStorage`
- `ecmascript/mem/shared_heap/shared_gc_marker.cpp:40-55` — `MarkAllLocalRoots`（遍历所有 Worker）
- `ecmascript/mem/shared_heap/shared_gc_marker.cpp:114` — `ProcessThenMergeBackRSetFromBoundJSThread`
