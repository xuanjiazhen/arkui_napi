# 统一审查

> 简单复杂度：仅决策 + 必要证据。跳过架构设计审查、规范符合性审查、纠正循环（按裁剪表）。

## 审查元数据

| 项 | 内容 |
|----|------|
| Review ID | REV-1 |
| 审查阶段 | 最终验证 |
| 关联文档 | spec.md / task.md |
| 复杂度 | 简单 |
| 涉及仓 | arkui_napi |
| Reviewer | Agent (代码审查) + 用户（最终批准） |
| 日期 | 2026-07-06 |
| Base SHA | 3119c6c6 (gitcode/master) |
| Head SHA | 待 commit |

## 审查输入

| 输入 | 路径 | 说明 |
|------|------|------|
| Requirement | `proposal.md` | Baselined |
| Spec | `spec.md` | Approved |
| Plan | `task.md` | Approved |
| Diff | 工作区改动（未 commit） | 2 个文件 |

---

## 一、设计/Spec 审批回顾

> Stage 2 已批准（见 gate-design.md）。回顾确认：

| 检查项 | 结论 | 证据 |
|--------|------|------|
| 设计决策已记录（简单：一句技术约束） | PASS | spec.md §设计要点摘要 / task.md §代码变更规格 |
| Spec 规则覆盖全部 P0/P1 AC | PASS | US-1 → AC-1/AC-2/AC-3 → FR-1 → VM-1/VM-2/VM-3 |
| 不涉及项已显式确认 | PASS | proposal.md §不涉及项确认 全部 N/A |

---

## 二、规范符合性审查

### 需求覆盖

| AC/规则 | 是否实现 | 证据 | 结论 |
|---------|----------|------|------|
| FR-1: `strcasecmp(temp->name, ...)` 仅在 `temp->name != nullptr` 时执行 | 是 | `native_module_manager.cpp:1634` 改为 `(temp->name != nullptr && !strcasecmp(temp->name, moduleName))` | PASS |
| AC-1: name==nullptr 节点不崩溃、不误匹配 | 是 | `module_manager_test.cpp` 新增 `FindNativeModuleByCache_WhenNodeNameIsNullptrShouldNotCrash` 第二段（nodeA->next=nullptr，验证返回 nullptr） | PASS（代码层） |
| AC-2: name==nullptr 节点存在时 moduleName 匹配仍工作 | 是 | 同测试第一段（A->B 链表，验证返回 nodeB） | PASS（代码层） |
| AC-3: 既有用例不回归 | 是 | 修改范围严格限于 L1634，未触碰其他逻辑 | PASS（代码层） |

### 多余实现

| 实现内容 | 是否在 Spec/Plan 中 | 风险 | 处理 |
|----------|---------------------|------|------|
| 无 | - | - | - |

### 理解偏差

| 检查项 | 结论 | 证据 |
|--------|------|------|
| AC 理解是否正确 | PASS | 测试设计严格对应 AC-1/AC-2，WHEN/THEN 一致 |
| 边界和不做范围是否遵守 | PASS | 未触碰 L998-999、L1546-1553、其他 str*、入参保护、BUILD.gn |
| 适用规则是否遵守 | PASS | 修复代码与 task.md §代码变更规格 目标代码完全一致 |

---

## 三、代码质量审查（简单：仅决策）

| 检查项 | 结论 | 证据 |
|--------|------|------|
| 修复符合既有代码风格（短路判空模式） | PASS | 与 L1633 `(temp->moduleName && !strcmp(...))` 风格一致 |
| 修复符合 AGENTS.md 约束 | PASS | Google C++ Style；无新 API；无新依赖；HiLog 未变 |
| 测试符合既有用例命名/结构 | PASS | 参照 `FindNativeModuleByCache_*` 既有用例，含 `@tc.name`/`@tc.desc`/`@tc.type`/`@tc.require` 注释块 |
| 测试 SetUp/TearDown 一致 | PASS | 沿用 `ModuleManagerTest` fixture 既有约定（SetUp 中 `MockResetModuleManagerState`） |
| 无过度抽象 / 多余实现 | PASS | 仅 1 行修复 + 1 个测试函数 |
| 内存安全 | PASS | 测试中 `new NativeModule()` 与 `delete` 配对；既有测试约定（参见 L675-686） |

---

## 四、纠正循环

| 轮次 | 结论 | 处理动作 | 复检范围 |
|------|------|----------|----------|
| Review-1 | Approved（代码层） | 无 | - |

---

## 五、验证证据（关键）

| 证据类型 | 命令/路径 | 结果 | 说明 |
|---------|-----------|------|------|
| 修复代码 | `native_module_manager.cpp:1634` | PASS（静态审查） | 改动与 task.md 目标代码一致 |
| 测试代码 | `module_manager_test.cpp:1864-1927`（追加） | PASS（静态审查） | 语法、逻辑、命名均符合既有约定 |
| **编译验证** | `hb build napi -t` | **FAIL（环境问题）** | indenp 模式报 `OHOS component : (faultloggerd) not found` |
| **编译验证** | `./build.sh --product-name rk3568 --target-cpu arm64 --build-target napi_packages_test` | **FAIL（环境问题）** | GN 报 `is_host_product` 未定义（napi master 引入的新变量，本机 build/ 仓未跟上） |
| **运行验证（RED）** | 未运行 | **N/A** | 因编译失败，本机无法运行测试 |
| **运行验证（GREEN）** | 未运行 | **N/A** | 同上 |
| **运行验证（回归）** | 未运行 | **N/A** | 同上 |

