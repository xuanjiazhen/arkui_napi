---
target_release: 7.1
change_type: new-feature
issue: ""
author: ""
date: "2026-08-31"
status: Draft
---

# Proposal

## 背景与问题

ArkTS 已支持通过 `"use shared"` 声明共享模块：模块记录、模块命名空间与导出值进入共享堆，由进程级共享模块管理器统一管理，不同 VM 复用同一模块记录与共享 Namespace。

NAPI 模块当前仍按普通本地模块加载：每个 `ArkNativeEngine` 独立初始化模块、创建普通本地堆导出对象、在本 VM 内缓存。运行时已具备从 NAPI 创建 Sendable Class/Object/Array/Map/TypedArray 与 Sendable Native Function 的基础，以及进程级 `SendableGlobal` 句柄存储和跨 VM 的 Sendable Function 运行时环境切换，但 NAPI 模块本身未利用这些能力实现"原生 init 一次 + 跨 VM 同一导出根"。

因此存在能力差距：多 Worker 导入同一 NAPI 模块时，每个 VM 重复初始化、重复创建导出对象、重复占用原生状态，且导出对象身份无法跨 VM 共享。完整对齐 ArkTS 共享模块语义（共享模块记录、共享 Namespace、进程级模块状态统一）需要同时改造 `foundation/arkui/napi` 与 `ets_runtime`，代价过大。

本变更采用介于两者之间的方向：**共享导出根对象，模块记录保持每 VM 本地，先到先得 init**。native init 回调只执行一次、返回完整 Sendable Root、进程级缓存，不同 VM 获得同一导出根对象；接受 Namespace/模块状态仍为每 VM 本地。该方向**不需要改造 `ets_runtime`**（模块记录保持本地、不触发共享模块导出校验），依托现有 `SendableGlobal` 进程级存储即可实现。

opt-in 经**新增公共 API 注册函数** `napi_shared_module_register`（`interfaces/kits/napi/native_api.h`）激活：模块作者在 `.so` constructor 里调 `napi_shared_module_register(&mod)` 即声明为共享模块（内部置 `NativeModule.shared = 1`），取代 `napi_module_register`。老的 `napi_module_register` 路径不读 `napi_module` 任何新字段，老 `.so` 仍走普通路径——二进制兼容由"调哪个注册函数"决定，无需版本门控，也不改 `napi_module` 结构体。

> **ArkRuntime 约束注入（profile: arkruntime）**：本变更关注内存占用基线（共享导出根与 `.so` 常驻带来的共享堆增量需观测）、ETS/ArkTS 标准兼容（不破坏现有 NAPI 模块加载语义与 Sendable 类型约束）、并发初始化的内存/锁竞争可观测性。

## 用户场景与业务触发

### US-1: 多 Worker 共享同一 NAPI 模块的原生导出
- **角色**：应用或系统模块开发者，使用 NAPI 编写需跨 Worker 复用的原生能力。
- **业务触发**：应用启动多个 Worker 并 `import` 同一 NAPI 模块时，希望避免每个 Worker 重复初始化、重复创建导出对象、重复占用原生状态。
- **业务价值**：降低多 Worker 场景的原生初始化开销与内存占用；提供跨 Worker 一致的导出对象身份，简化原生状态共享与对象比对。

## 初始分级判断

| 判断项 | 结果 | 依据 |
|--------|------|------|
| 变更类型 | new-feature | 引入"NAPI 模块共享导出根（init 一次 + 跨 VM 同一导出根）"这一此前不存在的能力；opt-in 经新增公共 API 函数 `napi_shared_module_register` 激活 |
| 复杂度 | 复杂 | 跨 VM/Worker 并发初始化状态机、两段锁、`.so` 生命周期、allowlist 旁路、共享堆常驻；多文件协同 |
| 涉及仓数量 | 1 | 仅 `foundation/arkui/napi`；`ets_runtime` 不改动（模块记录保持本地，不触发共享模块导出校验） |
| 是否涉及 Public/System API | 是 | 新增公共 API 函数 `napi_shared_module_register`（`interfaces/kits/napi/native_api.h`，面向三方应用）；不改 `napi_module` 结构体、不读其新字段；`ace_napi.versionscript` 为 `global: *` 通配导出新符号 |
| 是否涉及安全/性能关键路径 | 是 | 跨 Worker 共享导出根（跨信任边界）、并发首次初始化、`.so` 生命周期安全、allowlist 旁路 |
| 是否跨 SIG | 否 | 均在 `foundation/arkui/napi` 内，不跨 SIG；`ets_runtime` 不改动 |

