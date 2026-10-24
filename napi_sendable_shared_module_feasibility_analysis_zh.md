# NAPI 模块导出 Sendable 共享模块设计稿

> 分析范围：`foundation/arkui/napi`、`arkcompiler/ets_runtime`
> 文档性质：源码级技术设计与实现规格，用于驱动代码生成
> 核心定义：**共享导出根对象，每个 VM 的模块记录保持本地，先到先得 init**

## 1. 背景与目标

### 1.1 目标

NAPI 模块声明为 sendable 时：

1. native init 回调（`nm_register_func`）**只在一个 VM 跑一次**，产出完整 Sendable 导出根对象。
2. 不同 Worker VM 通过进程级 `SendableGlobal<JSValueRef>` 句柄拿到**同一个**导出根对象。
3. 每个 VM 仍创建自己的本地模块记录——状态机、Namespace、依赖链接保持 VM 本地，不跨 VM 共享。

### 1.2 范围与非范围

**范围**：

- 共享导出根对象身份（跨 VM 同一 `napi_value`）。
- native init 只跑一次（先到先得）。
- Sendable method 在消费方 VM 上运行（运行时已具备）。
- `.so` 固定驻留，root 存活期不卸载。

**非范围**：

- 共享模块记录（跨 VM 同一模块记录）。
- 共享 Namespace（跨 VM 同一命名空间对象）。
- 进程级统一的模块状态机、循环依赖语义、错误状态。

接受这些语义不与 ArkTS `"use shared"` 模块完整对齐，是本设计的明确定位。

## 2. 当前实现分析

### 2.1 NAPI 模块加载现状

入口链（`foundation/arkui/napi`）：

1. `ArkNativeEngine::RequireNapi`（`native_engine/impl/ark/ark_native_engine.cpp:928`）。
2. `CheckAndGetModule`（`:956`）走 `NativeModuleManager::LoadNativeModule` 加载 `.so` / 校验。
3. `ArkNativeEngine::LoadNativeModule`（`:838`）执行模块初始化：
   - `:849` `loadedModules_.find(module)` 查 VM 本地缓存（`std::map<NativeModule*, Global<JSValueRef>>`，`ark_native_engine.h:556`）。
   - `:884` `Local<ObjectRef> exportObj = ObjectRef::New(vm_)` 创建**本地堆普通对象**。
   - `:889` `module->registerCallback(env, exportObj)` 模块填属性，**返回值被忽略**。
   - `:894` `Local<ObjectRef> exportCopy = ObjectRef::New(vm_)` 又一个本地堆对象。
   - `:896` `CheckArkApiAllowList(module, context, exportCopy)` allowlist 过滤拷贝。
   - `:900` `loadedModules_[module] = Global<JSValueRef>(vm_, exports)` 写 VM 本地缓存。

### 2.2 已具备的 Sendable 基础

- `napi_create_sendable_object_with_properties()`（`native_engine/native_api.cpp:1725`）创建 sendable 对象。
- `napi_set_named_property()` 在 sendable 对象上**可用**（`test/unittest/test_sendable_napi.cpp:1876`、`:2603` 跨 env 修改 sendable 属性成功）。因此"先建空 sendable 对象、再逐步塞属性"技术可行（`:2870` 证明 0 属性空 sendable 对象可建）。
- Sendable Native Function 被跨 VM 调用时，运行时动态取当前 VM 的 env（`ark_native_engine.cpp:1221-1226`）：
  ```cpp
  if (info->isSendable) {
      env = reinterpret_cast<napi_env>(JSNApi::GetEnv(vm));
  } else {
      env = info->env;
  }
  ```
  这条机制已经处理好跨 Worker 的 env 切换，本设计不需要再管。

### 2.3 `isMainEnvContext_` 的真实语义（关键）

`IsMainEnvContext()` **不是**"主线程 vs Worker"标记，而是"该 VM 的主 context vs ark 子 context"标记：

| 构造函数 | `isMainEnvContext_` | 谁用它 |
|---|---|---|
| `ArkNativeEngine(EcmaVM* vm, void* jsEngine, bool isLimitedWorker=false)`（`ark_native_engine.cpp:495-498`） | `true` | **主引擎 和 Worker 引擎**（`CreateRuntimeFunc` `:2375` 走这条；测试 `test_ark_idle_monitor.cpp:137-138` 的 `workerEngine1/2` 也走这条） |
| `ArkNativeEngine(NativeEngine* parent, EcmaVM* vm, const Local<JSValueRef>& context)`（`:574-581`） | `false` | **只**被 `ArkNativeEngine::New`（`:665-679`）调用 ← **只**被 `napi_create_ark_context`（`native_api.cpp:4802`）调用 |