**关键说明：** 本机构建系统版本与 arkui_napi master 分支不匹配。`is_host_product` 是 master 分支 BUILD.gn 引入的新 GN 变量，但本机 `build/` 仓未定义该变量，导致 `gn gen` 失败。这是环境版本不匹配问题，不是本次代码改动引入的问题。

按硬规则"证据先于声明"，本次审查**不能声称"测试通过"**。代码改动正确性已通过静态审查保证，但测试运行验证需要在能构建的环境（如 OH CI、SIG 持续集成或用户本地修复环境后）完成。

---

## 六、Open Issues

| 类型 | 问题 | 处理方式 | Owner |
|------|------|----------|-------|
| blocker（环境） | 本机 `is_host_product` 未定义，无法本地构建/运行测试 | 用户自行在能构建的环境（CI 或本地修复 build/ 仓）验证 | 用户 |
| follow-up | 简单变更跳过 L998-999 同类隐患；如未来需要统一加固，单独发起 SDD 流程 | 后续 | - |

---

## 七、审查决策

| 项 | 内容 |
|----|------|
| Decision | **代码层 Approved，验证 Deferred** |
| 下一阶段 | 用户决定跳过本机验证（环境 `is_host_product` 未定义）。代码修复已就绪，验证由 CI 或后续环境补齐。本 issue 暂停在 Stage 3 末尾，等所有问题修复完成后统一提交/合入。 |
| Recheck Scope | 后续 CI 提供测试运行证据（RED 应 CRASH/FAIL、GREEN 应 PASS、既有用例回归全 PASS） |
| 修改意见 | 阻塞项：无（用户已接受跳过本机验证）；建议项：CI 必须运行测试通过后方可合入 |

**审查摘要：**
- **结论**：代码层规范符合性 + 代码质量审查全部 PASS。修复最小、风格一致、无扩范围、无多余实现。
- **必须修复项**：无
- **可接受风险**：本机测试验证跳过（用户决定），由 CI 后续补齐证据

---

## 八、问题 #2 审查（同模式追加：systemFilePath 加固）

### 审查输入（问题 #2）

| 输入 | 路径 | 说明 |
|------|------|------|
| Requirement | `proposal.md` §问题 #2 | 待批准（基线扩展） |
| Spec | `spec.md` §问题 #2 区段 | 待批准 |
| Plan | `task.md` TASK-2 | 待批准 |
| Diff | `native_module_manager.cpp:1637-1643` + `module_manager_test.cpp:1921-1973` | 已实现 |

### 规范符合性审查（问题 #2）

| AC/规则 | 是否实现 | 证据 | 结论 |
|---------|----------|------|------|
| FR-2: L1637 strcmp 前 `systemFilePath != nullptr` | 是 | `native_module_manager.cpp:1637-1639` 改为含 `temp->systemFilePath != nullptr &&` 短路 | PASS |
| FR-2: L1642 `!strcmp` 改为 `(systemFilePath != nullptr && !strcmp(...))` | 是 | `native_module_manager.cpp:1642-1643` | PASS |
| AC-2.1: systemFilePath==nullptr + moduleName 匹配 → 不崩溃并返回该节点 | 是 | `module_manager_test.cpp` 新增 `FindNativeModuleByCache_WhenNodeSystemFilePathIsNullptrShouldNotCrash` 第一段，EXPECT_EQ(result, nodeX) | PASS（代码层） |
| AC-2.2: 同时含 nullptr 节点和正常节点 → 不崩溃 | 是 | 同测试第二段（追加 nodeY systemFilePath=""），EXPECT_EQ(result, nodeY) | PASS（代码层） |
| AC-2.3: 既有用例不回归 | 是 | 修改范围严格限于 L1637/L1642，未触碰其他逻辑 | PASS（代码层） |

### 多余实现

| 实现内容 | 是否在 Spec/Plan 中 | 风险 | 处理 |
|----------|---------------------|------|------|
| 无 | - | - | - |

### 理解偏差（问题 #2）

| 检查项 | 结论 | 证据 |
|--------|------|------|
| AC 理解是否正确 | PASS | spec AC-2.1 已修正为匹配最小修复语义（nullptr 节点被视为 path 0 匹配，返回该节点） |
| 边界和不做范围是否遵守 | PASS | 未触碰 L1460/1508 strdup、字段初始化、默认值调整、其他已判空使用点 |
| 适用规则是否遵守 | PASS | 修复代码与 task.md §TASK-2 目标代码完全一致 |

### 代码质量审查（问题 #2）

| 检查项 | 结论 | 证据 |
|--------|------|------|
| 修复符合既有短路判空模式 | PASS | 与 L1633 `(temp->moduleName && ...)` 和问题 #1 的 `(temp->name != nullptr && ...)` 模式一致 |
| 修复符合 AGENTS.md 约束 | PASS | Google C++ Style；无新 API/依赖；HiLog 未变 |
| 测试符合既有用例命名/结构 | PASS | 参照 `FindNativeModuleByCache_*` 既有约定，含 4 行 `@tc.*` 注释块 |
| 无过度抽象 / 多余实现 | PASS | 仅 L1637/L1642 修复 + 1 个测试函数（含两段验证） |
| 内存安全 | PASS | 测试中 `new NativeModule()` 与 `delete` 配对 |

### 行为变更可接受性（问题 #2）

