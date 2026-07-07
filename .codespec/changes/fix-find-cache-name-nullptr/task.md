# 任务规格

> 最小可执行单元。AI Agent 据此独立完成编码实现。
>
> **复杂度裁剪**：简单（单点加固 + 1 测试）。1 个 Task 内含 TDD RED-GREEN-REFACTOR。跳过 execution-plan.md。

## 任务元数据

| 字段 | 内容 |
|------|------|
| Task ID | TASK-1 |
| 标题 | FindNativeModuleByCache nullptr 判空加固 + 单元测试 |
| 关联 Feature | fix-find-cache-name-nullptr |
| 目标仓库 | arkui_napi |
| 目标模块 | module_manager |
| 分支 | fix/find-cache-name-nullptr（基于 master 创建） |
| 优先级 | P1 |
| 复杂度 | 低 |
| 执行方式 | 主线程顺序执行 |

## 工作区边界（Stage 3 入口硬规则）

| 字段 | 内容 |
|------|------|
| Git 仓 | foundation/arkui/napi（远程 gitcode: openharmony/arkui_napi） |
| base 分支 | master（已 reset 到 gitcode/master 最新：3119c6c6） |
| 工作区状态 | 干净，仅 `.codespec/` 未跟踪（SDD 产物，保留） |
| 工作分支策略 | 基于 master 创建 `fix/find-cache-name-nullptr`，所有代码改动在该分支 |
| 多仓变更 | 否（单仓） |
| 生成文件 | 无（无 bridge/IDL/CAPI/cpptoc/ctocpp） |
| 允许修改文件 | `module_manager/native_module_manager.cpp`（L1634 一行） |
| 允许新增文件 | `module_manager/test/unittest/module_manager_test/module_manager_test.cpp`（追加测试用例，非新文件） |

## 任务描述

### 做什么

按 TDD RED-GREEN-REFACTOR：

1. **RED**：在 `module_manager_test.cpp` 末尾追加新测试 `FindNativeModuleByCache_WhenNodeNameIsNullptrShouldNotCrash`，注入 `name==nullptr` 的节点（`new NativeModule()` 默认 name=nullptr），调用 `FindNativeModuleByCache`，验证不崩溃且不误匹配。在未修复代码上运行 → 预期 CRASH（segfault）或失败，证明测试有效。
2. **GREEN**：修改 `native_module_manager.cpp:1634`，将 `|| !strcasecmp(temp->name, moduleName)` 改为 `|| (temp->name != nullptr && !strcasecmp(temp->name, moduleName))`。运行新测试 → 预期 PASS。
3. **REFACTOR**：审视改动，无进一步重构空间（最小变更）。
4. **回归**：运行全部 `FindNativeModuleByCache_*` 既有用例 → 预期全 PASS。

### 不做什么

- 不修改 L998-999 `CheckNativeListChanged`
- 不修改 L1546-1553 `RegisterByBuffer` 内存管理
- 不修改其他 `str*` 调用点
- 不添加 `moduleName` 入参的 nullptr 保护
- 不改 BUILD.gn（既有 test 目标已包含 module_manager_test.cpp）
- 不修改 .codespec/ 之外的任何文档

## 规格映射与边界

### AC 映射

| AC | 来源 | 验证方式 |
|----|------|----------|
| AC-1 | spec.md | 新测试中注入 name==nullptr 节点，断言不崩溃且返回值正确 |
| AC-2 | spec.md | 新测试中额外注入一个 moduleName 匹配的节点，断言能找到 |
| AC-3 | spec.md | 运行既有 FindNativeModuleByCache_* 全部用例 |

### 规则映射

| Rule ID | Must / Must Not |
|---------|-----------------|
| FR-1 | Must: `strcasecmp(temp->name, moduleName)` 仅在 `temp->name != nullptr` 时执行 |
| AGENTS.md-C++ | Must: 遵循 Google C++ Style；不引入裸 new/delete（测试中 `new NativeModule()` 是既有约定，配合 `delete`） |
| AGENTS.md-日志 | Must: 使用 HiLog，禁止 printf |
| 硬规则-不扩范围 | Must Not: 实际修改文件不超过本 Task 受影响文件清单 |

### 前置依赖

| 类型 | 编号 | 原因 |
|------|------|------|
| Spec | spec.md (Approved) | 实现必须基于已批准规格 |
| Stage 2 Gate | gate-design.md (Approved) | Plan 之前不得实现 |

### 完成判据

- [ ] 新测试 `FindNativeModuleByCache_WhenNodeNameIsNullptrShouldNotCrash` 已添加
- [ ] RED 验证：未修复时代码下新测试 CRASH/FAIL（证据：测试运行输出）
- [ ] 修复代码已合入 L1634
- [ ] GREEN 验证：修复后新测试 PASS
- [ ] 既有 `FindNativeModuleByCache_*` 测试全部 PASS（AC-3）
- [ ] 实际修改文件未超出受影响文件清单

### 停止条件

- 测试框架无法注入 name==nullptr 节点（已预验证可注入，但运行时若发现限制则暂停）
- 编译失败且无法在 1-2 次尝试内解决（升级为 build-error-resolver）
- 修复引入其他测试回归（暂停，回修源头）

## 受影响文件

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| 修改 | `module_manager/native_module_manager.cpp` | L1634 加 nullptr 短路判空 |
| 修改（追加用例） | `module_manager/test/unittest/module_manager_test/module_manager_test.cpp` | 末尾追加 `FindNativeModuleByCache_WhenNodeNameIsNullptrShouldNotCrash` |

## AC

- **AC-1:** WHEN 链表中存在某个节点 `name == nullptr` 且其他节点均不匹配传入的 moduleName THEN `FindNativeModuleByCache` 不崩溃且不误匹配
- **AC-2:** WHEN 链表中存在某个节点 `name == nullptr` 且另一节点通过 `moduleName` 字段匹配 THEN `FindNativeModuleByCache` 返回该匹配节点
- **AC-3:** WHEN 运行既有全部 `FindNativeModuleByCache_*` 单元测试 THEN 全部通过

## 代码变更规格

### 修改文件: `module_manager/native_module_manager.cpp`

**变更原因：** `||` 右侧 `strcasecmp(temp->name, moduleName)` 未判 `temp->name` 是否 nullptr，存在空指针解引用风险。

**变更摘要：** 在 L1634 的 `||` 右侧表达式前加 `temp->name != nullptr &&` 短路判空。

**关键行号：** L1632-1634（`FindNativeModuleByCache` 内的 for 循环 if 判断）

**目标代码（修复后）：**

```cpp
for (NativeModule* temp = headNativeModule_; temp != nullptr; temp = temp->next) {
    if ((temp->moduleName && !strcmp(temp->moduleName, moduleName))
        || (temp->name != nullptr && !strcasecmp(temp->name, moduleName))) {
```

**设计引用：**

| 设计要素 | 引用位置 | 关键结论 |
|---------|----------|---------|
| FR-1 | spec.md §功能规则 | `strcasecmp(temp->name, ...)` 仅在 `temp->name != nullptr` 时执行 |

**条件编译：** N/A（该函数无平台条件编译分支）

### 修改文件: `module_manager/test/unittest/module_manager_test/module_manager_test.cpp`

**变更原因：** 现有测试未覆盖链表中存在 `name==nullptr` 节点的场景。

**变更摘要：** 在文件末尾追加 `FindNativeModuleByCache_WhenNodeNameIsNullptrShouldNotCrash` 测试用例。参照既有 `FindNativeModuleByCache_*` 命名与写法（参照 L524-658）：
- 构造 `moduleManager`，手工设置 `headNativeModule_` 为一个 `new NativeModule()`（默认 name=nullptr, moduleName=nullptr）
- 在该节点后串一个 `moduleName="test.module.xyz"` 的节点（name 同样置 nullptr 以隔离变量）
- 调用 `FindNativeModuleByCache("test.module.xyz", ...)`，验证不崩溃且能找到第二个节点（覆盖 AC-1 + AC-2）
- 释放节点，清理 `headNativeModule_`

**关键行号：** 文件末尾追加（不修改既有行）

**测试命名规范：** `HWTEST_F(ModuleManagerTest, FindNativeModuleByCache_WhenNodeNameIsNullptrShouldNotCrash, TestSize.Level1)`，含 `@tc.name`/`@tc.desc`/`@tc.type`/`@tc.require` 注释块（参照既有用例）。

## BUILD.gn 变更

> N/A — 不改 BUILD.gn。`module_manager_test.cpp` 已在既有 test 目标 `//foundation/arkui/napi:napi_packages_test` 中。

## context-references

> 简单变更跳过。源码位置已在 spec.md 引用。

## 验证检查清单

- [ ] AC-1/AC-2/AC-3 全部有对应测试覆盖
- [ ] RED-GREEN-REFACTOR 执行（先证明测试有效，再修复）
- [ ] 构建通过（hb build napi -t 或对应单测目标）
- [ ] 未修改受影响文件清单之外的内容
- [ ] 完成证据已记录到 review.md

