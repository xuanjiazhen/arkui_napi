# 特性规格

> 固化用户可见行为和验收标准。
>
> **复杂度裁剪**：L0 简单（单点加固）。跳过 design.md；spec 仅核心 AC；接口规格、多设备适配、全局特性影响、行为场景等章节标记 N/A 或最小化。

## 概述

| 属性 | 值 |
|------|-----|
| 特性名称 | FindNativeModuleByCache nullptr 判空加固 |
| 特性编号 | fix-find-cache-name-nullptr |
| 所属 Epic | 无（独立修复） |
| 优先级 | P1 |
| 目标版本 | OpenHarmony-master |
| SIG 归属 | ArkUI SIG |
| 状态 | Approved |
| 复杂度 | 简单 |

## 本次变更范围（Delta）

| 类型 | 内容 | 说明 |
|------|------|------|
| MODIFIED | `FindNativeModuleByCache` 链表遍历对 `temp->name` 的判空 | 原崩溃路径（temp->name==nullptr）改为安全跳过 `strcasecmp`，行为兼容 |
| ADDED | 新增单元测试 `FindNativeModuleByCache_WhenNodeNameIsNullptrShouldNotCrash` | 覆盖 name==nullptr 节点查缓存场景 |

## 输入文档

| 文档 | 路径 | 状态 |
|------|------|------|
| Requirement | `proposal.md` | Baselined |

## 用户故事

### US-1: 链表节点 name 字段为 nullptr 时不崩溃

**作为** 系统开发者,
**我想要** `FindNativeModuleByCache` 在链表节点 `name` 字段为 nullptr 时不解引用空指针,
**以便** 在内存紧张等极端场景下保持模块加载查缓存路径的健壮性.

**验收标准：**

- **AC-1:** WHEN 链表中存在某个节点 `name == nullptr` 且其他节点均不匹配传入的 moduleName THEN `FindNativeModuleByCache` 不崩溃，且不会因 name==nullptr 节点产生误匹配（短路判空使该节点 `||` 右侧表达式为 false）
- **AC-2:** WHEN 链表中存在某个节点 `name == nullptr` 且另一节点通过 `moduleName` 字段匹配 THEN `FindNativeModuleByCache` 返回该匹配节点（验证短路左项 `temp->moduleName` 判空逻辑不受影响）
- **AC-3:** WHEN 运行既有全部 `FindNativeModuleByCache_*` 单元测试 THEN 全部通过（不引入回归）

## 验收追溯

| AC | 关联规则 | 关联 Task | 验证方式 | 证据 |
|----|----------|-----------|----------|------|
| AC-1 | FR-1 | TASK-1 | 单测 | `module_manager/test/unittest/module_manager_test/module_manager_test.cpp` 新增用例 |
| AC-2 | FR-1 | TASK-1 | 单测 | 同上（合并到同一用例的多段 ASSERT） |
| AC-3 | - | TASK-2 | 单测 | 既有用例运行结果 |

## 业务规则

> 简单变更，无业务规则。N/A。

## 功能规则

| 编号 | 规则描述 | 触发条件 | 作用对象 | 关联 AC |
|------|----------|----------|----------|---------|
| FR-1 | `strcasecmp(temp->name, moduleName)` 仅在 `temp->name != nullptr` 时执行；`temp->name == nullptr` 时短路为 false，跳过该项匹配 | 链表节点 `name` 字段为 nullptr | `FindNativeModuleByCache` 的 `||` 右侧表达式 | AC-1, AC-2 |

## 异常/豁免规则

> 简单变更，无新增异常码或豁免。N/A。

## 恢复契约

> N/A — 本修复本身就是避免崩溃的预防性加固，无恢复流程。

## 验证映射

| 编号 | 对应规格项 | 验证方式 | 验证重点 |
|------|------------|----------|----------|
| VM-1 | FR-1 / AC-1 | 单测 | 注入 name==nullptr 节点，调用 FindNativeModuleByCache 不崩溃、不误匹配 |
| VM-2 | FR-1 / AC-2 | 单测 | name==nullptr 节点存在时，moduleName 匹配仍正常工作 |
| VM-3 | AC-3 | 单测 | 既有 FindNativeModuleByCache_* 用例全部通过 |

## API 变更分析

> N/A — 无 API 变更。纯内部链表操作加固，不涉及任何 Public/System/InnerAPI。

## 接口规格

> N/A — 无新增/变更接口。API 行为无变化。

## 兼容性声明

- **已有 API 行为变更:** 否（无 API 变更）
- **配置文件格式变更:** 否
- **数据存储格式变更:** 否
- **最低支持版本:** 不变（master 分支）
- **API 版本号策略:** N/A（无 API 变更）

> 行为兼容性说明：原会崩溃的路径（temp->name==nullptr）改为安全跳过 `strcasecmp`。原本不会崩溃的路径行为完全不变（短路判空仅在 name==nullptr 时才改变结果）。

## 架构约束

> 简单变更，无新架构约束。延续既有 `FindNativeModuleByCache` 的链表遍历与互斥锁 (`nativeModuleListMutex_`) 设计。

## 非功能性需求

> N/A — proposal.md 不涉及项已确认。多一次 nullptr 判空对性能无可度量影响。

## 多设备适配声明

> 无差异。修复在所有支持平台（OHOS/Linux/Windows/macOS/iOS/Android）和架构（ARM64/ARM32/x86_64/x86）行为一致。

## 全局特性影响

| 特性 | 适用？ | 结论 | 关联场景 |
|------|--------|------|----------|
| 无障碍 | 否 | N/A | 非用户可见行为 |
| 大字体 | 否 | N/A | 同上 |
| 深色模式 | 否 | N/A | 同上 |
| 多窗口/分屏 | 否 | N/A | 同上 |
| 多用户 | 否 | N/A | 同上 |
| 版本升级 | 否 | N/A | 同上 |
| 生态兼容 | 否 | N/A | 行为兼容，不影响已有调用方 |

## 行为场景（可选，Gherkin）

> L0 简单变更，使用上方"用户故事 → AC"足够，不展开 Gherkin。

## Spec 自审清单

- [x] 无"待定""TBD""TODO"等占位符
- [x] 所有 AC 使用 WHEN/THEN 格式，可独立测试
- [x] 范围边界明确（仅 L1634 + 1 测试用例）
- [x] 无语义模糊表述
- [x] AC 与功能规则交叉一致（AC-1/AC-2 ↔ FR-1）

## context-references

> 简单变更跳过完整上下文检索日志。AGENTS.md 关键约束已记录到 proposal.md。本次仅引用以下源码位置：