| 项 | 评估 |
|----|------|
| 原行为 | systemFilePath==nullptr 进入 if 内层 → strcmp(nullptr,...) → SIGSEGV 崩溃 |
| 修复后行为 | nullptr 节点被视为 path 0 匹配（label=0 < NATIVE_PATH_NUMBER），返回该节点 |
| 可接受性 | P1 修复，"避免崩溃"为目标。RegisterByBuffer 创建的节点本意是加载使用，返回该节点符合加载意图。可接受。 |

### 验证证据（问题 #2）

| 证据类型 | 命令/路径 | 结果 | 说明 |
|---------|-----------|------|------|
| 修复代码 | `native_module_manager.cpp:1637-1643` | PASS（静态审查） | 改动与 task.md §TASK-2 目标代码一致 |
| 测试代码 | `module_manager_test.cpp:1921-1973`（追加） | PASS（静态审查） | 语法、逻辑、命名均符合既有约定 |
| 编译验证 | 同问题 #1 | FAIL（环境问题） | `is_host_product` 未定义 |
| 运行验证 | 未运行 | N/A | 同问题 #1，由 CI 后续补齐 |

### 审查决策（问题 #2）

| 项 | 内容 |
|----|------|
| Decision | **代码层 Approved，验证 Deferred** |
| 下一阶段 | 同问题 #1，等所有问题修完后统一由 CI 验证 |
| 修改意见 | 阻塞项：无；建议项：CI 必须运行测试通过后方可合入 |

**审查摘要（问题 #2）：**
- **结论**：代码层规范符合性 + 代码质量审查全部 PASS。修复最小、风格一致、与问题 #1 同模式。
- **必须修复项**：无
- **可接受风险**：本机测试验证跳过（用户决定）；行为变更（nullptr 视为 path 0 匹配）已评估为可接受

---

## 九、问题 #3 审查（TASK-3：EmplaceModuleBuffer 重复 key 内存泄漏）

### 一、规范符合性审查

| AC / 规则 | 实现位置 | 实现内容 | 符合性 | 偏差说明 |
|-----------|----------|----------|--------|----------|
| AC-3.1 | `native_module_manager.cpp:212-216` | 重复 key：`delete[] it->second; it->second = lib;` | 符合 | — |
| AC-3.2 | `native_module_manager.cpp:213` | 旧 buffer `delete[] it->second` 释放 | 符合 | ASan 由 CI 验证 |
| AC-3.3 | `native_module_manager.cpp:217-219` | 首次调用：`moduleBufMap_.emplace(moduleKey, lib)` 入库 | 符合 | — |
| AC-3.4 | — | 既有用例运行 | 待 CI | 同问题 #1/#2 |
| FR-3 | `native_module_manager.cpp:207-220` | find → 已存在则覆盖（delete[] 旧 + 替换）；否则 emplace | 符合 | — |

**规范符合性结论：** 全部 AC + FR-3 均有对应实现，无遗漏、无多余实现、无理解偏差。

### 二、代码质量审查

| 检查项 | 结论 | 证据 |
|--------|------|------|
| 锁内完整覆盖 find/replace/emplace | PASS | 三步均在 `std::lock_guard<std::mutex> lock(moduleBufMutex_)` 锁内（L210），无并发间隙 |
| `delete[]` 与分配方式匹配 | PASS | 上游 `GetFileBuffer`（L1320 附近）由 `std::make_unique<uint8_t[]>` + `release()` 入库，C 数组语义 → `delete[]` 正确 |
| HILOG 保留 | PASS | L209 `MODULEMNG_HILOG_DEBUG` 与原实现一致 |
| 入参 nullptr 处理 | PASS | L211-213 `if (lib == nullptr) { return; }` 早返，与原 `if (lib != nullptr)` 等价但更清晰 |
| 测试符合既有命名/结构 | PASS | 含 4 行 `@tc.*` 注释块；`HWTEST_F(..., TestSize.Level1)` 与既有用例一致 |
| 测试内存清理无泄漏 | PASS | lib1 由修复代码 `delete[]`；lib2 由测试自身 `delete[]`；map 析构不释放裸指针（既有契约） |
| 无过度抽象 / 多余实现 | PASS | 函数体仅从 3 行扩为 8 行，纯最小修复 |
| 不修改 EmplaceModuleLib | PASS | 严格遵守"每次仅修问题描述本身"规则（L173-180 未动） |

### 行为变更可接受性（问题 #3）

| 项 | 评估 |
|----|------|
| 原行为 | 重复 key：`emplace` 失败（map 不覆盖），新 `lib` 既未入库也未释放 → 堆泄漏 |
| 修复后行为 | 重复 key：旧 buffer 被 `delete[]`，map[key] 被替换为新 lib（覆盖语义） |
| 调用方影响 | 唯一调用方 `GetFileBuffer`（L1325 附近）语义为缓存，覆盖语义符合缓存预期 |
| 可接受性 | P1 修复，避免泄漏 + 提供合理覆盖语义。可接受。 |

### 验证证据（问题 #3）

| 证据类型 | 命令/路径 | 结果 | 说明 |
|---------|-----------|------|------|
| 修复代码 | `native_module_manager.cpp:207-220` | PASS（静态审查） | 改动与 task.md §TASK-3 目标代码逐字一致 |
| 测试代码 | `module_manager_test.cpp:1980-2025`（追加） | PASS（静态审查） | 两段 EXPECT_EQ 验证 size + 覆盖语义；含 AC-3.3 首次调用断言 |
| 编译验证 | 同问题 #1 | FAIL（环境问题） | `is_host_product` 未定义 |
| 运行验证 | 未运行 | N/A | 同问题 #1/#2，由 CI 后续补齐 |
| ASan（验证 AC-3.2 旧值释放） | 待 CI | 待运行 | 代码层已通过 `delete[]` 满足，运行时由 CI ASan 报告确认 |

