# RequireNapi 代码流程分析

`RequireNapi` 是 JS/ArkTS 侧 `requireNapi(...)` 全局函数的 C++ 实现，整体是「**缓存优先 + 加锁加载 + 引擎内再缓存**」三层结构。

## 1. 入口：`ArkNativeEngine::RequireNapi`

**位置：** `native_engine/impl/ark/ark_native_engine.cpp:928`

**TypeScript 声明：**

```ts
// iOS
function requireNapi(name: string): any;
// 其他平台
function requireNapi(
    name: string,
    appModule: boolean = false,
    path?: string,
    relativePath?: string): any;
```

**调用链入口：**

```
JS: requireNapi(name, isAppModule=false, path?, relativePath?)
  → FunctionRef 回调 → RequireNapi(info)
```

**步骤：**

1. `info->GetVM()` / `info->GetData()` 取回 `ecmaVm` 与 `ArkNativeEngine*`（构造时绑定的 `requireData`，见 `ark_native_engine.cpp:523-526`）
2. 无参 → 直接返回 undefined
3. 取 `moduleName`（arg0）与 `isAppModule`（arg1，默认 false）；PREVIEW 下 `file.fs` 被禁用直接返回 undefined（`:942`）
4. 调 `CheckAndGetModule(...)` → 得到返回码 `code` 与 `module`/`exports`（详见 §2）
5. 根据 `code` 分流：
   - `code != 0`（`-1` 失败 / `1` 已就绪）→ 直接 `Escape(exports)`
   - `code == 0`（找到 module，待加载）→
     - `module != nullptr`：调 `ArkNativeEngine::LoadNativeModule(...)` 真正执行模块 init
     - `module == nullptr`：`DFXJSNApi::InsertSoLoadFailure` 记录失败 + `CreateNativeModuleFailureInfo(errInfo)` 返回错误对象

## 2. 预检与分发：`CheckAndGetModule`

**位置：** `native_engine/impl/ark/ark_native_engine.cpp:785`

**返回码语义**（见头文件 `ark_native_engine.h:483-485`）：

| 返回值 | 含义 |
|---|---|
| `-1` | 加载失败 |
| `0` | 找到模块，需要继续加载 |
| `1` | 模块已就绪，可直接返回 exports |

**流程：**

```
isLimitedWorker_ && !isAppModule → CheckModuleRestricted 白名单
  不通过 → return -1
按 info->GetArgsNumber() 分支：
  3 参: 若 IsCJModule(name) → LoadCJModule 填 exports → return 1   (Cangjie 分支)
        否则 LoadNativeModule(name, path, isAppModule, ...)
  4 参: LoadNativeModule(name, nullptr, isAppModule, ..., relativePath)
  其他: LoadNativeModule(name, nullptr, isAppModule, ..., "")
return 0
```

**说明：**

- `CheckAndGetModule` 只做**预检 + 取 `NativeModule*`**，并不执行 `registerCallback`。
- IOS 平台为简化路径，直接走 `LoadNativeModule` 不区分参数数量（`:795-805`）。
- Cangjie 分支（`IsCJModule` 命中）直接 `LoadCJModule` 填好 `exports` 并返回 `1`，跳过后续 native init 流程。

## 3. 进程级加载：`NativeModuleManager::LoadNativeModule`

**位置：** `module_manager/native_module_manager.cpp:838`

这是 `NativeModuleManager` 侧的「找/加载 `.so`」实现，与引擎内同名函数（§4）是两回事。

**流程：**

```
1. 输入校验：moduleName / relativePath 非空、relativePath 不含 ".."
2. 黑名单：moduleLoadChecker_->CheckModuleLoadable(name, checker, isAppModule)
    不通过 → errInfo="... in blocklist" → return nullptr
3. 路径计算：GetNativeModulePath(name, prefix, relativePath, isAppModule, ...)
4. 缓存命中：FindNativeModuleByCache(key, nativeModulePath, ...) → 命中直接返回
5. 未命中：
   pthread_mutex_lock(&mutex_)                ← 进程级互斥
   ↓ 二次检查（CheckNativeListChanged）防并发重复加载
   ↓ FindNativeModuleByDisk(...) → dlopen .so、调 napi_module_register 回填 NativeModule
   pthread_mutex_unlock(&mutex_)
6. 把 apiAllowListChecker 转移到 nativeModule 上
7. return nativeModule   (失败 nullptr)
```

**关键点：**

- `mutex_` 是**进程级**互斥锁，保证同一 `.so` 只被 dlopen 一次。
- 缓存键：普通模块用 `moduleName`，app 模块用 `prefix/moduleName`（`:914-922`）。
- `FindNativeModuleByDisk` 内部走 `dlopen` + `napi_module_register`（`.so` 里的 `NAPI_MODULE(...)` 宏触发，回填 `NativeModuleManager::Register`）。
- `moduleLoadChecker_` 通过 `ModuleCheckerDelegate` 注入校验策略，不在 Manager 内硬编码。

## 4. 引擎级 init：`ArkNativeEngine::LoadNativeModule`

**位置：** `native_engine/impl/ark/ark_native_engine.cpp:838`

