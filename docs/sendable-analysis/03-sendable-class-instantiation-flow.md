# Sendable 类的实例化流程（端到端）

本文回答：通过 `napi_define_sendable_class` 创建的类，其生成的实例对象是否 sendable，以及从对象创建到 napi 回调 `ArkNativeFunctionCallBack` 的完整调用流程。

> 相关文档：
> - [01-sendable-architecture-overview.md](./01-sendable-architecture-overview.md)：Sendable 整体架构
> - [02-isjsshared-bit-setting.md](./02-isjsshared-bit-setting.md)：IsJSShared bit 如何设置（本文"HClass 创建"环节的基础）
> - [04-constructor-vs-newtarget.md](./04-constructor-vs-newtarget.md)：`NewJSObjectByConstructor` 两参数语义

## 核心结论

通过 `napi_define_sendable_class` 创建的类，其生成的实例对象**是 sendable 的**。实现分两个阶段：

1. **类定义阶段**：预埋共享实例 HClass（iHClass），存入构造器的 `ProtoOrHClass` 字段
2. **`new` 调用阶段**：`NewJSObjectByConstructor` 检测 iHClass 的 `IsJSShared`，路由到共享堆分配；随后执行构造器函数体，经 native 派发调到 `ArkNativeFunctionCallBack`

## 阶段一：类定义——预埋共享实例 HClass

`NapiDefineSendableClass`（`ark_native_engine.cpp:372`）→ `NewSendableClassFunction`（`jsnapi_expo.cpp:4120`）做三件事：

### 1. 建实例 HClass（iHClass），JSType = `JS_SHARED_OBJECT`

```
CreateSHClass (jsnapi_expo.cpp:4172)
└─ factory->NewSEcmaHClass(JSSharedObject::SIZE, JSType::JS_SHARED_OBJECT, length)  // js_hclass.cpp:1765
   └─ sHeap_->AllocateNonMovableOrHugeObject   // HClass 本身在共享堆
   └─ Initialize → IsJSTypeShared(JS_SHARED_OBJECT) → SetIsJSShared(true)  // js_hclass.cpp:214-215
```

iHClass 创建的细节见 [02-isjsshared-bit-setting.md](./02-isjsshared-bit-setting.md)。此处只强调：iHClass 在创建时就被打上 `IsJSShared` bit。

### 2. 把 iHClass 存进构造器的 `ProtoOrHClass` 字段

```cpp
constructor->SetProtoOrHClass(thread, iHClass);  // jsnapi_expo.cpp:4175
```

注意：`jsnapi_expo.cpp:4161` 先把 prototype 存进 `ProtoOrHClass`，`:4175` 再用 iHClass 覆盖。最终 `ProtoOrHClass` = iHClass。

### 3. 构造器本身的设置

- 构造器是 `JS_SHARED_FUNCTION`（共享堆），`FunctionKind::CLASS_CONSTRUCTOR`
- `constructor->SetCallNapi(callNapi)`（`jsnapi_expo.cpp:4164`）标记走 napi 回调路径
- `info->isSendable=true`（`ark_native_engine.cpp:388`）标记是 sendable
- 词法环境为 `Undefined`（`jsnapi_expo.cpp:4163`，避免跨 Worker 引用本地对象）
- `SetExtensible(false)`（`:4174`，sendable 类不可扩展）

## 阶段二：`new` 调用——检测 `IsJSShared` 路由共享堆

JS 侧执行 `new MySendableClass(...)` 时，运行时调 `NewJSObjectByConstructor`（`object_factory.cpp:1199`）创建 `this`：

```
NewJSObjectByConstructor(constructor, newTarget)              // object_factory.cpp:1199
├─ shared 一致性校验                                          // :1202-1205
│   if (constructor->IsJSShared() && !newTarget->IsJSShared())
│       THROW_TYPE_ERROR "shared ctor cannot assign unshared newTarget"
├─ JSFunction::GetInstanceJSHClass(thread, constructor, newTarget)  // :1210
│  └─ GetOrCreateInitialJSHClass(thread, constructor)        // js_function.cpp:194
│     ├─ protoOrHClass = fun->GetProtoOrHClass(thread)       // :196  ← 读阶段一存的 iHClass
│     └─ if (protoOrHClass.IsJSHClass()) return 它           // :197-198  ← 直接返回 iHClass
├─ if (jshclass->IsJSShared()) {                             // :1218  ← TRUE（iHClass 带 bit）
│     obj = NewSharedOldSpaceJSObject(jshclass);             // :1219  ← 共享堆分配
│     ... 复制 SNameDictionary 处理字典模式属性/elements ...
│  } else {
│     obj = NewJSObjectWithInit(jshclass);                   // 普通堆
│  }
```