### 审查决策（问题 #3）

| 项 | 内容 |
|----|------|
| Decision | **代码层 Approved，验证 Deferred** |
| 下一阶段 | 同问题 #1/#2，等所有问题修完后统一由 CI 验证 |
| 修改意见 | 阻塞项：无；建议项：CI 必须运行测试 + ASan 通过后方可合入 |

**审查摘要（问题 #3）：**
- **结论**：代码层规范符合性 + 代码质量审查全部 PASS。修复最小、锁内覆盖完整、`delete[]` 与上游分配方式匹配。
- **必须修复项**：无
- **可接受风险**：本机测试验证跳过（用户决定）；行为变更（重复 key 由"不覆盖+泄漏"改为"覆盖+释放"）已评估为可接受
- **范围合规**：严格遵守"每次仅修问题描述本身"规则，未触碰 EmplaceModuleLib 等同类点

---

## 十、问题 #4 审查（TASK-4：GetNativeModulePath operator[] 默认插入 nullptr）

### 一、规范符合性审查

| AC / 规则 | 实现位置 | 实现内容 | 符合性 | 偏差说明 |
|-----------|----------|----------|--------|----------|
| AC-4.1 | `native_module_manager.cpp:1058-1064` | `if (isAppModule)` 内 `find` 取值；map value==nullptr 时 `prefix` 保持 nullptr，进入 `else` fallback | 符合 | — |
| AC-4.2 | `native_module_manager.cpp:1060` | 用 `find`（不污染 map），未用 `operator[]` | 符合 | — |
| AC-4.3 | `native_module_manager.cpp:1065` + 原 L1066-1098 | value 非空时 `prefix != nullptr` 进入 if 内层（原 `#ifdef ANDROID_PLATFORM` 块保留） | 符合 | — |
| AC-4.4 | — | 既有用例运行 | 待 CI | 同问题 #1/#2/#3 |
| FR-4-a | `native_module_manager.cpp:1060` | 使用 `find` 而非 `operator[]` | 符合 | — |
| FR-4-b | `native_module_manager.cpp:1059` | `std::lock_guard<std::mutex> guard(appLibPathMapMutex_)` 持锁完整读 | 符合 | — |
| FR-4-c | `native_module_manager.cpp:1065` | 读取后判 `prefix != nullptr`，nullptr 走 fallback | 符合 | — |

**规范符合性结论：** 全部 AC + FR-4 (a/b/c) 均有对应实现，无遗漏、无多余实现、无理解偏差。

### 二、代码质量审查

| 检查项 | 结论 | 证据 |
|--------|------|------|
| lock_guard 替代手动 lock/unlock | PASS | L1059 用 RAII `std::lock_guard`，原 L1059/1061 手动 lock/unlock 已移除。无锁泄漏风险（异常安全） |
| find/end 迭代器正确 | PASS | L1060-1063 `auto it = ...find(path); if (it != .end()) { prefix = it->second; }` 标准 STL 用法 |
| 原 `#ifdef ANDROID_PLATFORM` 块原样保留 | PASS | L1066-1098 内容未动（仅外层包装由 `if (isAppModule && IsExistedPath)` 改为 `if (prefix != nullptr)`） |
| 原 `else` fallback 块（含 IOS_PLATFORM `appLibPathMap_["default"]`）原样保留 | PASS | L1099-1117 未动。该 `appLibPathMap_["default"]` 属其他使用点，按用户规则显式排除，未触碰 |
| 原死代码保留 | PASS | L1070-1072 `if (prefix == nullptr) return false;` 在新结构下为死代码（外层 `if (prefix != nullptr)` 已保证），按最小变更原则保留 |
| IsExistedPath 调用减少 | PASS | L1058 去掉冗余的 `IsExistedPath(path)` 调用（合并到 find）。`IsExistedPath` 实现本身未动 |
| 测试符合既有命名/结构 | PASS | 含 4 行 `@tc.*` 注释块；3 段断言覆盖 AC-4.1/4.2/4.3 |
| 测试内存清理 | PASS | 注入的 `const std::string` 局部对象，`appLibPathMap_.clear()` 清理；非堆内存 |
| 无过度抽象 / 多余实现 | PASS | if/else 块整体重构最小，原 30 行块改为 33 行（新增 6 行 find 逻辑 + 1 行 if 包装 - 4 行删去的 lock/operator[]/unlock/IsExistedPath） |
| 不修改 IsExistedPath 实现 | PASS | L1676-1681 未动 |
| 不修改其他 operator[] 使用点 | PASS | L1112 IOS_PLATFORM `appLibPathMap_["default"]` 未动（显式排除） |

### 行为变更可接受性（问题 #4）