铁证——`ark_native_engine.cpp:686`：
```cpp
return !isMainEnvContext_ &&  // Check whether current engine is context engine.
```
注释直接说明：`!isMainEnvContext_` = "是 context engine（ark 子 context）"。ark-context 是主线程特性（`native_api.cpp:4792-4794` "multi-context feature only support main thread"），Worker 不走 ark-context，Worker 引擎 `isMainEnvContext_(true)`。

**结论：Worker 的 `IsMainEnvContext()` 返回 true，能正常调 `napi_create_sendable_object_with_properties`。** `native_api.cpp:1734` 那条 `if (!engine->IsMainEnvContext())` 拦的是 ark 子 context，不是 Worker。这是本设计选择"先到先得、任意 Worker 可 init"的技术前提。

### 2.4 SendableGlobal 与进程级存储

`SendableGlobal<T>`（`arkcompiler/ets_runtime/ecmascript/napi/include/jsnapi_expo.h:553`）：

- 持有节点在进程级 `Runtime::sendableGlobalStorage_`（`runtime.cpp:813` `NewSendableGlobalHandle`），独立于任何 VM。
- `NewSendableGlobalHandle` 会 `ASSERT(value.IsSendable())`——**值必须是 sendable 类型**。
- 被所有共享/CMC GC 标记遍历，跨 VM 存活，VM 销毁后仍可访问。
- 重新具象化：`SendableGlobal::ToLocal(vm)` 在任意 VM 拿回 `Local<T>`。

对比：

- `Local<T>`：随 `EscapeLocalScope`（`ark_native_engine.cpp:848`）返回释放，存不进 map。
- `Global<T>`：绑死在创建 VM 上，VM 销毁时句柄没了，跨不了 VM。
- `SendableGlobal<T>`：进程级，唯一可跨 VM 的载体。

本设计的共享根对象必须用 `SendableGlobal<JSValueRef>` 持有。

## 3. 设计决策（已锁定）

| 项 | 决策 |
|---|---|
| **opt-in 标记** | 推迟到后续在 `napi_module` 结构体加新字段（非 `nm_flags` 位）。当前实现用占位谓词 `IsSharedModule(module)` 返回 `false`；字段落地后回填读取逻辑 |
| **共享载体** | `static std::map<std::string, SharedRootEntry> sharedRootMap_` + `static std::mutex sharedRootIndexMutex_`（仅做索引查找/插入） |
| **Entry 结构** | 见 §4.2，内嵌 `std::mutex`（`std::map` 节点稳定，合法） |
| **init-once 线程模型** | **先到先得**。两段锁：索引锁找/建 entry → entry 自身锁协调 init；**init 不持任何锁** |
| **callback 契约** | 用 `registerCallback` 的**返回值**（当前被忽略，`ark_native_engine.cpp:889-890`），模块作者须返回 `napi_create_sendable_object_with_properties` 构造的完整 sendable root |
| **导出校验** | 返回值非空 + `root->IsJSShared(vm_)` 才写入 map，否则记 `errored` |
| **allowlist** | sendable 分支**跳过** `CheckArkApiAllowList`（`:894-897`）和 `CopyPropertyApiFilter`（`:467-493`），避免本地堆拷贝破坏 sendable |
| **VM-local cache** | sendable 分支**不用** `loadedModules_`（`Global` 是 VM-local），改写进程级 static map |
| **`.so` 生命周期** | **固定驻留**：root 创建时 `MarkSharedSoPinned(key)`，永不 unpin |
| **模块作者契约** | 不缓存 `napi_env`/线程状态（运行时已处理 sendable function 的 env 切换，见 §2.2） |
| **canonical key** | 首版用 `moduleName`，后续加固 bundle/namespace 维度（见 §8.1） |
| **`SetNativeEngine`** | 见 §8.2，实现时验证处理 |

## 4. 接口与实现设计

### 4.1 Native 初始化回调契约

