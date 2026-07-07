# 需求文档

> 一份文档，从原始需求到基线结论。按阶段追加内容，不拆成多份独立文件。
>
> **复杂度裁剪**：简单（单仓小修）。仅填核心字段，跳过方案探索与上下文检索日志。

## 一、原始需求

### 基本信息

| 字段 | 内容 |
|------|------|
| 需求ID | fix-find-cache-name-nullptr |
| 需求名称 | FindNativeModuleByCache 增加 temp->name nullptr 判空 |
| 来源 | 缺陷反馈（用户提供） |
| 提出人 | 用户（zhang_hao_zheng） |
| 目标发行版本 | OpenHarmony-master |
| 候选 Profile | none（arkui_napi 仓位于 arkui 目录下，但 napi 是 native 模块加载基础设施，与 UI 渲染/交互无关，不命中 arkui profile 触发条件） |
| 优先级 | P1（潜在崩溃，但触发条件罕见） |
| 状态 | Baselined |

### 原始描述

**原始问题：** 在 `module_manager/native_module_manager.cpp:1632-1634` 的 `FindNativeModuleByCache` 中，遍历链表时直接对 `temp->name` 调用 `strcasecmp(temp->name, moduleName)`。`NativeModule::name` 默认值为 `nullptr`（见 `native_module_manager.h:58`），且 `RegisterByBuffer` 在 `strdup` 失败路径会把 `moduleName` 置 nullptr 并 return（line 1548-1553），此时节点 `name` 字段保持 nullptr。一旦链表中存在 `name == nullptr` 的节点，`strcasecmp(nullptr, ...)` 将解引用空指针导致崩溃。

**痛点：**

| 用户类型 | 当前痛点 | 影响 |
|----------|----------|------|
| 系统开发者 | 在内存紧张导致 `strdup` 失败等极端场景下，模块加载查缓存路径可能崩溃 | 设备重启或模块加载失败 |

**期望结果：** `FindNativeModuleByCache` 在 `temp->name` 为 nullptr 时跳过 `strcasecmp` 调用，不崩溃。

### 背景证据

| 证据类型 | 链接/路径 | 说明 |
|----------|-----------|------|
| 源码 | `module_manager/native_module_manager.cpp:1632-1634` | 问题点 |
| 源码 | `module_manager/native_module_manager.cpp:1546-1553` | strdup 失败导致 name=nullptr 的路径 |
| 源码 | `module_manager/native_module_manager.h:58` | `name = nullptr` 默认值定义 |

### 初始范围

**可能包含：**
- `FindNativeModuleByCache` 链表遍历的 nullptr 判空加固
- 对应单元测试（覆盖 name==nullptr 节点的查缓存场景）

**明确不包含：**
- `CheckNativeListChanged`（L998-999）的同类 `strcmp(->name, ->name)` 隐患
- `RegisterByBuffer`（L1546-1553）strdup 失败时的节点内存管理（与本问题无直接因果）
- 其他 `str*` 调用点的统一加固
- `moduleName` 入参的 nullptr 保护（所有调用点均为 `std::string::c_str()`，实际不会 nullptr）

### 初始假设

| 假设 | 类型 | 验证方式 | 状态 |
|------|------|----------|------|
| 修复仅触碰 L1634 一行 + 一个新单测 | 技术 | 源码核对 | 已验证 |
| 入参 moduleName 不会 nullptr | 技术 | 源码搜索所有 FindNativeModuleByCache 调用点 | 已验证 |

### 初始分级判断

| 判断项 | 结果 | 依据 |
|--------|------|------|
| 复杂度 | 简单 | 单仓单文件单点加固 + 1 个单元测试 |
| 涉及仓数量 | 1 | arkui_napi |
| 是否涉及 Public/System API | 否 | 纯内部链表操作，无 API 变更 |
| 是否涉及安全/性能关键路径 | 否 | 不在安全/性能关键路径 |
| 是否跨 SIG | 否 | 单仓内修复 |

### 进入澄清条件

- [x] 原始问题和期望结果已记录
- [x] 需求来源和责任人已明确
- [x] 初始范围和不包含项已记录
- [x] 关键假设和待澄清问题已列出
- [x] 复杂度有判断或明确为待定

---

## 二、澄清记录

> 简单复杂度，逐项澄清后请需求方确认。

### 待澄清问题

| 编号 | 问题 | 为什么需要澄清 | 状态 | 结论 |
|------|------|----------------|------|------|
| Q-1 | 修复范围如何界定？ | 同类隐患散布，需明确本次是否扩展到 L998-999 | 已澄清 | **仅修 L1634**，其他同类点本次不动 |
| Q-2 | 是否补充单元测试？ | 修复建议第 3 条提及，需明确是否纳入范围 | 已澄清 | **补单测**：覆盖 name==nullptr 节点查缓存场景 |
| Q-3 | 是否有关联 issue 编号？ | 用于 `.codespec/changes/` 目录命名 | 已澄清 | 无 issue 编号，使用 slug `fix-find-cache-name-nullptr` |

### 讨论记录

| 日期 | 参与人 | 讨论主题 | 结论 | 后续动作 |
|------|--------|----------|------|----------|
| 2026-07-06 | 用户 / Agent | 修复范围 | 仅 L1634 | 写 proposal |
| 2026-07-06 | 用户 / Agent | 单测 | 补 name==nullptr 用例 | 写 spec |
| 2026-07-06 | 用户 / Agent | issue 编号 | 无，用 slug | 目录命名 |

### 功能范围确认

| 问题 | 回答 | 确认人 | 状态 |
|------|------|--------|------|
| 核心功能包含哪些？ | L1634 判空加固 + 1 个 name==nullptr 单测 | 用户 | 已确认 |
| 明确不包含哪些？ | L998-999、L1546-1553 内存管理、其他 str* 调用点、moduleName 入参保护 | 用户 | 已确认 |
| 是否有分期策略？ | 否，一次性修复 | 用户 | 已确认 |

### 方案探索

> 简单变更跳过。修复方案唯一明确：在 L1634 `||` 右侧加 `(temp->name != nullptr && ...)` 短路判空。无其他可选技术路线。

### 上下文与知识源检索日志

> 简单变更按裁剪表跳过。但目标仓 Agent 指南（AGENTS.md）必须读取并记录关键约束（硬规则）。

**目标仓 Agent 指南约束（来自 `foundation/arkui/napi/AGENTS.md`）：**
- 构建系统：GN（非 Make/CMake），主入口 `BUILD.gn` + `napi.gni`
- 构建目标：`//foundation/arkui/napi:napi_packages`
- 测试目标：`//foundation/arkui/napi:napi_packages_test`
- 编译命令：`hb build napi -t`（含测试）/ `hb build napi -i`（不含测试）
- 实现要点：错误处理使用 `NativeErrorExtendedInfo`；线程安全需调用方保证；Public API 在 `interfaces/kits/napi/native_api.h`，内部 API 在 `interfaces/inner_api/napi/native_node_api.h`
- 平台：OHOS / Linux / Windows / macOS / iOS / Android；架构 ARM64/ARM32/x86_64/x86
- 数据保护在 ARM64 OHOS 平台自动启用

### 子系统影响

| 问题 | 回答 | 确认人 | 状态 |
|------|------|--------|------|
| 涉及哪些子系统？ | ArkUI 子系统下的 napi 部件 | 用户 | 已确认 |
| 是否需要新增子系统或部件？ | 否 | 用户 | 已确认 |

### API 变更评估

| 问题 | 回答 | 确认人 | 状态 |
|------|------|--------|------|
| 是否需要新增/修改 Public API？ | 否 | 用户 | 已确认 |
| 是否需要新增 System API？ | 否 | 用户 | 已确认 |
| 是否会废弃已有 API？ | 否 | 用户 | 已确认 |
| 是否需要新增权限声明？ | 否 | 用户 | 已确认 |

### 兼容性与非功能需求

| 类别 | 核心问题 | 结论 | 确认人 | 状态 |
|------|----------|------|--------|------|
| 兼容性 | 行为不变，仅在原本会崩溃的场景改为不崩溃 | 向前兼容 | 用户 | 已确认 |
| 性能 | 多一次 nullptr 判空，可忽略 | 无影响 | 用户 | 已确认 |
| 安全 | 不涉及 | N/A | 用户 | 已确认 |
| 可靠性 | 修复潜在崩溃，提升可靠性 | 提升 | 用户 | 已确认 |

### 依赖与风险

| 依赖项 | 类型 | 说明 | 状态 |
|--------|------|------|------|
| 无 | - | 单仓单文件修复 | 已确认 |

| 风险 | 类型 | 影响 | 缓解措施 | 状态 |
|------|------|------|----------|------|
| 单测无法注入 name==nullptr 节点 | 技术 | 低 | 已验证测试框架支持（`new NativeModule()` 默认 name=nullptr，且 `headNativeModule_` 在测试中可访问） | 已缓解 |

### AC 完整性

- [x] 每个用户故事有验收标准
- [x] AC 全部使用 WHEN/THEN 格式
- [x] 覆盖正常流程、异常流程、边界条件
- [x] AC 可测试、可度量

### 澄清结论

- [x] 功能范围已完全明确
- [x] 子系统影响已识别
- [x] API 变更已评估（无变更）
- [x] 兼容性和非功能需求已确认
- [x] 依赖和风险已识别且有缓解方案
- [x] AC 完整可测试
- [x] 简单变更跳过方案探索（理由：技术路线唯一）
- [x] 目标仓 Agent 指南（AGENTS.md）已读取，关键约束已记录