| 项 | 评估 |
|----|------|
| 原行为（key 不存在） | `IsExistedPath` 返回 false → 走 else (fallback)。**但** 若曾被污染，`IsExistedPath` 返回 true → 进 if → `operator[]` 取 nullptr → sprintf_s 解引用崩溃 |
| 原行为（key 存在但 value=nullptr） | `IsExistedPath` 返回 true → 进 if → `operator[]` 取 nullptr → 非 Android 路径 sprintf_s 解引用崩溃 |
| 修复后行为（key 不存在） | `find` 返回 end → prefix 保持 nullptr → 走 else (fallback)。**无 map 污染** |
| 修复后行为（key 存在但 value=nullptr） | `find` 命中 → prefix = nullptr → `if (prefix != nullptr)` false → 走 else (fallback)。**不崩溃** |
| 修复后行为（key 存在且 value 非空） | `find` 命中 → prefix = value → `if (prefix != nullptr)` true → 进 if 内层（原行为） |
| 调用方影响 | `GetNativeModulePath` 是 private 接口，调用方仅本类内部。修复使原本会崩溃的路径走 fallback，是合理降级 |
| 可接受性 | P1 修复，"避免崩溃"为目标。可接受 |

### 验证证据（问题 #4）

| 证据类型 | 命令/路径 | 结果 | 说明 |
|---------|-----------|------|------|
| 修复代码 | `native_module_manager.cpp:1057-1099` | PASS（静态审查） | 改动与 task.md §TASK-4 目标代码逐字一致；原 `#ifdef ANDROID_PLATFORM` 与 `else` fallback 块未动 |
| 测试代码 | `module_manager_test.cpp:2022-2073`（追加） | PASS（静态审查） | 3 段断言：nullptr 值不崩溃+不污染；不存在 key 不污染；非空值进入 if 内层 |
| 编译验证 | 同问题 #1 | FAIL（环境问题） | `is_host_product` 未定义 |
| 运行验证 | 未运行 | N/A | 同问题 #1/#2/#3，由 CI 后续补齐 |

### 审查决策（问题 #4）

| 项 | 内容 |
|----|------|
| Decision | **代码层 Approved，验证 Deferred** |
| 下一阶段 | 同问题 #1/#2/#3，等所有问题修完后统一由 CI 验证 |
| 修改意见 | 阻塞项：无；建议项：CI 必须运行测试通过后方可合入 |

**审查摘要（问题 #4）：**
- **结论**：代码层规范符合性 + 代码质量审查全部 PASS。修复符合用户建议（find + lock_guard + nullptr fallback 三项一次性完成），原 `#ifdef ANDROID_PLATFORM` 内层与 `else` fallback 块原样保留。
- **必须修复项**：无
- **可接受风险**：本机测试验证跳过（用户决定）；原 ANDROID_PLATFORM 块内 `if (prefix == nullptr) return false;` 死代码保留（最小变更原则）
- **范围合规**：严格遵守"每次仅修问题描述本身"规则，未触碰 `IsExistedPath` 实现、其他 `operator[]` 使用点（L1112 IOS_PLATFORM `appLibPathMap_["default"]`）、其他 `IsExistedPath` 调用点

---

## 十一、问题 #5 审查（TASK-5：SetAppLibPath 空字符串 back() UB）

### 一、规范符合性审查

| AC / 规则 | 实现位置 | 实现内容 | 符合性 | 偏差说明 |
|-----------|----------|----------|--------|----------|
| AC-5.1 | `native_module_manager.cpp:758` | 空 vector 时 `tmpPath=""`，`!tmpPath.empty()` 短路跳过 `back()` | 符合 | — |
| AC-5.2 | `native_module_manager.cpp:758` | 全空字符串元素时同样短路跳过 | 符合 | — |
| AC-5.3 | `native_module_manager.cpp:758` | 非空元素时 `!tmpPath.empty()` 为 true，原 `back()` + `pop_back()` 流程不变 | 符合 | — |
| AC-5.4 | — | 既有用例运行 | 待 CI | 同问题 #1-#4 |
| FR-5 | `native_module_manager.cpp:758` | `if (!tmpPath.empty() && tmpPath.back() == ':')` 短路判空 | 符合 | — |

**规范符合性结论：** 全部 AC + FR-5 均有对应实现，无遗漏、无多余实现、无理解偏差。

### 二、代码质量审查

| 检查项 | 结论 | 证据 |
|--------|------|------|
| 短路判空语法正确 | PASS | L758 `!tmpPath.empty() && tmpPath.back() == ':'`：`&&` 左操作数 false 时右操作数不求值，`back()` 不被调用 |
| 最小变更 | PASS | L758 一行改动（加 `!tmpPath.empty() &&` 前缀），其余代码（L750-757 循环、L759 pop_back、L762+ strdup/入库）原样未动 |
| 下游行为兼容 | PASS | L762 `strdup("")` 在 `tmpPath=""` 时返回有效空串指针（非 nullptr）；后续 `appLibPathMap_[moduleName]` 入库空串，与原非崩溃路径行为一致 |
| 测试符合既有命名/结构 | PASS | 含 4 行 `@tc.*` 注释块；3 段断言覆盖 AC-5.1/5.2/5.3 |
| 测试内存清理 | PASS | 段 3 验证入库值后，遍历 map `free()` 每个 strdup 的 `char*`，再 `clear()`，无测试自身泄漏 |
| 无过度抽象 / 多余实现 | PASS | 仅 1 行修复 + 1 个测试函数（含 3 段验证） |
| 不加函数入口校验 | PASS | 严格遵守"每次仅修问题描述本身"规则，函数入口 `if (appLibPath.empty()) return;` 未加（用户参考代码未含此项） |
| 不修改其他 back()/front() 使用点 | PASS | 仅 L758 一处变更 |
| 不修改 L762/769/770 等其他潜在 UB/operator[] 点 | PASS | 未触碰 |

### 行为变更可接受性（问题 #5）

