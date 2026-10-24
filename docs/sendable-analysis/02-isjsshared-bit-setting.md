# IsJSShared bit 的设置机制

本文回答：调用 `NewSharedOldSpaceJSObject` 创建共享堆对象时，`IsJSShared` bit 是如何、在何时被设上的。

> 相关文档：
> - [01-sendable-architecture-overview.md](./01-sendable-architecture-overview.md)：Sendable 整体架构
> - [03-sendable-class-instantiation-flow.md](./03-sendable-class-instantiation-flow.md)：实例化流程（本文是其中"HClass 创建"环节的展开）

## 核心结论

**`NewSharedOldSpaceJSObject` 本身不设置 `IsJSShared`**——它只是接收一个已经带好该 bit 的 `hclass` 参数并据此在共享堆上分配对象。bit 的设置发生在更早的 **HClass 创建阶段**，由 JSType 驱动。

证据：`NewSharedOldSpaceJSObject`（`shared_object_factory.cpp:370`）调用 `NewSharedOldSpaceObject`（`:349`）→ `sHeap_->AllocateOldOrHugeObject(thread_, hclass)`（`:352`），全程只是消费传入的 `hclass`，没有任何 `SetIsJSShared` 调用。

## 完整调用链

```
NewSendableClassFunction (jsnapi_expo.cpp:4120)
  └─ CreateSPrototypeHClass / CreateSConstructorHClass / CreateSHClass
     (js_hclass.cpp:1783,1775,1743)
      └─ factory->NewSEcmaHClass(SIZE, JSType::JS_SHARED_FUNCTION/OBJECT, length)  // js_hclass.cpp:1764
         └─ NewSEcmaHClass(hclass, size, type, inlinedProps)   // shared_object_factory.cpp:115
            ├─ sHeap_->AllocateNonMovableOrHugeObject(...)     // :120  ← HClass 本身分配在共享堆
            └─ newClass->Initialize(thread_, size, type, inlinedProps,
                                     GetHandledEmptySLayoutInfo())  // :121
               └─ JSHClass::Initialize (shared 版, js_hclass.cpp:207)
                  ├─ InitializeWithDefaultValue(...)           // :210
                  ├─ SetLayout(...)                             // :212
                  └─ if (IsJSTypeShared(type)) {                // :214  ← 关键判定
                        SetIsJSShared(true);                    // :215  ← bit 在这里设
                     }
```

## 两个关键点

### 1. bit 由 JSType 触发设置

`IsJSTypeShared(type)`（`js_hclass.cpp:160-194`）枚举了所有共享类型：

```cpp
bool JSHClass::IsJSTypeShared(JSType type)
{
    bool isShared = false;
    switch (type) {
        case JSType::JS_SHARED_OBJECT:
        case JSType::JS_SHARED_FUNCTION:
        case JSType::JS_SHARED_ASYNC_FUNCTION:
        case JSType::JS_API_BITVECTOR:
        case JSType::JS_SHARED_SET:
        case JSType::JS_SHARED_MAP:
        case JSType::JS_SHARED_ARRAY:
        case JSType::JS_SHARED_TYPED_ARRAY:
        case JSType::JS_SHARED_INT8_ARRAY:
        ... // 各 JS_SHARED_*_ARRAY
        case JSType::JS_SENDABLE_ARRAY_BUFFER:
        case JSType::BIGINT:
        case JSType::LINE_STRING:
        case JSType::SLICED_STRING:
        case JSType::TREE_STRING:
        case JSType::CACHED_EXTERNAL_STRING:
            isShared = true;
            break;
        default:
            break;
    }
    return isShared;
}
```

传给 `NewSEcmaHClass` 的 `JSType::JS_SHARED_FUNCTION`/`JS_SHARED_OBJECT` 命中此列表，所以 `SetIsJSShared(true)` 被调用。

### 2. 设置目标与机制

`SetIsJSShared` 写的是 **HClass 对象自己的 BitField**（不是它描述的 JS 对象的某个字段）：

```cpp
inline void SetIsJSShared(bool flag) const
{
    IsJSSharedBit::Set<uint32_t>(flag, GetBitFieldAddr());  // js_hclass.h:1168
}
```

`IsJSSharedBit` 是 BitField 里的一个标志位（`js_hclass.h:406`，注释标 `// 28`，即第 28 位）。之后 `JSTaggedValue::IsSendable`（`js_tagged_value-inl.h:1494`）→ `IsJSShared()`（`:1375`）→ `GetClass()->IsJSShared()`（`js_hclass.h:1160`）就能 decode 出该位。

## 与普通 Initialize 的对比

非共享路径走的是 `JSHClass::Initialize` 的另一个重载（`js_hclass.cpp:198`）：

```cpp
// 普通版
void JSHClass::Initialize(const JSThread *thread, uint32_t size, JSType type, uint32_t inlinedProps)
{
    InitializeWithDefaultValue(thread, size, type, inlinedProps);
    if (IsJSTypeObject(type)) {
        SetLayout(thread, thread->GlobalConstants()->GetEmptyLayoutInfo());
    }
    // ← 没有 SetIsJSShared
}
```