**完成证据：**

| 证据 | 命令/路径 | 结果 |
|------|-----------|------|
| RED 测试 | `hb build napi -t` + 运行 module_manager_test | 预期 CRASH/FAIL（待运行） |
| GREEN 测试 | 同上 | 预期 PASS（待运行） |
| 既有用例回归 | 同上 | 预期全 PASS（待运行） |

---

## TASK-2: 问题 #2 systemFilePath nullptr 判空加固 + 单元测试

| 字段 | 内容 |
|------|------|
| Task ID | TASK-2 |
| 标题 | FindNativeModuleByCache systemFilePath nullptr 判空加固 + 单元测试 |
| 关联 Feature | fix-find-cache-name-nullptr #2 |
| 优先级 | P1 |
| 复杂度 | 低 |
| 执行方式 | 主线程顺序执行（接续 TASK-1 之后） |

### 做什么（TDD RED-GREEN-REFACTOR）

1. **RED**：在 `module_manager_test.cpp` 末尾追加 `FindNativeModuleByCache_WhenNodeSystemFilePathIsNullptrShouldNotCrash`。构造一个通过 moduleName 匹配（让 if 外层成立）但 `systemFilePath==nullptr` 的节点，验证不崩溃且返回 nullptr。在未修复代码上运行 → 预期 CRASH（strcmp(nullptr, ...) 段错误）。
2. **GREEN**：修改 `native_module_manager.cpp` L1637 和 L1641，加 `temp->systemFilePath != nullptr` 判空。运行新测试 → 预期 PASS。
3. **REFACTOR**：最小变更，无重构空间。
4. **回归**：运行既有 FindNativeModuleByCache_* 用例 → 全 PASS。

### 不做什么

- 不动 L1460/1508 strdup 加固
- 不动字段初始化约束（创建路径）
- 不动 `.h:61` 默认值
- 不动其他已判空的使用点（L117/1581/1604）

### AC 映射

| AC | 来源 | 验证方式 |
|----|------|----------|
| AC-2.1 | spec.md §问题 #2 | 新测试第一段（nullptr 节点 + 匹配，验证返回 nullptr 不崩溃） |
| AC-2.2 | spec.md §问题 #2 | 新测试第二段（nullptr 节点 + 正常匹配节点，验证返回正常节点） |
| AC-2.3 | spec.md §问题 #2 | 既有用例运行 |

### 受影响文件

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| 修改 | `module_manager/native_module_manager.cpp` | L1637 + L1641 加 nullptr 判空 |
| 修改（追加用例） | `module_manager/test/unittest/module_manager_test/module_manager_test.cpp` | 末尾追加 `FindNativeModuleByCache_WhenNodeSystemFilePathIsNullptrShouldNotCrash` |

### 代码变更规格

#### 修改文件: `module_manager/native_module_manager.cpp`

**变更原因：** L1637/L1641 的 `strcmp(temp->systemFilePath, ...)` 未判 `temp->systemFilePath` 是否 nullptr，存在空指针解引用风险。

**变更摘要：** L1637 `strcmp` 前加 `temp->systemFilePath != nullptr &&`；L1641 `!strcmp` 改为 `(temp->systemFilePath != nullptr && !strcmp(temp->systemFilePath, ""))`。

**关键行号：** L1637 + L1641

**目标代码（修复后）：**

```cpp
#if !defined(ANDROID_PLATFORM) && !defined(IOS_PLATFORM)
while (label < NATIVE_PATH_NUMBER && temp->systemFilePath != nullptr
       && strcmp(temp->systemFilePath, nativeModulePath[label])) {
    label++;
}
#endif
if (label < NATIVE_PATH_NUMBER
    || (temp->systemFilePath != nullptr && !strcmp(temp->systemFilePath, ""))) {
```

**条件编译：** L1637 受 `#if !defined(ANDROID_PLATFORM) && !defined(IOS_PLATFORM)` 包裹；L1641 在 `#endif` 后（无条件）。修复保留此结构。

#### 修改文件: `module_manager/test/unittest/module_manager_test/module_manager_test.cpp`

**变更原因：** 现有测试未覆盖链表中存在 `systemFilePath==nullptr` 节点且该节点通过 moduleName 匹配的场景。

**变更摘要：** 末尾追加 `FindNativeModuleByCache_WhenNodeSystemFilePathIsNullptrShouldNotCrash`。设计：
- 节点 X：moduleName="test.module.syspath.nullptr.target"、name=nullptr（避免 L1634 干扰，让 if 外层只通过 moduleName 成立）、**systemFilePath=nullptr**（关键，触发 L1637/L1641）
- 第一段：链表只含 X，调用 FindNativeModuleByCache → 期望 nullptr（X 进入 if 内层但因 systemFilePath==nullptr 被判空短路，不崩溃）
- 第二段：链表含 X + Y（Y.moduleName 同样匹配、Y.systemFilePath=""），期望返回 Y（X 被跳过，Y 通过）
- 清理：delete 节点

**测试命名规范：** `HWTEST_F(ModuleManagerTest, FindNativeModuleByCache_WhenNodeSystemFilePathIsNullptrShouldNotCrash, TestSize.Level1)`，含 `@tc.name`/`@tc.desc`/`@tc.type`/`@tc.require` 注释块。

### BUILD.gn 变更

> N/A — 同 TASK-1。

### 验证检查清单（TASK-2）

- [ ] AC-2.1/AC-2.2/AC-2.3 全部有对应测试覆盖
- [ ] RED-GREEN-REFACTOR 执行
- [ ] 构建通过（待环境）
- [ ] 未修改受影响文件清单之外的内容
- [ ] 完成证据已记录到 review.md

**完成证据（TASK-2）：**

| 证据 | 命令/路径 | 结果 |
|------|-----------|------|
| RED 测试 | 待运行（环境问题，同 TASK-1） | 待运行 |
| GREEN 测试 | 待运行 | 待运行 |
| 既有用例回归 | 待运行 | 待运行 |

---

## TASK-3: 问题 #3 EmplaceModuleBuffer 重复 key 释放旧值覆盖新值

| 字段 | 内容 |
|------|------|
| Task ID | TASK-3 |
| 标题 | EmplaceModuleBuffer 重复 key 内存泄漏修复 |
| 关联 Feature | fix-find-cache-name-nullptr #3 |
| 优先级 | P1 |
| 复杂度 | 低 |
| 执行方式 | 主线程顺序执行（接续 TASK-2 之后） |

### 做什么（TDD RED-GREEN-REFACTOR）

1. **RED**：在 `module_manager_test.cpp` 末尾追加 `EmplaceModuleBuffer_ShouldReleaseOldBufferOnDuplicateKey`。同一 moduleKey 两次调用，第二次后 EXPECT_EQ(size, 1u) + EXPECT_EQ(map[key], lib2)。在未修复代码上运行 → 预期 FAIL（第二次 emplace 失败，map[key] 仍是 lib1，size 仍是 1，但 lib2 泄漏；EXPECT_EQ(map[key], lib2) 失败）。
2. **GREEN**：修改 `native_module_manager.cpp:207-214`，按用户建议加 find + 覆盖逻辑。运行新测试 → 预期 PASS。
3. **REFACTOR**：最小变更，无重构空间。
4. **回归**：运行既有用例 → 全 PASS。

### 不做什么

- 不动 EmplaceModuleLib (L173-180)
- 不动 RemoveModuleBuffer (L216-227) / RemoveModuleLib (L182-194)
- 不改 map 类型为 RAII
- 不动 GetFileBuffer 上游并发逻辑

### AC 映射

| AC | 来源 | 验证方式 |
|----|------|----------|
| AC-3.1 | spec.md §问题 #3 | 新测试第二次调用后 EXPECT_EQ(size, 1u) + EXPECT_EQ(map[key], lib2) |
| AC-3.2 | spec.md §问题 #3 | 代码审查 + ASan（运行时由 CI 验证） |
| AC-3.3 | spec.md §问题 #3 | 新测试第一次调用后 EXPECT_EQ(size, 1u) + EXPECT_EQ(map[key], lib1) |
| AC-3.4 | spec.md §问题 #3 | 既有用例运行 |

### 受影响文件

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| 修改 | `module_manager/native_module_manager.cpp` | L207-214 EmplaceModuleBuffer 加 find + 覆盖 |
| 修改（追加用例） | `module_manager/test/unittest/module_manager_test/module_manager_test.cpp` | 末尾追加 `EmplaceModuleBuffer_ShouldReleaseOldBufferOnDuplicateKey` |

### 代码变更规格

#### 修改文件: `module_manager/native_module_manager.cpp`

**变更原因：** L211-213 的 `moduleBufMap_.emplace(moduleKey, lib)` 在重复 key 时失败，新 lib 既未入库也未释放，泄漏。

