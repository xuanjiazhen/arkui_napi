# constructor vs newTarget 语义

本文解释 `NewJSObjectByConstructor` 的两个参数 `constructor` 与 `newTarget` 的含义，以及它们何时相同、何时不同。

> 相关文档：
> - [01-sendable-architecture-overview.md](./01-sendable-architecture-overview.md)：Sendable 整体架构
> - [03-sendable-class-instantiation-flow.md](./03-sendable-class-instantiation-flow.md)：`NewJSObjectByConstructor` 在实例化流程中的位置

## 核心结论

`newTarget` 在直接 `new Foo()` 时确实等于构造函数，但函数签名设计成两个参数是为了覆盖**继承**场景：`constructor` 是"谁在执行构造逻辑"，`newTarget` 是"`new` 指向谁（决定实例原型/HClass 派生）"。

## 两个参数的含义

| 参数 | 含义 | 来源 |
|---|---|---|
| `constructor` (`JSHandle<JSFunction>`) | **当前正在执行其 `[[Construct]]` 的函数**——即代码体正在运行的那个构造器 | `info->GetFunction()`（`js_function.cpp:735`） |
| `newTarget` (`JSHandle<JSTaggedValue>`) | **`new` 表达式最初指向的目标**，即 ECMAScript 的 `new.target` | `info->GetNewTarget()`（`js_function.cpp:736`） |

`newTarget` 类型是 `JSTaggedValue` 而非 `JSFunction`，因为它可能为 `undefined`（非 `new` 方式调用时）。`Construct` 会在 undefined 时把它补成 `func`（`js_function.cpp:582-585`）：

```cpp
JSHandle<JSTaggedValue> target = info->GetNewTarget();
if (target->IsUndefined()) {
    target = func;                                          // 补成 func
    info->SetNewTarget(target.GetTaggedValue());
}
if (!(func->IsConstructor() && target->IsConstructor())) {
    THROW_TYPE_ERROR_AND_RETURN(thread, "Constructor is false", JSTaggedValue::Exception());
}
```

`Construct` 还会校验 `func->IsConstructor() && target->IsConstructor()`（`:586`），即两者都必须是构造器。

## 何时相同、何时不同

### 直接 `new Base()` → `newTarget === constructor`

`GetInstanceJSHClass` 有专门快路径（`js_function.cpp:983-984`）：

```cpp
JSHandle<JSHClass> ctorInitialJSHClass(thread, JSFunction::GetOrCreateInitialJSHClass(thread, constructor));
if (newTarget.GetTaggedValue() == constructor.GetTaggedValue()) {  // js_function.cpp:983
    return ctorInitialJSHClass;   // 直接返回构造器预存的实例 HClass
}
```

这就是 napi sendable 类 `new MySendableClass()` 的常规情况——返回 `NewSendableClassFunction` 预存在 `ProtoOrHClass` 里的那个 iHClass。

### 继承 `class Derived extends Base` → 两者不同

当 `Derived` 构造器里调 `super()`，Base 的 `[[Construct]]` 被触发：此时
- `constructor` = `Base`（Base 的代码在跑）
- `newTarget` = `Derived`（`new Derived()` 的最初目标）

两者**必须不同**，因为实例的 `[[Prototype]]` 要取自 `newTarget.prototype`（即 `Derived.prototype`），否则继承链就断了。

`GetInstanceJSHClass` 在 `:988-1001` 处理这个分支，调 `GetOrCreateDerivedJSHClass` 基于 newTarget 派生出新的 HClass：

```cpp
if (newTarget->IsJSFunction()) {
    JSHandle<JSFunction> newTargetFunc = JSHandle<JSFunction>::Cast(newTarget);
    if (newTargetFunc->IsDerivedConstructor(thread)) {
        ...
        return GetOrCreateDerivedJSHClass(thread, newTargetFunc, ctorInitialJSHClass);  // :997
    }
}
```

## 为什么对象创建要用 newTarget

`NewJSObjectByConstructor`（`object_factory.cpp:1199`）用 `GetInstanceJSHClass(thread, constructor, newTarget)` 取实例 HClass——注意**两个都传**：
- 从 `constructor` 读它预存的初始 iHClass（`GetOrCreateInitialJSHClass` → `ProtoOrHClass`，`js_function.cpp:196-198`）
- 用 `newTarget` 决定是否要走派生 HClass / 用 newTarget 的 prototype

这是 ECMAScript 规范 `OrdinaryCreateFromConstructor(constructor, newTarget)` 的要求：**实例原型来自 newTarget，但构造逻辑来自 constructor**。两参数正是为了把"谁在执行"和"谁被 new 指向"这两个语义解耦。

## `ConstructInternal` 中 base vs derived 的区别

`ConstructInternal`（`js_function.cpp:730`）只在 base 构造器时创建对象：

```cpp
JSHandle<JSTaggedValue> obj(thread, JSTaggedValue::Undefined());
if (func->IsBase(thread)) {                                 // js_function.cpp:744
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    obj = JSHandle<JSTaggedValue>(factory->NewJSObjectByConstructor(func, newTarget));  // :746
    RETURN_EXCEPTION_IF_ABRUPT_COMPLETION(thread);
}
info->SetThis(obj.GetTaggedValue());                        // :751
```

- **Base 构造器**（如 `class Base` / napi sendable 类）：`IsBase(thread)=true`，自己创建 `this`
- **Derived 构造器**（`class Derived extends Base`）：`IsBase(thread)=false`，不创建对象；`this` 由父类 `super()` 调用 `NewJSObjectByConstructor` 时创建，此时 `constructor=Base`、`newTarget=Derived`

返回值处理（`:762-781`）也呼应：
- base 构造器返回预创建的 `this`（`:766-767`）
- derived 构造器返回 `super()` 创建的对象或自身的返回值（`:770-776`）

## sendable 特有的约束

`NewJSObjectByConstructor`（`object_factory.cpp:1202-1205`）有个只有 sendable 才需要的校验：

```cpp
if (constructor->IsJSShared() && !newTarget->IsJSShared()) {
    THROW_TYPE_ERROR_AND_RETURN(thread_, "shared ctor cannot assign unshared newTarget",
                                JSHandle<JSObject>(thread_, JSTaggedValue::Undefined()));
}
```

sendable 构造器不允许配一个非 sendable 的 newTarget——因为：
- 实例要在共享堆分配、HClass 要带 `IsJSShared`
- 派生 HClass 是从 newTarget 派生的（`GetOrCreateDerivedJSHClass`）
- 若 newTarget 不 shared，派生出的 HClass 就不会带 `IsJSShared`，实例就不是 sendable 了

这个校验从源头保证 sendable 类的实例一定是 sendable，不会出现"sendable 类产生非 sendable 实例"的不一致状态。

## 小结

| 场景 | constructor | newTarget | 走哪条路径 |
|---|---|---|---|
| `new Base()` 直接 new | Base | Base（相同） | 快路径 `:983-984`，返回初始 iHClass |
| `class Derived extends Base` + `super()` | Base | Derived（不同） | `GetOrCreateDerivedJSHClass` `:997`，派生 HClass |
| 非 `new` 调用（如 `Base()`） | Base | undefined→补成 Base | `Construct` `:582-585` 补正后等同直接 new |

napi sendable 类的常规 `new` 走快路径（两者相等、直接返回预存 iHClass），派生 `extends` 才会走分支。`newTarget` 不是"也是构造函数"那么简单——它是 ECMAScript `new.target` 元属性的运行时体现，承担"决定实例原型与派生 HClass"的职责。