分配出的对象 HClass 就是那个 `JS_SHARED_OBJECT` 类型的 iHClass → 对象天然 `IsJSShared()` → `JSTaggedValue::IsSendable()` 为 true。可被 `napi_create_strong_sendable_reference` 包装成 `SendableGlobal`（因为 `IsSendable` 校验通过）。

## 阶段三：执行构造器函数体到 napi 回调

`this` 对象创建好后，`ConstructInternal` 执行构造器函数体。

### 关键前提：构造器是 native 函数

`napi_define_sendable_class` 创建的构造器在引擎层是一个 **native 函数**（无字节码）：
- `Method` = `GetReadOnlyMethodForNativeFunction(CLASS_CONSTRUCTOR)`（只读 native 方法，`IsNativeWithCallField()=true`）
- 函数的 `NativePointer` = `ArkNativeFunctionCallBack`

证据链（`NewSendableClassFunction` → `NewSFunctionByHClass`）：
```
NewSendableClassFunction (jsnapi_expo.cpp:4139)
  factory->NewSFunctionByHClass(reinterpret_cast<void *>(nativeFunc), constructorHClass, CLASS_CONSTRUCTOR)
  └─ NewSFunctionByHClass(func, hclass, kind, builtinId=INVALID)  // shared_object_factory.cpp:332
     ├─ function->SetMethod(GetReadOnlyMethodForNativeFunction(kind))  // :342
     └─ function->SetNativePointer(const_cast<void *>(func))          // :344  ← func = ArkNativeFunctionCallBack
```

> **对比非 sendable**：非 sendable 的 `FunctionRef::New`（`jsnapi_expo.cpp:3830`）存的是 `Callback::RegisterCallback`（中转桥），sendable 直接存 `ArkNativeFunctionCallBack`，少一层中转。

### 完整调用链

```
JS: new MySendableClass(args)
  │
  ▼  [[Construct]] 内部方法
JSFunction::Construct(info)                              // js_function.cpp:573
  └─ ConstructInternal(info)                             // js_function.cpp:730  (经 :592)
     │
     ├─ 1) 创建 this（共享堆）—— 阶段二
     │   func->IsBase(thread) → true
     │   obj = factory->NewJSObjectByConstructor(func, newTarget)  // js_function.cpp:746
     │   info->SetThis(obj)                                       // js_function.cpp:751
     │
     ├─ 2) 执行构造器函数体
     │   if (func->IsCompiledCode() && thread->HasSwitchedToStwStub())  // :752
     │       resultValue = InvokeOptimizedEntrypoint(thread, func, info)  // :753
     │   else
     │       resultValue = EcmaInterpreter::Execute(info)  // :759
     │     └─ EcmaInterpreter::Execute                       // interpreter-inl.cpp:673
     │        method = callTarget->GetCallTarget(thread)      // :696
     │        if (method->IsNativeWithCallField())           // :697  ← TRUE
     │          └─ ExecuteNative(info)                       // :698 → 函数体 :640
     │             nativePointer = callTarget->GetNativePointer(thread)  // :654
     │               ↑ 对 sendable，这里就是 ArkNativeFunctionCallBack
     │             tagged = reinterpret_cast<EcmaEntrypoint>(nativePointer)(info)  // :659
     │                                                           ↓ 直接调用
     │  3) 进入 napi 回调
     │   ArkNativeFunctionCallBack(runtimeInfo)                  // ark_native_engine.cpp:1210
     │     info = NapiFunctionInfo*(runtimeInfo->GetData())      // :1216  拿到用户 cb+data
     │     if (info->isSendable)                                 // :1222  ← TRUE
     │        env = JSNApi::GetEnv(vm)                           // :1223  用当前 VM 的 env
     │     result = cb(env, runtimeInfo)                         // :1255/1259  用户构造器回调
     │        └─ napi_get_cb_info 取出共享堆 this（阶段二创建的）
     │
     └─ 4) 处理返回值（ECMAScript [[Construct]] 9.3.2 语义）
        if (resultValue.IsECMAObject()) return resultValue      // js_function.cpp:763-764
        if (func->IsBase(thread)) return obj                    // :766-767  返回预创建的 this
```

## 三个关键环节说明

### 1. `this` 在进入回调前就已创建（`js_function.cpp:744-751`）

`ConstructInternal` 先调 `NewJSObjectByConstructor` 把 `this` 分配好（sendable 走共享堆），再 `SetThis` 写进 call info，最后才执行函数体。所以 napi 回调里 `napi_get_cb_info` 拿到的 `this` 就是这个共享堆对象——这也是为什么实例天然是 sendable 的。

