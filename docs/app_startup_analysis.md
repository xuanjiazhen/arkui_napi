# OpenHarmony 应用启动流程分析(appspawn → ability_runtime → napi)

本文档整合 `base/startup/appspawn`、`foundation/ability/ability_runtime`、`foundation/arkui/napi` 三个仓的协同分析,覆盖从用户点击应用图标到 Ability 就绪的端到端调用链,以及 NativeEngine 的创建、`ReinitUVLoop` 的作用等关键机制。

> 说明:本文不涉及 Cangjie FFI 互操作路径,相关内容见 `docs/knowledge/cjffi.md`。

---

## 1. 三仓职责与接口边界

| 仓 | 职责 | 对外接口 |
|---|---|---|
| `base/startup/appspawn` | 进程孵化:接收启动请求、fork 子进程、设置 uid/gid/selinux/sandbox、调用 ability_runtime 入口 | **Unix domain socket**(非 Binder),路径 `/dev/unix/socket/AppSpawn` |
| `foundation/ability/ability_runtime` | 运行时编排:MainThread 进程入口、AppMgrService 调度、Runtime 创建、Ability 生命周期 | Binder(对 AppMgrService)+ 内部 socket client(对 appspawn) |
| `foundation/arkui/napi` | 引擎实现:`ArkNativeEngine`(即 `napi_env`)、`NativeModuleManager` 单例、`napi_module_register` 符号 | 被 `new ArkNativeEngine` 调用,无主动反向调用(校验通过 delegate 回调) |

**核心边界**:

- ability_runtime 是"驱动者":创建 EcmaVM、创建 ArkNativeEngine、设置应用 lib 路径与 ld namespace、注入校验 delegate、调度 AbilityStage 加载业务 ABC。
- napi 仓是"被驱动者":提供 `ArkNativeEngine` 类、`NativeModuleManager` 单例、`napi_module_register` 符号。
- 唯一的反向回调是 `ModuleCheckerDelegate::CheckModuleLoadable`,用于模块加载校验(符合"校验策略通过 delegate 注入,不在 Manager 内硬编码"的架构不变量)。
- appspawn 通过 `dlopen` 把 ACE 库(含 ability_runtime appkit + napi)载入自身地址空间,fork 后子进程继承,直接调用 `MainThread::Start()`。

---

## 2. 关键概念澄清

### 2.1 应用进程入口不是 `main()`,也不是 `execve`

OpenHarmony 的应用进程入口是 appspawn 在 fork 出的子进程内**直接调用 C++ 符号 `OHOS::AppExecFwk::MainThread::Start()`**,而非传统 `main()` 函数,也非 `execve` 一个新二进制。

机制:

1. appspawn 启动时(preload 阶段,fork 之前):
   - `ace_adapter.cpp:449` `RegChildLooper(&content, RunChildProcessor)` 注册子进程 looper 函数指针。
   - `ace_adapter.cpp:254` `dlopen(acelibdir, RTLD_NOW|RTLD_LOCAL)` 将 ACE 库(含 `MainThread`、`ArkNativeEngine`)载入 appspawn 自身地址空间。
   - `ace_adapter.cpp:93` `PreloadModule`(其内 `ace_adapter.cpp:107` 调 `Runtime::Create`)预创建 VM 和 ArkNativeEngine(可被 fork 后复用,加速启动)。
2. fork 时:`appspawn_server.c:175` `fork()` 子进程继承整个地址空间(含已 dlopen 的库、已注册的函数指针、预创建的 VM)。
3. fork 后(子进程):设置 uid/gid/selinux/sandbox 后,直接调用 `MainThread::Start()`(`main_thread.cpp:3446`),**不加载任何新二进制,不 execve**。

这种"appspawn 预加载 + fork 复用"是应用启动性能优化的核心,也是 appspawn 以 root + SCHED_FIFO 高优先级运行的原因。

