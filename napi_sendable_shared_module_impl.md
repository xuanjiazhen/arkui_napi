# NAPI Sendable 共享模块实现说明

> 本文件简述当前实现逻辑、关键决策点及选型理由。详细可行性分析见 `napi_sendable_shared_module_feasibility_analysis_zh_2.md`。

## 1. 实现目标

NAPI 模块经 `napi_shared_module_register` 注册为共享模块时：

- native init 回调（`nm_register_func`）**进程内只跑一次**，返回完整 Sendable 导出根。
- 不同 Worker VM 通过进程级 `SendableGlobal` 句柄拿到**同一个**导出根对象。
- 每个 VM 的模块记录仍保持本地（不共享模块记录/Namespace/状态），**不改动 `ets_runtime`**。

## 2. 实现逻辑

### 2.1 opt-in：`napi_shared_module_register`（新增公共 API 函数）

新增公共 API 函数 `napi_shared_module_register`（`interfaces/kits/napi/native_api.h` 声明、`native_engine/native_api.cpp` 实现）。模块作者在 `.so` constructor 调 `napi_shared_module_register(&mod)` 即声明共享（内部置 `NativeModule.shared = 1`），取代 `napi_module_register`。

老 `napi_module_register` **不读** `napi_module` 任何新字段、`shared` 保持 `0` → 普通路径。老 `.so` 只调 `napi_module_register` → 二进制兼容由 API 选择保证，**无需版本门控**，不改 `napi_module` 结构体。

`Register`（`native_module_manager.cpp`）head/tail 节点复制 `shared`。`IsSharedModule`（`ark_native_engine.cpp`）读 `module->shared != 0` 门控 sendable 分支。

### 2.2 `.so` 防卸载

`NativeModuleManager::IsSharedModule(moduleKey)`（`native_module_manager.cpp`）持 `nativeModuleListMutex_` 遍历链表，按名找到节点后返回 `module->shared != 0`。`UnloadNativeModule` 开头调用：是 shared → `return true` 跳过 `dlclose`。无需额外 pin set——复用已有 `shared` 字段，注册即保护。

### 2.3 共享根表 + 单锁

`ark_native_engine.h` 新增：

```cpp
static std::mutex sharedRootMutex_;
static std::map<NativeModule*, panda::SendableGlobal<JSValueRef>> sharedRootMap_;
```

map key 用 `NativeModule*`（和 `loadedModules_` 一致，shared module 无释放流程，指针不悬空）。值类型直接 `SendableGlobal<JSValueRef>`（用 `try_emplace` 原地构造，绕开 `SendableGlobal` 禁拷禁移）。

`LoadSharedNativeModule`（`ark_native_engine.cpp`）：

```cpp
std::lock_guard<std::mutex> lock(sharedRootMutex_);
auto it = sharedRootMap_.find(module);
if (it != sharedRootMap_.end()) {
    return it->second.ToLocal(vm_);                    // 已有 → 返回
}
Local<JSValueRef> newRoot = CreateSendableRoot(module);
bool ok = !newRoot.IsEmpty() && newRoot->IsJSShared(vm_);
if (ok) {
    auto result = sharedRootMap_.try_emplace(module, vm_, newRoot);
    return result.first->second.ToLocal(vm_);
}
HILOG_ERROR("shared module %{public}s init failed", key.c_str());
return JSValueRef::Undefined(vm_);
```

**一把 `lock_guard` 持锁全程**，无 cv、无 atomics、无 unlock/relock。init 失败不入 map（`find` 不到 → 下次重试，和普通模块一致）。

`LoadNativeModule` 在 `MoudleNameLocker` 后插入 `if (IsSharedModule(module)) return LoadSharedNativeModule(...)` early return，跳过本地堆导出/allowlist/`loadedModules_`。

## 3. 决策点与理由

| 决策 | 选择 | 理由 |
|---|---|---|
| opt-in 方式 | 新增公共 API 函数 `napi_shared_module_register` | opt-in 由"调哪个注册函数"决定，不改 `napi_module` 结构体、不读新字段；老 `.so` 只调 `napi_module_register`→普通路径，二进制兼容由 API 选择保证 |
| map key | `NativeModule*`（非 string） | 和 `loadedModules_` 一致；shared module 无释放流程（pin 在 `UnloadNativeModule` 拦住），指针不悬空 |
| map value | 直接 `SendableGlobal<JSValueRef>`（非 `unique_ptr` 包装） | `try_emplace` 原地构造，不拷不移，绕开 `SendableGlobal` 禁拷禁移 |
| 并发协调 | 单大锁 `sharedRootMutex_`，持锁全程 | 模块加载频率低，串行可接受；代码极简（无 cv/atomics/两段锁）；init 持锁，`registerCallback` 禁止重入 `requireNapi`（规格约束） |
| `.so` 防卸载 | `IsSharedModule(moduleKey)` 遍历链表查 `shared` | 复用已有 `shared` 字段，无需额外 pin set/mutex/方法 |
| init 失败 | 不入 map，下次重试 | 和普通模块行为一致 |
| callback 契约 | 传 `undefined`，消费返回值 | 返回值是唯一权威；`undefined` 占位让"忘记构造 root"在 `IsJSShared` 校验暴露 |
| allowlist | sendable 分支 early return 跳过 | `CopyPropertyApiFilter` 拷到本地堆 `ObjectRef::New`，破坏 sendable 身份 |
| `ets_runtime` | 不改动 | 模块记录保持本地 `UNSENDABLE_MODULE`，不触发 `StoreModuleValue` 共享校验 |
| canonical key | 首版 `moduleName`（pin 用） / `NativeModule*`（map 用） | 首版简化；跨 bundle/HSP 同名误共享为已知风险，后续加固 |
| `ace_napi.versionscript` | 不改 | `global: *` 通配导出新符号 `napi_shared_module_register` |

## 4. 模块作者契约

sendable function 可在不同 Worker 调用，模块作者**禁止**：

- 缓存创建 VM 专属的 `napi_env`（如 `static napi_env g_env;`）跨 Worker 使用；
- 假设 init 跑在固定线程并依赖 thread-local；
- 保存普通 `napi_ref` 或 LocalHeap 对象跨 Worker。

运行时已处理 sendable function 的跨 VM env 切换（`ark_native_engine.cpp:1222` `JSNApi::GetEnv(vm)`），模块作者只需不缓存。

## 5. 改动文件

| 文件 | 改动 |
|---|---|
| `module_manager/native_module_manager.h/.cpp` | `NativeModule.shared` 字段；`IsSharedModule(moduleKey)` 方法（遍历链表查 `shared`）；`Register` 复制 `shared` |
| `native_engine/native_node_api.cpp` | `napi_module_register` 不读 `nm_shared`（`shared` 保持 `0`） |
| `native_engine/native_api.cpp` | 新增 `napi_shared_module_register`（置 `shared=1`） |
| `interfaces/kits/napi/native_api.h` | 声明 `napi_shared_module_register`（公共 API） |
| `native_engine/impl/ark/ark_native_engine.h/.cpp` | `sharedRootMutex_` + `sharedRootMap_`（`NativeModule*` → `SendableGlobal`）+ `LoadSharedNativeModule`/`CreateSendableRoot`/`IsSharedModule` + `LoadNativeModule` 分支 |

不改：`napi_module` 结构体、`napi_module_with_js_register`、`ace_napi.versionscript`、`ets_runtime`。
