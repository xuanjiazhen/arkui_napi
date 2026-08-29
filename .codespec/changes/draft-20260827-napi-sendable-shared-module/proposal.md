---
target_release: 7.1
change_type: new-feature
issue: ""
author: ""
date: "2026-08-27"
status: Draft
---

# Proposal

## 背景与问题

ArkTS 已支持通过 `"use shared"` 声明共享模块：模块记录、命名空间与导出值进入共享堆，由进程级共享模块管理器统一管理，不同 VM 复用同一 ModuleRecord 与共享 Namespace。

NAPI 模块当前仍按**普通本地模块**加载：每个 `ArkNativeEngine` 独立初始化模块、创建普通 `ObjectRef` 作为 exports、在本 VM 内缓存。即使运行时已具备从 NAPI 创建 Sendable Class/Object/Array/Map 及跨 VM 调用 Sendable Native Function 的基础，NAPI 模块本身也无法跨 VM 共享导出对象身份或避免重复初始化。

本变更区分两类目标：
1. **NAPI 导出的值可 Sendable 且跨 VM 共享**——导出对象身份在 Worker 间共享，native 初始化只执行一次。
2. **NAPI 模块本身是完整共享模块**——模块只初始化一次，不同 VM 获取同一 ModuleRecord/Namespace/导出，语义对齐 ArkTS `"use shared"`。

本变更**采用「共享导出根对象、ModuleRecord 保持本地」路径**（可行性分析中的路径 B）：native 初始化一次、返回完整 Sendable Root、经进程级 Sendable 句柄持有；各 VM 仍创建自己的本地 ModuleRecord，但获得同一个导出根对象。该路径复用现有 Sendable Object/Function 与进程级 Sendable Global Storage，可满足「共享导出对象身份 + 单次原生初始化」需求，但**不**对齐完整 ArkTS 共享模块语义（共享 ModuleRecord/Namespace/统一模块状态属非目标，对应路径 C）。

## 用户场景与业务触发

### US-1: 多 Worker 共享 NAPI 模块导出
- **角色**：应用开发者 / 系统模块开发者
- **业务触发**：应用创建多个 Worker，Worker 间需共享同一 NAPI 模块（如计算密集型 native 库）的导出对象与原生状态
- **业务价值**：避免每个 Worker 重复初始化 native 模块（降低 RSS/PSS、减少首次 import 延迟），并保证跨 Worker 原生状态一致

### US-2: Sendable 生态跨 Worker 复用 NAPI 能力
- **角色**：上层业务（如 TaskPool / Sendable 业务）
- **业务触发**：上层经 Sendable 对象在 Worker 间传递数据，需直接调用 NAPI 模块导出的 Sendable 原生方法
- **业务价值**：NAPI 模块导出可直接参与 Sendable 跨 Worker 传递生态，无需额外 ArkTS 包装模块

## 初始分级判断

> 变更类型见 frontmatter `change_type`（primary: new-feature；secondary: enhancement——在现有 NAPI 模块加载与 Sendable API 上增加共享导出分支）。

| 判断项 | 结果 | 依据 |
|--------|------|------|
| 变更类型 | new-feature（兼 enhancement） | 引入此前不存在的「NAPI 模块跨 VM 共享导出根 + 单次原生初始化」能力与用户场景；同时在现有 NAPI 模块加载/Sendable API 上新增行为分支 |
| 复杂度 | 复杂 | 跨仓（napi + ets_runtime）；触及 Ark VM 线程安全不变量、共享堆、.so 生命周期、跨线程 native callback；预估 900~1300 行 |
| 涉及仓数量 | 2 | foundation/arkui/napi、arkcompiler/ets_runtime |
| 是否涉及 Public/System API | 是 | 需新增共享声明接口（nm_flags 标志或版本化注册函数），触及公共 NAPI 头与符号版本表 |
| 是否涉及安全/性能关键路径 | 是 | 跨 VM/Worker 传递共享对象、.so 生命周期（dlclose 悬空风险）、native callback 跨线程执行、Ark VM 线程安全不变量 |
| 是否跨 SIG | 是（待确认） | napi 属 arkui 域、ets_runtime 属 arkcompiler 域，跨仓通常跨 SIG |

## 目标