## 目标

- 实现 NAPI 模块共享导出根机制：native init 回调在进程内只执行一次、返回完整 Sendable Root、进程级缓存，不同 VM 获得同一导出根对象身份。
- 接受 Namespace/模块记录仍为每 VM 本地（不完整对齐 ArkTS 共享模块语义），不依赖 `ets_runtime` 改造。
- 保证多 Worker 并发首次 import 不重复初始化、不死锁。
- 保证共享导出根存活期间底层 `.so` 不被卸载（无函数地址悬空）。
- 保持普通（非共享）NAPI 模块初始化与导出行为完全不变。

次要类型：`enhancement`（扩展 NAPI 模块加载路径，增加共享分支），但主驱动为 `new-feature`（共享导出根能力此前不存在）。

## 非目标

- 不实现完整 ArkTS 共享模块语义（共享模块记录、共享 Namespace、进程级模块状态统一）——需改造 `ets_runtime` 模块解析/进程级缓存/并发初始化，超出本变更。
- 不改 `napi_module` 结构体（不读 `nm_shared` 等新字段；共享判定由调哪个注册函数决定）。
- 不改 `napi_module_with_js_register`（首版不支持 JS/ABC 混合模块的共享声明）。
- 不加固 canonical key 维度（首版用 moduleName，bundle/namespace 加固属后续 follow-up）。
- 不改动 `CheckArkApiAllowList` 自身（sendable 分支 early return 天然绕过；"构造前过滤 descriptor"属后续）。
- 不实现引用计数式 `.so` 卸载（首版固定驻留，换取简单且无悬空风险）。
- 不改动 `ets_runtime`（arkcompiler/ets_runtime）。

## Agent Scope Guard

| 维度 | 范围/限制 | 需人工确认的触发条件 |
|------|-----------|----------------------|
| 允许仓库 | `foundation/arkui/napi` | 改动 `ets_runtime` 需重新确认（本变更明确不改动） |
| 允许模块/目录 | `native_engine/`（含 `impl/ark/`）、`module_manager/`、`native_engine/native_node_api.cpp`、`interfaces/kits/napi/native_api.h` | 不触及 `ace_napi.versionscript`（`global: *` 通配导出新符号） |
| 禁止修改项 | `ets_runtime`；`ace_napi.versionscript`；`napi_module` 结构体；`napi_module_with_js_register`；现有普通 NAPI 模块加载语义 | 突破任一项需先更新本 proposal |
| 外部依赖/网络访问 | 无新增第三方依赖；无网络访问 | 引入新依赖需人工确认 |

## 成功标准

| 标准 | 可观察指标 | 验证方式 |
|------|-----------|---------|
| 多 Worker 导入同一共享 NAPI 模块获得相同导出根对象身份 | 跨 Worker `napi_strict_equals` 断言导出根为同一对象 | 多 Worker 导入后比对导出对象身份 |
| 共享 NAPI 模块的原生初始化在进程内仅执行一次 | 初始化回调调用次数 == 1 | 计数器/日志确认单次 init |
| 多 Worker 并发首次导入不发生死锁或重复初始化 | 无挂起；init 次数 == 1 | 并发首次 import 场景 |
| 共享导出根上的 Sendable 方法在消费方 Worker VM 中正确执行 | 方法返回值正确；运行时环境为当前 VM | 跨 Worker 调用导出方法 |
| 共享导出根在源 VM 销毁后仍可被其他 VM 使用 | 源 VM 销毁后其他 VM 调用导出方法不崩溃 | VM 销毁后调用导出方法 |
| 共享导出根存活期间底层 `.so` 不被卸载 | 无 `dlclose`/函数地址悬空崩溃 | 尝试卸载后调用导出方法 |
| 普通（非共享）NAPI 模块行为与变更前完全一致 | 调 `napi_module_register` 的模块初始化/导出/allowlist 行为不变 | 回归测试 |