**变更摘要：** 在 emplace 前先 find；若已存在，`delete[]` 旧 buffer 并替换为新 lib；否则 emplace。

**关键行号：** L207-214（EmplaceModuleBuffer 函数体）

**目标代码（修复后，按用户建议）：**

```cpp
void NativeModuleManager::EmplaceModuleBuffer(const std::string moduleKey, const uint8_t* lib)
{
    MODULEMNG_HILOG_DEBUG("module:'%{public}s'", moduleKey.c_str());
    std::lock_guard<std::mutex> lock(moduleBufMutex_);
    if (lib == nullptr) {
        return;
    }
    auto it = moduleBufMap_.find(moduleKey);
    if (it != moduleBufMap_.end()) {
        delete[] it->second;
        it->second = lib;
    } else {
        moduleBufMap_.emplace(moduleKey, lib);
    }
}
```

**注意事项：**
- `delete[] it->second`：旧 buffer 是 `uint8_t[]`（来自 `std::make_unique<uint8_t[]>` + `release`，见 GetFileBuffer L1320），用 `delete[]` 匹配
- 锁内操作：find/replace/emplace 全程在 `moduleBufMutex_` 锁内，无并发风险
- HILOG 保留不变

#### 修改文件: `module_manager/test/unittest/module_manager_test/module_manager_test.cpp`

**变更原因：** 现有测试未覆盖重复 key 场景。

**变更摘要：** 末尾追加 `EmplaceModuleBuffer_ShouldReleaseOldBufferOnDuplicateKey`。设计：
- 通过 `moduleManager.EmplaceModuleBuffer(key, lib1)` 第一次入库
- 通过 `moduleManager.EmplaceModuleBuffer(key, lib2)` 第二次入库（重复 key）
- EXPECT_EQ(moduleManager.moduleBufMap_.size(), 1u) —— map 只有一个 entry
- EXPECT_EQ(moduleManager.moduleBufMap_[key], lib2) —— 覆盖语义
- 同时验证 AC-3.3：第一次后 EXPECT_EQ(size, 1u) + EXPECT_EQ(map[key], lib1)
- 清理：手动 delete[] lib2（lib1 已被修复代码 delete[]；map 析构不释放裸指针）。注意：测试结束时 moduleBufMap_[key] 仍指向 lib2，但 moduleManager 局部对象析构会清空 map（不 delete[]），所以需要手动 delete[] lib2 避免测试自身泄漏

**测试命名规范：** `HWTEST_F(ModuleManagerTest, EmplaceModuleBuffer_ShouldReleaseOldBufferOnDuplicateKey, TestSize.Level1)`，含 4 行 `@tc.*` 注释块。

### BUILD.gn 变更

> N/A — 同 TASK-1/TASK-2。

### 验证检查清单（TASK-3）

- [ ] AC-3.1/3.2/3.3/3.4 全部有对应测试覆盖
- [ ] RED-GREEN-REFACTOR 执行（待环境）
- [ ] 构建通过（待环境）
- [ ] 未修改受影响文件清单之外的内容
- [ ] 完成证据已记录到 review.md

**完成证据（TASK-3）：**

| 证据 | 命令/路径 | 结果 |
|------|-----------|------|
| RED 测试 | 待运行（环境问题，同 TASK-1） | 待运行 |
| GREEN 测试 | 待运行 | 待运行 |
| 既有用例回归 | 待运行 | 待运行 |
| ASan（验证 AC-3.2 旧值释放） | 待 CI 运行 | 待运行 |

---

## TASK-4: 问题 #4 GetNativeModulePath operator[] 默认插入 nullptr 加固

| 字段 | 内容 |
|------|------|
| Task ID | TASK-4 |
| 标题 | GetNativeModulePath operator[] 默认插入 nullptr 加固 + 单元测试 |
| 关联 Feature | fix-find-cache-name-nullptr #4 |
| 优先级 | P1 |
| 复杂度 | 低 |
| 执行方式 | 主线程顺序执行（接续 TASK-3 之后） |

### 做什么（TDD RED-GREEN-REFACTOR）

1. **RED**：在 `module_manager_test.cpp` 末尾追加 `GetNativeModulePath_WhenAppLibPathValueIsNullptrShouldNotCrash`。三段断言：(a) 注入 `key→nullptr` 节点，调用 `GetNativeModulePath`，验证不崩溃 + map size 不变 + 走 fallback；(b) 空 map + 不存在 key 调用，验证 map size 不变（未被 operator[] 污染）；(c) 注入非空值节点，验证进入 if 内层。在未修复代码上运行 → 预期 (a) CRASH（sprintf_s 解引用 nullptr）。
2. **GREEN**：修改 `native_module_manager.cpp:1057-1095`，按用户建议：
   - 用 `if (isAppModule) { lock_guard + find + 赋值 }` 替换原外层 `if (isAppModule && IsExistedPath(path)) { lock + operator[] + unlock }`
   - 把原 `#ifdef ANDROID_PLATFORM ... #endif` 内层块包到新的 `if (prefix != nullptr)` 中
   - 原 `else` fallback 块原样保留
   运行新测试 → 预期 PASS。
3. **REFACTOR**：审视改动，原 `#ifdef ANDROID_PLATFORM` 块内的 `if (prefix == nullptr) return false;` 已成死代码（外层 `if (prefix != nullptr)` 已保证），但按最小变更原则不删。
4. **回归**：运行既有用例 → 全 PASS。

### 不做什么

- 不修改 `IsExistedPath` 实现（L1676-1681）
- 不修改其他 `operator[]` 使用点（L1109 IOS_PLATFORM 分支 `appLibPathMap_["default"]` 等）
- 不修改其他 `IsExistedPath` 调用点（L301/885/924/1126/1178/1201/1286/1404/1414）
- 不删除原 ANDROID_PLATFORM 块内死代码（`if (prefix == nullptr) return false;`）
- 不添加用户建议的代码审查清单项（超出代码修复范围）
- 不改 BUILD.gn
- 不改 `.codespec/` 之外的任何文档

### AC 映射

| AC | 来源 | 验证方式 |
|----|------|----------|
| AC-4.1 | spec.md §问题 #4 | 新测试第一段（注入 nullptr 值节点，断言不崩溃 + map size 不变 + 走 fallback） |
| AC-4.2 | spec.md §问题 #4 | 新测试第二段（空 map，断言调用后 size 不变） |
| AC-4.3 | spec.md §问题 #4 | 新测试第三段（注入非空值节点，断言进入 if 内层） |
| AC-4.4 | spec.md §问题 #4 | 既有用例运行 |

### 受影响文件

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| 修改 | `module_manager/native_module_manager.cpp` | L1057-1095 起始 if/else 块重构（find + lock_guard + nullptr fallback） |
| 修改（追加用例） | `module_manager/test/unittest/module_manager_test/module_manager_test.cpp` | 末尾追加 `GetNativeModulePath_WhenAppLibPathValueIsNullptrShouldNotCrash` |

### 代码变更规格

#### 修改文件: `module_manager/native_module_manager.cpp`

**变更原因：** L1058-1061 用 `operator[]` 读取 map，键不存在时插入 nullptr 污染 map，且并发场景下 `IsExistedPath` 与 `operator[]` 不在同一临界区。修复后 prefix 可能为 nullptr 时被 `sprintf_s` 解引用崩溃。

**变更摘要：**
1. L1057-1061 外层 `if (isAppModule && IsExistedPath(path)) { lock + operator[] + unlock }` 替换为 `if (isAppModule) { lock_guard + find + 赋值 }`
2. 原 L1062-1094 `#ifdef ANDROID_PLATFORM ... #endif` 内层块包到新的 `if (prefix != nullptr) { ... }` 中
3. 原 L1095-1113 `else` fallback 块原样保留

**关键行号：** L1057-1095（GetNativeModulePath 起始 if/else 块）

**目标代码（修复后）：**

```cpp
const char* prefix = nullptr;
if (isAppModule) {
    std::lock_guard<std::mutex> guard(appLibPathMapMutex_);
    auto it = appLibPathMap_.find(path);
    if (it != appLibPathMap_.end()) {
        prefix = it->second;
    }
}
if (prefix != nullptr) {
#ifdef ANDROID_PLATFORM
    for (int32_t i = 0; i < lengthOfModuleName; i++) {
        dupModuleName[i] = tolower(dupModuleName[i]);
    }
    if (prefix == nullptr) {
        return false;
    }
    /*
     * The requirement is to ensure the abc format file is placed in the path
     * ...（原注释保留不变）...
     */
    std::string prefixStr(prefix);
    /* ... 原 arkui-x 逻辑保留不变 ... */
#endif
} else {
    if (relativePath[0]) {
        if (previewSearchPath_.empty()) {
            sysPrefix = sysPrefix + "/" + relativePath;
        } else {
            sysPrefix = previewSearchPath_ + "/module";
        }
    }
    prefix = sysPrefix.c_str();
    for (int32_t i = 0; i < lengthOfModuleName; i++) {
        dupModuleName[i] = tolower(dupModuleName[i]);
    }
#ifdef IOS_PLATFORM
    if (appLibPathMap_.find("default") != appLibPathMap_.end()) {
        std::string sandboxPath(appLibPathMap_["default"]);
        sysAbcPrefix = sandboxPath + "/systemres/abc";
    }
#endif
}
```