- `module_manager/native_module_manager.cpp:1632-1634` — 修复点
- `module_manager/native_module_manager.cpp:1546-1553` — strdup 失败导致 name==nullptr 的来源路径
- `module_manager/native_module_manager.h:58` — `name = nullptr` 默认值定义
- `module_manager/test/unittest/module_manager_test/module_manager_test.cpp` — 测试新增位置

---

## 问题 #2 区段（同模式追加：systemFilePath 加固）

### 概述（问题 #2）

| 属性 | 值 |
|------|-----|
| 特性名称 | FindNativeModuleByCache systemFilePath nullptr 判空加固 |
| 特性编号 | fix-find-cache-name-nullptr #2 |
| 优先级 | P1 |
| 状态 | Draft（待批准） |
| 复杂度 | 简单 |

### 变更范围（Delta）

| 类型 | 内容 | 说明 |
|------|------|------|
| MODIFIED | `FindNativeModuleByCache` if 内层对 `temp->systemFilePath` 的判空（L1637 + L1641） | 原崩溃路径（systemFilePath==nullptr 进入内层）改为安全跳过 strcmp |
| ADDED | 新增测试 `FindNativeModuleByCache_WhenNodeSystemFilePathIsNullptrShouldNotCrash` | 覆盖 systemFilePath==nullptr 节点查缓存场景 |

### 用户故事 US-2

**作为** 系统开发者,
**我想要** `FindNativeModuleByCache` 在节点 `systemFilePath` 字段为 nullptr 时不解引用空指针,
**以便** 在 RegisterByBuffer/strdup 失败等场景下保持模块加载路径健壮性.

**验收标准：**
- **AC-2.1:** WHEN 链表中某节点通过 moduleName/name 匹配（if 外层成立）但 `systemFilePath == nullptr` THEN FindNativeModuleByCache 不崩溃，并返回该节点（按最小修复实现：nullptr 在 while 短路，label 保持 0 < NATIVE_PATH_NUMBER，节点被视为 path 0 匹配；此为"避免崩溃"的可接受行为变更）
- **AC-2.2:** WHEN 链表中存在 `systemFilePath == nullptr` 节点 + 另一正常节点（systemFilePath=""）匹配 THEN FindNativeModuleByCache 不崩溃（验证 nullptr 节点存在不影响其他节点的匹配流程）
- **AC-2.3:** WHEN 运行既有 FindNativeModuleByCache_* 测试 THEN 全部通过

### 验收追溯

| AC | 关联规则 | 关联 Task | 验证方式 | 证据 |
|----|----------|-----------|----------|------|
| AC-2.1 | FR-2 | TASK-2 | 单测 | 新测试第一段（构造 systemFilePath==nullptr 且 moduleName 匹配节点，验证不崩溃并返回该节点） |
| AC-2.2 | FR-2 | TASK-2 | 单测 | 新测试第二段（追加 systemFilePath="" 匹配节点，验证不崩溃） |
| AC-2.3 | - | TASK-2 | 单测 | 既有用例运行结果 |

### 功能规则

| 编号 | 规则描述 | 触发条件 | 作用对象 | 关联 AC |
|------|----------|----------|----------|---------|
| FR-2 | L1637 `strcmp(temp->systemFilePath, nativeModulePath[label])` 仅在 `temp->systemFilePath != nullptr` 时执行（while 短路判空）；L1641 `!strcmp(temp->systemFilePath, "")` 同样加 `temp->systemFilePath != nullptr` 判空。`systemFilePath == nullptr` 时：while 不执行（label 保持 0），L1641 if 通过 `label < NATIVE_PATH_NUMBER` 成立 → 节点被视为 path 0 匹配并返回。此为"避免崩溃"的可接受行为变更（原崩溃路径改为返回节点） | 节点 systemFilePath 字段为 nullptr 且 if 外层条件成立 | FindNativeModuleByCache 内层 L1637/L1641 | AC-2.1, AC-2.2 |

### 验证映射

| 编号 | 对应规格项 | 验证方式 | 验证重点 |
|------|------------|----------|----------|
| VM-2.1 | FR-2 / AC-2.1 | 单测 | 注入 systemFilePath==nullptr 且 moduleName 匹配节点，调用不崩溃并返回该节点 |
| VM-2.2 | FR-2 / AC-2.2 | 单测 | 同时存在 nullptr 节点和正常节点，验证不崩溃 |
| VM-2.3 | AC-2.3 | 单测 | 既有用例全过 |

### 兼容性声明（问题 #2）

- **已有 API 行为变更:** 否（无 API 变更）
- **行为变更:** 原崩溃路径（systemFilePath==nullptr 进入 if 内层）改为不崩溃并返回该节点（被视为 path 0 匹配）。原本不崩溃的路径行为完全不变。
- **行为变更可接受性:** P1 修复，"避免崩溃"为目标。nullptr 节点被匹配的实际影响有限（RegisterByBuffer 创建的节点本意是被加载使用，返回该节点符合加载意图）。

### 不涉及项（问题 #2）

| 维度 | 涉及？ | 依据 |
|------|--------|------|
| 性能/安全/IPC/构建/国际化/数据迁移/API | 否 | 同问题 #1，纯内部加固 |
| L1460/1508 strdup 加固 | 否 | 显式排除（不在本问题范围） |
| 字段初始化约束 / 默认值调整 | 否 | 显式排除 |

---

## 问题 #3 区段（同模式追加：EmplaceModuleBuffer 重复 key 内存泄漏）

### 概述（问题 #3）

| 属性 | 值 |
|------|-----|
| 特性名称 | EmplaceModuleBuffer 重复 key 释放旧值覆盖新值 |
| 特性编号 | fix-find-cache-name-nullptr #3 |
| 优先级 | P1 |
| 状态 | Draft（待批准） |
| 复杂度 | 简单 |

### 变更范围（Delta）

| 类型 | 内容 | 说明 |
|------|------|------|
| MODIFIED | `EmplaceModuleBuffer`（L207-214）处理重复 key | 原"重复不覆盖+新值泄漏" → "重复时释放旧值+入库新值"（覆盖语义） |
| ADDED | 新增测试 `EmplaceModuleBuffer_ShouldReleaseOldBufferOnDuplicateKey` | 覆盖重复 key 场景，验证 map size + 覆盖语义 |

### 用户故事 US-3

**作为** 系统开发者,
**我想要** EmplaceModuleBuffer 在重复 moduleKey 时释放旧 buffer 并入库新 buffer,
**以便** 长时间运行的模块加载流程不积累堆内存泄漏.