**结论:** 通过

---

## 三、需求基线

> 澄清完成后固化。manifest.md 是事实源，此处为审批结论。

### 基线信息

| 字段 | 内容 |
|------|------|
| 基线版本 | v1.0 |
| 基线日期 | 2026-07-06 |
| Owner | zhang_hao_zheng |
| 确认人 | 用户（zhang_hao_zheng） |
| 复杂度 | 简单 |
| Profile | none |
| 目标发行版本 | OpenHarmony-master |
| 版本状态 | committed |

### 问题陈述

`FindNativeModuleByCache` 在遍历模块链表时，对 `temp->name` 调用 `strcasecmp` 未判空。`NativeModule::name` 默认 nullptr，且 `RegisterByBuffer` 在 strdup 失败路径会留下 name==nullptr 的节点。一旦命中，触发空指针解引用崩溃。本次修复在该路径加 nullptr 短路，避免崩溃。

### 目标和成功指标

| 目标 | 成功指标 | 验证方式 |
|------|----------|----------|
| 避免 strcasecmp 空指针解引用 | 链表中存在 name==nullptr 节点时，FindNativeModuleByCache 不崩溃且返回正常结果 | 新增单元测试通过 |

### 用户故事与 AC

| Story ID | 用户故事 | 优先级 |
|----------|----------|--------|
| US-1 | 作为系统开发者，我希望 FindNativeModuleByCache 在链表节点 name 字段为 nullptr 时不崩溃，以便在内存紧张等极端场景下保持模块加载路径的健壮性。 | P1 |

| AC编号 | 验收标准 | 类型 | 关联Story |
|--------|----------|------|-----------|
| AC-1 | WHEN 链表中存在某个节点 `name == nullptr` 且其他节点均不匹配传入的 moduleName THEN FindNativeModuleByCache 正常返回（不崩溃），返回结果为 nullptr 或匹配节点（不含 name==nullptr 节点的误匹配） | 边界 | US-1 |
| AC-2 | WHEN 链表中存在某个节点 `name == nullptr` 且另一节点 moduleName 匹配 THEN FindNativeModuleByCache 返回匹配节点（验证短路左项 moduleName 仍可正常工作，name==nullptr 节点不影响匹配） | 正常 | US-1 |
| AC-3 | WHEN 调用所有现有 FindNativeModuleByCache 相关单元测试 THEN 全部通过（不引入回归） | 回归 | US-1 |

### 范围边界

**包含：**
- `module_manager/native_module_manager.cpp:1632-1634` 的 `||` 右侧加 nullptr 短路判空
- 在 `module_manager/test/unittest/module_manager_test/module_manager_test.cpp` 新增 1 个 HWTEST 用例（参照 `FindNativeModuleByCache_*` 命名约定）

**不包含：**
- L998-999 `CheckNativeListChanged` 同类隐患
- L1546-1553 `RegisterByBuffer` strdup 失败的节点内存管理
- 其他 `str*` 调用点加固
- `moduleName` 入参 nullptr 保护

### 影响范围

| 子系统 | 仓库 | 模块/路径 | 当前职责 | 影响类型 | Owner |
|--------|------|-----------|----------|----------|-------|
| ArkUI | arkui_napi | `module_manager/native_module_manager.cpp` | native 模块链表与加载 | 修改 | zhang_hao_zheng |
| ArkUI | arkui_napi | `module_manager/test/unittest/module_manager_test/module_manager_test.cpp` | 单元测试 | 新增测试 | zhang_hao_zheng |

### API 变更项清单

> N/A — 无 API 变更，纯内部链表操作加固。

### 不涉及项确认

| 维度 | 涉及？ | 依据 | 若涉及，进入哪个下游文档 |
|------|--------|------|--------------------------|
| 性能 | 否 | 仅多一次 nullptr 判空，可忽略 | N/A |
| 安全与权限 | 否 | 不涉及权限/隐私/加密 | N/A |
| 兼容性 | 否 | 行为兼容（原崩溃路径改为不崩溃） | N/A |
| API/SDK | 否 | 无 API 变更 | N/A |
| IPC/跨进程 | 否 | 同进程链表操作 | N/A |
| 构建与部件 | 否 | 不改 BUILD.gn / bundle.json | N/A |
| 国际化/无障碍 | 否 | 不涉及用户可见字符串或 UI | N/A |
| 数据迁移 | 否 | 无数据迁移 | N/A |

### 变更控制

| 变更类型 | 触发条件 | 处理规则 |
|----------|----------|----------|
| 范围新增 | 决定顺带修 L998-999 或其他点 | 重新评估复杂度（可能升级为标准） |
| AC 变更 | 修改可观察行为 | 重新审批基线 |

### 进入设计/Spec 条件

- [x] 所有 P0/P1 用户故事有 AC
- [x] 每条 AC 可测试、可度量
- [x] 范围内/外已确认
- [x] `manifest.target_release` 已确认
- [x] `manifest.profile` 已确认（none）
- [x] 涉及仓、模块、SIG 已识别
- [x] 不涉及项已标记 N/A
- [x] 变更控制规则已确认
- [x] 简单变更跳过逐项澄清的方案探索（理由：技术路线唯一）
- [x] 简单变更跳过完整上下文检索日志（按裁剪表）；AGENTS.md 关键约束已记录
- [x] 目标仓 Agent 指南已检查并记录关键约束

**基线结论:** 通过（用户于 2026-07-06 批准，进入 Stage 2）

---

## 四、问题 #2（基线扩展：同模式追加）

> 用户决定：本 SDD 流程作为"批量 NativeModule nullptr 加固"容器，后续 ~20 个同模式问题共用本流程，每个问题独立判断、独立 AC/TASK/测试。问题 #1 已批准（见上方一/二/三章），本节为问题 #2 的独立判断。

### 问题 #2 基本信息

| 字段 | 内容 |
|------|------|
| 问题编号 | #2 |
| 问题名称 | FindNativeModuleByCache systemFilePath nullptr 判空加固 |
| 来源 | 缺陷反馈（用户提供，2026-07-06） |
| 优先级 | P1（潜在崩溃，触发条件罕见） |
| 复杂度 | 简单（同问题 #1 模式） |
| 状态 | Clarifying |

### 问题 #2 原始描述

**问题：** 在 `native_module_manager.cpp:1637-1641`，`FindNativeModuleByCache` 在 if 条件成立后的内层中，调用 `strcmp(temp->systemFilePath, nativeModulePath[label])`（L1637）和 `!strcmp(temp->systemFilePath, "")`（L1641），未判 `temp->systemFilePath` 是否 nullptr。`NativeModule::systemFilePath` 默认 nullptr（`native_module_manager.h:61`），且：
- `RegisterByBuffer`（L1546-1556）路径未设置 `systemFilePath`，依赖 `new NativeModule()` 默认 nullptr
- `RegisterByDisk`（L1460/1508）路径 `strdup(loadPath)` 失败也产生 nullptr

一旦命中 `systemFilePath == nullptr` 节点进入 if 内层，`strcmp(nullptr, ...)` 段错误。

**期望结果：** `systemFilePath` 为 nullptr 时，安全跳过 `strcmp`，不崩溃。

### 问题 #2 澄清记录

| 编号 | 问题 | 结论 |
|------|------|------|
| Q2-1 | 修复策略 | **最小修复**（L1637/1641 加 `temp->systemFilePath != nullptr` 判空，不动创建路径、不动默认值） |
| Q2-2 | 单测补充 | **补单测**（新增 `FindNativeModuleByCache_WhenNodeSystemFilePathIsNullptrShouldNotCrash`，同问题 #1 模式） |

### 问题 #2 范围边界

**包含：**
- `native_module_manager.cpp:1637` `strcmp` 第一个参数前加 `temp->systemFilePath != nullptr &&`
- `native_module_manager.cpp:1641` `!strcmp` 改为 `(temp->systemFilePath != nullptr && !strcmp(temp->systemFilePath, ""))`
- 新增测试 `FindNativeModuleByCache_WhenNodeSystemFilePathIsNullptrShouldNotCrash`

**不包含：**
- L1460/1508 `strdup(loadPath)` 失败路径加固
- 字段初始化约束（在所有创建路径强制 `systemFilePath = ""`）
- 结构体默认值调整（`.h:61` `nullptr` → `""`）
- 其他 `systemFilePath` 使用点（L117/1581/1604 已判空，无需改）

### 问题 #2 用户故事与 AC

| Story ID | 用户故事 | 优先级 |
|----------|----------|--------|
| US-2 | 作为系统开发者，我希望 FindNativeModuleByCache 在链表节点 systemFilePath 字段为 nullptr 时不崩溃，以便在 RegisterByBuffer/strdup 失败等场景下保持模块加载路径健壮性 | P1 |