**例外**:
- 开发者模式原生/render 进程(`MSG_SPAWN_NATIVE_PROCESS`)走 `RunChildByRenderCmd`(`ace_adapter.cpp:418`)用 `execvp` —— 这是唯一的 execve 路径。
- 冷启动(`app_cold`):appspawn 二进制自身被重新执行,从 mmap 文件读消息,不 fork,但仍调用 `MainThread::Start()`。

### 2.2 App 与 Ability 的关系

- **App(应用/HAP 包)** ≠ Ability。一个 App 包含**一个或多个 Ability**(UIAbility 有界面,ExtensionAbility 无界面),Ability 是应用内功能页/组件的抽象。
- **进程 ↔ Runtime 是 1:1**:`MainThread` 启动时 `Runtime::Create` 只建**一次**(`main_thread.cpp:1951`),进程内所有 Ability 共享同一个 `NativeEngine`/`napi_env`。
- **Ability 是 1:N**:`OHOSApplication::AddAbilityStage`(`ohos_application.cpp:548`)每加载一个 HAP 模块的 AbilityStage/Ability 就调一次,可多次调用。
- **进程模型**:默认"一个应用一个进程";可在 `module.json5` 配 `process` 标签让不同 HAP 模块跑在独立进程里,此时每个进程都会各自走一遍完整启动流程。

### 2.3 `napi_env` / `napi_value` 是指针 reinterpret

`napi_env` ↔ `ArkNativeEngine*`(不是 C++ 包装类),由 `js_environment.cpp:70` 创建。ability_runtime 中所有 `reinterpret_cast<NativeEngine*>(env)` 都依赖此约定。

---

## 3. NativeEngine 的创建流程

### 3.1 创建链路

NativeEngine 实际类型是 `ArkNativeEngine`,创建分主线程和子线程(worker)两条路径,最终汇聚到同一创建点。

**主线程路径**:

```
main_thread.cpp:1951  Runtime::Create(options)
  → runtime.cpp:30          按语言分发 → JsRuntime::Create
  → js_runtime.cpp:815       JsRuntime::CreateJsEnv       构造 pandaOption
  → js_runtime.cpp:861       jsEnv_ = make_shared<JsEnvironment>(OHOSJsEnvironmentImpl)
  → js_runtime.cpp:862       jsEnv_->Initialize(pandaOption, this)   // this=JsRuntime*
  → js_environment.cpp:61    JsEnvironment::Initialize
```

**子线程路径(worker / `napi_create_ark_runtime`)**:

```
js_runtime_lite.cpp:145  RegCreateNapiEnvCallback(CreateNapiEnv)   启动时注册
  → js_runtime_lite.cpp:75   CreateNapiEnv(env)
  → js_runtime_lite.cpp:88   JsRuntimeLite::CreateJsEnv(options, jsEnv)
  → js_runtime_lite.cpp:185  jsEnv = make_shared<JsEnvironment>(...)
  → js_runtime_lite.cpp:186  jsEnv->Initialize(pandaOption, this)
  → js_runtime_lite.cpp:191  jsEnv->GetNativeEngine()->MarkNativeThread()  标记线程亲和
```

### 3.2 唯一创建点

`js_environment.cpp:61-72`:

```cpp
bool JsEnvironment::Initialize(const panda::RuntimeOption& pandaOption, void* jsEngine)
{
    vm_ = panda::JSNApi::CreateJSVM(pandaOption);   // 1. 先创建 Ark VM
    if (vm_ == nullptr) { return false; }
    engine_ = new ArkNativeEngine(vm_, jsEngine);     // 2. 再创建 NativeEngine
    return true;
}
```

`jsEngine` 参数:主线程传 `JsRuntime*`,子线程传 `JsRuntimeLite*`,存入 `NativeEngine::jsEngine_` 供回调用。

### 3.3 ArkNativeEngine 构造函数内部

`ark_native_engine.cpp:495-566` 构造函数做了 4 件事:

1. **基类构造** `NativeEngine(jsEngine)`(`native_engine.cpp:83-90`):`SetMainThreadEngine` / `SetAlive` / `InitUvField` 清零 uv 字段 / `new WorkerThreadState`。
2. **`JSNApi::SetEnv(vm, this)`**(`ark_native_engine.cpp:502`):把 `this`(= `napi_env`)注册到 VM,后续 `JSNApi::GetEnv` 可反查。
3. **安装 `requireNapi` / `requireInternal` 到 global**(`ark_native_engine.cpp:522-529`):`requireData = this` 让 JS 调用时能取回 `ArkNativeEngine*`。这是后续 NAPI 模块加载的触发入口。
4. **`Init()`**(`ark_native_engine.cpp:539` → `native_engine.cpp:136`):基类初始化。

随后注册 finalize/async/promise-rejection/timer 回调,`NotifyEnvInitialized`,`ArkIdleMonitor::EnableIdleGC`。

### 3.4 基类 `Init` 与 `InitWithoutUV`

**`NativeEngine::Init()`**(`native_engine.cpp:136-155`):

```cpp
void NativeEngine::Init()
{
    InitWithoutUV();                     // ↓ 见下
    loop_ = new uv_loop_t;              // 创建 libuv loop
    uv_loop_init(loop_);
    uv_async_init(loop_, &uvAsync_, nullptr);
    uv_sem_init(&uvSem_, 0);
    NativeEvent::CreateDefaultFunction(this, defaultFunc_, eventMutex_);
}
```

**`InitWithoutUV()`**(`native_engine.cpp:189-196`)—— 拿单例、建管理器:

```cpp
void NativeEngine::InitWithoutUV()
{
    moduleManager_ = NativeModuleManager::GetInstance();     // 模块管理单例(全局共享)
    referenceManager_ = new NativeReferenceManager();         // 引用管理
    callbackScopeManager_ = new NativeCallbackScopeManager(); // 作用域管理
    tid_ = pthread_self();                                    // 记录线程(供 CROSS_THREAD_CHECK)
    sysTid_ = GetCurSysTid();
}
```

### 3.5 创建流程时序

```
JsRuntime::CreateJsEnv / JsRuntimeLite::CreateJsEnv
  │  构造 panda::RuntimeOption (GC/AOT/JIT/asmInterpreter)
  │  jsEnv_ = make_shared<JsEnvironment>(OHOSJsEnvironmentImpl)
  ▼
JsEnvironment::Initialize(pandaOption, jsEngine)
  │  vm_ = JSNApi::CreateJSVM(pandaOption)          ── Ark VM 就绪
  ▼
new ArkNativeEngine(vm_, jsEngine)
  │
  ├─ NativeEngine(jsEngine)                          [基类构造]
  │    ├─ SetMainThreadEngine(this) / SetAlive()
  │    ├─ InitUvField()  (清零 uv 字段)
  │    └─ new WorkerThreadState()
  │
  ├─ JSNApi::SetEnv(vm, this)                        napi_env ↔ VM 双向绑定
  ├─ 安装 requireNapi/requireInternal 到 global      JS 侧入口
  ├─ SetUnloadNativeModuleCallback
  │
  ├─ Init()                                          [基类 Init]
  │    ├─ InitWithoutUV()
  │    │    ├─ moduleManager_ = NativeModuleManager::GetInstance()
  │    │    ├─ referenceManager_ = new NativeReferenceManager()
  │    │    ├─ callbackScopeManager_ = new NativeCallbackScopeManager()
  │    │    └─ tid_ / sysTid_  记录线程亲和
  │    ├─ loop_ = new uv_loop_t + uv_loop_init
  │    ├─ uv_async_init / uv_sem_init
  │    └─ CreateDefaultFunction
  │
  ├─ JSNApi::SetLoop(vm, loop_)                      uv_loop 注册给 VM
  ├─ 注册 finalize/async/promise-rejection/timer 回调
  ├─ NotifyEnvInitialized(vm)                        通知 VM env 就绪
  └─ ArkIdleMonitor::EnableIdleGC(this)
```