**验收标准：**
- **AC-3.1:** WHEN 同一 moduleKey 第二次调用 EmplaceModuleBuffer(key, lib2) 且 map 已有 lib1 THEN moduleBufMap_.size() == 1，moduleBufMap_[key] == lib2（覆盖）
- **AC-3.2:** WHEN 同一 moduleKey 第二次调用且 map 已有 lib1 THEN lib1 被 `delete[]`（避免泄漏）
- **AC-3.3:** WHEN 首次调用 EmplaceModuleBuffer(key, lib) THEN 行为不变（emplace 入库），向后兼容
- **AC-3.4:** WHEN 运行既有全部单元测试 THEN 全部通过

### 验收追溯

| AC | 关联规则 | 关联 Task | 验证方式 | 证据 |
|----|----------|-----------|----------|------|
| AC-3.1 | FR-3 | TASK-3 | 单测 | 新测试第二次调用后 EXPECT_EQ(size, 1u) + EXPECT_EQ(map[key], lib2) |
| AC-3.2 | FR-3 | TASK-3 | 单测 | 由 ASan/valgrind 验证（或代码层 EXPECT 不崩 + 信任 delete[] 实现） |
| AC-3.3 | FR-3 | TASK-3 | 单测 | 新测试第一次调用后 EXPECT_EQ(size, 1u) + EXPECT_EQ(map[key], lib1) |
| AC-3.4 | - | TASK-3 | 单测 | 既有用例运行 |

### 功能规则

| 编号 | 规则描述 | 触发条件 | 作用对象 | 关联 AC |
|------|----------|----------|----------|---------|
| FR-3 | EmplaceModuleBuffer 在 lib != nullptr 时：(a) 若 map 已有 moduleKey → `delete[]` 旧 buffer，将 map[key] 替换为 lib（覆盖）；(b) 若 map 无 moduleKey → emplace(key, lib) 入库。语义：重复 key 时旧值释放、新值入库，避免泄漏 | 同一 moduleKey 多次调用 | EmplaceModuleBuffer L207-214 | AC-3.1, AC-3.2, AC-3.3 |

### 验证映射

| 编号 | 对应规格项 | 验证方式 | 验证重点 |
|------|------------|----------|----------|
| VM-3.1 | FR-3 / AC-3.1 | 单测 | 重复调用后 map size + 值覆盖 |
| VM-3.2 | FR-3 / AC-3.2 | 单测 + ASan | 旧值被 delete[]（代码层覆盖 + 运行时 ASan 报告无 leak） |
| VM-3.3 | FR-3 / AC-3.3 | 单测 | 首次调用行为不变 |
| VM-3.4 | AC-3.4 | 单测 | 既有用例全过 |

### 兼容性声明（问题 #3）

- **已有 API 行为变更:** 否（无 API 变更，EmplaceModuleBuffer 是 private 接口）
- **行为变更:** 原"重复 key 不覆盖 + 新值泄漏" → "重复 key 覆盖 + 旧值释放"
- **行为变更可接受性:** 调用方仅 GetFileBuffer（L1325），其语义是缓存，覆盖语义符合预期。可接受。

### 不涉及项（问题 #3）

| 维度 | 涉及？ | 依据 |
|------|--------|------|
| EmplaceModuleLib (L173-180) | 否 | 显式排除（按用户规则留作单独问题） |
| RemoveModuleBuffer/RemoveModuleLib erase 不释放 | 否 | 显式排除（另一类问题） |
| map 类型改为 RAII | 否 | 显式排除（架构级重构） |
| GetFileBuffer 并发间隙 | 否 | 显式排除 |
| 性能/安全/IPC/构建/国际化/数据迁移/API | 否 | 同前问题，纯内部加固 |

---

## 问题 #4 区段（同模式追加：GetNativeModulePath operator[] 默认插入 nullptr）

### 概述（问题 #4）

| 属性 | 值 |
|------|-----|
| 特性名称 | GetNativeModulePath operator[] 默认插入 nullptr 加固 |
| 特性编号 | fix-find-cache-name-nullptr #4 |
| 优先级 | P1 |
| 状态 | Draft（待批准） |
| 复杂度 | 简单 |

### 变更范围（Delta）

| 类型 | 内容 | 说明 |
|------|------|------|
| MODIFIED | `GetNativeModulePath`（L1057-1095）起始 if/else 块 | 原 `operator[]` 默认插入 + `sprintf_s` 解引用崩溃路径改为 `find` + lock_guard + nullptr 走 fallback |
| ADDED | 新增测试 `GetNativeModulePath_WhenAppLibPathValueIsNullptrShouldNotCrash` | 覆盖 nullptr 值节点查缓存场景 |

### 用户故事 US-4

**作为** 系统开发者,
**我想要** `GetNativeModulePath` 在读取 `appLibPathMap_` 时不污染 map、不解引用空指针,
**以便** 应用模块路径构造在并发或异常数据场景下不崩溃.

**验收标准：**
- **AC-4.1:** WHEN `appLibPathMap_` 中某 key 存在但值为 nullptr 且 isAppModule=true THEN `GetNativeModulePath` 不崩溃（prefix 短路 nullptr 走 fallback），且 map 未被新增污染（size 不变）
- **AC-4.2:** WHEN `appLibPathMap_` 中 key 不存在（find 返回 end）且 isAppModule=true THEN `GetNativeModulePath` 不通过 operator[] 插入 `key → nullptr` 污染 entry（map size 不变），走 fallback
- **AC-4.3:** WHEN `appLibPathMap_` 中 key 存在且值非空 THEN `GetNativeModulePath` 行为与原修复前一致（prefix 取自 map，进入 if 内层），向后兼容
- **AC-4.4:** WHEN 运行既有全部单元测试 THEN 全部通过

### 验收追溯

| AC | 关联规则 | 关联 Task | 验证方式 | 证据 |
|----|----------|-----------|----------|------|
| AC-4.1 | FR-4 | TASK-4 | 单测 | 新测试第一段（注入 nullptr 值节点，验证不崩溃 + map size 不变 + 走 fallback） |
| AC-4.2 | FR-4 | TASK-4 | 单测 | 新测试第二段（空 map + 不存在 key 调用，验证 map size 不变） |
| AC-4.3 | FR-4 | TASK-4 | 单测 | 新测试第三段（注入非空值节点，验证进入 if 内层） |
| AC-4.4 | - | TASK-4 | 单测 | 既有用例运行 |

### 功能规则

| 编号 | 规则描述 | 触发条件 | 作用对象 | 关联 AC |
|------|----------|----------|----------|---------|
| FR-4 | `GetNativeModulePath` 中读取 `appLibPathMap_` 必须：(a) 使用 `find` 而非 `operator[]`（不污染 map）；(b) `find` 与读取在同一 `lock_guard` 临界区内（合并 `IsExistedPath` + 读，消除并发间隙）；(c) 读取后判 `prefix != nullptr`，nullptr 时走 fallback 分支（不进入 if 内层避免 sprintf_s 解引用）。原本 key 存在且值非空时行为完全不变（AC-4.3） | isAppModule=true 调用 GetNativeModulePath | L1057-1095 if/else 块 | AC-4.1, AC-4.2, AC-4.3 |