1. NAPI 模块可经显式 opt-in 声明为「共享导出」，native init callback 在进程内只执行一次。
2. 不同 `ArkNativeEngine`/VM 导入同一共享 NAPI 模块时，获得**对象身份相同**的 Sendable 导出根。
3. 共享导出根由进程级 Sendable 句柄（复用现有 `SendableGlobal<T>` / Sendable Global Storage）持有，不新建进程级缓存基础设施。
4. 导出根上的 Sendable method 在消费方 VM 上运行时动态获取当前 `napi_env`（复用已有 Sendable Native Function 跨 VM env 切换基础）。
5. 导出根构造时校验 Sendable 类型，禁止将 LocalHeap 对象写入共享导出根。
6. .so 生命周期受保护：共享导出根存活期间禁止 `dlclose`，避免 native callback 地址悬空。
7. 明确并落地的线程安全契约：native callback 可在不同 Worker 线程执行，模块原生状态须线程安全。

> secondary 类型（enhancement）体现为：在现有 NAPI 模块加载协议与 Sendable API 上新增共享导出分支，而非替换现有加载语义。

## 非目标

1. **共享 ModuleRecord**——每 VM 仍创建本地 Native ModuleRecord（属路径 C）。
2. **共享 Namespace**——每 VM 仍持有本地 Namespace（属路径 C）。
3. **全局统一的模块状态/循环依赖检测/错误状态传播**——模块级状态仍为每 VM 本地（属路径 C）。
4. **完整对齐 ArkTS `"use shared"` 模块语义**——不接入 `SharedModuleManager` 的解析/竞争插入/共享 Namespace（属路径 C）。
5. 不修改 ArkTS 共享模块编译器与 ABC 产物。
6. 不含编译器、IDE、SDK 发布流程，不做大规模应用迁移。
7. 不改变普通 NAPI 模块（未声明共享）的加载与 exports 赋值语义。

## Agent Scope Guard

> 约束 AI/Agent 的探索和修改边界。若需要突破边界，先更新本 proposal 并获得确认。

| 维度 | 范围/限制 | 需人工确认的触发条件 |
|------|-----------|----------------------|
| 允许仓库 | foundation/arkui/napi、arkcompiler/ets_runtime | 需触及第三仓 |
| 允许模块/目录 | napi: `native_engine/`、`module_manager/`、`interfaces/kits/napi/`、`interfaces/inner_api/napi/`；ets_runtime: `ecmascript/`（jsnapi_expo、runtime、sendable global storage 相关） | 需修改范围外模块/目录 |
| 禁止修改项 | 公共 API 签名/错误码/生命周期语义（未经确认）；Ark VM 线程安全不变量；跨线程传递 `napi_env`；`napi_module_register` 之外的 .so 手动加载；现有 DFX/日志/错误码删除 | 任一禁止项需被触碰 |
| 外部依赖/网络访问 | 无新增三方依赖；无网络访问 | 新增依赖或需联网 |

## 成功标准

> 系统级能力达成（可观察、可量化）。禁内部实现 + 禁接口细节（属 spec AC）。

| 标准 | 可观察指标 | 验证方式 |
|------|-----------|---------|
| 多 Worker 共享导出身份 | ≥2 Worker 导入同一共享 NAPI 模块，获得的导出根对象身份相同（跨 VM 引用相等） | 多 Worker 导入 + 对象身份比对 |
| 单次原生初始化 | 多 Worker 并发首次 import，native init 回调执行次数 = 1 | 并发导入 + 初始化计数 |
| 向后兼容 | 未声明共享的普通 NAPI 模块，加载与 exports 赋值语义与变更前逐项一致 | 普通 NAPI 模块回归用例 |
| 导出根 Sendable 约束 | 共享模块导出根含非 Sendable 值（普通 Object/Function/LocalHeap 引用）时加载被拒绝，模块进入一致错误状态 | 异常用例：返回普通 Object/Function/空值/含 LocalHeap 引用 |
| 跨 VM 生命周期 | 一个 Worker VM 销毁后，其余 VM 调用共享导出函数不崩溃且行为正确 | VM 销毁 + 其余 VM 调用 |
| .so 不悬空 | 共享导出根存活期间对应 .so 不被卸载，无函数地址悬空崩溃 | 共享根存活期间触发卸载路径 |
| 内存占用基线 | 多 Worker 场景 RSS/PSS 较「每 VM 独立加载」不升高，共享堆常驻增量有界 | 多 Worker 内存采集与对比 |