### 3.6 关键设计点

1. **VM 先于 engine**:必须先 `CreateJSVM` 再 `new ArkNativeEngine`,因为构造时要把 `this` 通过 `JSNApi::SetEnv(vm, this)` 注册回 VM —— 这是 `napi_env` 能从 VM 侧反查的根基。
2. **`requireNapi` 安装时机**:构造函数里就装好,所以 JS 代码一执行就能调用 `requireNapi` —— 这是后续 NAPI 模块加载的触发入口。`requireData = this` 让回调能取回 `ArkNativeEngine*`。
3. **单例共享**:`moduleManager_ = NativeModuleManager::GetInstance()` 是全局单例,所以主线程 engine、子线程 worker engine 都共享同一个 `NativeModuleManager` —— 这也解释了为什么 `SetModuleLoadChecker` 只需注入一次就能全局生效。
4. **线程亲和记录**:`tid_ = pthread_self()` 在 `InitWithoutUV` 中记录,供后续 `CROSS_THREAD_CHECK` 校验。子线程创建时会额外调 `MarkNativeThread()`。

---

## 4. 应用启动端到端时序

### 阶段 A:用户点击 → AppMgrService 决定拉进程(ability_runtime)

1. 桌面 launcher 发起 StartAbility → AbilityManagerService。
2. `AbilityRecord::LoadAbility` → `AppScheduler::LoadAbility`(`app_scheduler.cpp:70`)→ `appMgrClient_->LoadAbility(...)`(Binder)→ `AppMgrServiceInner::LoadAbility`(`app_mgr_service_inner.cpp:2022`)。
3. (无已有进程)`LoadAbilityNoAppRecord` → `StartProcess`(`app_mgr_service_inner.cpp:5979`)。
4. 构建 `AppSpawnStartMsg`(bundleName/uid/gid/gids/accessToken/apl/flags/权限/fds 等)。
5. `GetSpawnClient()->StartProcess(startMsg, pid)`(`app_spawn_client.cpp:657`)。

### 阶段 B:AppSpawnClient → appspawn(Unix socket IPC)

6. `AppSpawnClientInit("appspawn", &handle)`(`appspawn_client.c:417`)连接 socket。
7. `AppSpawnReqMsgCreate(MSG_APP_SPAWN, procName)` 构建请求。
8. `AppspawnCreateDefaultMsg` 填充 DAC/权限/ext(`app_spawn_client.cpp:681`)。
9. `AppSpawnClientSendMsg(handle, reqHandle, &result)`(`appspawn_client.c:520`)发送 + 等响应。

**消息格式**(`appspawn_msg.h:135-142`):`AppSpawnMsg`(magic=0xEF201234 + msgType + TLV body),通过 SCM_RIGHTS 传递 fd。

### 阶段 C:appspawn 接收 → fork → 设置子进程属性(appspawn)