| 项 | 评估 |
|----|------|
| 原行为（空 vector 或全空元素） | L758 `tmpPath.back()` 解引用空串 → UB（多数实现崩溃） |
| 修复后行为（空 vector 或全空元素） | L758 `!tmpPath.empty()` 短路为 false → 跳过 `back()` + `pop_back()`。后续 `strdup("")` 入库（与原非崩溃路径一致） |
| 原行为（含非空元素） | `tmpPath.back()` 取末尾冒号 → `pop_back` 去除 → 入库 |
| 修复后行为（含非空元素） | 完全不变（`!tmpPath.empty()` 为 true，原流程保留） |
| 调用方影响 | `SetAppLibPath` 是 public 接口。原崩溃路径改为安全入库空串，是合理降级 |
| 可接受性 | P1 修复，"避免崩溃"为目标。可接受 |

### 验证证据（问题 #5）

| 证据类型 | 命令/路径 | 结果 | 说明 |
|---------|-----------|------|------|
| 修复代码 | `native_module_manager.cpp:758` | PASS（静态审查） | 改动与 task.md §TASK-5 目标代码逐字一致 |
| 测试代码 | `module_manager_test.cpp:2076-2117`（追加） | PASS（静态审查） | 3 段断言：空 vector 不崩溃；全空字符串不崩溃；非空元素入库值正确 |
| 编译验证 | 同问题 #1 | FAIL（环境问题） | `is_host_product` 未定义 |
| 运行验证 | 未运行 | N/A | 同问题 #1-#4，由 CI 后续补齐 |

### 审查决策（问题 #5）

| 项 | 内容 |
|----|------|
| Decision | **代码层 Approved，验证 Deferred** |
| 下一阶段 | 同问题 #1-#4，等所有问题修完后统一由 CI 验证 |
| 修改意见 | 阻塞项：无；建议项：CI 必须运行测试通过后方可合入 |

**审查摘要（问题 #5）：**
- **结论**：代码层规范符合性 + 代码质量审查全部 PASS。修复最小（1 行短路判空），下游 strdup 入库行为兼容。
- **必须修复项**：无
- **可接受风险**：本机测试验证跳过（用户决定）
- **范围合规**：严格遵守"每次仅修问题描述本身"规则，未加函数入口校验、未触碰其他 back()/front() 使用点、未触碰 L762/769/770 其他 UB/operator[] 点

---

## 十二、问题 #6 审查（TASK-6：LoadNativeModule relativePath 路径穿越校验加固）

### 一、规范符合性审查

| AC / 规则 | 实现位置 | 实现内容 | 符合性 | 偏差说明 |
|-----------|----------|----------|--------|----------|
| AC-6.1 | `native_module_manager.cpp:1698-1706` | 分段循环只检查完整分段是否为 `.`/`..`/空，"my..pkg" 这类含 ".." 子串但分段非 `..` 的不被拒 | 符合 | — |
| AC-6.2 | `native_module_manager.cpp:1693-1694` | `p.front() == '/'` → false，拒绝绝对路径 | 符合 | — |
| AC-6.3 | `native_module_manager.cpp:1699-1700` | 分段为空 / `.` / `..` → false | 符合 | — |
| AC-6.4 | `native_module_manager.cpp:1702-1705` | 字符非 alnum 且非 `_`/`.` → false（白名单） | 符合 | — |
| AC-6.5 | `native_module_manager.cpp:1690-1692, 1708` | 空串→true；合法分段字符通过 → true | 符合 | — |
| AC-6.6 | — | 既有用例运行 | 待 CI | 同问题 #1-#5 |
| FR-6 | `native_module_manager.cpp:1688-1709` + L864-868 | helper 实现 + LoadNativeModule 调用点替换 | 符合 | — |

**规范符合性结论：** 全部 AC + FR-6 均有对应实现，无遗漏、无多余实现、无理解偏差。

### 二、代码质量审查

| 检查项 | 结论 | 证据 |
|--------|------|------|
| helper 声明为 `static` 成员函数 | PASS | `native_module_manager.h:168` `static bool IsSafeRelativePath(const std::string& p);`。不访问 `this`，符合 static 语义；mock 头 `#define private public` 仍可暴露 |
| helper 实现简洁 | PASS | 21 行，无嵌套超过 2 层；条件短路返回，可读性高 |
| `std::isalnum` 安全使用 | PASS | L1703 `static_cast<unsigned char>(c)` 避免 char 符号扩展问题（标准用法） |
| `<cctype>` 显式包含 | PASS | `native_module_manager.cpp:18` 新增 `#include <cctype>`，避免依赖传递包含的可移植性问题 |
| `<sstream>`、`<string>` 已包含 | PASS | `<sstream>` 在 L25；`<string>` 经 `native_module_manager.h` L25 包含 |
| 调用点替换最小 | PASS | L864-868 仅替换 if 条件与 HILOG 文案；原 `SetLoadErrInfo(...)`、`return nullptr` 流程保留 |
| HILOG 文案更新 | PASS | L866 由 `"failed"` 改为 `"invalid relativePath: rejected by whitelist"`，更明确 |
| 测试覆盖 6 类场景 | PASS | 22 个 EXPECT_TRUE/FALSE 断言覆盖 AC-6.1-6.5 全部场景 |
| 测试通过 mock 头直接调用 | PASS | `NativeModuleManager::IsSafeRelativePath(...)`（static 成员，无需实例）；mock 头已用 `#define private public` 暴露 |
| 无过度抽象 / 多余实现 | PASS | helper 单一职责，调用点最小替换 |
| 不加 realpath + 前缀校验 | PASS | 用户参考代码未含此项，严格遵守"每次仅修问题描述本身"规则 |
| 不加 URL 解码 / 符号链接检测 | PASS | 显式排除 |
| 不修改其他路径校验点 | PASS | 未触碰 `moduleName`/`path` 入参校验、`GetNativeModulePath` 拼接等 |
| 不修改 `LoadNativeModule` 其他校验逻辑 | PASS | L848-860 nullptr 检查等未动 |