**注意事项：**
- 原 `#ifdef ANDROID_PLATFORM` 块内的 `if (prefix == nullptr) return false;` 在新结构下为死代码（外层 `if (prefix != nullptr)` 已保证）。按最小变更原则保留不删，避免引入风险或语义偏移。
- 原 `else` 块（含 `#ifdef IOS_PLATFORM` 内的 `appLibPathMap_["default"]`）**完全不变**——属其他使用点，按用户规则不在本问题范围。
- `IsExistedPath` 在 L1058 的冗余调用被去除（合并到 find），但 `IsExistedPath` 实现本身不动。

**条件编译：** 修复保留原 `#ifdef ANDROID_PLATFORM` / `#ifdef IOS_PLATFORM` 结构不变。

#### 修改文件: `module_manager/test/unittest/module_manager_test/module_manager_test.cpp`

**变更原因：** 现有测试未覆盖 `appLibPathMap_` 中存在 nullptr 值节点 / 不存在 key / 非空值节点 三种场景下 `GetNativeModulePath` 的行为。

**变更摘要：** 末尾追加 `GetNativeModulePath_WhenAppLibPathValueIsNullptrShouldNotCrash`。设计（三段）：
- **段 1（AC-4.1）**：构造 `moduleManager`，向 `appLibPathMap_` 插入 `{"test.app.path.nullptr" → nullptr}`。记录 map size。调用 `moduleManager.GetNativeModulePath("somemodule", "test.app.path.nullptr", "", nativeModulePath, NAPI_PATH_MAX, isAppModule=true)`。断言：返回 true（fallback 分支也会填充 nativeModulePath[0]）；`appLibPathMap_.size()` 调用前后不变（未被 operator[] 污染）；不崩溃。
- **段 2（AC-4.2）**：清空 `appLibPathMap_`。记录 size（0）。调用 `GetNativeModulePath(..., "nonexistent.key", ..., isAppModule=true)`。断言：返回 true（走 fallback）；调用后 size 仍为 0（未被 operator[] 污染）。
- **段 3（AC-4.3）**：向 `appLibPathMap_` 插入 `{"test.app.path.valid" → "/some/valid/prefix"}`。调用 `GetNativeModulePath(..., "test.app.path.valid", ..., isAppModule=true)`。断言：返回 true；`nativeModulePath[0]` 以 `/some/valid/prefix/` 开头（验证进入 if 内层，prefix 取自 map）。
- 清理：清空 `appLibPathMap_`

**测试命名规范：** `HWTEST_F(ModuleManagerTest, GetNativeModulePath_WhenAppLibPathValueIsNullptrShouldNotCrash, TestSize.Level1)`，含 `@tc.name`/`@tc.desc`/`@tc.type`/`@tc.require` 注释块。

**注意事项：**
- `GetNativeModulePath` 是 private，通过 mock 头的 `#define private public` 暴露（既有机制）
- `appLibPathMap_` 是 `std::unordered_map<std::string, const char*>`（或类似），直接通过暴露的 private 成员注入
- `isAppModule` 是入参，无需 mock 内部状态
- 测试用例需包含 `<string>` 头以构造 std::string，但既有测试文件已包含

### BUILD.gn 变更

> N/A — 同 TASK-1/TASK-2/TASK-3。

### 验证检查清单（TASK-4）

- [ ] AC-4.1/4.2/4.3/4.4 全部有对应测试覆盖
- [ ] RED-GREEN-REFACTOR 执行（待环境）
- [ ] 构建通过（待环境）
- [ ] 未修改受影响文件清单之外的内容
- [ ] 完成证据已记录到 review.md

**完成证据（TASK-4）：**

| 证据 | 命令/路径 | 结果 |
|------|-----------|------|
| RED 测试 | 待运行（环境问题，同 TASK-1） | 待运行 |
| GREEN 测试 | 待运行 | 待运行 |
| 既有用例回归 | 待运行 | 待运行 |

---

## TASK-5: 问题 #5 SetAppLibPath 空字符串 back() UB 加固 + 单元测试

| 字段 | 内容 |
|------|------|
| Task ID | TASK-5 |
| 标题 | SetAppLibPath 空字符串 back() UB 加固 + 单元测试 |
| 关联 Feature | fix-find-cache-name-nullptr #5 |
| 优先级 | P1 |
| 复杂度 | 低 |
| 执行方式 | 主线程顺序执行（接续 TASK-4 之后） |

### 做什么（TDD RED-GREEN-REFACTOR）

1. **RED**：在 `module_manager_test.cpp` 末尾追加 `SetAppLibPath_WhenAppLibPathIsEmptyShouldNotCrash`，3 段断言：(a) 空 vector 调用 `SetAppLibPath`，验证不崩溃；(b) 全空字符串 vector 调用，验证不崩溃；(c) 非空元素 vector 调用，验证 `appLibPathMap_[moduleName]` 入库值正确（拼接 + `pop_back` 去末尾冒号）。在未修复代码上运行 → 预期 (a)/(b) CRASH（`tmpPath.back()` 解引用空串）。
2. **GREEN**：修改 `native_module_manager.cpp:758`，把 `if (tmpPath.back() == ':')` 改为 `if (!tmpPath.empty() && tmpPath.back() == ':')`。运行新测试 → 预期 PASS。
3. **REFACTOR**：审视改动，无进一步重构空间（最小变更，1 行）。
4. **回归**：运行既有用例 → 全 PASS。

### 不做什么

- 不加函数入口校验（`if (appLibPath.empty()) return;`）
- 不修改其他 `back()/front()` 使用点
- 不修改 L762/769/770 等其他潜在 UB / operator[] 使用点
- 不修改 `tmpPath.c_str()` 空串 strdup 下游行为
- 不加用户建议的代码审查清单项
- 不改 BUILD.gn
- 不改 `.codespec/` 之外的任何文档

### AC 映射

| AC | 来源 | 验证方式 |
|----|------|----------|
| AC-5.1 | spec.md §问题 #5 | 新测试第一段（空 vector，验证不崩溃） |
| AC-5.2 | spec.md §问题 #5 | 新测试第二段（全空字符串 vector，验证不崩溃） |
| AC-5.3 | spec.md §问题 #5 | 新测试第三段（非空元素，验证 `appLibPathMap_[moduleName]` 入库值正确） |
| AC-5.4 | spec.md §问题 #5 | 既有用例运行 |

### 受影响文件

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| 修改 | `module_manager/native_module_manager.cpp` | L758 加 `!tmpPath.empty() &&` 短路判空 |
| 修改（追加用例） | `module_manager/test/unittest/module_manager_test/module_manager_test.cpp` | 末尾追加 `SetAppLibPath_WhenAppLibPathIsEmptyShouldNotCrash` |

### 代码变更规格

#### 修改文件: `module_manager/native_module_manager.cpp`

**变更原因：** L758 `tmpPath.back()` 在 `tmpPath` 为空串时（`appLibPath` 空 vector 或全空元素）是 UB，多数实现解引用越界指针崩溃。

**变更摘要：** L758 `if (tmpPath.back() == ':')` 改为 `if (!tmpPath.empty() && tmpPath.back() == ':')`。

**关键行号：** L758（`SetAppLibPath` 内 `tmpPath.back()` 调用）

**目标代码（修复后）：**

```cpp
std::string tmpPath = "";
for (size_t i = 0; i < appLibPath.size(); i++) {
    if (appLibPath[i].empty()) {
        continue;
    }
    tmpPath += appLibPath[i];
    tmpPath += ":";
}
if (!tmpPath.empty() && tmpPath.back() == ':') {
    tmpPath.pop_back();
}
```

**设计引用：**

| 设计要素 | 引用位置 | 关键结论 |
|---------|----------|---------|
| FR-5 | spec.md §功能规则 | `tmpPath.back()` 前先判 `!tmpPath.empty()` 短路 |

**条件编译：** N/A（该函数无平台条件编译分支）

#### 修改文件: `module_manager/test/unittest/module_manager_test/module_manager_test.cpp`

**变更原因：** 现有测试未覆盖 `appLibPath` 为空 vector 或全空字符串元素的场景。