| AC编号 | 验收标准 | 类型 | 关联Story |
|--------|----------|------|-----------|
| AC-2.1 | WHEN 链表中某节点通过 moduleName/name 匹配（if 外层成立）但 `systemFilePath == nullptr` THEN FindNativeModuleByCache 不崩溃，并返回该节点（按最小修复实现：nullptr 在 while 短路，label 保持 0 < NATIVE_PATH_NUMBER，被视为 path 0 匹配；此为"避免崩溃"的可接受行为变更） | 边界 | US-2 |
| AC-2.2 | WHEN 链表中存在 `systemFilePath == nullptr` 节点 + 另一正常节点（systemFilePath=""）匹配 THEN FindNativeModuleByCache 不崩溃（验证 nullptr 节点存在不影响其他节点的匹配流程） | 正常 | US-2 |
| AC-2.3 | WHEN 运行既有全部 FindNativeModuleByCache_* 单元测试 THEN 全部通过（不引入回归） | 回归 | US-2 |

### 问题 #2 兼容性

行为兼容：原本会崩溃的路径（systemFilePath==nullptr 进入 if 内层）改为安全跳过。原本不崩溃的路径行为完全不变（短路判空仅在 nullptr 时改变结果）。

### 问题 #2 基线结论

**通过**（待用户批准本基线扩展）

---

## 五、问题 #3（基线扩展：EmplaceModuleBuffer 重复 key 内存泄漏）

### 问题 #3 基本信息

| 字段 | 内容 |
|------|------|
| 问题编号 | #3 |
| 问题名称 | EmplaceModuleBuffer 重复 moduleKey 导致 buffer 堆内存泄漏 |
| 来源 | 缺陷反馈（用户提供，2026-07-06） |
| 优先级 | P1（长时间运行导致内存增长） |
| 复杂度 | 简单（单函数修复 + 1 测试） |
| 状态 | Clarifying |

### 问题 #3 原始描述

**问题：** `EmplaceModuleBuffer`（L207-214）调用 `moduleBufMap_.emplace(moduleKey, lib)`。当 moduleKey 已存在时，emplace 不覆盖旧值，新分配的 lib 缓冲区既未入库也未释放。上游 `GetFileBuffer`（L1320-1326）通过 `buffer.release()` 将所有权转移给管理器，重复加载/并发场景下这块堆内存永久泄漏。

**期望结果：** 重复 key 时，释放旧 buffer，入库新 buffer（覆盖语义），不泄漏。

### 问题 #3 澄清记录

| 编号 | 问题 | 结论 |
|------|------|------|
| Q3-1 | 修复范围 | **仅修 EmplaceModuleBuffer（L207-214）**。问题描述里附带提到 EmplaceModuleLib (L173-180) "存在同样问题"——按用户规则"每次仅修问题描述本身"，EmplaceModuleLib 显式排除，留作单独问题 |
| Q3-2 | 修复策略 | **覆盖前释放旧值**（按用户给的参考代码：find → 重复则 `delete[] it->second; it->second = lib;` 否则 emplace） |
| Q3-3 | 单测补充 | **补单测**（同前问题模式）：构造同一 moduleKey 两次调用，验证 map size=1 + 覆盖语义 + 旧值被释放 |

### 问题 #3 范围边界

**包含：**
- `native_module_manager.cpp:207-214` EmplaceModuleBuffer 修复（find + 覆盖或释放）
- 新增测试 `EmplaceModuleBuffer_ShouldReleaseOldBufferOnDuplicateKey`

**不包含：**
- **EmplaceModuleLib (L173-180)**：留作单独问题（按用户规则）
- RemoveModuleBuffer (L216-227) / RemoveModuleLib (L182-194)：erase 不 delete[]，是另一类问题，不在本次范围
- moduleBufMap_ / moduleLibMap_ 类型改为 RAII 智能指针：超出范围（架构级重构）
- 接口契约统一（"调用方所有权语义"）：超出范围
- 上游 GetFileBuffer 并发查缓存间隙：超出范围（如需修，单独提）

### 问题 #3 用户故事与 AC

| Story ID | 用户故事 | 优先级 |
|----------|----------|--------|
| US-3 | 作为系统开发者，我希望 EmplaceModuleBuffer 在重复 moduleKey 时释放旧 buffer 入库新 buffer，以便长时间运行的模块加载流程不积累堆内存泄漏 | P1 |

| AC编号 | 验收标准 | 类型 | 关联Story |
|--------|----------|------|-----------|
| AC-3.1 | WHEN 同一 moduleKey 第二次调用 EmplaceModuleBuffer(key, lib2) 且 map 中已有 lib1 THEN 修复后 moduleBufMap_.size() == 1，moduleBufMap_[key] == lib2（覆盖语义） | 正常 | US-3 |
| AC-3.2 | WHEN 同一 moduleKey 第二次调用且 map 中已有 lib1 THEN 修复后 lib1 被 `delete[]`（旧值释放，避免泄漏） | 边界 | US-3 |
| AC-3.3 | WHEN 首次调用 EmplaceModuleBuffer(key, lib) THEN 行为不变（emplace 入库），向后兼容 | 回归 | US-3 |
| AC-3.4 | WHEN 调用既有全部单元测试 THEN 全部通过（不引入回归） | 回归 | US-3 |

### 问题 #3 兼容性

行为变更：原"重复 key 不覆盖、新值泄漏" → 修复后"重复 key 覆盖、旧值释放"。原本不重复的路径行为完全不变（AC-3.3）。原本依赖"重复 key 不覆盖"的调用方需重新审视——经核对调用方仅 GetFileBuffer（L1325），其语义本就是缓存，覆盖语义符合预期。

### 问题 #3 基线结论

**通过**（待用户批准本基线扩展）

---

## 问题 #4 区段（同模式追加：GetNativeModulePath operator[] 默认插入 nullptr）

### 问题 #4 原始需求

**问题概述：** operator[] 默认插入 nullptr 后被 sprintf_s 解引用。

**问题描述：** `module_manager/native_module_manager.cpp` 第 1058-1061 行 `GetNativeModulePath` 中，`prefix = appLibPathMap_[path]` 使用 `std::map::operator[]`，键不存在时插入默认构造值（nullptr），随后 prefix 被传给 `sprintf_s("%s/...", prefix, ...)`，触发空指针解引用。即使外层 `IsExistedPath(path)` 已确认键存在，但 `IsExistedPath` 与 `operator[]` 不在同一临界区（`IsExistedPath` 内部加锁释放后返回，再独立加锁读），并发场景下另一线程可能通过 `SetAppLibPath`/析构删除该键。

更严重的是：即便不考虑竞态，`operator[]` 写法本身会污染 map（向 map 插入 `path → nullptr`），后续 `IsExistedPath` 类查询会返回 true 但值为空，最终 `sprintf_s` 解引用 nullptr 触发崩溃。

**用户建议修复（一次性完成三项）：**
1. 使用 `find` 替代 `operator[]`：避免默认插入
2. 持锁内做完整读取：将 `IsExistedPath` 的判断与读取合并到同一个临界区
3. 空指针校验：读取后判断 `prefix != nullptr` 再使用，nullptr 时走 fallback 分支

**用户建议参考代码：**
```cpp
if (isAppModule) {
    std::lock_guard<std::mutex> guard(appLibPathMapMutex_);
    auto it = appLibPathMap_.find(path);
    if (it != appLibPathMap_.end()) {
        prefix = it->second;
    }
}
if (prefix == nullptr) {
    // 走 fallback 分支
}
```

**影响评估：**
- 严重程度：可导致 native 进程崩溃
- 影响范围：应用模块路径构造
- 触发条件：map 被污染或并发删除
- 潜在后果：拒绝服务 / 应用启动失败
- 修复优先级：本周内完成

### 问题 #4 澄清记录

| 编号 | 问题 | 结论 |
|------|------|------|
| Q4-1 | 单元测试 | **补单测**（同 #1/#2/#3 模式）：通过 mock 头的 `#define private public` 暴露 `GetNativeModulePath`（private），向 `appLibPathMap_` 注入 `key → nullptr` 节点，验证不崩溃、不污染 map、走 fallback |
| Q4-2 | 修复粒度 | **按用户建议完整修复**（一次性完成 find + lock_guard + nullptr fallback 三项）。原 if 内的 `#ifdef ANDROID_PLATFORM` 块（含其自带的 `if (prefix == nullptr) return false;`）原样保留——属"无害死代码"，最小变更不删 |

### 问题 #4 范围边界

**包含：**
- `native_module_manager.cpp:1057-1095` `GetNativeModulePath` 起始 if/else 块重构（按用户建议）
- 新增测试 `GetNativeModulePath_WhenAppLibPathValueIsNullptrShouldNotCrash`

**不包含（严格遵守"每次仅修问题描述本身"规则）：**
- **`IsExistedPath` 实现本身**（L1676-1681）：不在本问题范围，仅减少 L1058 处的冗余调用
- **其他 `operator[]` 使用点**（如 L1109 IOS_PLATFORM 分支 `appLibPathMap_["default"]`、L301/885/924/1126/1178/1201/1286/1404/1414 的 `IsExistedPath` 调用）：留作单独问题
- **代码审查清单项**（用户建议"加入禁止对 string-keyed 原始指针 map 使用 operator[] 检查项"）：超出代码修复范围，不在本次实现内
- **map 类型改为 RAII**：架构级重构，超出范围
- **删除原 ANDROID_PLATFORM 块内已死代码 `if (prefix == nullptr) return false;`**：最小变更原则不删

### 问题 #4 用户故事与 AC

| Story ID | 用户故事 | 优先级 |
|----------|----------|--------|
| US-4 | 作为系统开发者，我希望 `GetNativeModulePath` 在读取 `appLibPathMap_` 时不污染 map、不解引用空指针，以便应用模块路径构造在并发或异常数据场景下不崩溃 | P1 |