### 验证映射

| 编号 | 对应规格项 | 验证方式 | 验证重点 |
|------|------------|----------|----------|
| VM-4.1 | FR-4 / AC-4.1 | 单测 | 注入 nullptr 值节点，调用不崩溃、不污染 map、走 fallback |
| VM-4.2 | FR-4 / AC-4.2 | 单测 | 空 map + 不存在 key，验证不污染（size 不变） |
| VM-4.3 | FR-4 / AC-4.3 | 单测 | 注入非空值节点，验证进入 if 内层（prefix 取自 map） |
| VM-4.4 | AC-4.4 | 单测 | 既有用例全过 |

### 兼容性声明（问题 #4）

- **已有 API 行为变更:** 否（无 API 变更，GetNativeModulePath 是 private 接口）
- **行为变更:** 原崩溃路径（`operator[]` 污染 + 解引用 nullptr）改为不崩溃并走 fallback。原正常路径（key 存在且值非空）行为完全不变
- **行为变更可接受性:** P1 修复，"避免崩溃"为目标。原崩溃路径改为走 fallback 是合理降级。可接受

### 不涉及项（问题 #4）

| 维度 | 涉及？ | 依据 |
|------|--------|------|
| `IsExistedPath` 实现（L1676-1681） | 否 | 显式排除（仅减少 L1058 处的冗余调用，不改 IsExistedPath 本身） |
| 其他 `operator[]` 使用点（L1109 等） | 否 | 显式排除（按用户规则留作单独问题） |
| 其他 `IsExistedPath` 调用点（L301/885/924/1126/1178/1201/1286/1404/1414） | 否 | 显式排除（按用户规则留作单独问题） |
| 删除原 ANDROID_PLATFORM 块内死代码（`if (prefix == nullptr) return false;`） | 否 | 最小变更原则不删 |
| 代码审查清单项（用户建议） | 否 | 超出代码修复范围，不在本次实现内 |
| map 类型改为 RAII | 否 | 架构级重构 |
| 性能/安全/IPC/构建/国际化/数据迁移/API | 否 | 同前问题，纯内部加固 |

---

## 问题 #5 区段（同模式追加：SetAppLibPath 空字符串 back() UB）

### 概述（问题 #5）

| 属性 | 值 |
|------|-----|
| 特性名称 | SetAppLibPath 空字符串 back() UB 加固 |
| 特性编号 | fix-find-cache-name-nullptr #5 |
| 优先级 | P1 |
| 状态 | Draft（待批准） |
| 复杂度 | 简单 |

### 变更范围（Delta）

| 类型 | 内容 | 说明 |
|------|------|------|
| MODIFIED | `SetAppLibPath`（L758）`tmpPath.back()` 调用 | 原空串调用 `back()` 的 UB 路径改为 `!tmpPath.empty() &&` 短路判空，行为兼容 |
| ADDED | 新增测试 `SetAppLibPath_WhenAppLibPathIsEmptyShouldNotCrash` | 覆盖空 vector + 全空字符串元素两种触发条件 |

### 用户故事 US-5

**作为** 系统开发者,
**我想要** `SetAppLibPath` 在 `appLibPath` 为空 vector 或元素全为空字符串时不解引用空串的 `back()`,
**以便** 应用库路径注册流程在异常输入下保持健壮.

**验收标准：**
- **AC-5.1:** WHEN `appLibPath` 为空 vector THEN `SetAppLibPath` 不崩溃（`tmpPath.back()` 短路判空），`tmpPath` 保持 ""，strdup 入库行为兼容
- **AC-5.2:** WHEN `appLibPath` 元素全为空字符串 THEN `SetAppLibPath` 不崩溃，行为同 AC-5.1
- **AC-5.3:** WHEN `appLibPath` 含至少一个非空元素 THEN `SetAppLibPath` 行为与原修复前一致（`tmpPath` = "p1:p2"，末尾冒号被 `pop_back` 去除），向后兼容
- **AC-5.4:** WHEN 运行既有全部单元测试 THEN 全部通过

### 验收追溯

| AC | 关联规则 | 关联 Task | 验证方式 | 证据 |
|----|----------|-----------|----------|------|
| AC-5.1 | FR-5 | TASK-5 | 单测 | 新测试第一段（空 vector 调用 SetAppLibPath，验证不崩溃） |
| AC-5.2 | FR-5 | TASK-5 | 单测 | 新测试第二段（全空字符串 vector 调用，验证不崩溃） |
| AC-5.3 | FR-5 | TASK-5 | 单测 | 新测试第三段（非空元素，验证 `tmpPath` 正确拼接 + `pop_back` 生效） |
| AC-5.4 | - | TASK-5 | 单测 | 既有用例运行 |

### 功能规则

| 编号 | 规则描述 | 触发条件 | 作用对象 | 关联 AC |
|------|----------|----------|----------|---------|
| FR-5 | `SetAppLibPath` 中 `tmpPath.back()` 调用前必须先判 `!tmpPath.empty()` 短路：空串时不调用 `back()`、不调用 `pop_back()`；非空串时行为不变（`pop_back` 去除末尾冒号）。后续 `strdup("")` 入库行为保留（兼容原非崩溃路径） | `appLibPath` 为空 vector 或所有元素均为空字符串 | L758 `if (tmpPath.back() == ':')` | AC-5.1, AC-5.2, AC-5.3 |

### 验证映射

| 编号 | 对应规格项 | 验证方式 | 验证重点 |
|------|------------|----------|----------|
| VM-5.1 | FR-5 / AC-5.1 | 单测 | 空 vector 调用不崩溃 |
| VM-5.2 | FR-5 / AC-5.2 | 单测 | 全空字符串元素调用不崩溃 |
| VM-5.3 | FR-5 / AC-5.3 | 单测 | 非空元素时 `tmpPath` 正确拼接 + `pop_back` 生效 |
| VM-5.4 | AC-5.4 | 单测 | 既有用例全过 |

### 兼容性声明（问题 #5）

- **已有 API 行为变更:** 否（无 API 变更，SetAppLibPath 是 public 接口但行为兼容）
- **行为变更:** 原崩溃路径（空串 `back()` UB）改为安全跳过 `pop_back`。原非崩溃路径（含非空元素）行为完全不变
- **行为变更可接受性:** P1 修复，"避免崩溃"为目标。可接受

### 不涉及项（问题 #5）