### 行为变更可接受性（问题 #6）

| 项 | 评估 |
|----|------|
| 原行为（含 ".." 子串） | `find("..")` 命中 → 拒绝（误报：合法路径 "my..pkg" 被拒） |
| 修复后行为（含 ".." 子串非完整分段） | helper 接受（消除误报） |
| 原行为（绝对路径） | `find("..")` 未命中 → 接受（漏报："/etc/passwd" 放行） |
| 修复后行为（绝对路径） | helper 拒绝（前导 `/`） |
| 原行为（`./`、`../`、`//` 分段） | 接受（漏报） |
| 修复后行为 | helper 拒绝（分段为 `.`/`..`/空） |
| 原行为（非白名单字符） | 接受（漏报） |
| 修复后行为 | helper 拒绝（白名单） |
| 原行为（合法相对路径） | 接受 |
| 修复后行为（合法相对路径） | 接受（不变） |
| 调用方影响 | `LoadNativeModule` 是 public 接口。原合法调用方传 "module/sub" 等正常路径行为不变；传含穿越向量的调用方被拒（修复目标） |
| 可接受性 | P1 安全修复，"消除误报 + 阻断穿越向量"为目标。可接受 |

### 验证证据（问题 #6）

| 证据类型 | 命令/路径 | 结果 | 说明 |
|---------|-----------|------|------|
| helper 声明 | `native_module_manager.h:168` | PASS（静态审查） | `static bool IsSafeRelativePath(const std::string& p);` |
| helper 实现 | `native_module_manager.cpp:1688-1709` | PASS（静态审查） | 与 task.md §TASK-6 目标代码逐字一致 |
| 调用点替换 | `native_module_manager.cpp:864-868` | PASS（静态审查） | `find("..")` 已替换为 `!IsSafeRelativePath(...)` |
| 头文件包含 | `native_module_manager.cpp:18` | PASS | 新增 `#include <cctype>` |
| 测试代码 | `module_manager_test.cpp:2120-2164`（追加） | PASS（静态审查） | 22 个断言覆盖 AC-6.1-6.5 全场景 |
| 编译验证 | 同问题 #1 | FAIL（环境问题） | `is_host_product` 未定义 |
| 运行验证 | 未运行 | N/A | 同问题 #1-#5，由 CI 后续补齐 |

### 审查决策（问题 #6）

| 项 | 内容 |
|----|------|
| Decision | **代码层 Approved，验证 Deferred** |
| 下一阶段 | 同问题 #1-#5，等所有问题修完后统一由 CI 验证 |
| 修改意见 | 阻塞项：无；建议项：CI 必须运行测试通过后方可合入 |

**审查摘要（问题 #6）：**
- **结论**：代码层规范符合性 + 代码质量审查全部 PASS。helper 实现简洁、调用点替换最小、测试覆盖完整（6 类场景 22 个断言）。
- **必须修复项**：无
- **可接受风险**：本机测试验证跳过（用户决定）；行为变更双向（放宽 + 收紧）已评估为可接受
- **范围合规**：严格遵守"每次仅修问题描述本身"规则，未加 realpath + 前缀校验、未加 URL 解码/符号链接检测、未触碰其他路径校验点、未修改 `LoadNativeModule` 其他校验逻辑

---

## 十三、问题 #7 审查（TASK-7：greylist.json lib 名严格字符集校验）

### 一、规范符合性审查

| AC / 规则 | 实现位置 | 实现内容 | 符合性 | 偏差说明 |
|-----------|----------|----------|--------|----------|
| AC-7.1 | `native_module_manager.cpp:1712-1724` helper | 字符白名单循环中 `:` 不在允许集 → false | 符合 | — |
| AC-7.2 | `native_module_manager.cpp:1718-1720` | 空格/斜杠/反斜杠/`$`/`%` 等均非 alnum/_/-/. → false | 符合 | — |
| AC-7.3 | `native_module_manager.cpp:1723` | `libName.find("...") == npos` → 含 `...` 时 false | 符合 | — |
| AC-7.4 | `native_module_manager.cpp:1715-1717` | `substr(size-3) != ".so"` → false | 符合 | — |
| AC-7.5 | `native_module_manager.cpp:1712-1714` | `empty() || size() >= NAME_MAX` → false | 符合 | — |
| AC-7.6 | `native_module_manager.cpp:1712-1724` 全部检查通过 | 合法名 → true | 符合 | — |
| AC-7.7 | — | 既有用例运行 | 待 CI | 同问题 #1-#6 |
| FR-7 | `native_module_manager.cpp:1711-1725` + L515 调用点 | helper 实现 + LoadGreylistConfig 调用点替换 | 符合 | — |

**规范符合性结论：** 全部 AC + FR-7 均有对应实现，无遗漏、无多余实现、无理解偏差。

### 二、代码质量审查