| AC编号 | 验收标准 | 类型 | 关联Story |
|--------|----------|------|-----------|
| AC-4.1 | WHEN `appLibPathMap_` 中某 key 存在但值为 nullptr 且 isAppModule=true THEN `GetNativeModulePath` 不崩溃（prefix 短路 nullptr 走 fallback），且 map 未被新增污染（size 不变） | 边界 | US-4 |
| AC-4.2 | WHEN `appLibPathMap_` 中 key 不存在（find 返回 end）且 isAppModule=true THEN `GetNativeModulePath` 不通过 operator[] 插入 `key → nullptr` 污染 entry（map size 不变），走 fallback | 边界 | US-4 |
| AC-4.3 | WHEN `appLibPathMap_` 中 key 存在且值非空 THEN `GetNativeModulePath` 行为与原修复前一致（prefix 取自 map，进入 if 内层），向后兼容 | 回归 | US-4 |
| AC-4.4 | WHEN 调用既有全部单元测试 THEN 全部通过（不引入回归） | 回归 | US-4 |

### 问题 #4 兼容性

**行为变更：**
- 原行为（key 不存在或值为 nullptr）：`operator[]` 污染 map + 进入 if 分支后 `sprintf_s` 解引用 nullptr 崩溃
- 修复后行为：`find` 不污染 map；prefix==nullptr 时走 fallback 分支（不进入 if 内层），不崩溃
- 原行为（key 存在且值非空）：进入 if 内层使用 prefix，**完全不变**（AC-4.3）

**行为变更可接受性：** P1 修复，"避免崩溃"为目标。原崩溃路径改为走 fallback 是合理降级。原本正常工作的路径（key 存在且值非空）行为完全不变。可接受。

### 问题 #4 基线结论

**通过**（待用户批准本基线扩展）

---

## 问题 #5 区段（同模式追加：SetAppLibPath 空字符串 back() UB）

### 问题 #5 原始需求

**问题概述：** 空字符串调用 `tmpPath.back()` 触发未定义行为。

**问题描述：** `module_manager/native_module_manager.cpp` 第 758 行 `SetAppLibPath` 中，循环把 `appLibPath` 拼接为 `tmpPath`，随后直接调用 `tmpPath.back()` 检查末尾冒号。当传入 `appLibPath` 为空 vector 或所有元素均为空字符串时，循环不会向 `tmpPath` 追加任何内容，`tmpPath` 保持为空串。`std::string::back()` 在空串上调用是未定义行为，多数实现会解引用越界指针导致崩溃。

**用户建议修复（用户已确认仅采用 L758 判空保护）：**
```cpp
// module_manager/native_module_manager.cpp:758
if (!tmpPath.empty() && tmpPath.back() == ':') {
    tmpPath.pop_back();
}
```

**影响评估：**
- 严重程度：可导致未定义行为 / 崩溃
- 影响范围：应用库路径注册
- 触发条件：`appLibPath` 为空或元素全为空
- 潜在后果：拒绝服务
- 修复优先级：本月内完成

### 问题 #5 澄清记录

| 编号 | 问题 | 结论 |
|------|------|------|
| Q5-1 | 修复范围 | **仅修 L758 判空保护**。用户描述列了 3 项建议（判空保护、函数入口校验、覆盖测试），但参考代码块只展示判空保护。函数入口校验（`if (appLibPath.empty()) return;`）不在参考代码内，留作单独问题 |
| Q5-2 | 单元测试 | **补单测**（同 #1-#4 模式）：覆盖两种触发条件——空 vector、所有元素均为空字符串 |

### 问题 #5 范围边界

**包含：**
- `native_module_manager.cpp:758` 一行判空保护（`tmpPath.back()` 前加 `!tmpPath.empty() &&`）
- 新增测试 `SetAppLibPath_WhenAppLibPathIsEmptyShouldNotCrash`

**不包含（严格遵守"每次仅修问题描述本身"规则）：**
- **函数入口校验**（`if (appLibPath.empty()) return;` 早返）：用户参考代码未含此项，留作单独问题
- **代码审查清单项**（用户建议"加入容器/字符串访问首元素前必须判空检查项"）：超出代码修复范围
- **L769/770/762/769 等其他潜在 UB / operator[] 使用点**：超出范围
- **`tmpPath.c_str()` 空串 strdup 后存入 map 的下游影响**：超出范围（保留原行为：空串入库，行为兼容）
- **`CreateLdNamespace` 副作用**：Linux 测试环境 `#if !defined(LINUX_PLATFORM)` 该函数为空，无副作用

### 问题 #5 用户故事与 AC

| Story ID | 用户故事 | 优先级 |
|----------|----------|--------|
| US-5 | 作为系统开发者，我希望 `SetAppLibPath` 在 `appLibPath` 为空 vector 或元素全为空字符串时不解引用空串的 `back()`，以便应用库路径注册流程在异常输入下保持健壮 | P1 |

| AC编号 | 验收标准 | 类型 | 关联Story |
|--------|----------|------|-----------|
| AC-5.1 | WHEN `appLibPath` 为空 vector THEN `SetAppLibPath` 不崩溃（`tmpPath.back()` 短路判空），`tmpPath` 保持 ""，strdup 入库行为兼容 | 边界 | US-5 |
| AC-5.2 | WHEN `appLibPath` 元素全为空字符串（如 `{"", "", ""}`）THEN `SetAppLibPath` 不崩溃，`tmpPath` 保持 ""，行为同 AC-5.1 | 边界 | US-5 |
| AC-5.3 | WHEN `appLibPath` 含至少一个非空元素（如 `{"path1", "path2"}`）THEN `SetAppLibPath` 行为与原修复前一致（`tmpPath` = "path1:path2"，末尾冒号被 `pop_back` 去除），向后兼容 | 回归 | US-5 |
| AC-5.4 | WHEN 调用既有全部单元测试 THEN 全部通过（不引入回归） | 回归 | US-5 |

### 问题 #5 兼容性

**行为变更：**
- 原行为（空 vector 或全空元素）：`tmpPath.back()` 解引用空串 → UB / 多数实现崩溃
- 修复后行为：短路判空，`tmpPath` 保持 ""，后续 `strdup("")` 入库（行为兼容，原本非崩溃路径就是这样）
- 原行为（含非空元素）：完全不变（AC-5.3）

**行为变更可接受性：** P1 修复，"避免崩溃"为目标。原崩溃路径改为安全跳过 `pop_back`，后续流程（`strdup("")` 入库）行为与原非崩溃路径一致。可接受。

### 问题 #5 基线结论

**通过**（待用户批准本基线扩展）

---

## 问题 #6 区段（同模式追加：LoadNativeModule relativePath 路径穿越校验加固）

### 问题 #6 原始需求

**问题概述：** `relativePath` 路径穿越校验过于宽松。

**问题描述：** `module_manager/native_module_manager.cpp` 第 862-867 行 `LoadNativeModule` 仅以 `relativePathStr.find("..") != npos` 作为路径穿越判据。该判据存在两类问题：
- **误报**：合法相对路径包含连续两个英文点（例如 `"my..pkg"`、`"a..b/module"`）会被误判为非法
- **漏报**：未拦截绝对路径（`"/etc/passwd"`）、符号链接、URL 编码、回退分隔符等穿越向量，最终拼接出的 `nativeModulePath` 仍可能越界访问文件系统

**用户建议修复（用户已确认采用）：**
1. **规范化校验**：禁止以 `/` 开头、禁止路径分段等于 `.`/`..`/空分段
2. **白名单字符**：限定 `relativePath` 仅允许字母数字、`_`、`.`、`/`
3. 复用安全库（realpath + 前缀校验）：用户描述里提到，但参考代码块未含此项 → 显式排除

**用户建议参考代码：**
```cpp
static bool IsSafeRelativePath(const std::string& p) {
    if (p.empty()) return true;
    if (p.front() == '/') return false;
    std::stringstream ss(p);
    std::string seg;
    while (std::getline(ss, seg, '/')) {
        if (seg.empty() || seg == "." || seg == "..") return false;
        for (char c : seg) {
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '.') return false;
        }
    }
    return true;
}
if (!IsSafeRelativePath(relativePathStr)) {
    SetLoadErrInfo(loadErrInfo, "invalid relativePath");
    MODULEMNG_HILOG_ERROR("invalid relativePath: rejected by whitelist");
    return nullptr;
}
```

**影响评估：**
- 严重程度：路径穿越可读任意文件
- 影响范围：模块加载文件读取
- 触发条件：攻击者控制 `relativePath`
- 潜在后果：信息泄露 / 任意代码加载
- 修复优先级：本月内完成

### 问题 #6 澄清记录

| 编号 | 问题 | 结论 |
|------|------|------|
| Q6-1 | helper 函数位置与实现 | **作为 private static 成员函数**（在 `native_module_manager.h` 私有段声明，`.cpp` 实现）。原因：(a) 不访问 `this`，可声明 `static`；(b) mock 头的 `#define private public` 可直接暴露供单测调用；(c) 比 static free function in cpp 更便于测试 |
| Q6-2 | 单元测试方式 | **直接对 helper 测试**（通过 mock 暴露的 `IsSafeRelativePath` 直接调用），覆盖：原误报场景（"my..pkg" 现接受）、原漏报场景（"/etc/passwd"、"a/../b"、"a//b"、"a;b" 现拒绝）、合法路径（"module/sub" 接受）、空串（接受） |