## 影响范围

| 子系统 | 仓库 | 模块/路径 | 影响类型 |
|--------|------|-----------|---------|
| ArkRuntime / NAPI 引擎 | foundation/arkui/napi | `native_engine/impl/ark/`（共享导出根机制、分支接入、`SharedRootEntry`） | 新增能力 |
| ArkRuntime / NAPI 引擎 | foundation/arkui/napi | `module_manager/`（`NativeModule.shared` 字段 + `.so` pin + `Register` 复制） | 扩展 |
| ArkRuntime / NAPI 引擎 | foundation/arkui/napi | `native_engine/native_node_api.cpp`（新增 `napi_shared_module_register`；`napi_module_register` 不再读 `nm_shared`） | 扩展 |
| ArkRuntime / NAPI 引擎 | foundation/arkui/napi | `interfaces/kits/napi/native_api.h`（声明 `napi_shared_module_register`，公共 API） | 扩展 |

## 假设与开放问题

### 假设
- `ets_runtime` 的 `SendableGlobal<T>` 与进程级 Sendable Global Storage 稳定可用，本变更不改动 `ets_runtime`。
- Worker 的 `IsMainEnvContext()` 返回 true，任意 Worker 可成为 init 方（源码已澄清，非"主线程 vs Worker"标记）。
- Sendable Native Function 跨 VM 调用时的运行时环境切换机制（`JSNApi::GetEnv(vm)`）已稳定，本变更不复管。
- `.so` 固定驻留的内存代价可接受（换取无悬空风险与实现简单）。
- 二进制兼容由 API 选择保证：老 `.so` 只调 `napi_module_register`（不读 `napi_module` 新字段、不调 `napi_shared_module_register`），自然走普通路径；新共享模块调 `napi_shared_module_register`。无需版本门控，不改 `napi_module` 结构体。

### 开放问题
- canonical key 首版用 moduleName，跨 bundle/HSP 同名模块误共享风险待加固。
- `SetNativeEngine` 在 sendable 分支是否需跳过或每 VM 各设，待实现验证。

## 不涉及项确认

> `arkruntime` profile 要求 `perf / compatibility / api-sdk` 必填为「是」。

| 维度 | 是否涉及 | 依据 |
|------|---------|------|
| 性能 | 是 | 多 Worker 重复初始化消除、首次 import 延迟与 entry 锁等待、共享堆常驻增量、稳态 map 命中成本均需观测（profile 注入：内存占用基线） |
| 安全/权限 | 是 | 跨 Worker 共享导出根（跨信任边界）、`.so` 生命周期安全（函数地址悬空风险）、allowlist 旁路（API 暴露面变更）、canonical key 同名模块误共享风险 |
| 兼容性 | 是 | opt-in 不得破坏调 `napi_module_register` 的普通模块行为；避开 `nm_flags==100` App Module 历史值；`napi_module_with_js` 混合模块（首版不支持共享声明）；allowlist 对普通模块仍生效（profile 注入：ETS/ArkTS 标准兼容） |
| API/SDK | 是 | 新增公共 API 函数 `napi_shared_module_register`（`interfaces/kits/napi/native_api.h`，面向三方应用）；不改 `napi_module` 结构体、不读其新字段；`ace_napi.versionscript` 为 `global: *` 通配导出新符号 |
| IPC/跨进程 | 否 | 变更在进程内跨 VM/Worker（共享进程地址空间），不跨进程；无 Binder/IPC/Socket |
| 构建/组件 | 否 | 本变更不新增 `.cpp`、不改 `napi.gni`/`BUILD.gn`；不动 `ace_napi.versionscript` |
| 国际化/无障碍 | 否 | 引擎/模块管理器层，无 UI 文本/locale/无障碍语义 |
| 数据迁移 | 否 | 无持久化数据格式/配置迁移；模块缓存为运行时内存态 |