sendable 模块的 `registerCallback` 必须返回一个完整构造好的 Sendable Root，不再使用"loader 建空对象、callback 逐项赋值"模式。模块作者示例：

```cpp
napi_value InitSharedModule(napi_env env, napi_value /*unused*/)
{
    napi_property_descriptor properties[] = {
        // value 必须是 sendable；method 会被创建为 Sendable Function
    };
    napi_value exports = nullptr;
    napi_status status = napi_create_sendable_object_with_properties(
        env, std::size(properties), properties, &exports);
    if (status != napi_ok) {
        return nullptr;  // loader 见 nullptr 记 errored
    }
    return exports;  // loader 用返回值
}
```

loader 校验顺序：

1. `registerCallback` 返回值非空、非 `undefined`。
2. 返回值通过 `root->IsJSShared(vm_)`（或公共 `napi_is_sendable()`）校验为 sendable 类型。
3. 普通 Object / 普通 Function / LocalHeap 引用一律报错，记 `errored`。
4. 失败状态在 `SharedRootEntry::errored` 中对所有 VM 一致可见。

### 4.2 SharedRootEntry 数据结构

`native_engine/impl/ark/ark_native_engine.h` 新增：

```cpp
struct SharedRootEntry {
    panda::SendableGlobal<panda::JSValueRef> root;  // 进程级句柄，空直到 ready
    std::mutex mtx;                                  // 协调 init-once
    std::condition_variable cv;                     // 等待 init 完成
    std::atomic<bool> initializing{false};          // 是否有 VM 正在 init
    std::atomic<bool> errored{false};               // init 是否失败
};
```

静态成员：

```cpp
static std::mutex sharedRootIndexMutex_;                      // 仅做 map 索引锁
static std::map<std::string, SharedRootEntry> sharedRootMap_;  // 进程级共享根表
```

注：`std::map` 节点稳定（插入/删除不搬移已有节点），内嵌不可拷贝的 `std::mutex`/`std::condition_variable` 合法。访问 entry 前先在 `sharedRootIndexMutex_` 下取得 entry 引用，引用在 entry 被删除前不失效。

### 4.3 LoadSharedNativeModule 流程

`ark_native_engine.cpp` 新增方法，核心两段锁算法：

```
LoadSharedNativeModule(moduleManager, module, key):
  // 第一段：索引锁——找/建 entry
  SharedRootEntry& entry;
  {
    lock_guard lock(sharedRootIndexMutex_);
    entry = sharedRootMap_[key];   // 不存在则插入新 entry（root 空，flags false）
  }
  // 索引锁已释放

  // 第二段：entry 锁——协调 init-once
  {
    unique_lock lock(entry.mtx);
    if (entry.root 已就绪) { return entry.root.ToLocal(vm_); }
    while (entry.initializing) {
      entry.cv.wait(lock);
    }
    // 被唤醒后重查
    if (entry.root 已就绪) { return entry.root.ToLocal(vm_); }
    if (entry.errored) { HILOG_ERROR(...); return Local<>(); /* 空 exports */ }
    entry.initializing = true;
  }
  // entry 锁已释放——init 不持任何锁

  // 第三段：执行 init（可 GC / 可慢，不影响其它 Worker 在 entry 锁上等待以外的路径）
  Local<JSValueRef> root = CreateSendableRoot(module);
  bool ok = !root.IsEmpty() && root->IsJSShared(vm_);

  // 第四段：写回 entry
  {
    unique_lock lock(entry.mtx);
    if (ok) {
      entry.root = SendableGlobal<JSValueRef>(vm_, root);
      moduleManager->MarkSharedSoPinned(key);  // pin .so
    } else {
      entry.errored = true;
      HILOG_ERROR("shared module %{public}s init failed", key.c_str());
    }
    entry.initializing = false;
    entry.cv.notify_all();
  }

  return ok ? entry.root.ToLocal(vm_) : Local<>();
```

### 4.4 CreateSendableRoot

```cpp
Local<JSValueRef> ArkNativeEngine::CreateSendableRoot(NativeModule* module)
{
    if (module->registerCallback == nullptr) {
        return Local<JSValueRef>();
    }
    Local<JSValueRef> undef = JSValueRef::Undefined(vm_);
    napi_value cbResult = module->registerCallback(
        reinterpret_cast<napi_env>(this), JsValueFromLocalValue(undef));
    if (cbResult == nullptr) {
        HILOG_ERROR("shared module %{public}s callback returned null", module->name);
        return Local<JSValueRef>();
    }
    Local<JSValueRef> root = LocalValueFromJsValue(cbResult);
    if (!root->IsJSShared(vm_)) {
        HILOG_ERROR("shared module %{public}s callback returned non-sendable root", module->name);
        return Local<JSValueRef>();
    }
    return root;
}
```