| 维度 | 涉及？ | 依据 |
|------|--------|------|
| 函数入口校验（`if (appLibPath.empty()) return;`） | 否 | 用户参考代码未含此项，留作单独问题 |
| 其他 `back()/front()` 使用点 | 否 | 显式排除（按用户规则留作单独问题） |
| L769/770/762 等其他潜在 UB / operator[] 使用点 | 否 | 显式排除 |
| `tmpPath.c_str()` 空串 strdup 下游影响 | 否 | 保留原行为（空串入库，兼容） |
| 代码审查清单项 | 否 | 超出代码修复范围 |
| 性能/安全/IPC/构建/国际化/数据迁移/API | 否 | 同前问题，纯内部加固 |

---

## 问题 #6 区段（同模式追加：LoadNativeModule relativePath 路径穿越校验加固）

### 概述（问题 #6）

| 属性 | 值 |
|------|-----|
| 特性名称 | LoadNativeModule relativePath 路径穿越校验加固 |
| 特性编号 | fix-find-cache-name-nullptr #6 |
| 优先级 | P1 |
| 状态 | Draft（待批准） |
| 复杂度 | 简单（含一个新私有静态成员函数 + 调用点替换） |

### 变更范围（Delta）

| 类型 | 内容 | 说明 |
|------|------|------|
| ADDED | `native_module_manager.h` 私有段新增 `static bool IsSafeRelativePath(const std::string& p)` 声明 | 助手函数，规范化白名单校验 |
| ADDED | `native_module_manager.cpp` 实现 `NativeModuleManager::IsSafeRelativePath` | 含无前导 `/` + 分段非 `.`/`..`/空 + 字符白名单（alnum/_/./`/`）三项检查 |
| MODIFIED | `LoadNativeModule` L862-867 的 `find("..")` 检查替换为 `!IsSafeRelativePath(...)` 调用 | 同时消除误报（"my..pkg"）与漏报（"/etc/passwd"、"a/../b" 等） |
| ADDED | 新增测试 `IsSafeRelativePath_PathValidationTest` | 直接对 helper 验证多场景（接受/拒绝各类输入） |

### 用户故事 US-6

**作为** 系统开发者,
**我想要** `LoadNativeModule` 对 `relativePath` 做规范化白名单校验,
**以便** 既不误拦合法路径也不放行穿越向量.

**验收标准：**
- **AC-6.1:** WHEN `relativePath` 包含 ".." 作为子串但非完整分段（如 `"my..pkg"`、`"a..b/c"`）THEN `IsSafeRelativePath` 返回 true（不再误报）
- **AC-6.2:** WHEN `relativePath` 是绝对路径（如 `"/etc/passwd"`）THEN `IsSafeRelativePath` 返回 false
- **AC-6.3:** WHEN `relativePath` 含 `.`/`..`/空 分段（如 `"a/./b"`、`"a/../b"`、`"a//b"`）THEN `IsSafeRelativePath` 返回 false
- **AC-6.4:** WHEN `relativePath` 含非白名单字符（如 `"a;b"`、`"a$b"`、URL 编码 `%2e`）THEN `IsSafeRelativePath` 返回 false
- **AC-6.5:** WHEN `relativePath` 是合法相对路径（如 `"module/sub"`、`"a.b.c"`、空串）THEN `IsSafeRelativePath` 返回 true，行为兼容
- **AC-6.6:** WHEN 运行既有全部单元测试 THEN 全部通过

### 验收追溯

| AC | 关联规则 | 关联 Task | 验证方式 | 证据 |
|----|----------|-----------|----------|------|
| AC-6.1 | FR-6 | TASK-6 | 单测 | 新测试中 `EXPECT_TRUE(IsSafeRelativePath("my..pkg"))` 等 |
| AC-6.2 | FR-6 | TASK-6 | 单测 | 新测试中 `EXPECT_FALSE(IsSafeRelativePath("/etc/passwd"))` |
| AC-6.3 | FR-6 | TASK-6 | 单测 | 新测试中 `EXPECT_FALSE(IsSafeRelativePath("a/./b"))` 等多场景 |
| AC-6.4 | FR-6 | TASK-6 | 单测 | 新测试中 `EXPECT_FALSE(IsSafeRelativePath("a;b"))` 等 |
| AC-6.5 | FR-6 | TASK-6 | 单测 | 新测试中 `EXPECT_TRUE(IsSafeRelativePath("module/sub"))` 等 |
| AC-6.6 | - | TASK-6 | 单测 | 既有用例运行 |

### 功能规则

| 编号 | 规则描述 | 触发条件 | 作用对象 | 关联 AC |
|------|----------|----------|----------|---------|
| FR-6 | 新增 `static bool IsSafeRelativePath(const std::string& p)`：(a) `p.empty()` → true；(b) `p.front() == '/'` → false（拒绝绝对路径）；(c) 以 `/` 分段，任一分段为空/`.`/`..` → false；(d) 任一字符非 alnum 且非 `_`/`.`/`/` → false（白名单）。`LoadNativeModule` 中原 `find("..")` 检查替换为 `!IsSafeRelativePath(relativePathStr)`，失败时同原逻辑 `SetLoadErrInfo` + `MODULEMNG_HILOG_ERROR("invalid relativePath: rejected by whitelist")` + `return nullptr` | `LoadNativeModule` 入参 `relativePath` 非空（已过 L855-860 nullptr 检查） | `native_module_manager.h` 私有段（声明） + `native_module_manager.cpp`（实现） + `LoadNativeModule` L862-867（调用） | AC-6.1, AC-6.2, AC-6.3, AC-6.4, AC-6.5 |

### 验证映射

| 编号 | 对应规格项 | 验证方式 | 验证重点 |
|------|------------|----------|----------|
| VM-6.1 | FR-6 / AC-6.1 | 单测 | helper 对 ".." 子串（非完整分段）返回 true |
| VM-6.2 | FR-6 / AC-6.2 | 单测 | helper 对绝对路径返回 false |
| VM-6.3 | FR-6 / AC-6.3 | 单测 | helper 对含 `.`/`..`/空 分段返回 false |
| VM-6.4 | FR-6 / AC-6.4 | 单测 | helper 对非白名单字符返回 false |
| VM-6.5 | FR-6 / AC-6.5 | 单测 | helper 对合法相对路径返回 true |
| VM-6.6 | AC-6.6 | 单测 | 既有用例全过 |

### 兼容性声明（问题 #6）

- **已有 API 行为变更:** 否（`LoadNativeModule` 是 public 接口，但签名/返回值不变；只是 `relativePath` 入参的接受/拒绝规则改变）
- **行为变更（双向）:** 见 proposal.md §问题 #6 兼容性。原误报路径（"my..pkg"）改为接受；原漏报路径（"/etc/passwd"、"a/../b"、"a;b" 等）改为拒绝；合法路径行为不变
- **行为变更可接受性:** P1 安全修复，"消除误报 + 阻断穿越向量"为目标。可接受