```cpp
// 共享堆版
void JSHClass::Initialize(const JSThread *thread, uint32_t size, JSType type,
    uint32_t inlinedProps, const JSHandle<JSTaggedValue> &layout)  // js_hclass.cpp:207
{
    InitializeWithDefaultValue(thread, size, type, inlinedProps);
    if (IsJSTypeObject(type)) {
        SetLayout(thread, layout);
    }
    if (IsJSTypeShared(type)) {       // :214  关键判定
        SetIsJSShared(true);          // :215
    }
}
```

非共享重载**没有** `if (IsJSTypeShared(type)) SetIsJSShared(true)` 这段——这就是为什么只有共享 HClass 才带这个 bit。判定的唯一依据就是构造时传入的 `JSType`。

`NewSEcmaHClass`（`shared_object_factory.cpp:115`）调用的是带 `layout` 参数的 5 参重载（`:121` 传 `GetHandledEmptySLayoutInfo()`），即共享堆版的 `Initialize`；而普通的 `NewEcmaHClass`（`object_factory.cpp`）调用 4 参重载（非共享版）。这就是分支的入口差异。

## HClass 创建的三个工厂函数

`NewSendableClassFunction`（`jsnapi_expo.cpp:4120`）调用三个 HClass 工厂，它们都最终走 `CreateSHClass`：

| 工厂 | 入口 | 用途 | 额外标记 |
|---|---|---|---|
| `CreateSPrototypeHClass` | `js_hclass.cpp:1783` | prototype 的 HClass | `SetClassPrototype(true)`、`SetIsPrototype(true)`（:1786-1787） |
| `CreateSConstructorHClass` | `js_hclass.cpp:1775` | constructor 的 HClass | `SetClassConstructor(true)`、`SetConstructor(true)`（:1778-1779） |
| `CreateSHClass` | `js_hclass.cpp:1743` | 实例 HClass（iHClass） | 无额外标记 |

三者都调 `CreateSHClass`（`:1743`）→ `factory->NewSEcmaHClass(...)`（`:1764`），区别在传入的 `JSType`（`JS_SHARED_FUNCTION` for isFunction，`JS_SHARED_OBJECT` 否则）和 `isFunction` 标志：

```cpp
JSHandle<JSHClass> hclass =
    isFunction ? factory->NewSEcmaHClass(JSSharedFunction::SIZE, JSType::JS_SHARED_FUNCTION, length)
               : factory->NewSEcmaHClass(JSSharedObject::SIZE, JSType::JS_SHARED_OBJECT, length);
```

## 其它 SetIsJSShared 设位点

除了 `CreateSHClass` 主路径（经 `Initialize`），还有几处直接补设，保证进入共享堆的 HClass 必带 `IsJSShared`：

| 调用点 | 文件:行 | 场景 |
|---|---|---|
| `NewSEcmaReadOnlySharedHClass` | `shared_object_factory.cpp:187` | 只读共享 HClass，显式补设 |
| `NewSObjectWithAccessor` 路径 | `shared_object_factory.cpp:204` | accessor 场景对 newClass 补设 |
| `InitHClassInCompositeBaseClass` | `shared_object_factory.cpp:204` | string 的 BaseClass |
| 测试 | `tests/layout_info_test.cpp:221` | 测试直接设 |

## 判定链回顾

设好 bit 后，整条判定链是：

```
JSTaggedValue::IsSendable()                            // js_tagged_value-inl.h:1494
  └─ IsJSShared()                                      // :1375
       └─ IsHeapObject() && GetTaggedObject()->GetClass()->IsJSShared()  // js_hclass.h:1160
            └─ IsJSSharedBit::Decode(bits)              // js_hclass.h:1163  ← 读第 28 位
```

这条链是 `SendableGlobal` 准入校验（`runtime.cpp:816`、`jsnapi_expo.cpp:7325`）和 `IsInSharedHeap()`（`js_hclass.h:1155`）的基础。

## 小结

| 阶段 | 关键动作 | 位置 |
|---|---|---|
| HClass 创建 | 建 `JS_SHARED_OBJECT`/`JS_SHARED_FUNCTION` 类型 HClass | `js_hclass.cpp:1764` |
| HClass 初始化 | `IsJSTypeShared(type)` → `SetIsJSShared(true)` | `js_hclass.cpp:214-215` |
| bit 存储 | 写 HClass 自己的 BitField 第 28 位 | `js_hclass.h:1168` |
| 对象分配 | `NewSharedOldSpaceJSObject` 消费已带 bit 的 hclass | `shared_object_factory.cpp:370` |

对象是否 sendable，取决于它的 HClass 是否在创建时被标为 shared；而 bit 的设置发生在 HClass 的 `Initialize`（共享堆版），由 `JSType` 是否属于共享类型集合驱动。napi 通过 `NewSendableClassFunction`/`NewSendable` 在类/函数定义阶段传入 `JS_SHARED_*` 类型，从而保证了这个 bit 被设上。