### 2. native 派发是「读 NativePointer 直接 cast 调用」（`interpreter-inl.cpp:654,659`）

```cpp
void *nativePointer = callTarget->GetNativePointer(thread);               // interpreter-inl.cpp:654
tagged = reinterpret_cast<EcmaEntrypoint>(nativePointer)(info);          // :659
```

`EcmaEntrypoint` 是 `JSTaggedValue(*)(EcmaRuntimeCallInfo*)`。sendable 构造器的 NativePointer 存的是 `ArkNativeFunctionCallBack`，所以这里直接调用它——**不经过 `Callback::RegisterCallback` 中转**（非 sendable 才走中转）。

`ArkNativeFunctionCallBack` 的签名 `panda::JSValueRef ArkNativeFunctionCallBack(JsiRuntimeCallInfo *runtimeInfo)`（`ark_native_engine.cpp:1210`）与 `EcmaEntrypoint` 的 ABI 兼容（参数 `EcmaRuntimeCallInfo*`，返回 `JSTaggedValue`/`JSValueRef`），故 `reinterpret_cast` 可行。

### 3. `isSendable` 决定 env 来源（`ark_native_engine.cpp:1222-1226`）

```cpp
if (info->isSendable) {
    env = reinterpret_cast<napi_env>(JSNApi::GetEnv(vm));   // 用当前执行线程的 VM
} else {
    env = info->env;                                        // 用注册时缓存的 env
}
```

sendable 函数可能被任意 Worker 调用，所以 env 不能用注册时的固定 env，必须从 `vm` 现取。这是 sendable 与普通 napi 函数在回调路径上的本质差异。

## 入口的两条路径

到达 `ArkNativeFunctionCallBack` 有两条入口，都汇入 `ConstructInternal`：

| 入口 | 调用方 | 链路 |
|---|---|---|
| JS 字节码 `new` | 解释器 construct opcode | → `JSFunction::Construct` → `ConstructInternal` |
| C++ 主动调用 | `FunctionRef::Call`/`NewInstance`（`jsnapi_expo.cpp`） | → 构造 `EcmaRuntimeCallInfo` → `JSFunction::Construct(info)`（`jsnapi.cpp:162`）→ `ConstructInternal` |

两条路径在 `ConstructInternal`（`js_function.cpp:730`）汇合，后续完全一致。

## 关键不变量

1. **this 在回调前已创建于共享堆**：`ConstructInternal` 先 `NewJSObjectByConstructor`（`:746`）→ `SetThis`（`:751`）→ `Execute`（`:759`）。napi 回调拿到的 `this` 即此对象。
2. **native 派发是读 NativePointer 直接 cast 调用**：`ExecuteNative` 读 `GetNativePointer()`（`interpreter-inl.cpp:654`）→ `reinterpret_cast<EcmaEntrypoint>(...)(info)`（`:659`）。sendable 的 NativePointer = `ArkNativeFunctionCallBack`，直接调用；非 sendable 经 `Callback::RegisterCallback` 中转。
3. **isSendable 决定 env 来源**：sendable 用 `JSNApi::GetEnv(vm)`（当前 VM），非 sendable 用缓存的 `info->env`。
4. **shared 一致性校验**：`NewJSObjectByConstructor`（`object_factory.cpp:1202`）保证 sendable 构造器的 newTarget 也必须 sendable，从源头杜绝"sendable 类产生非 sendable 实例"。

## 小结

| 阶段 | 关键动作 | 位置 |
|---|---|---|
| 类定义 | 建 `JS_SHARED_OBJECT` 类型 iHClass 并 `SetIsJSShared` | `jsnapi_expo.cpp:4172`→`js_hclass.cpp:214` |
| 类定义 | 把 iHClass 存入 `constructor->ProtoOrHClass` | `jsnapi_expo.cpp:4175` |
| `new` | 读 `ProtoOrHClass` 拿 iHClass | `js_function.cpp:196-198` |
| `new` | `if (jshclass->IsJSShared())` → `NewSharedOldSpaceJSObject` 共享堆分配 | `object_factory.cpp:1218-1219` |
| 回调 | `isSendable` 用当前 VM 的 env，回调拿到共享堆 `this` | `ark_native_engine.cpp:1222` |

所以 sendable 的"传递性"是靠 **HClass 的 `IsJSShared` bit 在分配时驱动空间选择**实现的——对象是否 sendable，取决于它的 HClass 是否在创建时被标为 shared，而 napi 通过 `NewSendableClassFunction` 在类定义阶段就保证了这个 bit 被设上。