### 问题 #6 范围边界

**包含：**
- `native_module_manager.h`：私有段新增 `static bool IsSafeRelativePath(const std::string& p);` 声明
- `native_module_manager.cpp`：实现 `NativeModuleManager::IsSafeRelativePath`；L862-867 的 `find("..")` 检查替换为 `!IsSafeRelativePath(...)` 调用
- 必要时新增 `#include <cctype>`（用于 `std::isalnum`）
- 新增测试 `IsSafeRelativePath_PathValidationTest`（直接对 helper 验证多场景）

**不包含（严格遵守"每次仅修问题描述本身"规则）：**
- **realpath + 前缀校验**：用户描述里提到但参考代码块未含，留作单独问题
- **代码审查清单项**（用户建议"加入路径输入必须规范化校验检查项"）：超出代码修复范围
- **其他路径校验点**（如 `moduleName`、`path` 入参校验，`GetNativeModulePath` 内的路径拼接等）：留作单独问题
- **`nativeModulePath` 拼接后的 realpath 校验**：超出范围
- **URL 解码 / 符号链接检测**：超出范围（白名单字符已能拦截绝大多数穿越向量）
- **修改 `LoadNativeModule` 其他校验逻辑**（L848-860 的 nullptr 检查等）：未触碰

### 问题 #6 用户故事与 AC

| Story ID | 用户故事 | 优先级 |
|----------|----------|--------|
| US-6 | 作为系统开发者，我希望 `LoadNativeModule` 对 `relativePath` 做规范化白名单校验，以便既不误拦合法路径（如 "my..pkg"）也不放行穿越向量（如 "/etc/passwd"、"a/../b"） | P1 |

| AC编号 | 验收标准 | 类型 | 关联Story |
|--------|----------|------|-----------|
| AC-6.1 | WHEN `relativePath` 包含 ".." 作为子串但非完整分段（如 `"my..pkg"`、`"a..b/c"`）THEN `IsSafeRelativePath` 返回 true（不再误报），`LoadNativeModule` 不因此拦截 | 正常 | US-6 |
| AC-6.2 | WHEN `relativePath` 是绝对路径（如 `"/etc/passwd"`）THEN `IsSafeRelativePath` 返回 false，`LoadNativeModule` 拒绝并返回 nullptr | 边界 | US-6 |
| AC-6.3 | WHEN `relativePath` 含 `.`/`..`/空 分段（如 `"a/./b"`、`"a/../b"`、`"a//b"`）THEN `IsSafeRelativePath` 返回 false，`LoadNativeModule` 拒绝 | 边界 | US-6 |
| AC-6.4 | WHEN `relativePath` 含非白名单字符（如 `"a;b"`、`"a$b"`、URL 编码 `%2e`）THEN `IsSafeRelativePath` 返回 false | 边界 | US-6 |
| AC-6.5 | WHEN `relativePath` 是合法相对路径（如 `"module/sub"`、`"a.b.c"`、空串）THEN `IsSafeRelativePath` 返回 true，行为与原修复前一致（向后兼容） | 回归 | US-6 |
| AC-6.6 | WHEN 调用既有全部单元测试 THEN 全部通过（不引入回归） | 回归 | US-6 |

### 问题 #6 兼容性

**行为变更（双向）：**
- **放宽**（原误报 → 接受）：
  - `"my..pkg"`：原 `find("..")` 命中 → 拒绝；新 `IsSafeRelativePath` → 接受（分段 `my..pkg` 仅含白名单字符）
  - `"a..b/c"`：同上
- **收紧**（原漏报 → 拒绝）：
  - `"/etc/passwd"`：原 `find("..")` 未命中 → 接受；新 `IsSafeRelativePath` → 拒绝（前导 `/`）
  - `"a/./b"`、`"a/../b"`、`"a//b"`：原接受；新拒绝（分段为 `.`/`..`/空）
  - `"a;b"`、`"a$b"`、`"%2e"` 等：原接受；新拒绝（非白名单字符）
- **不变**（合法路径）：
  - `"module/sub"`、`"a.b.c"`、`""` 空串：原接受；新继续接受

**行为变更可接受性：**
- 放宽：消除误报，符合模块命名实践（如 `"com.example..foo"` 这类含连续点的合法名字）
- 收紧：阻断路径穿越向量，符合安全加固目标
- 调用方影响：`LoadNativeModule` 是 public 接口。原合法调用方传 `"module/sub"` 等正常路径，行为完全不变；传含非白名单字符的调用方（如果有）需调整，但实际调用方路径均符合白名单
- 可接受

### 问题 #6 基线结论

**通过**（待用户批准本基线扩展）

---

## 问题 #7 区段（同模式追加：greylist.json lib 名严格字符集校验）

### 问题 #7 原始需求

**问题概述：** `greylist.json` 解析无转义处理可被注入绕过 namespace 隔离。

**问题描述：** `module_manager/native_module_manager.cpp` 第 475-525 行 `LoadGreylistConfig` 使用字符串 `find('[')`/`rfind(']')` 与按逗号分割的方式解析 JSON。该实现存在以下安全弱点：
- 不处理 JSON 转义（`\"`、`\\`）、对象/嵌套、注释、Unicode 转义；恶意构造的 `greylist.json` 可让 `libName` 中混入冒号 `:` 等分隔符
- 解析得到的 lib 名随后被拼入 `sharedLibsSonames_`（L598-617），并作为 `dlns_inherit` 的允许库名传入 musl 命名空间（L679/687），库名中的 `:` 是分隔符，可导致规则被绕过或拼接出非预期条目
- 配置文件路径 `/data/service/el0/public/for-all-app/...` 是公共可写区域，缺乏完整性校验
- `IsValidLibName`（L442-456）仅校验 `.so` 后缀，未限制内容字符集，无法阻止冒号等危险字符进入共享库允许列表

**用户建议修复（用户已确认仅采用 IsValidLibNameStrict 严格字符集校验）：**
1. 使用真正的 JSON 解析库（cJSON / nlohmann::json）→ 用户参考代码未含此项，**留作单独问题**
2. **严格字符集校验**（用户参考代码内 `IsValidLibNameStrict`）→ 本次实施
3. 配置完整性校验（签名/哈希/目录权限）→ 用户参考代码未含此项，**留作单独问题**

**用户建议参考代码：**
```cpp
static bool IsValidLibNameStrict(const std::string& libName) {
    if (libName.empty() || libName.size() >= NAME_MAX) return false;
    if (libName.substr(libName.size() - 3) != ".so") return false;
    for (char c : libName) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '_' && c != '-') return false;
    }
    return libName.find("...") == std::string::npos;
}
```

**影响评估：**
- 严重程度：可绕过命名空间隔离加载非授权共享库
- 影响范围：应用命名空间继承共享库列表
- 触发条件：攻击者写入恶意 `greylist.json`
- 潜在后果：权限提升 / 沙箱逃逸
- 修复优先级：本月内完成

### 问题 #7 澄清记录

| 编号 | 问题 | 结论 |
|------|------|------|
| Q7-1 | helper 函数位置 | **作为 private static 成员函数**（在 `native_module_manager.h` 私有段声明，`.cpp` 实现）。同 #6 `IsSafeRelativePath` 模式：mock 头的 `#define private public` 可直接暴露供单测调用。**保留原 `IsValidLibName`（L442-456）不动**——它是 static free function in cpp，未在用户问题描述范围内，避免修改超范围 |
| Q7-2 | 单元测试 | **补单测**（同 #1-#6 模式）：直接对 helper 验证多场景：合法名（`libfoo.so`）、含冒号（`liba:b.so`）、含空格、含斜杠、含 `...`、不以 `.so` 结尾、超长（≥ NAME_MAX）、空串、含非白名单字符（`liba$b.so`） |

### 问题 #7 范围边界

**包含：**
- `native_module_manager.h`：私有段新增 `static bool IsValidLibNameStrict(const std::string& libName);` 声明
- `native_module_manager.cpp`：实现 `NativeModuleManager::IsValidLibNameStrict`（按用户建议：empty/size≥NAME_MAX→false；不以 `.so` 结尾→false；字符白名单 alnum/_/-/.；含 `...`→false）；L515 调用点由 `IsValidLibName` 改为 `NativeModuleManager::IsValidLibNameStrict`
- 必要时新增 `#include <climits>`（用于 `NAME_MAX`）
- 新增测试 `IsValidLibNameStrict_LibNameValidationTest`（直接对 helper 验证多场景）

**不包含（严格遵守"每次仅修问题描述本身"规则）：**
- **替换为 JSON 解析器**（cJSON/nlohmann::json）：用户参考代码未含，留作单独问题
- **配置完整性校验**（签名/哈希/目录权限）：用户参考代码未含，留作单独问题
- **原 `IsValidLibName` 函数（L442-456 static free function）**：不在用户问题描述范围内，未触碰（即便其成为孤立死代码也保留，最小变更原则）
- **JSON 解析逻辑（L483-521）**：未触碰（仍用手写 find/rfind/split）
- **`sharedLibsSonames_` 拼接逻辑（L598-617）**：未触碰
- **`dlns_inherit` 调用点（L679/687）**：未触碰
- **其他路径完整性问题**：超出范围
- **代码审查清单项**（用户建议"加入不可信配置文件必须使用成熟解析器检查项"）：超出代码修复范围