### 4.5 LoadNativeModule 分支接入

`ark_native_engine.cpp:838` 的 `LoadNativeModule`，在 `loadedModules_` 查询之后、`strModuleName` 构造之后插入 sendable 分支：

```cpp
Local<JSValueRef> ArkNativeEngine::LoadNativeModule(
    NativeModuleManager* moduleManager, Local<StringRef> &moduleName,
    NativeModule* module, Local<JSValueRef> exports, std::string &errInfo)
{
    if (module == nullptr) { return exports; }
    panda::EscapeLocalScope scope(vm_);
    auto it = loadedModules_.find(module);
    if (it != loadedModules_.end()) {
        return scope.Escape(it->second.ToLocal(vm_));
    }
    std::string strModuleName = moduleName->ToString(vm_);
    moduleManager->SetNativeEngine(strModuleName, this);
    MoudleNameLocker nameLocker(strModuleName);

    // ===== sendable 分支：early return，不走下面的本地路径 =====
    if (IsSharedModule(module)) {
        return scope.Escape(LoadSharedNativeModule(moduleManager, module, strModuleName));
    }
    // ===== 以下原逻辑（jsABCCode / registerCallback / allowlist / loadedModules_）保持不变 =====

    if (module->jsCode == nullptr && module->getABCCode != nullptr) { ... }
    ...
}
```

sendable 分支**必须跳过**：

- `:884` `ObjectRef::New(vm_)` 本地 exports 创建。
- `:889-890` 调 `registerCallback` 并忽略返回值（改为 `CreateSendableRoot` 用返回值）。
- `:894-897` `CheckArkApiAllowList` + `exportCopy` 本地堆拷贝。
- `:900` `loadedModules_[module] = Global<...>`（VM-local）。

### 4.6 IsSharedModule 占位谓词

```cpp
// opt-in 字段落地前返回 false；字段加入 napi_module 后改为读字段
bool ArkNativeEngine::IsSharedModule(NativeModule* module) const
{
    if (module == nullptr || module->napiModulePtr == nullptr) {
        return false;
    }
    // TODO: opt-in 字段落地后：return module->napiModulePtr->nm_<新字段> != 0;
    return false;
}
```

`module->napiModulePtr`（`NativeModule` 的 `napi_module*` 字段）的透传见 §5.3。

## 5. 实现步骤（按文件）

### 5.1 `native_engine/impl/ark/ark_native_engine.h`

- 新增 §4.2 的 `SharedRootEntry` 结构体。
- 新增静态成员：`sharedRootIndexMutex_`、`sharedRootMap_`。
- 新增方法声明：`LoadSharedNativeModule`、`CreateSendableRoot`、`IsSharedModule`。
- 新增辅助：`SharedRootEntry& GetOrCreateSharedRootEntry(const std::string& key);`（索引锁下找/建）。

### 5.2 `native_engine/impl/ark/ark_native_engine.cpp`

- 实现 §4.3 `LoadSharedNativeModule`、§4.4 `CreateSendableRoot`、§4.6 `IsSharedModule`。
- 在 `LoadNativeModule`（`:838`）按 §4.5 插入 sendable 分支。
- 包含必要头：`<condition_variable>`、`<chrono>`、`<unordered_set>`（如本文件未包含）。

### 5.3 `module_manager/native_module_manager.h` / `.cpp`

`NativeModule` 结构体（`native_module_manager.h` 约 `:60`）新增字段：

```cpp
napi_module* napiModulePtr = nullptr;
```

新增 `.so` pin 机制：

```cpp
// .h
void MarkSharedSoPinned(const std::string& moduleKey);
bool IsSharedSoPinned(const std::string& moduleKey);
private:
    std::mutex sharedSoPinMutex_;
    std::unordered_set<std::string> pinnedSharedSo_;
```