**变更摘要：** 末尾追加 `SetAppLibPath_WhenAppLibPathIsEmptyShouldNotCrash`。设计（3 段）：
- **段 1（AC-5.1）**：构造 `moduleManager`，调用 `moduleManager.SetAppLibPath("test.module.empty_vector", {}, false)`。断言：不崩溃（运行到此行即为通过）。
- **段 2（AC-5.2）**：调用 `moduleManager.SetAppLibPath("test.module.all_empty_strings", {"", "", ""}, false)`。断言：不崩溃。
- **段 3（AC-5.3）**：调用 `moduleManager.SetAppLibPath("test.module.normal", {"/path1", "/path2"}, false)`。断言：`appLibPathMap_["test.module.normal"]` 不为 nullptr，且字符串值等于 "/path1:/path2"（拼接 + `pop_back` 去末尾冒号）。
- 清理：清空 `appLibPathMap_`（释放 strdup 的 char* 后 clear）

**测试命名规范：** `HWTEST_F(ModuleManagerTest, SetAppLibPath_WhenAppLibPathIsEmptyShouldNotCrash, TestSize.Level1)`，含 `@tc.name`/`@tc.desc`/`@tc.type`/`@tc.require` 注释块。

**注意事项：**
- `SetAppLibPath` 是 public 接口（header L140 附近），可直接调用
- `appLibPathMap_` 是 private（`std::map<std::string, char*>`），通过 mock 头 `#define private public` 暴露
- 段 3 验证入库值时，`appLibPathMap_[key]` 返回 `char*`，比较 `std::string(appLibPathMap_[key]) == "/path1:/path2"`
- 清理：每个 moduleName 对应的 `char*` 是 `strdup` 分配的（堆），需 `free()` 释放后再 `clear()`，避免测试自身泄漏

### BUILD.gn 变更

> N/A — 同 TASK-1/TASK-2/TASK-3/TASK-4。

### 验证检查清单（TASK-5）

- [ ] AC-5.1/5.2/5.3/5.4 全部有对应测试覆盖
- [ ] RED-GREEN-REFACTOR 执行（待环境）
- [ ] 构建通过（待环境）
- [ ] 未修改受影响文件清单之外的内容
- [ ] 完成证据已记录到 review.md

**完成证据（TASK-5）：**

| 证据 | 命令/路径 | 结果 |
|------|-----------|------|
| RED 测试 | 待运行（环境问题，同 TASK-1） | 待运行 |
| GREEN 测试 | 待运行 | 待运行 |
| 既有用例回归 | 待运行 | 待运行 |

---

## TASK-6: 问题 #6 LoadNativeModule relativePath 路径穿越校验加固 + 单元测试

| 字段 | 内容 |
|------|------|
| Task ID | TASK-6 |
| 标题 | LoadNativeModule relativePath 路径穿越校验加固（IsSafeRelativePath helper）+ 单元测试 |
| 关联 Feature | fix-find-cache-name-nullptr #6 |
| 优先级 | P1 |
| 复杂度 | 低-中（含一个新私有静态成员函数 + 调用点替换） |
| 执行方式 | 主线程顺序执行（接续 TASK-5 之后） |

### 做什么（TDD RED-GREEN-REFACTOR）

1. **RED**：在 `module_manager_test.cpp` 末尾追加 `IsSafeRelativePath_PathValidationTest`。直接调用 `NativeModuleManager::IsSafeRelativePath(...)`（mock 头 `#define private public` 已暴露），覆盖 6 类场景：(a) ".." 子串非分段（"my..pkg"、"a..b/c"）→ true；(b) 绝对路径（"/etc/passwd"）→ false；(c) `.`/`..`/空 分段（"a/./b"、"a/../b"、"a//b"）→ false；(d) 非白名单字符（"a;b"、"a$b"、"%2e"）→ false；(e) 合法路径（"module/sub"、"a.b.c"、"" 空串）→ true；(f) 单段合法路径（"module"）→ true。在未修复代码上运行 → 编译失败（`IsSafeRelativePath` 未定义），证明测试依赖新代码。
2. **GREEN**：
   - 在 `native_module_manager.h` 私有段（`IsExistedPath` 后）新增 `static bool IsSafeRelativePath(const std::string& p);` 声明
   - 在 `native_module_manager.cpp` 实现 `NativeModuleManager::IsSafeRelativePath`（按用户建议：empty→true；前导 `/`→false；分段空/`.`/`..`→false；字符白名单 alnum/_/./`/`）
   - 替换 `LoadNativeModule` L862-867 的 `find("..")` 检查为 `!IsSafeRelativePath(relativePathStr)` 调用
   - 必要时新增 `#include <cctype>`（用于 `std::isalnum`）
   运行新测试 → 预期 PASS。
3. **REFACTOR**：审视改动，无进一步重构空间（helper 简洁，调用点最小）。
4. **回归**：运行既有用例 → 全 PASS。

### 不做什么

- 不加 realpath + 前缀校验（用户参考代码未含）
- 不加 URL 解码 / 符号链接检测
- 不修改其他路径校验点（moduleName/path 入参校验、GetNativeModulePath 拼接等）
- 不修改 `LoadNativeModule` 其他校验逻辑（L848-860 nullptr 检查等）
- 不加用户建议的代码审查清单项
- 不改 BUILD.gn
- 不改 `.codespec/` 之外的任何文档

### AC 映射

| AC | 来源 | 验证方式 |
|----|------|----------|
| AC-6.1 | spec.md §问题 #6 | 新测试 (a) 类断言（"my..pkg" 等返回 true） |
| AC-6.2 | spec.md §问题 #6 | 新测试 (b) 类断言（"/etc/passwd" 返回 false） |
| AC-6.3 | spec.md §问题 #6 | 新测试 (c) 类断言（"a/./b" 等返回 false） |
| AC-6.4 | spec.md §问题 #6 | 新测试 (d) 类断言（"a;b" 等返回 false） |
| AC-6.5 | spec.md §问题 #6 | 新测试 (e)(f) 类断言（合法路径返回 true） |
| AC-6.6 | spec.md §问题 #6 | 既有用例运行 |

### 受影响文件

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| 修改 | `module_manager/native_module_manager.h` | 私有段（L167 `IsExistedPath` 后）新增 `static bool IsSafeRelativePath(const std::string& p);` 声明 |
| 修改 | `module_manager/native_module_manager.cpp` | (a) 实现 `NativeModuleManager::IsSafeRelativePath`；(b) L862-867 调用点替换；(c) 必要时加 `#include <cctype>` |
| 修改（追加用例） | `module_manager/test/unittest/module_manager_test/module_manager_test.cpp` | 末尾追加 `IsSafeRelativePath_PathValidationTest` |

### 代码变更规格

#### 修改文件: `module_manager/native_module_manager.h`

**变更原因：** 新增 `IsSafeRelativePath` 私有静态成员函数声明，供 `LoadNativeModule` 调用与单测直接验证。

**变更摘要：** 私有段 L167 `IsExistedPath` 声明后追加一行 `static bool IsSafeRelativePath(const std::string& p);`。

**关键行号：** L167 后插入（即 L168 新增一行）

**目标代码（修复后）：**

```cpp
bool IsExistedPath(const char* pathKey) const;
static bool IsSafeRelativePath(const std::string& p);
```

#### 修改文件: `module_manager/native_module_manager.cpp`

**变更原因 (1)：** 实现 `IsSafeRelativePath` 助手函数，提供规范化白名单校验。

**变更摘要 (1)：** 在 `IsExistedPath` 实现之后（L1681 后）追加 `NativeModuleManager::IsSafeRelativePath` 实现。

**目标代码（实现）：**

```cpp
bool NativeModuleManager::IsSafeRelativePath(const std::string& p)
{
    if (p.empty()) {
        return true;
    }
    if (p.front() == '/') {
        return false;
    }
    std::stringstream ss(p);
    std::string seg;
    while (std::getline(ss, seg, '/')) {
        if (seg.empty() || seg == "." || seg == "..") {
            return false;
        }
        for (char c : seg) {
            if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '.') {
                return false;
            }
        }
    }
    return true;
}
```

**变更原因 (2)：** `LoadNativeModule` L862-867 用 `find("..")` 子串匹配，既误报（"my..pkg"）又漏报（"/etc/passwd" 等）。

**变更摘要 (2)：** L862-867 替换为：

**目标代码（调用点）：**

```cpp
std::string relativePathStr(relativePath);
if (!NativeModuleManager::IsSafeRelativePath(relativePathStr)) {
    SetLoadErrInfo(loadErrInfo, "invalid relativePath");
    MODULEMNG_HILOG_ERROR("invalid relativePath: rejected by whitelist");
    return nullptr;
}
```

**变更原因 (3)：** `std::isalnum` 需要 `<cctype>` 头。当前 `.cpp` 未显式包含，需补充以保证可移植性。

**变更摘要 (3)：** 在 `#include <cerrno>` 等系列头文件区段（L18 附近）追加 `#include <cctype>`。

**条件编译：** N/A（该函数无平台条件编译分支）

#### 修改文件: `module_manager/test/unittest/module_manager_test/module_manager_test.cpp`

**变更原因：** 现有测试未覆盖 `IsSafeRelativePath` 各类场景。