### 问题 #7 用户故事与 AC

| Story ID | 用户故事 | 优先级 |
|----------|----------|--------|
| US-7 | 作为系统开发者，我希望 `LoadGreylistConfig` 解析得到的 lib 名通过严格字符集校验（alnum/_/-/.，禁止冒号/空格/斜杠/`...`），以便恶意 `greylist.json` 无法注入分隔符绕过 namespace 隔离 | P1 |

| AC编号 | 验收标准 | 类型 | 关联Story |
|--------|----------|------|-----------|
| AC-7.1 | WHEN `libName` 含冒号（如 `liba:b.so`）THEN `IsValidLibNameStrict` 返回 false（阻断 namespace 注入向量） | 边界 | US-7 |
| AC-7.2 | WHEN `libName` 含空格/斜杠/反斜杠/其他非白名单字符（如 `lib a.so`、`lib/a.so`、`lib\\a.so`、`lib$a.so`）THEN `IsValidLibNameStrict` 返回 false | 边界 | US-7 |
| AC-7.3 | WHEN `libName` 含 `...` 序列（如 `lib...so`、`a...b.so`）THEN 返回 false（防路径穿越式 tricks） | 边界 | US-7 |
| AC-7.4 | WHEN `libName` 不以 `.so` 结尾（如 `libfoo`、`libfoo.txt`）THEN 返回 false | 边界 | US-7 |
| AC-7.5 | WHEN `libName` 为空或长度 ≥ `NAME_MAX` THEN 返回 false | 边界 | US-7 |
| AC-7.6 | WHEN `libName` 是合法名（如 `libfoo.so`、`lib_foo-bar.so`、`lib123.so`）THEN 返回 true，行为兼容 | 正常 | US-7 |
| AC-7.7 | WHEN 调用既有全部单元测试 THEN 全部通过（不引入回归） | 回归 | US-7 |

### 问题 #7 兼容性

**行为变更：**
- 原行为：`IsValidLibName` 仅校验 `.so` 后缀 → 接受 `liba:b.so` 等含危险字符的名字
- 修复后行为：`IsValidLibNameStrict` 严格字符集校验 → 拒绝含冒号/空格/斜杠等非白名单字符的名字
- 合法 lib 名（仅含 alnum/_/-/. 且以 `.so` 结尾、长度 < NAME_MAX、不含 `...`）：行为完全不变（AC-7.6）

**行为变更可接受性：**
- P1 安全修复，"阻断 namespace 注入"为目标
- 现有 greylist.json 中所有合法 lib 名均符合白名单（标准命名规范 `lib<name>.so`）
- 含非白名单字符的 lib 名（如果有）会被拒绝，但实际配置中不应存在
- 可接受

### 问题 #7 基线结论

**通过**（待用户批准本基线扩展）

---

## 问题 #8：UnloadNativeModule 存在 TOCTOU 竞态导致双重释放或使用已释放句柄

### 原始需求

**问题描述（用户原话）：**
> `UnloadNativeModule` 存在 TOCTOU（Time-of-check-to-time-of-use）竞态：`GetNativeModuleHandle` 查询句柄后释放锁，`RemoveNativeModule` 再次操作 `moduleLibMap_` 之前，其他线程可能以同一 key 再次进入 `UnloadNativeModule`，拿到同一句柄并执行 `dlclose`，导致双重释放或使用已释放句柄。
>
> **修复建议：**
> 1. 合并临界区：在同一把锁内完成"查询-移除-卸载"。
> 2. 引入引用计数：对模块句柄使用 shared_ptr/refCount，仅当引用归零时 dlclose。
> 3. 复测覆盖：增加并发 unload 的压力测试。

**用户建议代码：**
```cpp
bool NativeModuleManager::UnloadNativeModule(const std::string& moduleKey)
{
    LIBHANDLE handle = nullptr;
    bool removed = false;
    {
        std::lock_guard<std::mutex> lk1(moduleLibMutex_);
        std::lock_guard<std::mutex> lk2(nativeModuleListMutex_);
        auto it = moduleLibMap_.find(moduleKey);
        if (it == moduleLibMap_.end()) return false;
        handle = it->second;
        moduleLibMap_.erase(it);
        removed = RemoveNativeModuleLocked(moduleKey); // 内部不再加锁
    }
    if (removed) {
        return UnloadModuleLibrary(handle);
    }
    return false;
}
```

**当前 buggy 代码（`native_module_manager.cpp:261-276`）：**
```cpp
bool NativeModuleManager::UnloadNativeModule(const std::string& moduleKey)
{
    LIBHANDLE handle = GetNativeModuleHandle(moduleKey);  // lock → find → unlock
    if (handle == nullptr) return false;
    // TOCTOU 窗口：此处其他线程可拿到同一 handle 并 dlclose
    if (RemoveNativeModule(moduleKey) == false) return false;  // 内部 3 次独立加锁
    return UnloadModuleLibrary(handle);  // 可能 dlclose 已被其他线程执行 → 双重释放
}
```

**TOCTOU 漏洞链：**
1. 线程 A：`GetNativeModuleHandle("foo")` 返回 `handle=0x1234`，释放 `moduleLibMutex_`
2. 线程 B：`GetNativeModuleHandle("foo")` 返回同一 `handle=0x1234`，释放 `moduleLibMutex_`
3. 线程 A：`RemoveNativeModule("foo")` 成功，`moduleLibMap_` 移除 "foo"，`UnloadModuleLibrary(0x1234)` → `dlclose` 成功
4. 线程 B：抢先 `dlclose(0x1234)` → 线程 A 后续访问 → 双重释放 / UB；或线程 B `RemoveNativeModule` 失败但 handle 已悬空

**影响评估：**
- 严重程度：高（双重释放可触发 heap corruption，潜在 RCE）
- 影响范围：所有 `UnloadNativeModule` 调用路径
- 触发条件：多线程并发 unload 同一 moduleKey
- 潜在后果：heap corruption / use-after-free / 崩溃 / 可能的代码执行
- 修复优先级：本月内完成

### 问题 #8 澄清记录

| 编号 | 问题 | 结论 |
|------|------|------|
| Q8-1 | 实现路径选择 | **采纳用户建议方案 1（合并临界区）**：在 `moduleLibMutex_` + `nativeModuleListMutex_` 双锁保护下原子完成"查询-擦除-缓存移除"，`dlclose` 在锁外执行。不采纳方案 2（引用计数）——改动面大（需重构 NativeModule 生命周期管理 + 所有 Get/Find 路径），超出"每次仅修问题描述本身"规则。**锁顺序分析**：`moduleLibMutex_ → nativeModuleListMutex_`（全仓首次出现双锁嵌套；grep 静态审查现有代码 L106/L332/L1002/L1462/L1576/L1639 持 `nativeModuleListMutex_` 时不再取 `moduleLibMutex_`，L178/L188/L201 持 `moduleLibMutex_` 时不再取 `nativeModuleListMutex_` → 无反向持有 → 无死锁风险） |
| Q8-2 | `RemoveNativeModuleLocked` 实现方式 | **新增 2 个 private helper**：`RemoveNativeModuleLocked(moduleKey)`（调用方持双锁，`moduleLibMap_` 已由调用方擦除；内部仅调 `RemoveModuleBuffer` 取 `moduleBufMutex_` + `RemoveNativeModuleByCacheLocked`）+ `RemoveNativeModuleByCacheLocked(moduleKey)`（与 `RemoveNativeModuleByCache`（L1574-1629）同体但**移除 `std::lock_guard<std::mutex> lock(nativeModuleListMutex_);` 一行**）。**保留原 `RemoveNativeModule`/`RemoveNativeModuleByCache`/`RemoveModuleLib`/`RemoveModuleBuffer` 不动**（仍被其他路径使用） |
| Q8-3 | 单元测试 | **补单测（基础功能性测试）**：`UnloadNativeModule_WhenModuleNotInLibMapShouldReturnFalseSafely`——空 `moduleLibMap_` 调用 `UnloadNativeModule("nonexistent")`，验证返回 false 且不崩溃（覆盖 TOCTOU 加固后的"未找到早退"路径，且该路径无需 dlclose 不会崩溃）。**并发压力测试不在单测范围**（gtest 并发测试 flaky，且本机环境构建失败）——留待 CI/集成测试覆盖，文档明确标注 |

### 问题 #8 范围边界

**包含：**
- `native_module_manager.h`：私有段新增 2 个声明（紧邻 L168-#169 既有的 IsSafeRelativePath/IsValidLibNameStrict）：
  - `bool RemoveNativeModuleLocked(const std::string& moduleKey);`
  - `bool RemoveNativeModuleByCacheLocked(const std::string& moduleKey);`
- `native_module_manager.cpp`：
  - 重构 `UnloadNativeModule`（L261-276）为双锁临界区 + 锁外 `dlclose`，按用户建议代码
  - 新增 `RemoveNativeModuleLocked` 实现：调用方持双锁且已擦除 `moduleLibMap_` 条目；内部调 `RemoveModuleBuffer`（取 `moduleBufMutex_`）+ `RemoveNativeModuleByCacheLocked`（不加锁）
  - 新增 `RemoveNativeModuleByCacheLocked` 实现：与 `RemoveNativeModuleByCache`（L1574-1629）同体，**移除 `std::lock_guard<std::mutex> lock(nativeModuleListMutex_);` 一行**