10. `OnReceiveRequest`(`appspawn_service.c:502`)接收 + `SO_PEERCRED` 校验 uid。
11. `ProcessRecvMsg` → `MSG_APP_SPAWN` → `ProcessSpawnReqMsg`(`appspawn_service.c:1481`)。
12. `STAGE_PARENT_PRE_FORK` hook(挂载共享目录、debug sandbox、pid namespace)。
13. `NormalSpawnChild` → `AppSpawnProcessMsg` → `AppSpawnForkChildProcess`(`appspawn_server.c:194`)。
14. **`fork()`**(`appspawn_server.c:175`)—— 子进程诞生。
    - 父进程:等 pipe 结果,返回 pid 给 AppSpawnClient。
    - 子进程:`AppSpawnChild` hook 链(`appspawn_server.c:79`):
      - **`STAGE_CHILD_PRE_COLDBOOT`**:`SpawnInitSpawningEnv`(`appspawn_common.c:525`)—— `prctl(PR_SET_NAME)` 设置进程名、`ClearEnvironment`、`SetAppAccessToken`。
      - **`STAGE_CHILD_EXECUTE`**:
        - `SetAppSandboxProperty`(`sandbox_core.cpp:1110`):`EnableSandboxNamespace`(mount namespace)、挂载/符号链接(解析 `appdata-sandbox*.json`)、`ChangeCurrentDir`、可能 `chroot`。
        - `SpawnSetProperties`(`appspawn_common.c:588`):
          - `SetKeepCapabilities`:`prctl(PR_SET_KEEPCAPS)`。
          - **`SetUidGid`**(`appspawn_common.c:323`):`setgroups` → `setresgid` → `SetSeccompFilter` → `setresuid` —— **切换应用 uid**。
          - `SetCapabilities`:`prctl(PR_CAP_AMBIENT)`。
          - **`SetSelinuxCon`**(`appspawn_adapter.cpp:118`):`hapContext.HapDomainSetcontext` —— **切换 SELinux 域**。
      - **`STAGE_CHILD_PRE_RELY`**:`write(pipefd, &ret)` 向父进程报告成功。

### 阶段 D:appspawn → ability_runtime 衔接(关键跨仓点)

15. `content->runChildProcessor(content, client)`(`appspawn_server.c:122`)。
    - 该函数指针由 ace_adapter 在 preload 阶段注册:`RegChildLooper(&content, RunChildProcessor)`(`ace_adapter.cpp:449`)。
16. `RunChildProcessor` → `RunChildThread`(`ace_adapter.cpp:367`)。
17. `ClearEnvAndReturnSuccess` → `write(pipe)` 报成功。
18. **`OHOS::AppExecFwk::MainThread::Start()`**(`ace_adapter.cpp:386` → `main_thread.cpp:3446`)。
    - fork 子进程内直接调用 C++ 符号,非 execve,非 main()。

### 阶段 E:MainThread 启动 → Runtime::Create → ArkNativeEngine

19. `MainThread::Start`(`main_thread.cpp:3446`):
    - `EventRunner::GetMainEventRunner()` 获取主事件 runner。
    - `new MainThread()` → `Init(runner)`(`main_thread.cpp:3240`)。
    - `Attach()` → `ConnectToAppMgr()`(Binder 连回 AppMgrService,注册 IAppThread 代理)。
    - `runner->Run()` 进入事件循环,等 AppMgrService 下发 launch 指令。
20. AppMgrService 检测到 Attach → `ScheduleLaunchApplication(AppLaunchData)`。
21. `MainThread::HandleLaunchApplication`(`main_thread.cpp:1656`):
    - `LoadAceAbilityLibrary()` → `dlopen(libace_*.z.so)`。
    - `JsRuntime::SetAppLibPath(appLibPaths)` → `NativeModuleManager::SetAppLibPath`(设置应用 lib 路径 + 创建 ld namespace)。
    - `SetModuleLoadChecker(delegate)` → `NativeModuleManager::SetModuleLoadChecker` → `ModuleLoadChecker::SetDelegate`(注入校验策略)。
22. **`Runtime::Create(options)`**(`main_thread.cpp:1951` → `runtime.cpp:30` → `js_runtime.cpp:146`)。
23. `JsRuntime::Initialize`(`js_runtime.cpp:695`)→ `CreateJsEnv`(`js_runtime.cpp:815`)→ `jsEnv_->Initialize`(`js_environment.cpp:61`)。
24. **`engine_ = new ArkNativeEngine(vm_, jsEngine)`**(`js_environment.cpp:70`)—— napi 仓创建点。
25. `ArkNativeEngine` 构造(`ark_native_engine.cpp:495`):
    - `JSNApi::SetEnv(vm, this)` 双向绑定。
    - 安装 `requireNapi`/`requireInternal` 到 global。
    - `Init()` → `InitWithoutUV()`:`moduleManager_ = NativeModuleManager::GetInstance()`。
    - `JSNApi::NotifyEnvInitialized(vm)`。