**变更摘要：** 末尾追加 `IsSafeRelativePath_PathValidationTest`。设计（按 AC 6 类场景）：

```cpp
// AC-6.1: ".." as substring but not as full segment -> accept
EXPECT_TRUE(NativeModuleManager::IsSafeRelativePath("my..pkg"));
EXPECT_TRUE(NativeModuleManager::IsSafeRelativePath("a..b/c"));

// AC-6.2: absolute path -> reject
EXPECT_FALSE(NativeModuleManager::IsSafeRelativePath("/etc/passwd"));
EXPECT_FALSE(NativeModuleManager::IsSafeRelativePath("/"));

// AC-6.3: segment is . / .. / empty -> reject
EXPECT_FALSE(NativeModuleManager::IsSafeRelativePath("a/./b"));
EXPECT_FALSE(NativeModuleManager::IsSafeRelativePath("a/../b"));
EXPECT_FALSE(NativeModuleManager::IsSafeRelativePath("a//b"));
EXPECT_FALSE(NativeModuleManager::IsSafeRelativePath("."));
EXPECT_FALSE(NativeModuleManager::IsSafeRelativePath(".."));

// AC-6.4: non-whitelisted char -> reject
EXPECT_FALSE(NativeModuleManager::IsSafeRelativePath("a;b"));
EXPECT_FALSE(NativeModuleManager::IsSafeRelativePath("a$b"));
EXPECT_FALSE(NativeModuleManager::IsSafeRelativePath("%2e"));
EXPECT_FALSE(NativeModuleManager::IsSafeRelativePath("a\\b"));   // backslash

// AC-6.5: legitimate relative path -> accept
EXPECT_TRUE(NativeModuleManager::IsSafeRelativePath(""));
EXPECT_TRUE(NativeModuleManager::IsSafeRelativePath("module"));
EXPECT_TRUE(NativeModuleManager::IsSafeRelativePath("module/sub"));
EXPECT_TRUE(NativeModuleManager::IsSafeRelativePath("a.b.c"));
EXPECT_TRUE(NativeModuleManager::IsSafeRelativePath("com.example.foo"));
```

**测试命名规范：** `HWTEST_F(ModuleManagerTest, IsSafeRelativePath_PathValidationTest, TestSize.Level1)`，含 `@tc.name`/`@tc.desc`/`@tc.type`/`@tc.require` 注释块。

**注意事项：**
- `IsSafeRelativePath` 是 `static` 成员函数，可直接通过 `NativeModuleManager::IsSafeRelativePath(...)` 调用，无需实例化
- mock 头的 `#define private public` 暴露私有段，static 成员亦可见
- 测试无副作用（纯函数），无需 SetUp/TearDown

### BUILD.gn 变更

> N/A — 同 TASK-1-#5。

### 验证检查清单（TASK-6）

- [ ] AC-6.1/6.2/6.3/6.4/6.5/6.6 全部有对应测试覆盖
- [ ] RED-GREEN-REFACTOR 执行（待环境）
- [ ] 构建通过（待环境）
- [ ] 未修改受影响文件清单之外的内容
- [ ] 完成证据已记录到 review.md

**完成证据（TASK-6）：**

| 证据 | 命令/路径 | 结果 |
|------|-----------|------|
| RED 测试 | 待运行（环境问题，同 TASK-1） | 待运行 |
| GREEN 测试 | 待运行 | 待运行 |
| 既有用例回归 | 待运行 | 待运行 |

---

## TASK-7: 问题 #7 greylist.json lib 名严格字符集校验 + 单元测试

| 字段 | 内容 |
|------|------|
| Task ID | TASK-7 |
| 标题 | greylist.json lib 名严格字符集校验（IsValidLibNameStrict helper）+ 单元测试 |
| 关联 Feature | fix-find-cache-name-nullptr #7 |
| 优先级 | P1 |
| 复杂度 | 低-中（含一个新私有静态成员函数 + 调用点替换） |
| 执行方式 | 主线程顺序执行（接续 TASK-6 之后） |

### 做什么（TDD RED-GREEN-REFACTOR）

1. **RED**：在 `module_manager_test.cpp` 末尾追加 `IsValidLibNameStrict_LibNameValidationTest`。直接调用 `NativeModuleManager::IsValidLibNameStrict(...)`（mock 头已暴露），覆盖 7 类场景：(a) 含冒号（`liba:b.so`）→ false；(b) 含空格/斜杠/反斜杠/非白名单字符（`lib a.so`、`lib/a.so`、`lib\\a.so`、`lib$a.so`）→ false；(c) 含 `...`（`lib...so`、`a...b.so`）→ false；(d) 不以 `.so` 结尾（`libfoo`、`libfoo.txt`）→ false；(e) 空串/超长（≥ NAME_MAX）→ false；(f) 合法名（`libfoo.so`、`lib_foo-bar.so`、`lib123.so`）→ true。在未修复代码上运行 → 编译失败（`IsValidLibNameStrict` 未定义），证明测试依赖新代码。
2. **GREEN**：
   - 在 `native_module_manager.h` 私有段（`IsSafeRelativePath` 后）新增 `static bool IsValidLibNameStrict(const std::string& libName);` 声明
   - 在 `native_module_manager.cpp` 实现 `NativeModuleManager::IsValidLibNameStrict`（按用户建议：empty/size≥NAME_MAX→false；不以 `.so` 结尾→false；字符白名单 alnum/_/-/.；含 `...`→false）
   - 替换 `LoadGreylistConfig` L515 调用点：`IsValidLibName(libName)` → `NativeModuleManager::IsValidLibNameStrict(libName)`
   - 新增 `#include <climits>`（用于 `NAME_MAX`）
   运行新测试 → 预期 PASS。
3. **REFACTOR**：审视改动，无进一步重构空间（helper 简洁，调用点最小替换）。
4. **回归**：运行既有用例 → 全 PASS。

### 不做什么

- 不替换为 JSON 解析器（cJSON/nlohmann::json）（用户参考代码未含）
- 不加配置完整性校验（签名/哈希/目录权限）（用户参考代码未含）
- 不修改原 `IsValidLibName`（L442-456 static free function）（不在用户问题描述范围）
- 不修改 JSON 解析逻辑（L483-521 find/rfind/split）
- 不修改 `sharedLibsSonames_` 拼接逻辑（L598-617）
- 不修改 `dlns_inherit` 调用点（L679/687）
- 不加用户建议的代码审查清单项
- 不改 BUILD.gn
- 不改 `.codespec/` 之外的任何文档

### AC 映射

| AC | 来源 | 验证方式 |
|----|------|----------|
| AC-7.1 | spec.md §问题 #7 | 新测试 (a) 类断言（`liba:b.so` 等 → false） |
| AC-7.2 | spec.md §问题 #7 | 新测试 (b) 类断言（含空格/斜杠等 → false） |
| AC-7.3 | spec.md §问题 #7 | 新测试 (c) 类断言（`lib...so` 等 → false） |
| AC-7.4 | spec.md §问题 #7 | 新测试 (d) 类断言（不以 `.so` 结尾 → false） |
| AC-7.5 | spec.md §问题 #7 | 新测试 (e) 类断言（空/超长 → false） |
| AC-7.6 | spec.md §问题 #7 | 新测试 (f) 类断言（合法名 → true） |
| AC-7.7 | spec.md §问题 #7 | 既有用例运行 |

### 受影响文件

| 操作 | 文件路径 | 说明 |
|------|----------|------|
| 修改 | `module_manager/native_module_manager.h` | 私有段（L168 `IsSafeRelativePath` 后）新增 `static bool IsValidLibNameStrict(const std::string& libName);` 声明 |
| 修改 | `module_manager/native_module_manager.cpp` | (a) 实现 `NativeModuleManager::IsValidLibNameStrict`；(b) L515 调用点替换；(c) 加 `#include <climits>` |
| 修改（追加用例） | `module_manager/test/unittest/module_manager_test/module_manager_test.cpp` | 末尾追加 `IsValidLibNameStrict_LibNameValidationTest` |

### 代码变更规格

#### 修改文件: `module_manager/native_module_manager.h`

**变更原因：** 新增 `IsValidLibNameStrict` 私有静态成员函数声明。

**变更摘要：** 私有段 `IsSafeRelativePath` 声明后追加一行 `static bool IsValidLibNameStrict(const std::string& libName);`。

**目标代码（修复后）：**

```cpp
bool IsExistedPath(const char* pathKey) const;
static bool IsSafeRelativePath(const std::string& p);
static bool IsValidLibNameStrict(const std::string& libName);
```

#### 修改文件: `module_manager/native_module_manager.cpp`

**变更原因 (1)：** 实现 `IsValidLibNameStrict` 助手函数。

**变更摘要 (1)：** 在 `IsSafeRelativePath` 实现之后追加 `NativeModuleManager::IsValidLibNameStrict` 实现。

**目标代码（实现）：**