```cpp
// .cpp
void NativeModuleManager::MarkSharedSoPinned(const std::string& moduleKey)
{
    std::lock_guard<std::mutex> lock(sharedSoPinMutex_);
    pinnedSharedSo_.insert(moduleKey);
}

bool NativeModuleManager::IsSharedSoPinned(const std::string& moduleKey)
{
    std::lock_guard<std::mutex> lock(sharedSoPinMutex_);
    return pinnedSharedSo_.find(moduleKey) != pinnedSharedSo_.end();
}
```

`UnloadNativeModule`（`native_module_manager.cpp:263` 附近）开头加跳过：

```cpp
bool NativeModuleManager::UnloadNativeModule(const std::string& moduleKey)
{
    ...
    if (IsSharedSoPinned(moduleKey)) {
        MODULEMNG_HILOG_INFO("module:'%{public}s' pinned by shared root, skip dlclose",
                             moduleKey.c_str());
        return true;
    }
    return UnloadModuleLibrary(handle);
}
```

`Register`（`:348-370` 附近）head/tail 节点复制时透传 `napiModulePtr`：

```cpp
tailNativeModule_->napiModulePtr = nativeModule->napiModulePtr;
headNativeModule_->napiModulePtr = nativeModule->napiModulePtr;
```

### 5.4 `native_engine/native_node_api.cpp`

`napi_module_register`（`:39` 附近）透传 `napi_module*` 指针，供 `IsSharedModule` 延迟读取 opt-in 字段（老 `.so` 二进制兼容——不要求 `napi_module` 结构体立刻有新字段）：

```cpp
NAPI_EXTERN void napi_module_register(napi_module* mod)
{
    ...
    module.flags = mod->nm_flags;
    module.napiModulePtr = mod;          // 新增
    module.registerCallback = (RegisterCallback)mod->nm_register_func;
    ...
}
```

### 5.5 opt-in 字段落地（后续单独变更）

opt-in 字段加入 `napi_module` 时需同步：

- `interfaces/kits/napi/native_api.h` 或 `interfaces/inner_api/napi/native_node_api.h`：加字段。
- `napi_module_with_js`（`native_node_api.h:35-44`）同步加字段 + `napi_module_with_js_register` 透传。
- `IsSharedModule` 改为读字段。
- `ace_napi.versionscript` 同步（公共 API 改动）。
- 若复用 `nm_flags` 而非新字段，须把 App Module 判等（`native_module_manager.cpp:45` `IS_APP_MODULE_FLAGS = 100`、`:327` `flags == 100`）改为位与，并补齐 flags 持久化。当前决策是**加新字段，不动 flags**。

### 5.6 构建配置

- 本变更只改现有文件，不新增 .cpp，无需改 `napi.gni` 的 `napi_sources`。
- opt-in 字段落地时（§5.5）才需同步 `ace_napi.versionscript`。

## 6. 并发与生命周期约束

### 6.1 init-once 状态机

`SharedRootEntry` 的有效状态：

```
未初始化 (root 空, initializing=false, errored=false)
   │ 首个 VM 抢到
   ↓
初始化中 (initializing=true)  ──── 其它 VM 在 entry.cv 上 wait
   │
   ├─ 成功 → 就绪 (root 非空, initializing=false)
   └─ 失败 → 错误 (errored=true, initializing=false)
```

- 多 Worker 并发首次 import：首个 VM 置 `initializing=true` 跑 init，其余 VM `cv.wait`。
- init 完成后 `notify_all` 唤醒所有等待者，重查 root/errored。
- **init 不持 entry 锁**：避免 `registerCallback` 进 VM 触发 GC / 重入 `RequireNapi` 时死锁。
- 不同模块的 entry 互不影响（各自 entry 锁）。
- 同模块递归自导入：`initializing=true` 期间同线程重入，按"返回空 exports + HILOG_ERROR"处理（NAPI 模块罕见，首版不深度处理）。

### 6.2 Native 状态契约（模块作者）

sendable function 可在不同 Worker 线程调用，模块 native 状态必须线程安全。模块作者**禁止**：

- 保存创建 VM 专属的 `napi_env`（如 `static napi_env g_env;`）并在其它 Worker 回调里使用。
- 假设 init 跑在固定线程并依赖 thread-local。
- 保存普通 `napi_ref` 或 LocalHeap 对象跨 Worker。
- 未同步访问 native 静态全局变量。
- finalizer 依赖已销毁的 VM/env。