### 阶段 F:Ability 就绪

26. `application_->SetRuntime(std::move(runtime))`(`ohos_application.cpp:179`)。
27. AppMgrService 调 `ScheduleAbilityStage` → `OHOSApplication::AddAbilityStage`(`ohos_application.cpp:548`)。
    - `JsAbilityStage::LoadModule`(加载 AbilityStage.abc,走 Ark ES module loader)。
    - `JsAbilityStage::OnCreate`(执行用户 JS,内部可能 `requireNapi`)。
28. AppMgrService 调 `ScheduleAbilityLifecycle` → Ability 启动 → 界面就绪。

---

## 5. 预加载-fork-复用机制与 `ReinitUVLoop`

### 5.1 预加载-复用模式

appspawn 在启动时(fork 之前)就完成部分初始化,fork 后子进程继承并复用,以加速应用启动:

```
appspawn 启动(主进程):
  PreloadModule → Runtime::Create 预创建 VM/ArkNativeEngine   ← loop_ 此时已建
  → SavePreloaded 保存                                          runtime.cpp:53
  → JSNApi::PreFork(vm)                                        js_runtime.cpp:636  fork 前准备

fork():                                                         appspawn_server.c:175
  子进程继承地址空间(包括预创建的 VM 和 loop_)

fork 后(子进程,在 MainThread 里走 Initialize):
  JsRuntime::Initialize                                         js_runtime.cpp:695
    if (preloaded_) {                                           js_runtime.cpp:721
        PostPreload(options);                                    js_runtime.cpp:722
    }
      → JSNApi::PostFork(vm, postOption)                        js_runtime.cpp:669  VM 完成 fork 后重建
      → reinterpret_cast<NativeEngine*>(env)->ReinitUVLoop()    js_runtime.cpp:671  重建 uv_loop
      → napi_get_uv_event_loop(env, &loop)
      → JSNApi::SetLoop(vm, loop)                               js_runtime.cpp:674  新 loop 注册给 VM
```

### 5.2 `ReinitUVLoop` 的作用

`NativeEngine::ReinitUVLoop()`(`native_engine.cpp:242`)用于**进程 fork 之后重建 uv_loop**。代码注释:

```cpp
// should only be called if process is forked, other case would cause fd_leak
bool NativeEngine::ReinitUVLoop()
```

它做的事(`native_engine.cpp:242-282`):

1. **销毁旧 loop**:`uv_walk` 关闭所有 handle → `uv_run` 跑完残留 → `uv_loop_close` → `delete loop_`。
2. **更新线程亲和**:`tid_ = pthread_self(); sysTid_ = GetCurSysTid(); UpdateCrossThreadCheckStatus()` —— fork 后是新进程,tid 变了。
3. **新建 loop**:`new uv_loop_t` + `uv_loop_init` + `uv_async_init` + `uv_sem_init`。
4. **重建 default function** + `NotifyEnvInitialized`。

### 5.3 为什么 fork 后必须重建

fork 之后,父子进程共享 fd 表、epoll fd、uv handle 的状态不一致;uv_loop 里记的 tid 还是 appspawn 进程的;旧 uv_sem 在单线程下失效。直接复用会导致:

- fd 泄漏(注释明示 "other case would cause fd_leak")。
- 异步任务紊乱。
- `CROSS_THREAD_CHECK` 误报(tid 不匹配)。

因此预创建的 VM 和 loop_ 是 appspawn 进程的,fork 后必须 `ReinitUVLoop` 重新初始化 loop 才能在子进程里安全使用。

### 5.4 关键约束