| 检查项 | 结论 | 证据 |
|--------|------|------|
| helper 声明为 `static` 成员函数 | PASS | `native_module_manager.h:169` `static bool IsValidLibNameStrict(const std::string& libName);` |
| helper 实现简洁 | PASS | 14 行，条件短路返回，无嵌套超过 2 层 |
| `std::isalnum` 安全使用 | PASS | L1719 `static_cast<unsigned char>(c)` 避免 char 符号扩展（标准用法） |
| `NAME_MAX` 显式包含 | PASS | `native_module_manager.cpp:20` 新增 `#include <climits>` |
| `<cctype>` 已含（来自 #6） | PASS | L18 已含 `#include <cctype>`，本 helper 复用 |
| 调用点替换最小 | PASS | L515 仅替换函数名，HILOG 文案、push_back 流程保留 |
| 原 `IsValidLibName`（L442-456 static free function）保留 | PASS | 未触碰。即便成为孤立死代码（L515 唯一调用点已替换），按"每次仅修问题描述本身"规则不删 |
| 测试覆盖 7 类场景 | PASS | 19 个 EXPECT_TRUE/FALSE 断言覆盖 AC-7.1-7.6 全场景 |
| 测试通过 mock 头直接调用 | PASS | `NativeModuleManager::IsValidLibNameStrict(...)`（static 成员，无需实例）；mock 头已用 `#define private public` 暴露 |
| 超长字符串构造正确 | PASS | `std::string(NAME_MAX, 'a')` 长度 == NAME_MAX，触发 `size() >= NAME_MAX` 拒绝 |
| 无过度抽象 / 多余实现 | PASS | helper 单一职责，调用点最小替换 |
| 不替换为 JSON 解析器 | PASS | 用户参考代码未含此项 |
| 不加配置完整性校验 | PASS | 用户参考代码未含此项 |
| 不修改 JSON 解析逻辑（L483-521） | PASS | 未触碰 |
| 不修改 `sharedLibsSonames_` 拼接（L598-617） | PASS | 未触碰 |
| 不修改 `dlns_inherit` 调用点（L679/687） | PASS | 未触碰 |

### 行为变更可接受性（问题 #7）

| 项 | 评估 |
|----|------|
| 原行为（含冒号等危险字符的 lib 名） | `IsValidLibName` 仅校验 `.so` 后缀 → 接受 `liba:b.so` → 进入 `sharedLibsSonames_`（L601 sprintf_s `"%s:"` 拼接）→ `dlns_inherit` 解析时 `:` 被视为分隔符 → 绕过 namespace 隔离 |
| 修复后行为 | `IsValidLibNameStrict` 字符白名单 → 拒绝含 `:`/空格/斜杠等非白名单字符的 lib 名 → 无法注入分隔符 |
| 原行为（合法 lib 名） | `IsValidLibName` 通过 → 入库 |
| 修复后行为（合法 lib 名） | `IsValidLibNameStrict` 全部检查通过 → 入库（不变） |
| 调用方影响 | `LoadGreylistConfig` 是 public 接口。现有 greylist.json 中所有合法 lib 名均符合白名单（标准命名规范 `lib<name>.so`）。含非白名单字符的 lib 名（实际不应存在）会被拒绝。 |
| 可接受性 | P1 安全修复，"阻断 namespace 注入"为目标。可接受 |

### 验证证据（问题 #7）

| 证据类型 | 命令/路径 | 结果 | 说明 |
|---------|-----------|------|------|
| helper 声明 | `native_module_manager.h:169` | PASS（静态审查） | `static bool IsValidLibNameStrict(const std::string& libName);` |
| helper 实现 | `native_module_manager.cpp:1711-1725` | PASS（静态审查） | 与 task.md §TASK-7 目标代码逐字一致 |
| 调用点替换 | `native_module_manager.cpp:516` | PASS（静态审查） | `IsValidLibName(libName)` 已替换为 `NativeModuleManager::IsValidLibNameStrict(libName)` |
| 头文件包含 | `native_module_manager.cpp:20` | PASS | 新增 `#include <climits>` |
| 测试代码 | `module_manager_test.cpp:2167-2207`（追加） | PASS（静态审查） | 19 个断言覆盖 AC-7.1-7.6 全场景 |
| 编译验证 | 同问题 #1 | FAIL（环境问题） | `is_host_product` 未定义 |
| 运行验证 | 未运行 | N/A | 同问题 #1-#6，由 CI 后续补齐 |

### 审查决策（问题 #7）

| 项 | 内容 |
|----|------|
| Decision | **代码层 Approved，验证 Deferred** |
| 下一阶段 | 同问题 #1-#6，等所有问题修完后统一由 CI 验证 |
| 修改意见 | 阻塞项：无；建议项：CI 必须运行测试通过后方可合入 |

**审查摘要（问题 #7）：**
- **结论**：代码层规范符合性 + 代码质量审查全部 PASS。helper 实现简洁、调用点替换最小、测试覆盖完整（7 类场景 19 个断言）。
- **必须修复项**：无
- **可接受风险**：本机测试验证跳过（用户决定）；原 `IsValidLibName` 成为孤立死代码（保留不删，最小变更原则）；行为变更（含危险字符 lib 名由"接受"改为"拒绝"）已评估为可接受
- **范围合规**：严格遵守"每次仅修问题描述本身"规则，未替换为 JSON 解析器、未加配置完整性校验、未触碰 JSON 解析逻辑/`sharedLibsSonames_` 拼接/`dlns_inherit` 调用点/原 `IsValidLibName`