**正确做法**：每个回调用 `napi_get_cb_info` 传入的 `env`（sendable function 回调里它已是当前 VM 的 env，见 §2.2 `JSNApi::GetEnv(vm)`）。运行时已替模块作者处理好跨 VM env 切换，模块作者只需不缓存。

### 6.3 动态库生命周期

sendable root 里的 Sendable Function 指向 `.so` 里的 native 回调地址。root 存活时若 `dlclose`，函数地址悬空 → 崩溃。

采用**固定驻留**语义：

- root 创建成功时 `MarkSharedSoPinned(key)`。
- 之后 `UnloadNativeModule` 见 `IsSharedSoPinned` 直接返回 true，跳过 `dlclose`。
- 永不 unpin（直到最后一个 VM 销毁 / 进程结束）。

代价：`.so` 常驻内存。可接受，换取简单且无悬空风险。

## 7. allowlist 处理

### 7.1 allowlist 现状

`ArkNativeEngine::CheckArkApiAllowList`（`ark_native_engine.cpp:453-465`）+ `CopyPropertyApiFilter`（`:467-493`）：

- `module_checker_delegate.h:21` `typedef std::function<bool(const std::string&)> ApiAllowListChecker;` —— 点分 API 路径 → 是否允许。
- 注入：`ModuleLoadChecker::CheckModuleLoadable`（`module_load_checker.cpp:20-30`）通过 `ModuleCheckerDelegate`（外部策略委托）取 checker → `NativeModuleManager::MoveApiAllowListCheckerPtr`（`native_module_manager.cpp:830-834`）move 到 `NativeModule::apiAllowListChecker`。**按模块注入，策略在宿主，不在 Manager 硬编码**。
- 执行：遍历 `exportObj` 属性，拼点分路径，查白名单，只把允许的属性拷到新的 `ObjectRef::New(vm_)`（`:479`/`:894`）。

### 7.2 对 sendable 模块的影响

`apiAllowListChecker` 默认 `nullptr`（`module_load_checker.cpp:24`）。是否非空**完全由宿主按模块名决定**，与模块是否 sendable 无关：

- sendable 模块若未被宿主 gating → checker 为 nullptr → `CheckArkApiAllowList`（`:457` `if (apiAllowListChecker != nullptr)`）直接跳过 → 不拷贝 → sendable root 不受影响。
- sendable 模块若被宿主 gating（如 `@ohos.xxx` 系统 module）→ `CheckArkApiAllowList` 会跑 → `CopyPropertyApiFilter` 拷到本地堆 `ObjectRef::New` → sendable 身份破坏。

### 7.3 处理决策

sendable 分支**无条件跳过** `CheckArkApiAllowList` 和 `CopyPropertyApiFilter`：

- sendable 模块的 API 暴露面由模块作者在构造 sendable root 时自决（descriptor 层面）。
- 事后拷贝过滤与 sendable 语义不兼容（拷贝产物是本地堆）。
- 若将来宿主需要对 sendable 系统 module 做 API 控制，应改为"构造前过滤 descriptor"，作为后续 follow-up，不阻塞当前实现。

在 §4.5 的 sendable 分支中，`early return` 天然绕过 `:894-897` 这段，无需额外改 `CheckArkApiAllowList` 自身。

## 8. 开放项与后续

### 8.1 canonical key 维度（首版用 moduleName）

首版 `sharedRootMap_` 的 key 用 `strModuleName`。风险：不同 bundle / HSP 的同名模块会错误共享。

后续加固方向：key 加入 bundle id / namespace / 加载域信息，避免跨应用同名模块误共享。属后续 follow-up，不阻塞首版。

### 8.2 `SetNativeEngine` 验证项

`ark_native_engine.cpp:854` `moduleManager->SetNativeEngine(strModuleName, this)` 把 module 绑到当前 engine（VM-local）。sendable 模块跨 VM 使用，此绑定是否合适需在实现时验证：

- 可能需在 sendable 分支跳过，或每个 VM 各自设。
- 不影响导出根共享（共享的是 `SendableGlobal` 句柄，不是 engine 绑定）。

### 8.3 opt-in 字段形状

推迟。落地时为 `napi_module` 加新字段（如 `nm_shared` 或类似），并同步 §5.5 列出的全部点位。当前实现以占位谓词 `IsSharedModule` 返回 `false` 暂时不启用 sendable 分支，待字段落地后回填。