## 影响范围

| 子系统 | 仓库 | 模块/路径 | 影响类型 |
|--------|------|-----------|---------|
| arkui | foundation/arkui/napi | `native_engine/impl/ark/`（共享 Load 分支、callback 返回值、Sendable Root 校验）、`module_manager/`（shared 属性持久化、canonical key、.so 生命周期）、`interfaces/kits/napi/`、`interfaces/inner_api/napi/`（共享声明接口） | 修改 |
| arkcompiler | arkcompiler/ets_runtime | `ecmascript/`（`jsnapi_expo`、`runtime`、sendable global storage——提供共享根持有与桥接能力） | 修改/复用 |

## 假设与开放问题

### 假设
- 路径 B 复用现有 `SendableGlobal<T>` 与进程级 Sendable Global Storage，不新建进程级缓存基础设施。
- Sendable Native Function 跨 VM env 切换基础已具备（消费方 VM 动态获取当前 env）。
- 普通 NAPI 模块语义必须保持不变，共享能力为显式 opt-in。
- Ark VM 非线程安全：native callback 可在不同 Worker 线程执行，模块原生状态须自行线程安全。
- 内存占用需维持基线：多 Worker RSS/PSS 不升高，共享堆常驻增量有界。

### 开放问题
- **共享声明接口形态**：复用 `nm_flags`（需先修复 flags 未持久化 + 历史值 100 兼容）还是新增版本化注册函数？——影响 spec API AC 与公共符号。
- **进程级模块 key 的 canonical 形式与隔离维度**（bundle/namespace/加载域）——影响 spec 隔离 AC，避免不同应用/HSP 同名模块误共享。
- **.so 生命周期语义**：进程驻留 vs 强引用计数延迟卸载——影响 design 生命周期设计。
- **allowlist 兼容**：当前 allowlist 路径生成 LocalHeap 副本会破坏共享根，需改为构造前过滤——影响 design。
- **首期支持的模块类型范围**（APP/OHOS/INTERNAL/STATIC + `napi_module_with_js`）——影响 spec 范围。
- **`napi_module_with_js` 共享语义来源**：由 ABC Shared 标记还是 Native flags 决定——影响 spec。

## 不涉及项确认

> 逐维度主动判断「涉及/不涉及」并填写依据；不允许留空；标「不涉及」必须写明理由。
> Profile `arkruntime` required_dimensions: [perf, compatibility, api-sdk] → 三维判「是」。

| 维度 | 是否涉及 | 依据 |
|------|---------|------|
| 性能 | 是 | 共享根减少重复初始化/重复导出对象；但共享根+共享堆常驻+模块锁竞争需观测（init 次数、import 延迟、RSS/PSS、共享堆增量）；profile 要求 |
| 安全/权限 | 是 | 跨 VM/Worker 经共享堆传递对象（跨信任边界）；.so 生命周期（dlclose 悬空内存安全）；native callback 跨线程执行；触及 Ark VM 线程安全不变量——命中安全判定，design 阶段须产出安全基础检查；命中「关键安全组件」（Ark VM 线程安全不变量 + .so 生命周期内存安全），建议进一步评估 `threat-model.md` |
| 兼容性 | 是 | 普通 NAPI 模块语义必须不变（opt-in）；`nm_flags` 历史值 100 兼容；公共 API 向后兼容；profile 要求 |
| API/SDK | 是 | 需新增共享声明接口（nm_flags flag 或版本化注册函数）；公共 NAPI 符号、符号版本表、SDK 接口版本需同步维护；profile 要求 |
| IPC/跨进程 | 否 | 共享范围为进程级（同进程多 VM/Worker），不跨进程；无 Binder/Socket/共享内存跨进程通信 |
| 构建/组件 | 否 | 不新增构建 target/特性开关；公共符号变更时同步 `ace_napi.versionscript` 属 API/SDK 维度连带，非独立构建改动；新增 .cpp 按 AGENTS.md 规则加入 `napi_sources` 属常规 |
| 国际化/无障碍 | 否 | 纯运行时/模块加载能力，无 UI 文案、无无障碍、无多语言资源 |
| 数据迁移 | 否 | 无持久化数据格式变更；模块缓存为运行时内存态，非持久化存储 |