```cpp
bool NativeModuleManager::IsValidLibNameStrict(const std::string& libName)
{
    if (libName.empty() || libName.size() >= NAME_MAX) {
        return false;
    }
    if (libName.substr(libName.size() - 3) != ".so") {
        return false;
    }
    for (char c : libName) {
        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '.' && c != '_' && c != '-') {
            return false;
        }
    }
    return libName.find("...") == std::string::npos;
}
```

**变更原因 (2)：** `LoadGreylistConfig` L515 调用原 `IsValidLibName` 仅校验 `.so` 后缀，未限字符集。

**变更摘要 (2)：** L515 由 `IsValidLibName(libName)` 改为 `NativeModuleManager::IsValidLibNameStrict(libName)`。

**变更原因 (3)：** `NAME_MAX` 需要 `<climits>` 头。当前 `.cpp` 未包含。

**变更摘要 (3)：** 在 `#include <cctype>` 后追加 `#include <climits>`。

**注意事项：**
- 原 `IsValidLibName`（L442-456 static free function）保留不动。它将变成孤立死代码（L515 是其唯一调用点，已被替换）。按"每次仅修问题描述本身"规则不删
- 调用点替换不改 HILOG 文案，最小变更

#### 修改文件: `module_manager/test/unittest/module_manager_test/module_manager_test.cpp`

**变更原因：** 现有测试未覆盖 `IsValidLibNameStrict` 各类场景。

**变更摘要：** 末尾追加 `IsValidLibNameStrict_LibNameValidationTest`。设计（按 AC 7 类场景）：

```cpp
// AC-7.1: contains colon -> reject (namespace injection vector)
EXPECT_FALSE(NativeModuleManager::IsValidLibNameStrict("liba:b.so"));

// AC-7.2: contains space/slash/backslash/non-whitelist char -> reject
EXPECT_FALSE(NativeModuleManager::IsValidLibNameStrict("lib a.so"));
EXPECT_FALSE(NativeModuleManager::IsValidLibNameStrict("lib/a.so"));
EXPECT_FALSE(NativeModuleManager::IsValidLibNameStrict("lib\\a.so"));
EXPECT_FALSE(NativeModuleManager::IsValidLibNameStrict("lib$a.so"));
EXPECT_FALSE(NativeModuleManager::IsValidLibNameStrict("lib%a.so"));

// AC-7.3: contains "..." sequence -> reject
EXPECT_FALSE(NativeModuleManager::IsValidLibNameStrict("lib...so"));
EXPECT_FALSE(NativeModuleManager::IsValidLibNameStrict("a...b.so"));

// AC-7.4: does not end with .so -> reject
EXPECT_FALSE(NativeModuleManager::IsValidLibNameStrict("libfoo"));
EXPECT_FALSE(NativeModuleManager::IsValidLibNameStrict("libfoo.txt"));
EXPECT_FALSE(NativeModuleManager::IsValidLibNameStrict("lib.so.txt"));

// AC-7.5: empty or oversized -> reject
EXPECT_FALSE(NativeModuleManager::IsValidLibNameStrict(""));
EXPECT_FALSE(NativeModuleManager::IsValidLibNameStrict(std::string(NAME_MAX, 'a').replace(0, 3, "lib").append(".so")));

// AC-7.6: legitimate names -> accept
EXPECT_TRUE(NativeModuleManager::IsValidLibNameStrict("libfoo.so"));
EXPECT_TRUE(NativeModuleManager::IsValidLibNameStrict("lib_foo-bar.so"));
EXPECT_TRUE(NativeModuleManager::IsValidLibNameStrict("lib123.so"));
EXPECT_TRUE(NativeModuleManager::IsValidLibNameStrict("a.so"));
```

**测试命名规范：** `HWTEST_F(ModuleManagerTest, IsValidLibNameStrict_LibNameValidationTest, TestSize.Level1)`，含 `@tc.name`/`@tc.desc`/`@tc.type`/`@tc.require` 注释块。

**注意事项：**
- `IsValidLibNameStrict` 是 `static` 成员函数，直接通过 `NativeModuleManager::IsValidLibNameStrict(...)` 调用，无需实例化
- mock 头的 `#define private public` 暴露私有段
- 测试无副作用（纯函数），无需 SetUp/TearDown
- 超长字符串构造：`std::string(NAME_MAX, 'a')` 创造长度为 NAME_MAX 的全 'a' 字符串（≥ NAME_MAX 触发 false）

### BUILD.gn 变更

> N/A — 同 TASK-1-#6。

### 验证检查清单（TASK-7）

- [ ] AC-7.1/7.2/7.3/7.4/7.5/7.6/7.7 全部有对应测试覆盖
- [ ] RED-GREEN-REFACTOR 执行（待环境）
- [ ] 构建通过（待环境）
- [ ] 未修改受影响文件清单之外的内容
- [ ] 完成证据已记录到 review.md

**完成证据（TASK-7）：**

| 证据 | 命令/路径 | 结果 |
|------|-----------|------|
| RED 测试 | 待运行（环境问题，同 TASK-1） | 待运行 |
| GREEN 测试 | 待运行 | 待运行 |
| 既有用例回归 | 待运行 | 待运行 |

---

## TASK-8：UnloadNativeModule TOCTOU 竞态修复

### 范围

| 项 | 内容 |
|----|------|
| 关联 AC | AC-8.1 / AC-8.2 / AC-8.3 / AC-8.4 / AC-8.5 / AC-8.6 / AC-8.7 |
| 关联 FR | FR-8.a / FR-8.b / FR-8.c / FR-8.d |
| 修改文件 | `module_manager/native_module_manager.h` + `module_manager/native_module_manager.cpp` + `module_manager/test/unittest/module_manager_test/module_manager_test.cpp` |
| 不涉及 | 引用计数方案、CloseModuleLibrary、其他锁粒度、dlclose 失败处理、既有 4 个 Remove* 函数（保留不动） |

### 目标代码（TASK-8）

#### 1. `native_module_manager.h` 私有段新增 2 个声明

紧邻 L168-#169 既有的 `IsSafeRelativePath`/`IsValidLibNameStrict`，追加：

```cpp
    bool RemoveNativeModuleByCacheLocked(const std::string& moduleKey);
    bool RemoveNativeModuleLocked(const std::string& moduleKey);
```

#### 2. `native_module_manager.cpp` 重构 `UnloadNativeModule`（L261-276）

**原代码：**
```cpp
bool NativeModuleManager::UnloadNativeModule(const std::string& moduleKey)
{
    MODULEMNG_HILOG_DEBUG("module:'%{public}s'", moduleKey.c_str());
    LIBHANDLE handle = GetNativeModuleHandle(moduleKey);
    if (handle == nullptr) {
        MODULEMNG_HILOG_ERROR("failed");
        return false;
    }
    if (RemoveNativeModule(moduleKey) == false) {
        MODULEMNG_HILOG_ERROR("failed");
        return false;
    }
    return UnloadModuleLibrary(handle);
}
```

**目标代码：**
```cpp
bool NativeModuleManager::UnloadNativeModule(const std::string& moduleKey)
{
    MODULEMNG_HILOG_DEBUG("module:'%{public}s'", moduleKey.c_str());
    LIBHANDLE handle = nullptr;
    bool removed = false;
    {
        std::lock_guard<std::mutex> lk1(moduleLibMutex_);
        std::lock_guard<std::mutex> lk2(nativeModuleListMutex_);
        auto it = moduleLibMap_.find(moduleKey);
        if (it == moduleLibMap_.end()) {
            MODULEMNG_HILOG_ERROR("module '%{public}s' not found in lib map", moduleKey.c_str());
            return false;
        }
        handle = it->second;
        moduleLibMap_.erase(it);
        removed = RemoveNativeModuleLocked(moduleKey);
    }
    if (!removed) {
        MODULEMNG_HILOG_ERROR("remove native module failed for '%{public}s'", moduleKey.c_str());
        return false;
    }
    return UnloadModuleLibrary(handle);
}
```

#### 3. `native_module_manager.cpp` 新增 `RemoveNativeModuleLocked` 实现

紧邻 `RemoveNativeModule`（L250-259）后追加（约 L260 处插入）：

```cpp
bool NativeModuleManager::RemoveNativeModuleLocked(const std::string& moduleKey)
{
    // Caller holds moduleLibMutex_ and nativeModuleListMutex_
    // moduleLibMap_ entry already erased by caller (UnloadNativeModule)
    bool bufferRemoved = RemoveModuleBuffer(moduleKey);  // takes moduleBufMutex_ internally
    bool moduleRemoved = RemoveNativeModuleByCacheLocked(moduleKey);  // caller holds nativeModuleListMutex_
    MODULEMNG_HILOG_DEBUG("bufferRemoved is %{public}d, moduleRemoved is %{public}d",
        bufferRemoved, moduleRemoved);
    return moduleRemoved;
}
```