- 新增测试 `UnloadNativeModule_WhenModuleNotInLibMapShouldReturnFalseSafely`

**不包含（严格遵守"每次仅修问题描述本身"规则）：**
- **引用计数方案（用户建议方案 2）**：改动面超出范围，留作单独问题
- **`CloseModuleLibrary` 路径加固**：不在用户问题描述范围内
- **其他锁粒度问题**（`EmplaceModuleLib`/`EmplaceModuleBuffer` 非原子等）：不在用户问题描述范围内
- **`UnloadModuleLibrary`/`dlclose` 失败处理**：未触碰（保持原有错误处理逻辑）
- **既有 `RemoveNativeModule`（L250-259）等 4 个函数**：保留不动（仍被其他路径使用）
- **并发压力测试**：留待 CI（单测框架不适合并发 flaky 测试）

### 问题 #8 用户故事与 AC

| Story ID | 用户故事 | 优先级 |
|----------|----------|--------|
| US-8 | 作为系统开发者，我希望 `UnloadNativeModule` 在多线程并发调用同一 moduleKey 时不会发生双重释放或使用已释放句柄，以便系统在高并发 unload 场景下稳定安全 | P1 |

| AC编号 | 验收标准 | 类型 | 关联Story |
|--------|----------|------|-----------|
| AC-8.1 | WHEN `moduleLibMap_` 不存在 moduleKey THEN `UnloadNativeModule` 在双锁临界区内 `find == end()` 早退返回 false，不触碰 cache / buffer / handle | 边界 | US-8 |
| AC-8.2 | WHEN `moduleLibMap_` 存在 moduleKey THEN `UnloadNativeModule` 在双锁临界区内原子完成：(a) `find` + `erase` `moduleLibMap_`；(b) `RemoveNativeModuleLocked` 移除 cache 与 buffer；然后**释放双锁**后 `UnloadModuleLibrary(handle)` 执行 `dlclose` | 正常 | US-8 |
| AC-8.3 | WHEN 两线程并发 `UnloadNativeModule` 同一 moduleKey THEN 仅一线程拿到 handle 并 dlclose，另一线程在 `find == end()` 早退（不拿到悬空 handle） | 竞态 | US-8 |
| AC-8.4 | WHEN `RemoveNativeModuleLocked` 被调用 THEN 其内部不在 `moduleLibMutex_`/`nativeModuleListMutex_` 上重复加锁（避免非递归 mutex 死锁），仅 `RemoveModuleBuffer` 内部取 `moduleBufMutex_`（独立锁） | 设计 | US-8 |
| AC-8.5 | WHEN `RemoveNativeModuleByCacheLocked` 被调用 THEN 假设调用方已持有 `nativeModuleListMutex_`，函数体不加该锁 | 设计 | US-8 |
| AC-8.6 | WHEN 锁顺序分析 THEN 全仓 `moduleLibMutex_` + `nativeModuleListMutex_` 双锁嵌套仅出现在新 `UnloadNativeModule`，顺序固定 `moduleLib → nativeModuleList`，无反向持有，无死锁风险 | 设计 | US-8 |
| AC-8.7 | WHEN 调用既有全部单元测试 THEN 全部通过（不引入回归） | 回归 | US-8 |

### 问题 #8 兼容性

**对外 API 行为变更：** 无（`UnloadNativeModule` 签名/返回值语义不变）

**内部行为变更：**
- 原行为：`GetNativeModuleHandle` 与 `RemoveNativeModule` 在两次独立锁操作中完成，存在 TOCTOU 窗口
- 修复后行为：在双锁临界区内原子完成"查询-擦除-移除"，`dlclose` 在锁外执行

**边界行为差异（可接受）：**
- 原行为：若 `RemoveNativeModule` 失败（cache 不一致），`moduleLibMap_` 条目仍存在，可重试
- 修复后行为：`moduleLibMap_` 条目在临界区内已擦除，若 `RemoveNativeModuleLocked` 失败则 handle 不再 dlclose（潜在泄漏）
- 评估：cache 不一致在实际中不应发生（`moduleLibMap_` 与 cache 在 load 路径中协同填充）；若发生不一致，handle 泄漏（可观测、可调试）优于 heap corruption（安全漏洞）。**可接受**

### 问题 #8 基线结论

**通过**（待用户批准本基线扩展）

---

## 问题 #9：EmplaceModuleLib 重复 key 时新句柄泄漏

### 原始需求

**问题描述（用户原话）：**
> 在文件 `module_manager/native_module_manager.cpp` 第 173-180 行处，`EmplaceModuleLib` 调用 `moduleLibMap_.emplace(moduleKey, lib)`。当 `moduleKey` 已存在时 `emplace` 不覆盖，调用方 `LoadModuleLibrary` 已对新句柄执行了 `dlopen`，但既未入库也未 `dlclose`，构成句柄泄漏。
>
> **修复建议：**
> 1. 检测重复并 dlclose：发现 key 已存在时 `UnloadModuleLibrary(lib)`
> 2. 覆盖语义：使用 `insert_or_assign`，并先释放旧句柄
> 3. 测试覆盖：增加重复加载用例验证句柄计数

**用户建议代码：**
```cpp
std::lock_guard<std::mutex> lock(moduleLibMutex_);
if (lib == nullptr) return;
auto it = moduleLibMap_.find(moduleKey);
if (it != moduleLibMap_.end()) {
    UnloadModuleLibrary(lib); // 丢弃重复句柄
} else {
    moduleLibMap_.emplace(moduleKey, lib);
}
```

**当前 buggy 代码（`native_module_manager.cpp:175-182`）：**
```cpp
void NativeModuleManager::EmplaceModuleLib(std::string moduleKey, const LIBHANDLE lib)
{
    MODULEMNG_HILOG_DEBUG("module:'%{public}s'", moduleKey.c_str());
    std::lock_guard<std::mutex> lock(moduleLibMutex_);
    if (lib != nullptr) {
        moduleLibMap_.emplace(moduleKey, lib);  // emplace 不覆盖，重复 key 时新句柄泄漏
    }
}
```

**泄漏链：**
1. 调用方 `LoadModuleLibrary` 对新句柄执行 `dlopen`（引用计数 +1）
2. 调用 `EmplaceModuleLib(key, newLib)`
3. 若 key 已存在，`emplace` 不覆盖，`newLib` 既未入库也未 `dlclose`
4. `newLib` 引用计数永久 +1 → 句柄泄漏

**影响评估：**

| 维度 | 评估 |
|------|------|
| 严重程度 | 句柄/内存持续增长 |
| 影响范围 | 模块加载 |
| 触发条件 | 同一 moduleKey 多次加载 |
| 潜在后果 | 资源耗尽 |
| 修复优先级 | 本月内完成 |

### 问题 #9 澄清记录

| 编号 | 问题 | 结论 |
|------|------|------|
| Q9-1 | 方案选择 | **采纳用户建议方案 1（检测重复并 dlclose 新句柄）**，与用户参考代码逐字一致。不采纳方案 2（`insert_or_assign` 覆盖语义）——会破坏既有句柄（`it->second` 被替换前需 `dlclose` 旧值，但旧值仍可能被其他持有者使用，覆盖风险大）；方案 1 仅丢弃新句柄（重复加载本身是无意义的，新 `dlopen` 句柄与旧句柄实际指向同一 lib，`dlclose` 新句柄只是减引用计数，不破坏旧句柄）。**理由：保守策略，不破坏既有句柄** |
| Q9-2 | 锁内 dlclose 性能 | 接受。`UnloadModuleLibrary` 内部仅调用 `dlclose`（不取 `moduleLibMutex_`），持锁 `dlclose` 不会死锁；性能影响短暂可接受；改造为"锁外 dlclose"超出"每次仅修问题描述本身"范围 |
| Q9-3 | 单元测试 | **补单测（覆盖 nullptr 早退路径 + 未重复 emplace 入库路径）**：`EmplaceModuleLib_ShouldNotEmplaceWhenLibIsNullptr`（nullptr 早退）+ `EmplaceModuleLib_ShouldEmplaceWhenKeyNotExists`（首次入库）。**重复 key 路径不补单测**——`UnloadModuleLibrary(lib)` 会触发真实 `dlclose`，单测无法安全模拟（无效指针崩溃；`dlopen` 真实库环境依赖）。重复路径靠静态审查 + CI 集成测试覆盖 |

### 问题 #9 范围边界

**包含：**
- `native_module_manager.cpp:175-182`：重构 `EmplaceModuleLib` 为 `find` → 重复 → `UnloadModuleLibrary(lib)` 丢弃新句柄；否则 `emplace`
- 新增测试 `EmplaceModuleLib_ShouldNotEmplaceWhenLibIsNullptr` + `EmplaceModuleLib_ShouldEmplaceWhenKeyNotExists`

**不包含（严格遵守"每次仅修问题描述本身"规则）：**
- **`insert_or_assign` 覆盖语义（用户建议方案 2）**：超范围，破坏既有句柄
- **其他 `emplace` 调用点**（`EmplaceModuleBuffer` 已在 #3 修复，`nativeEngineList_.emplace` L172 等其他点）：不在问题描述范围内
- **`LoadModuleLibrary` 调用方逻辑**：未触碰（保持调用方对 newLib 的处理逻辑）
- **`dlclose` 失败处理**：保持 `UnloadModuleLibrary` 原有逻辑
- **锁外 `dlclose` 改造**：超范围，性能改进不属于本问题描述