### 不涉及项（问题 #6）

| 维度 | 涉及？ | 依据 |
|------|--------|------|
| realpath + 前缀校验 | 否 | 用户参考代码未含此项，留作单独问题 |
| 其他路径校验点（moduleName/path 入参校验、GetNativeModulePath 拼接等） | 否 | 显式排除（按用户规则留作单独问题） |
| URL 解码 / 符号链接检测 | 否 | 超出范围（白名单已能拦截绝大多数） |
| `nativeModulePath` 拼接后的 realpath 校验 | 否 | 超出范围 |
| `LoadNativeModule` 其他校验逻辑（L848-860 nullptr 检查等） | 否 | 未触碰 |
| 代码审查清单项 | 否 | 超出代码修复范围 |
| 性能/IPC/构建/国际化/数据迁移/API | 否 | 同前问题，纯内部加固（含安全维度改进） |

---

## 问题 #7 区段（同模式追加：greylist.json lib 名严格字符集校验）

### 概述（问题 #7）

| 属性 | 值 |
|------|-----|
| 特性名称 | greylist.json lib 名严格字符集校验加固 |
| 特性编号 | fix-find-cache-name-nullptr #7 |
| 优先级 | P1 |
| 状态 | Draft（待批准） |
| 复杂度 | 简单（含一个新私有静态成员函数 + 调用点替换） |

### 变更范围（Delta）

| 类型 | 内容 | 说明 |
|------|------|------|
| ADDED | `native_module_manager.h` 私有段新增 `static bool IsValidLibNameStrict(const std::string& libName);` 声明 | 助手函数，严格字符集校验 |
| ADDED | `native_module_manager.cpp` 实现 `NativeModuleManager::IsValidLibNameStrict` | 含 size<NAME_MAX + `.so` 后缀 + alnum/_/-/. 白名单 + 禁 `...` 序列 |
| MODIFIED | `LoadGreylistConfig` L515 调用点：`IsValidLibName(libName)` → `NativeModuleManager::IsValidLibNameStrict(libName)` | 替换为严格校验 |
| ADDED | 新增测试 `IsValidLibNameStrict_LibNameValidationTest` | 直接对 helper 验证多场景 |

### 用户故事 US-7

**作为** 系统开发者,
**我想要** `LoadGreylistConfig` 解析得到的 lib 名通过严格字符集校验,
**以便** 恶意 `greylist.json` 无法注入分隔符绕过 namespace 隔离.

**验收标准：**
- **AC-7.1:** WHEN `libName` 含冒号（如 `liba:b.so`）THEN `IsValidLibNameStrict` 返回 false
- **AC-7.2:** WHEN `libName` 含空格/斜杠/反斜杠/其他非白名单字符 THEN 返回 false
- **AC-7.3:** WHEN `libName` 含 `...` 序列 THEN 返回 false
- **AC-7.4:** WHEN `libName` 不以 `.so` 结尾 THEN 返回 false
- **AC-7.5:** WHEN `libName` 为空或长度 ≥ `NAME_MAX` THEN 返回 false
- **AC-7.6:** WHEN `libName` 是合法名（如 `libfoo.so`、`lib_foo-bar.so`）THEN 返回 true，行为兼容
- **AC-7.7:** WHEN 运行既有全部单元测试 THEN 全部通过

### 验收追溯

| AC | 关联规则 | 关联 Task | 验证方式 | 证据 |
|----|----------|-----------|----------|------|
| AC-7.1 | FR-7 | TASK-7 | 单测 | `EXPECT_FALSE(IsValidLibNameStrict("liba:b.so"))` |
| AC-7.2 | FR-7 | TASK-7 | 单测 | `EXPECT_FALSE(IsValidLibNameStrict("lib a.so"))` 等多场景 |
| AC-7.3 | FR-7 | TASK-7 | 单测 | `EXPECT_FALSE(IsValidLibNameStrict("lib...so"))` 等 |
| AC-7.4 | FR-7 | TASK-7 | 单测 | `EXPECT_FALSE(IsValidLibNameStrict("libfoo"))` 等 |
| AC-7.5 | FR-7 | TASK-7 | 单测 | `EXPECT_FALSE(IsValidLibNameStrict(""))`、超长字符串 |
| AC-7.6 | FR-7 | TASK-7 | 单测 | `EXPECT_TRUE(IsValidLibNameStrict("libfoo.so"))` 等 |
| AC-7.7 | - | TASK-7 | 单测 | 既有用例运行 |

### 功能规则

| 编号 | 规则描述 | 触发条件 | 作用对象 | 关联 AC |
|------|----------|----------|----------|---------|
| FR-7 | 新增 `static bool IsValidLibNameStrict(const std::string& libName)`：(a) `empty() || size() >= NAME_MAX` → false；(b) 不以 `.so` 结尾 → false；(c) 任一字符非 alnum 且非 `_`/`-`/`.` → false；(d) 含 `...` 子串 → false；否则 true。`LoadGreylistConfig` L515 调用点由 `IsValidLibName` 替换为 `NativeModuleManager::IsValidLibNameStrict`。原 `IsValidLibName`（L442-456 static free function）保留不动 | `LoadGreylistConfig` 解析得到候选 libName | `native_module_manager.h` 私有段（声明） + `native_module_manager.cpp`（实现 + L515 调用） | AC-7.1-7.6 |

### 验证映射

| 编号 | 对应规格项 | 验证方式 | 验证重点 |
|------|------------|----------|----------|
| VM-7.1 | FR-7 / AC-7.1 | 单测 | helper 对含冒号 lib 名返回 false |
| VM-7.2 | FR-7 / AC-7.2 | 单测 | helper 对含空格/斜杠等返回 false |
| VM-7.3 | FR-7 / AC-7.3 | 单测 | helper 对含 `...` 返回 false |
| VM-7.4 | FR-7 / AC-7.4 | 单测 | helper 对非 `.so` 后缀返回 false |
| VM-7.5 | FR-7 / AC-7.5 | 单测 | helper 对空/超长返回 false |
| VM-7.6 | FR-7 / AC-7.6 | 单测 | helper 对合法 lib 名返回 true |
| VM-7.7 | AC-7.7 | 单测 | 既有用例全过 |

### 兼容性声明（问题 #7）

- **已有 API 行为变更:** 否（`LoadGreylistConfig` 是 public 接口但签名/返回值不变；只是 lib 名接受/拒绝规则变严）
- **行为变更:** 原接受含危险字符（冒号/空格/斜杠等）的 lib 名 → 改为拒绝。原合法 lib 名行为不变。
- **行为变更可接受性:** P1 安全修复，"阻断 namespace 注入"为目标。可接受

### 不涉及项（问题 #7）