> 说明：lib 已擦除等价 `handleRemoved = true`，原 `RemoveNativeModule` 返回逻辑 `((handleRemoved || handleAbcRemoved) && moduleRemoved)` 简化为 `moduleRemoved`。

#### 4. `native_module_manager.cpp` 新增 `RemoveNativeModuleByCacheLocked` 实现

紧邻 `RemoveNativeModuleByCache`（L1574-1629）后追加。函数体与 `RemoveNativeModuleByCache` **完全相同**，仅**移除首行 `std::lock_guard<std::mutex> lock(nativeModuleListMutex_);`**（调用方持锁）。

### 测试设计（TASK-8）

**测试名：** `UnloadNativeModule_WhenModuleNotInLibMapShouldReturnFalseSafely`

**覆盖 AC：** AC-8.1（空 libMap 早退 false）+ AC-8.7（既有用例回归）

**断言：**
- `moduleLibMap_` 不含目标 key
- 调用 `UnloadNativeModule("test_unload_no_exist")` → 返回 false
- 无崩溃（验证 TOCTOU 加固后的"未找到早退"路径不需要 dlclose）

**测试骨架：**
```cpp
HWTEST_F(ModuleManagerTest, UnloadNativeModule_WhenModuleNotInLibMapShouldReturnFalseSafely, TestSize.Level1)
{
    /**
     * @tc.name: UnloadNativeModule_WhenModuleNotInLibMapShouldReturnFalseSafely
     * @tc.desc: 验证 UnloadNativeModule 在 moduleLibMap_ 不存在 key 时安全早退
     * @tc.type: FUNC
     * @tc.require: issue #2157 问题 #8
     */
    NativeModuleManager& mgr = NativeModuleManager::GetInstance();
    // moduleLibMap_ 默认为空（SetUp 不主动填充），调用应早退 false 不崩溃
    bool result = mgr.UnloadNativeModule("test_unload_no_exist_key");
    EXPECT_FALSE(result);
}
```

**注意事项：**
- 不主动向 `moduleLibMap_` 插入条目（避免真实 `dlclose` 悬空指针崩溃）
- 该测试验证的是"早退路径"，覆盖 AC-8.1 的双锁临界区 `find == end()` 分支
- 并发压力测试（AC-8.3）不在单测范围，留待 CI

### BUILD.gn 变更

> N/A — 同 TASK-1-#7。

### 验证检查清单（TASK-8）

- [ ] AC-8.1/8.2/8.3/8.4/8.5/8.6/8.7 全部有对应验证（单测 + 静态审查）
- [ ] RED-GREEN-REFACTOR 执行（待环境）
- [ ] 构建通过（待环境）
- [ ] 未修改受影响文件清单之外的内容
- [ ] 完成证据已记录到 review.md

**完成证据（TASK-8）：**

| 证据 | 命令/路径 | 结果 |
|------|-----------|------|
| RED 测试 | 待运行（环境问题，同 TASK-1） | 待运行 |
| GREEN 测试 | 待运行 | 待运行 |
| 既有用例回归 | 待运行 | 待运行 |
| 锁顺序静态审查 | grep 全仓 `moduleLibMutex_` + `nativeModuleListMutex_` 双锁嵌套 | 待审查 |

---

## TASK-9：EmplaceModuleLib 重复 key 句柄泄漏修复

### 范围

| 项 | 内容 |
|----|------|
| 关联 AC | AC-9.1 / AC-9.2 / AC-9.3 / AC-9.4 / AC-9.5 |
| 关联 FR | FR-9.a / FR-9.b / FR-9.c |
| 修改文件 | `module_manager/native_module_manager.cpp` + `module_manager/test/unittest/module_manager_test/module_manager_test.cpp` |
| 不涉及 | `insert_or_assign` 方案、其他 `emplace` 调用点、`LoadModuleLibrary` 调用方逻辑、dlclose 失败处理、锁外 dlclose 改造 |

### 目标代码（TASK-9）

#### `native_module_manager.cpp` 重构 `EmplaceModuleLib`（L175-182）

**原代码：**
```cpp
void NativeModuleManager::EmplaceModuleLib(std::string moduleKey, const LIBHANDLE lib)
{
    MODULEMNG_HILOG_DEBUG("module:'%{public}s'", moduleKey.c_str());
    std::lock_guard<std::mutex> lock(moduleLibMutex_);
    if (lib != nullptr) {
        moduleLibMap_.emplace(moduleKey, lib);
    }
}
```

**目标代码（按用户建议代码，与方案 A 一致）：**
```cpp
void NativeModuleManager::EmplaceModuleLib(std::string moduleKey, const LIBHANDLE lib)
{
    MODULEMNG_HILOG_DEBUG("module:'%{public}s'", moduleKey.c_str());
    std::lock_guard<std::mutex> lock(moduleLibMutex_);
    if (lib == nullptr) {
        return;
    }
    auto it = moduleLibMap_.find(moduleKey);
    if (it != moduleLibMap_.end()) {
        MODULEMNG_HILOG_WARN("module '%{public}s' already exists, discard duplicate handle", moduleKey.c_str());
        UnloadModuleLibrary(lib);  // discard duplicate handle, refcount -1
        return;
    }
    moduleLibMap_.emplace(moduleKey, lib);
}
```

### 测试设计（TASK-9）

**测试 1：** `EmplaceModuleLib_ShouldNotEmplaceWhenLibIsNullptr`

覆盖 AC-9.1（nullptr 早退路径）。

```cpp
HWTEST_F(ModuleManagerTest, EmplaceModuleLib_ShouldNotEmplaceWhenLibIsNullptr, TestSize.Level1)
{
    /**
     * @tc.name: EmplaceModuleLib_ShouldNotEmplaceWhenLibIsNullptr
     * @tc.desc: 验证 lib == nullptr 时早退，不触碰 map
     * @tc.type: FUNC
     * @tc.require: issue #2157 问题 #9
     */
    NativeModuleManager& mgr = NativeModuleManager::GetInstance();
    size_t before = mgr.moduleLibMap_.size();
    mgr.EmplaceModuleLib("test_emplace_nullptr_key", nullptr);
    EXPECT_EQ(mgr.moduleLibMap_.size(), before);
    EXPECT_EQ(mgr.moduleLibMap_.count("test_emplace_nullptr_key"), 0u);
}
```

**测试 2：** `EmplaceModuleLib_ShouldEmplaceWhenKeyNotExists`

覆盖 AC-9.2（首次入库路径）。注意使用 sentinel 指针（不触发 dlclose，因为首次入库不走 `UnloadModuleLibrary`）。

```cpp
HWTEST_F(ModuleManagerTest, EmplaceModuleLib_ShouldEmplaceWhenKeyNotExists, TestSize.Level1)
{
    /**
     * @tc.name: EmplaceModuleLib_ShouldEmplaceWhenKeyNotExists
     * @tc.desc: 验证 key 不存在时 emplace 入库
     * @tc.type: FUNC
     * @tc.require: issue #2157 问题 #9
     */
    NativeModuleManager& mgr = NativeModuleManager::GetInstance();
    LIBHANDLE sentinel = reinterpret_cast<LIBHANDLE>(0xDEADBEEF);
    mgr.EmplaceModuleLib("test_emplace_new_key", sentinel);
    EXPECT_EQ(mgr.moduleLibMap_.count("test_emplace_new_key"), 1u);
    EXPECT_EQ(mgr.moduleLibMap_["test_emplace_new_key"], sentinel);
    // 清理 sentinel，避免污染后续测试（不是真实 dlopen 句柄，不能 dlclose）
    mgr.moduleLibMap_.erase("test_emplace_new_key");
}
```

**注意事项：**
- 测试 1 直接覆盖早退路径，map 状态不变
- 测试 2 用 sentinel 指针（reinterpret_cast<LIBHANDLE>(0xDEADBEEF)）模拟 dlopen 句柄，验证首次入库 map 状态正确；测试结束手动 erase 清理（避免后续测试触发 dlclose sentinel 崩溃）
- 重复 key 路径（AC-9.3/9.4）不在单测覆盖，原因：会触发真实 `dlclose`，单测无法安全模拟

### BUILD.gn 变更

> N/A — 同 TASK-1-#8。

### 验证检查清单（TASK-9）

- [ ] AC-9.1/9.2/9.3/9.4/9.5 全部有对应验证（单测 + 静态审查）
- [ ] RED-GREEN-REFACTOR 执行（待环境）
- [ ] 构建通过（待环境）
- [ ] 未修改受影响文件清单之外的内容
- [ ] 完成证据已记录到 review.md

**完成证据（TASK-9）：**

| 证据 | 命令/路径 | 结果 |
|------|-----------|------|
| RED 测试 | 待运行（环境问题，同 TASK-1） | 待运行 |
| GREEN 测试 | 待运行 | 待运行 |
| 既有用例回归 | 待运行 | 待运行 |
| 重复路径静态审查 | `native_module_manager.cpp` EmplaceModuleLib L175+ | 待审查 |