拿到 `NativeModule*` 后真正执行 init 并产出 `exports`：

```
module == nullptr → 直接返回 exports
loadedModules_.find(module) 命中 → 返回缓存的 Global<JSValueRef>     ← VM-local 二级缓存
否则：
  moduleManager->SetNativeEngine(name, this)   ← 让模块回调能拿到 env
  MoudleNameLocker nameLocker(name)            ← 模块名级互斥（防同模块并发 init）

  分支 A（混合/JS+ABC 模块）：module->jsABCCode 或 jsCode 非空
    sprintf fileName = "lib{name}.z.so/{name}.js"
    buffer = jsABCCode ?: jsCode
    exports = LoadArkModule(buffer, len, fileName)   ← 走 Ark VM 跑 ABC
    缓存 loadedModules_[module] = Global(exports)

  分支 B（纯 native 模块）：registerCallback != nullptr
    exportObj = ObjectRef::New(vm_)
    SetModuleName(exportObj, module->name)
    module->registerCallback(env, exportObj)         ← 调模块的 Init
    CheckArkApiAllowList(module, ctx, exportCopy)
      通过 → 返回 exportCopy（过滤后的副本）
      不通过 → exports=exportObj，缓存 loadedModules_[module]

  分支 C：无 ABC、无 registerCallback → HILOG_ERROR，返回空 exports
```

**说明：**

- `loadedModules_` 是引擎本地（per-VM）缓存，保证同一 VM 内同一模块只 init 一次。
- `MoudleNameLocker` 按模块名加锁，防止同模块在不同调用栈并发 init（与进程级 `mutex_` 作用域不同）。
- 分支 A 走 ABC 字节码执行路径（混合模块的 ArkTS 半边），分支 B 走 native `Init` 回调路径（纯 native 模块或混合模块的 native 半边）。
- `CheckArkApiAllowList` 对 exports 做白名单过滤，返回的是过滤后的 `exportCopy`，原始 `exportObj` 仍缓存供后续命中。

## 5. 整体三层缓存与锁

| 层 | 数据结构 | 作用域 | 互斥 |
|---|---|---|---|
| 进程级 | `NativeModuleManager` 的 `nativeModuleList_`（`FindNativeModuleByCache` 命中） | 整个进程，所有 VM 共享 | `mutex_`（dlopen 互斥） |
| 引擎级 | `ArkNativeEngine::loadedModules_`（`Global<JSValueRef>`） | 单个 VM/Worker | `MoudleNameLocker`（按模块名互斥） |
| 模块回调级 | 模块内自管的单例 exports | `.so` 全局 | 模块自己保证 |

## 6. 关键调用链一览

```
JS requireNapi(name, ...)
  └─ RequireNapi                               ark_native_engine.cpp:928
      └─ CheckAndGetModule                    ark_native_engine.cpp:785
          ├─ CheckModuleRestricted             (limitedWorker 白名单)
          ├─ IsCJModule → LoadCJModule          (Cangjie 分支，返回 code=1)
          └─ NativeModuleManager::LoadNativeModule   native_module_manager.cpp:838
              ├─ moduleLoadChecker_->CheckModuleLoadable  (黑名单)
              ├─ FindNativeModuleByCache                (进程缓存)
              └─ FindNativeModuleByDisk                (dlopen + register 回填)
      └─ ArkNativeEngine::LoadNativeModule    ark_native_engine.cpp:838
          ├─ loadedModules_ 命中 → 返回              (VM 缓存)
          ├─ LoadArkModule(buffer)                  (ABC 分支)
          └─ registerCallback(env, exports)         (native 分支)
              └─ CheckArkApiAllowList              (API 过滤)
```

## 7. 失败路径

| 失败场景 | 位置 | 返回 |
|---|---|---|
| `CheckAndGetModule` 返回 `-1`（白名单拦截） | `ark_native_engine.cpp:808-813` | `Escape(exports)`（通常为 undefined） |
| `module == nullptr`（黑名单 / 路径错 / dlopen 失败） | `ark_native_engine.cpp:963-967` | `InsertSoLoadFailure` 记 DFX + `CreateNativeModuleFailureInfo(errInfo)` |
| `LoadArkModule` 返回 undefined | `ark_native_engine.cpp:876-879` | 返回空 exports |
| `registerCallback == nullptr` | `ark_native_engine.cpp:901-904` | HILOG_ERROR + 返回空 exports |

## 8. 相关变体

- **`RequireNapiForCtxEnv`**（`ark_native_engine.cpp:984`）：子 context engine 专用变体，禁用 app 模块（`HILOG_FATAL`），且强制走 `moduleValidateCallback_` 校验，注册时用 `FunctionRef::New(vm, context, ...)` 绑定到子 context（`:612`）。
- **`RequireInternal`**（`ark_native_engine.cpp:1043`）：混合模块 ArkTS 半边访问自己 native 半边的专用通道，直接调 `moduleManager->LoadNativeModule(..., false, ..., false, "")`（appModule=false、internal=false），不走 `CheckAndGetModule` 的参数分发，但仍受 `isLimitedWorker_` 白名单约束。