- **只能在 fork 后调用**:非 fork 场景调用会泄漏 fd(因为 `uv_loop_close` 前会 close 掉正在用的 handle 的 fd)。
- **必须配套 `JSNApi::PostFork`**:VM 侧也有自己的 fork 后重建(`js_runtime.cpp:669`),顺序是先 `PostFork`(VM)再 `ReinitUVLoop`(engine)再 `SetLoop`(把新 loop 给 VM)。
- `tid_` 在 `InitWithoutUV` 中初设,`ReinitUVLoop` 在 fork 后刷新,这是 `CROSS_THREAD_CHECK`(不要跨线程传递 `napi_env`)能在 fork 后继续生效的保障。

---

## 6. 关键文件索引

### appspawn 仓

| 功能 | 文件:行 |
|---|---|
| main 入口 | `standard/appspawn_main.c:101` |
| 服务创建 | `standard/appspawn_service.c:1984` (`StartSpawnService`) |
| 事件循环 | `standard/appspawn_service.c:1913` (`AppSpawnRun`) |
| Socket 接收 | `standard/appspawn_service.c:502` (`OnReceiveRequest`) |
| 消息分发 | `standard/appspawn_service.c:2355` (`ProcessRecvMsg`) |
| Spawn 请求处理 | `standard/appspawn_service.c:1481` (`ProcessSpawnReqMsg`) |
| fork() 调用 | `common/appspawn_server.c:175` (`AppSpawnForkChildProcess`) |
| AppSpawnChild hook 链 | `common/appspawn_server.c:79` |
| 进程属性设置 | `modules/common/appspawn_common.c:588` (`SpawnSetProperties`) |
| SetUidGid | `modules/common/appspawn_common.c:323` |
| SetSelinuxCon | `modules/common/appspawn_adapter.cpp:118` |
| 沙箱设置 | `modules/sandbox/normal/sandbox_core.cpp:1110` |
| **ace_adapter 衔接** | `modules/ace_adapter/ace_adapter.cpp:425` (`RunChildProcessor`) |
| **MainThread::Start 调用** | `modules/ace_adapter/ace_adapter.cpp:386` |
| dlopen ACE 库 | `modules/ace_adapter/ace_adapter.cpp:254` |
| 客户端 C 接口 | `interfaces/innerkits/client/appspawn_client.c:520` (`AppSpawnClientSendMsg`) |
| AppSpawnMsg 结构 | `modules/module_engine/include/appspawn_msg.h:135` |

### ability_runtime 仓

| 功能 | 文件:行 |
|---|---|
| AppSpawnClient(C++) | `services/appmgr/src/app_spawn_client.cpp:657` (`StartProcess`) |
| RemoteClientManager | `services/appmgr/src/remote_client_manager.cpp:25` |
| AppScheduler::LoadAbility | `services/abilitymgr/src/app_scheduler.cpp:70` |
| AppMgrServiceInner::LoadAbility | `services/appmgr/src/app_mgr_service_inner.cpp:2022` |
| AppMgrServiceInner::StartProcess | `services/appmgr/src/app_mgr_service_inner.cpp:5979` |
| **MainThread::Start** | `frameworks/native/appkit/app/main_thread.cpp:3446` |
| MainThread::Init | `frameworks/native/appkit/app/main_thread.cpp:3240` |
| MainThread::Attach | `frameworks/native/appkit/app/main_thread.cpp:447` |
| HandleLaunchApplication | `frameworks/native/appkit/app/main_thread.cpp:1656` |
| Runtime::Create | `frameworks/native/runtime/runtime.cpp:30` |
| Runtime::SavePreloaded/GetPreloaded | `frameworks/native/runtime/runtime.cpp:53,61` |
| JsRuntime::Create | `frameworks/native/runtime/js_runtime.cpp:146` |
| JsRuntime::Initialize | `frameworks/native/runtime/js_runtime.cpp:695` |
| JsRuntime::CreateJsEnv | `frameworks/native/runtime/js_runtime.cpp:815` |
| JsRuntime::PostPreload(含 ReinitUVLoop) | `frameworks/native/runtime/js_runtime.cpp:639` |
| JsRuntime::SetAppLibPath | `frameworks/native/runtime/js_runtime.cpp:906` |
| JsRuntime::SetModuleLoadChecker | `frameworks/native/runtime/js_runtime.cpp:1811` |
| JsRuntime::LoadModule | `frameworks/native/runtime/js_runtime.cpp:1111` |
| OHOSApplication::AddAbilityStage | `frameworks/native/appkit/app/ohos_application.cpp:548` |
| JsAbilityStage::LoadModule | `frameworks/native/appkit/ability_runtime/app/js_ability_stage.cpp:208` |
| JsEnvironment::Initialize(★ 创建点) | `js_environment/frameworks/js_environment/src/js_environment.cpp:61` |
| OHOSJsEnvironmentImpl::InitWorkerModule | `frameworks/native/runtime/ohos_js_environment_impl.cpp:190` |
| JsRuntimeLite::CreateNapiEnv(子线程) | `frameworks/native/runtime/js_runtime_lite.cpp:75` |