| 维度 | 涉及？ | 依据 |
|------|--------|------|
| 替换为 JSON 解析器（cJSON/nlohmann::json） | 否 | 用户参考代码未含，留作单独问题 |
| 配置完整性校验（签名/哈希/目录权限） | 否 | 用户参考代码未含，留作单独问题 |
| 原 `IsValidLibName`（L442-456） | 否 | 不在用户问题描述范围内，最小变更保留 |
| JSON 解析逻辑（L483-521） | 否 | 未触碰（仍用手写 find/rfind/split） |
| `sharedLibsSonames_` 拼接逻辑（L598-617） | 否 | 未触碰 |
| `dlns_inherit` 调用点（L679/687） | 否 | 未触碰 |
| 其他路径完整性问题 | 否 | 超出范围 |
| 代码审查清单项 | 否 | 超出代码修复范围 |
| 性能/IPC/构建/国际化/数据迁移/API | 否 | 同前问题，纯内部加固（含安全维度改进） |

---

## 问题 #8 规格：UnloadNativeModule TOCTOU 竞态修复

### AC 矩阵

| AC编号 | 关联 FR | 关联 TASK | 验证方式 | 验证要点 |
|--------|--------|-----------|----------|----------|
| AC-8.1 | FR-8.a | TASK-8 | 单测 | 空 libMap 调用 → 返回 false，无崩溃 |
| AC-8.2 | FR-8.a/8.b/8.c | TASK-8 | 静态审查 + 单测 | 双锁临界区 + 锁外 dlclose 结构 |
| AC-8.3 | FR-8.a | TASK-8 | CI 并发压力测试（延后） | 两线程并发不双重释放 |
| AC-8.4 | FR-8.b | TASK-8 | 静态审查 | `RemoveNativeModuleLocked` 不重复加双锁 |
| AC-8.5 | FR-8.c | TASK-8 | 静态审查 | `RemoveNativeModuleByCacheLocked` 不加 nativeModuleListMutex_ |
| AC-8.6 | FR-8.d | TASK-8 | 静态审查（grep） | 双锁顺序固定，无反向持有 |
| AC-8.7 | - | TASK-8 | 单测 | 既有用例运行 |

### 功能规则

| 编号 | 规则描述 | 触发条件 | 作用对象 | 关联 AC |
|------|----------|----------|----------|---------|
| FR-8.a | `UnloadNativeModule` 重构为：嵌套 `{}` 内 `lock_guard<mutex> lk1(moduleLibMutex_)` + `lk2(nativeModuleListMutex_)`；`moduleLibMap_.find` → `end()` 早退 false；非 end 则 `handle = it->second` + `moduleLibMap_.erase(it)` + `removed = RemoveNativeModuleLocked(moduleKey)`；出 `{}` 后 `if (removed) return UnloadModuleLibrary(handle);` 否则 false | 调用 `UnloadNativeModule` | `native_module_manager.cpp` UnloadNativeModule | AC-8.1/8.2/8.3 |
| FR-8.b | 新增 `private bool RemoveNativeModuleLocked(const std::string& moduleKey)`：**调用方持 `moduleLibMutex_` + `nativeModuleListMutex_`，`moduleLibMap_` 已擦除**；函数体：`RemoveModuleBuffer(moduleKey)`（内部取 `moduleBufMutex_`）+ `RemoveNativeModuleByCacheLocked(moduleKey)`（不加锁）；返回 `moduleRemoved`（lib 已擦除等价 handleRemoved=true） | `UnloadNativeModule` 双锁临界区内 | `native_module_manager.cpp` + `.h` 私有段 | AC-8.4 |
| FR-8.c | 新增 `private bool RemoveNativeModuleByCacheLocked(const std::string& moduleKey)`：与 `RemoveNativeModuleByCache`（L1574-1629）函数体**完全相同**，但**移除 `std::lock_guard<std::mutex> lock(nativeModuleListMutex_);` 一行**（调用方持锁） | `RemoveNativeModuleLocked` 内部 | `native_module_manager.cpp` + `.h` 私有段 | AC-8.5 |
| FR-8.d | 锁顺序契约：`moduleLibMutex_` 永远先于 `nativeModuleListMutex_` 获取。全仓 `moduleLibMutex_` + `nativeModuleListMutex_` 双锁嵌套仅出现在新 `UnloadNativeModule`，无反向持有路径 | 任何持有双锁的代码路径 | 全仓锁审计 | AC-8.6 |

### 验证映射

| 编号 | 对应规格项 | 验证方式 | 验证重点 |
|------|------------|----------|----------|
| VM-8.1 | FR-8.a / AC-8.1 | 单测 | 空 libMap → 早退 false |
| VM-8.2 | FR-8.a/8.b/8.c / AC-8.2 | 静态审查 + 单测 | 双锁结构 + 锁外 dlclose |
| VM-8.3 | FR-8.a / AC-8.3 | CI 并发测试（延后） | 并发 unload 不双重释放 |
| VM-8.4 | FR-8.b / AC-8.4 | 静态审查 | `RemoveNativeModuleLocked` 不重复加双锁 |
| VM-8.5 | FR-8.c / AC-8.5 | 静态审查 | `RemoveNativeModuleByCacheLocked` 不加 nativeModuleListMutex_ |
| VM-8.6 | FR-8.d / AC-8.6 | 静态审查（grep 全仓） | 锁顺序一致 |
| VM-8.7 | AC-8.7 | 单测 | 既有用例全过 |

### 兼容性声明（问题 #8）

- **已有 API 行为变更:** 否（`UnloadNativeModule` 签名/返回值语义不变）
- **内部行为变更:** 是（双锁临界区 + 锁外 dlclose；cache 不一致时 handle 可能泄漏——可接受，优于 TOCTOU 双重释放）
- **行为变更可接受性:** P1 安全修复，消除双重释放/Use-After-Free 为目标。可接受

### 不涉及项（问题 #8）

| 维度 | 涉及？ | 依据 |
|------|--------|------|
| 引用计数方案（shared_ptr/refCount） | 否 | 改动面超出"每次仅修问题描述本身"规则 |
| `CloseModuleLibrary` 路径加固 | 否 | 不在用户问题描述范围内 |
| 其他锁粒度问题（EmplaceModuleLib/EmplaceModuleBuffer 非原子等） | 否 | 不在用户问题描述范围内 |
| `UnloadModuleLibrary`/`dlclose` 失败处理 | 否 | 保持原有逻辑 |
| 既有 4 个 Remove* 函数 | 否 | 保留不动（仍被其他路径使用） |
| 并发压力测试 | 否 | 单测框架不适合 flaky 并发测试，留待 CI |
| 性能/IPC/构建/国际化/数据迁移/API | 否 | 纯内部加固（含并发安全维度改进） |