### 8.4 与现有补丁的关系

仓内已有补丁 `0001-feat-support-sendable-shared-NAPI-module-export-root.patch`，其 `LoadNativeModule` sendable 分支采用"main-env only + `napi_send_event` 投递 + condvar 等 5s 超时"。本设计改用"先到先得、任意 Worker init、两段锁、init 不持锁"。两者 `LoadNativeModule` sendable 分支冲突。

实现时建议**重新实现该分支**（按本设计 §4.3–§4.5），不复用补丁的 main-env + 超时逻辑。`.so` pin（§5.3）和 `napiModulePtr` 透传（§5.3–§5.4）与补丁一致，可直接采用。

## 9. 验证方案

### 9.1 功能验证

- 单 Worker：sendable 模块 init 一次、exports 属性可访问。
- 多 Worker：两 Worker `requireNapi` 同一 sendable 模块，`napi_strict_equals` 断言拿到同一对象；native init 计数 == 1。
- sendable function env 切换：Worker B 调 Worker A 创建的 sendable method，回调内 `napi_env` 是 B 的（`JSNApi::GetEnv(vm)`）。
- 覆盖 Sendable Object、Class、Array、Map、ArrayBuffer、TypedArray 导出值。
- 覆盖 default import、named import、动态 import、re-export。

### 9.2 异常验证

- `registerCallback` 返回普通 Object → `errored`。
- `registerCallback` 返回普通 Function → `errored`。
- `registerCallback` 返回空值 / `undefined` → `errored`。
- Sendable root 属性包含 LocalHeap 对象 → 构造侧 `napi_create_sendable_object_with_properties` 已拦。
- `registerCallback` 抛异常 → `errored`。
- 并发初始化期间一个 Worker 销毁 → 不崩溃（`SendableGlobal` 跨 VM 存活）。

### 9.3 生命周期验证

- 一个 VM 销毁后，其它 VM 继续调用导出函数。
- root 存活时 `UnloadNativeModule` 不 `dlclose`（`IsSharedSoPinned` 生效）。
- 模块卸载 / 重新注册不形成 callback 悬空。

### 9.4 兼容性验证

- 普通 NAPI 模块行为不变（`IsSharedModule` 默认 false，不进 sendable 分支）。
- APP / OHOS / INTERNAL / STATIC 模块类型不受影响。
- `napi_module_with_js` JS/ABC 混合模块不受影响。
- allowlist 对普通模块仍生效（sendable 分支才跳过）。

### 9.5 回归测试

- `test/unittest/test_sendable_napi.cpp`、`test/unittest/test_napi.cpp`、`test/unittest/test_napi_context.cpp`。
- 模块管理 `module_manager/test/unittest/module_manager_test/` + Fuzz `test/fuzztest/loadarkmodule_fuzzer/`。
- 多 Worker 场景需补充板侧证据。

## 10. 遵守的约束（AGENTS.md）

- 不在 `nm_register_func` 做阻塞 / 重初始化——init 仍只跑一次，且不持锁跑。
- `.so` 不绕过 `napi_module_register` 加载。
- 不在 `ModuleManager` 持有模块业务对象——static map 持的是 `SendableGlobal` 句柄，非业务对象。
- 不跨线程传 `napi_env`——sendable function 运行时动态取 env。
- 不为通过测试删日志 / 错误码 / DFX 信息。
- 公共 API 改动（opt-in 字段落地时）同步 `ace_napi.versionscript`。
- 新增 .cpp 须入 `napi_sources`（本变更不新增 .cpp）。

## 11. 实现顺序建议

1. §5.3 `NativeModuleManager` 加 `napiModulePtr` 字段 + `.so` pin 机制 + `Register` 透传。
2. §5.4 `napi_module_register` 透传 `napiModulePtr`。
3. §5.1 `ark_native_engine.h` 加 `SharedRootEntry` + 静态成员 + 方法声明。
4. §5.2 `ark_native_engine.cpp` 实现 `LoadSharedNativeModule` / `CreateSendableRoot` / `IsSharedModule` + `LoadNativeModule` 分支接入。
5. arm64 构建 ace_napi 验证编译。
6. §9 验证项逐项跑。
7. opt-in 字段落地（§5.5 / §8.3）作为后续独立变更。