### napi 仓

| 功能 | 文件:行 |
|---|---|
| **ArkNativeEngine 构造** | `native_engine/impl/ark/ark_native_engine.cpp:495` |
| ArkNativeEngine::RequireNapi | `native_engine/impl/ark/ark_native_engine.cpp:928` |
| ArkNativeEngine::CheckAndGetModule | `native_engine/impl/ark/ark_native_engine.cpp:785` |
| ArkNativeEngine::LoadNativeModule | `native_engine/impl/ark/ark_native_engine.cpp:838` |
| NativeEngine 基类构造 | `native_engine/native_engine.cpp:83` |
| NativeEngine::Init | `native_engine/native_engine.cpp:136` |
| NativeEngine::InitWithoutUV | `native_engine/native_engine.cpp:189` |
| **NativeEngine::ReinitUVLoop** | `native_engine/native_engine.cpp:242` |
| NativeEngine::SetModuleLoadChecker | `native_engine/native_engine.cpp:1183` |
| napi_module_register | `native_engine/native_node_api.cpp:35` |
| NativeModuleManager::Register | `module_manager/native_module_manager.cpp:315` |
| NativeModuleManager::LoadNativeModule | `module_manager/native_module_manager.cpp:838` |
| NativeModuleManager::FindNativeModuleByDisk | `module_manager/native_module_manager.cpp:1367` |
| NativeModuleManager::LoadModuleLibrary(dlopen) | `module_manager/native_module_manager.cpp:1237` |
| NativeModuleManager::SetAppLibPath | `module_manager/native_module_manager.cpp:738` |
| NativeModuleManager::SetModuleLoadChecker | `module_manager/native_module_manager.cpp:1674` |
| ModuleCheckerDelegate(校验接口) | `module_manager/module_checker_delegate.h:27` |

---

## 7. 关键架构不变量对应

- **公共 API 稳定性**:`napi_module_register` 签名固定;`napi_env`/`napi_value` 是指针 reinterpret。
- **校验策略通过 delegate 注入**:`ModuleCheckerDelegate` 由 ability_runtime 实现,通过 `NativeEngine::SetModuleLoadChecker` → `NativeModuleManager::SetModuleLoadChecker` → `ModuleLoadChecker::SetDelegate` 注入,napi 仓不硬编码校验。
- **Ark VM 线程安全不变量**:`tid_` 记录线程亲和,`CROSS_THREAD_CHECK` 校验;fork 后 `ReinitUVLoop` 刷新 `tid_` 保障校验继续生效;不要跨线程传递 `napi_env`。
- **不要手动加载 .so 绕过 `napi_module_register`**:全程通过 `NativeModuleManager::LoadNativeModule` → `dlopen` 触发 constructor → `napi_module_register`。
- **模块自注册是惰性的**:不是启动时遍历注册表,而是 JS 调 `requireNapi` 时 `dlopen` 对应 `.so`,链接器执行 `__attribute__((constructor))` → `napi_module_register` → 挂入 manager 链表 tail → 随即执行 `registerCallback`。