### 问题 #9 用户故事与 AC

| Story ID | 用户故事 | 优先级 |
|----------|----------|--------|
| US-9 | 作为系统开发者，我希望 `EmplaceModuleLib` 在 key 已存在时不泄漏新 `dlopen` 句柄，以便长时间运行的进程不会因重复加载导致资源耗尽 | P2 |

| AC编号 | 验收标准 | 类型 | 关联Story |
|--------|----------|------|-----------|
| AC-9.1 | WHEN `lib == nullptr` THEN `EmplaceModuleLib` 早退，不触碰 map | 边界 | US-9 |
| AC-9.2 | WHEN `moduleLibMap_` 不存在 moduleKey THEN `EmplaceModuleLib` 调用 `emplace(moduleKey, lib)`，map 新增条目 | 正常 | US-9 |
| AC-9.3 | WHEN `moduleLibMap_` 已存在 moduleKey THEN `EmplaceModuleLib` 调用 `UnloadModuleLibrary(lib)` 丢弃新句柄，map 保持不变（旧值未被覆盖） | 正常 | US-9 |
| AC-9.4 | WHEN 重复 key 路径触发 THEN 调用方 `LoadModuleLibrary` 持有的新 `dlopen` 句柄被 `dlclose`（引用计数减 1），不泄漏 | 资源 | US-9 |
| AC-9.5 | WHEN 调用既有全部单元测试 THEN 全部通过（不引入回归） | 回归 | US-9 |

### 问题 #9 兼容性

**对外 API 行为变更：** 无（`EmplaceModuleLib` 签名/返回值语义不变；返回 void）

**内部行为变更：**
- 原行为：重复 key 时新句柄既未入库也未 `dlclose` → 泄漏
- 修复后行为：重复 key 时新句柄被 `UnloadModuleLibrary` `dlclose`（引用计数减 1）→ 不泄漏
- 既有句柄（map 中的旧值）：完全不变

**行为变更可接受性：**
- P2 资源泄漏修复，"消除重复加载句柄泄漏"为目标
- 调用方 `LoadModuleLibrary` 本不应在 key 已存在时调用 `EmplaceModuleLib`（这是上游逻辑的冗余调用）；新行为是更安全的防御性处理
- 既有句柄不变 → 不影响其他持有该句柄的代码
- 可接受

### 问题 #9 基线结论

**通过**（待用户批准本基线扩展）

---

## 问题 #10：strdup(loadPath) 返回值未校验即赋给 systemFilePath

### 原始需求

**问题描述（用户原话）：**
> 在文件 `module_manager/native_module_manager.cpp` 第 1460 行处，`tailNativeModule_->systemFilePath = strdup(loadPath);` 未检查 strdup 返回值。OOM 时返回 nullptr，后续 `FindNativeModuleByCache` 中 `strcmp(temp->systemFilePath, ...)` 与析构 `free(...)` 路径均会出问题。
>
> **修复建议：**
> 1. 检查 strdup 返回值：失败时回滚并返回错误
> 2. 统一内存管理：用 std::string 或 unique_ptr<char[]> 替代裸 strdup/free

**当前代码（`native_module_manager.cpp:1487-1504`，原 L1460 因前述 #1-#9 修复偏移至 L1498）：**
```cpp
std::lock_guard<std::mutex> lock(nativeModuleListMutex_);
if (tailNativeModule_ && !abcBuffer) {
    const char* moduleName = strdup(moduleKey.c_str());
    if (moduleName == nullptr) {
        // 既有错误处理（已正确）
        SetLoadErrInfo(loadErrInfo, "internal error: out of memory");
        MODULEMNG_HILOG_ERROR("strdup faied");
        SetLoadingNativeModuleKey("");
        return nullptr;
    }
    tailNativeModule_->moduleName = moduleName;
    tailNativeModule_->systemFilePath = strdup(loadPath);  // ← 未检查返回值（问题点）
    if (tailNativeModule_->name && strcmp(tailNativeModule_->moduleName, tailNativeModule_->name)) {
        ...
    }
}
```

**问题链：**
1. `strdup(loadPath)` 在 OOM 时返回 nullptr
2. `tailNativeModule_->systemFilePath` 被赋 nullptr
3. 后续路径：`FindNativeModuleByCache` 中 `strcmp(temp->systemFilePath, ...)`（已被 #2 修复加判空保护，但仍违反"应有有效路径"契约）+ 析构 `RemoveNativeModuleByCache` 的 `free(systemFilePath)` 路径（有判空保护，但 systemFilePath == nullptr 是错误状态）

**影响评估：**

| 维度 | 评估 |
|------|------|
| 严重程度 | OOM 下空指针访问 / 契约违反 |
| 影响范围 | 模块加载（`LoadNativeModule` 错误恢复路径） |
| 触发条件 | 内存紧张（OOM） |
| 潜在后果 | 崩溃 / 资源状态不一致 |
| 修复优先级 | 本月内完成 |

### 问题 #10 澄清记录

| 编号 | 问题 | 结论 |
|------|------|------|
| Q10-1 | 方案选择 | **采纳用户建议方案 1（检查 strdup 返回值，失败时回滚并返回错误）**。不采纳方案 2（`std::string`/`unique_ptr` 替代裸 `strdup`/`free`）——改动面巨大（`NativeModule` 是 POD 结构，所有字段都是 `const char*`，替换为 `std::string` 需重构整个模块管理子系统，影响 `Register`/`FindNativeModuleByCache`/`RemoveNativeModuleByCache` 等数十处使用点），超出"每次仅修问题描述本身"规则 |
| Q10-2 | 回滚策略 | strdup 失败时回滚已 strdup 的 `moduleName`（`free` + 置 nullptr），与 L1489-1495 既有 `strdup(moduleKey)` 失败处理风格**完全一致**：`SetLoadErrInfo(loadErrInfo, "internal error: out of memory")` + `MODULEMNG_HILOG_ERROR` + `SetLoadingNativeModuleKey("")` + `return nullptr` |
| Q10-3 | 单元测试 | **不补单测**。理由：(a) 修复点在 `LoadNativeModule` 内部（L1488 `if (tailNativeModule_ && !abcBuffer)` 分支），无法独立调用 L1498 这一行；(b) 触发该分支需要构造 `dlopen` 失败 + `GetFileBuffer` 失败 + `tailNativeModule_` 已创建的复杂错误条件；(c) OOM 路径（`strdup` 失败）单测无法模拟（无 mock 注入点）。**既有 `LoadNativeModule` 用例覆盖回归**（AC-10.4） |

### 问题 #10 范围边界

**包含：**
- `native_module_manager.cpp:1498-1503`：在 `strdup(loadPath)` 后追加 nullptr 检查，失败时回滚 `moduleName`（`free` + 置 nullptr）+ 错误处理 + `return nullptr`

**不包含（严格遵守"每次仅修问题描述本身"规则）：**
- **方案 2 `std::string`/`unique_ptr` 替代**：超范围，改动面巨大
- **L1546 另一处 `strdup(loadPath)`**（`RegisterByBuffer` 分支）：不在问题描述范围内（问题描述明确指向 L1460 即 L1498 这处），留作单独问题
- **`NativeModule` 结构体重构**：超范围
- **既有 `strdup(moduleKey)` 失败处理**（L1489-1495）：保留不动（已正确处理）

### 问题 #10 用户故事与 AC

| Story ID | 用户故事 | 优先级 |
|----------|----------|--------|
| US-10 | 作为系统开发者，我希望 `strdup(loadPath)` 在 OOM 时被检测并安全回滚，以便模块加载在内存紧张时优雅降级而非崩溃或留下不一致状态 | P2 |

| AC编号 | 验收标准 | 类型 | 关联Story |
|--------|----------|------|-----------|
| AC-10.1 | WHEN `strdup(loadPath)` 返回 nullptr THEN `tailNativeModule_->systemFilePath` 不被保留为 nullptr，调用方收到 `nullptr` 返回 + `loadErrInfo = "internal error: out of memory"` | 边界 | US-10 |
| AC-10.2 | WHEN strdup 失败 THEN 已 strdup 的 `moduleName` 被回滚（`free` + 置 nullptr），不泄漏 | 资源 | US-10 |
| AC-10.3 | WHEN strdup 失败 THEN `SetLoadingNativeModuleKey("")` 被调用（重置加载状态） | 状态 | US-10 |
| AC-10.4 | WHEN 调用既有全部单元测试 THEN 全部通过（不引入回归） | 回归 | US-10 |

### 问题 #10 兼容性

**对外 API 行为变更：** 无（`LoadNativeModule` 签名/返回值语义不变）

**内部行为变更：**
- 原行为：`strdup(loadPath)` OOM 时 `systemFilePath` 被赋 nullptr，后续路径可能崩溃或违反契约
- 修复后行为：OOM 时检测到 strdup 失败，回滚 `moduleName` + 返回 `nullptr` + 设置 `loadErrInfo`

**行为变更可接受性：**
- P2 OOM 健壮性修复
- OOM 时返回 `nullptr` 是合理的优雅降级（与既有 `strdup(moduleKey)` 失败处理风格一致）
- 不影响正常路径（非 OOM 时 strdup 成功，行为不变）
- 可接受

### 问题 #10 基线结论

**通过**（待用户批准本基线扩展）