---

## 问题 #9 规格：EmplaceModuleLib 重复 key 句柄泄漏修复

### AC 矩阵

| AC编号 | 关联 FR | 关联 TASK | 验证方式 | 验证要点 |
|--------|--------|-----------|----------|----------|
| AC-9.1 | FR-9.a | TASK-9 | 单测 | nullptr 早退 |
| AC-9.2 | FR-9.b | TASK-9 | 单测 | 未重复 emplace 入库 |
| AC-9.3 | FR-9.c | TASK-9 | 静态审查 + CI | 重复路径：find 命中 → UnloadModuleLibrary(lib)，map 不变 |
| AC-9.4 | FR-9.c | TASK-9 | CI（dlclose 引用计数） | 重复路径触发 dlclose |
| AC-9.5 | - | TASK-9 | 单测 | 既有用例运行 |

### 功能规则

| 编号 | 规则描述 | 触发条件 | 作用对象 | 关联 AC |
|------|----------|----------|----------|---------|
| FR-9.a | `EmplaceModuleLib` 重构为：`lock_guard<mutex> lock(moduleLibMutex_)`；`if (lib == nullptr) return;`（早退） | `lib == nullptr` | `native_module_manager.cpp` EmplaceModuleLib | AC-9.1 |
| FR-9.b | 否则 `auto it = moduleLibMap_.find(moduleKey);` 若 `it == end()` 则 `moduleLibMap_.emplace(moduleKey, lib);`（首次入库） | key 不存在 | 同上 | AC-9.2 |
| FR-9.c | 若 `it != end()`（重复 key）则 `UnloadModuleLibrary(lib);`（丢弃新句柄，dlclose 引用计数 -1），**不修改 map**（保持既有句柄） | key 已存在 | 同上 | AC-9.3/9.4 |

### 验证映射

| 编号 | 对应规格项 | 验证方式 | 验证重点 |
|------|------------|----------|----------|
| VM-9.1 | FR-9.a / AC-9.1 | 单测 | nullptr 早退 |
| VM-9.2 | FR-9.b / AC-9.2 | 单测 | 首次入库 |
| VM-9.3 | FR-9.c / AC-9.3 | 静态审查 + CI | 重复路径不覆盖 |
| VM-9.4 | FR-9.c / AC-9.4 | CI（dlclose 引用计数） | 新句柄 dlclose |
| VM-9.5 | AC-9.5 | 单测 | 既有用例全过 |

### 兼容性声明（问题 #9）

- **已有 API 行为变更:** 否（`EmplaceModuleLib` 签名/返回值不变）
- **内部行为变更:** 是（重复 key 由"不覆盖+泄漏新句柄"改为"不覆盖+dlclose 新句柄"）
- **行为变更可接受性:** P2 资源泄漏修复。既有句柄不变，不影响其他持有者。可接受

### 不涉及项（问题 #9）

| 维度 | 涉及？ | 依据 |
|------|--------|------|
| `insert_or_assign` 覆盖语义（用户建议方案 2） | 否 | 破坏既有句柄，超范围 |
| 其他 `emplace` 调用点（`nativeEngineList_.emplace` 等） | 否 | 不在问题描述范围内 |
| `LoadModuleLibrary` 调用方逻辑 | 否 | 保持原样 |
| `dlclose` 失败处理 | 否 | 保持 `UnloadModuleLibrary` 原样 |
| 锁外 `dlclose` 改造 | 否 | 性能改进不属于本问题描述 |
| 性能/IPC/构建/国际化/数据迁移/API | 否 | 纯内部加固（资源管理维度） |

---

## 问题 #10 规格：strdup(loadPath) 返回值校验

### AC 矩阵

| AC编号 | 关联 FR | 关联 TASK | 验证方式 | 验证要点 |
|--------|--------|-----------|----------|----------|
| AC-10.1 | FR-10 | TASK-10 | 静态审查 + CI（OOM 注入） | strdup 失败 → 回滚 + 返回 nullptr + loadErrInfo |
| AC-10.2 | FR-10 | TASK-10 | 静态审查 + CI | moduleName 被 free |
| AC-10.3 | FR-10 | TASK-10 | 静态审查 | SetLoadingNativeModuleKey("") 调用 |
| AC-10.4 | - | TASK-10 | 单测 | 既有用例运行 |

### 功能规则

| 编号 | 规则描述 | 触发条件 | 作用对象 | 关联 AC |
|------|----------|----------|----------|---------|
| FR-10 | 在 `native_module_manager.cpp:1498` `tailNativeModule_->systemFilePath = strdup(loadPath);` 之后追加 nullptr 检查：失败时 (a) `free(const_cast<char*>(tailNativeModule_->moduleName)); tailNativeModule_->moduleName = nullptr;`（回滚）；(b) `SetLoadErrInfo(loadErrInfo, "internal error: out of memory");`；(c) `MODULEMNG_HILOG_ERROR("strdup systemFilePath failed");`；(d) `SetLoadingNativeModuleKey("");`；(e) `return nullptr;` | `strdup(loadPath)` 返回 nullptr | `native_module_manager.cpp` LoadNativeModule 内 | AC-10.1/10.2/10.3 |

### 验证映射

| 编号 | 对应规格项 | 验证方式 | 验证重点 |
|------|------------|----------|----------|
| VM-10.1 | FR-10 / AC-10.1 | 静态审查 + CI（OOM 注入） | 回滚 + 返回值 + loadErrInfo |
| VM-10.2 | FR-10 / AC-10.2 | 静态审查 + CI | moduleName 被 free |
| VM-10.3 | FR-10 / AC-10.3 | 静态审查 | SetLoadingNativeModuleKey 重置 |
| VM-10.4 | AC-10.4 | 单测 | 既有用例全过 |

### 兼容性声明（问题 #10）

- **已有 API 行为变更:** 否（`LoadNativeModule` 签名/返回值不变）
- **内部行为变更:** 是（OOM 时由"赋 nullptr 继续执行"改为"回滚 + 返回 nullptr"）
- **行为变更可接受性:** P2 OOM 健壮性修复。可接受

### 不涉及项（问题 #10）

| 维度 | 涉及？ | 依据 |
|------|--------|------|
| 方案 2（`std::string`/`unique_ptr` 替代） | 否 | 改动面巨大（POD 结构重构），超范围 |
| L1546 另一处 `strdup(loadPath)` | 否 | 不在问题描述范围内，留作单独问题 |
| `NativeModule` 结构体重构 | 否 | 超范围 |
| 既有 `strdup(moduleKey)` 失败处理 | 否 | 已正确处理，保留不动 |
| 性能/IPC/构建/国际化/数据迁移/API | 否 | 纯内部加固（OOM 健壮性维度） |
