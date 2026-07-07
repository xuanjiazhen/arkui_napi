# Stage 3 (Implement) Gate 检查结果

> 阶段：实现 → 发布
> 复杂度：简单
> Profile：none
> 日期：2026-07-06

## 入口检查

- [x] 当前 git 仓、分支、目标远端和允许修改文件清单已记录（见 task.md §工作区边界）
  - 仓：foundation/arkui/napi（远程 gitcode: openharmony/arkui_napi）
  - base 分支：master（已 reset 到 gitcode/master 3119c6c6）
  - 工作分支：fix/find-cache-name-nullptr（待创建）
  - 允许修改文件：`module_manager/native_module_manager.cpp` + `module_manager/test/unittest/module_manager_test/module_manager_test.cpp`
- [x] 工作区状态已检查：干净，仅 `.codespec/` 未跟踪（SDD 产物，保留，无脏改冲突）
- [x] 多仓变更：N/A（单仓）
- [x] 生成文件清单：N/A（无 bridge/IDL/CAPI/cpptoc/ctocpp 等生成器产物）
- [x] Plan 已审批（用户 2026-07-06 批准 task.md，批准发生在生产代码修改之前）

### Profile 入口 Gate

> Profile = none，跳过。

## 执行计划检查（简单变更裁剪）

> 简单变更跳过完整 execution-plan.md，task.md 即 Plan。

- [x] AC 到 Task 有完整追溯（AC-1/AC-2/AC-3 → TASK-1，验证方式明确）
- [x] Task 文件范围明确（2 个文件，行号定位精确）
- [x] Task 不做范围明确（L998-999、L1546-1553、其他 str*、入参保护、BUILD.gn 均排除）
- [x] Task 粒度合理（单 Task 形成独立可验证能力闭环：RED-GREEN-REFACTOR）
- [x] 交接信息完整（工作区边界、分支策略、停止条件齐全）

## Task 实现检查

- [x] RED 阶段：在 `module_manager_test.cpp:1864-1927` 追加 `FindNativeModuleByCache_WhenNodeNameIsNullptrShouldNotCrash`，构造 name==nullptr 节点链表
- [ ] RED 运行验证：**未运行**（本机环境构建失败，详见下方验证证据）
- [x] GREEN 阶段：`native_module_manager.cpp:1634` 已改为 `(temp->name != nullptr && !strcasecmp(temp->name, moduleName))`
- [ ] GREEN 运行验证：**未运行**（同上）
- [ ] 回归运行验证：**未运行**（同上）
- [x] REFACTOR：审视完成，无进一步重构空间（最小变更）
- [x] 实际修改文件未超出 Task/Plan（仅 2 个文件：`native_module_manager.cpp` + `module_manager_test.cpp`）
- [x] 手工修改生成文件：N/A

## 审查检查

- [x] 规范符合性审查通过（详见 review.md §二，所有 AC + 规则有对应实现，无多余实现，无理解偏差）
- [x] 代码质量审查通过（详见 review.md §三，简单仅决策全 PASS）
- [x] 纠正循环：N/A（Review-1 直接 Approved，无需纠正）
- [x] Open Issues 已处理或明确接受（环境 blocker 已记录，由用户处理；L998-999 等同类点显式排除）

## 验证证据（关键）

| 命令 | 结果 | 原因 |
|------|------|------|
| `hb build napi -t` | FAIL | indenp 模式：`OHOS component : (faultloggerd) not found` |
| `./build.sh --product-name rk3568 --target-cpu arm64 --build-target napi_packages_test` | FAIL | GN：`is_host_product` 未定义（napi master 引入的新变量，本机 build/ 仓未跟上） |

按硬规则"证据先于声明"，本机无法声称测试通过。代码改动正确性已通过静态审查保证。

## 出口检查

- [x] Implement Gate 结论已反映到 review.md（Decision: Blocked）
- [ ] 测试运行证据（待用户提供）

### Profile 出口 Gate

> Profile = none，跳过。

## 总结论

**Blocked（代码层 Approved，验证待环境）**

- 代码修复 + 测试代码已就绪（问题 #1 + 问题 #2），规范符合性 + 代码质量审查全 PASS
- 本机构建环境与 napi master 不匹配（`is_host_product` 未定义），无法运行测试
- 需要用户在能构建的环境（CI 或本地修复 build/ 仓）提供测试运行证据
- 证据齐备后回写为 Approved 并进入 Stage 4

---

## TASK-2 实现（问题 #2：systemFilePath 加固）

- [x] RED 阶段：在 `module_manager_test.cpp:1921-1973` 追加 `FindNativeModuleByCache_WhenNodeSystemFilePathIsNullptrShouldNotCrash`，构造 systemFilePath==nullptr 节点（moduleName 匹配使 if 外层成立）
- [ ] RED 运行验证：未运行（同 TASK-1 环境问题）
- [x] GREEN 阶段：`native_module_manager.cpp:1637-1643` 已加 `temp->systemFilePath != nullptr` 判空（L1637 while 短路 + L1642 if 短路）
- [ ] GREEN 运行验证：未运行
- [ ] 回归运行验证：未运行
- [x] REFACTOR：最小变更，无重构空间
- [x] 实际修改文件未超出 Task/Plan（仅 2 个文件，同 TASK-1）
- [x] 规范符合性 + 代码质量审查通过（详见 review.md §八）
- [x] 行为变更（nullptr 视为 path 0 匹配）已评估为可接受

---

## TASK-3 实现（问题 #3：EmplaceModuleBuffer 重复 key 内存泄漏）

- [x] RED 阶段：在 `module_manager_test.cpp:1980-2025` 追加 `EmplaceModuleBuffer_ShouldReleaseOldBufferOnDuplicateKey`，两次调用 EmplaceModuleBuffer 同 key，EXPECT_EQ(size, 1u) + EXPECT_EQ(map[key], lib2)（覆盖 AC-3.1 + AC-3.3）
- [ ] RED 运行验证：未运行（同 TASK-1 环境问题）
- [x] GREEN 阶段：`native_module_manager.cpp:207-220` 已改为 find → 已存在则 `delete[] it->second` + `it->second = lib`；否则 emplace（覆盖 AC-3.1/3.2/3.3）
- [ ] GREEN 运行验证：未运行
- [ ] 回归运行验证：未运行
- [ ] ASan 验证（AC-3.2 旧值释放）：待 CI
- [x] REFACTOR：最小变更（函数体从 3 行扩为 8 行），无重构空间
- [x] 实际修改文件未超出 Task/Plan（仅 2 个文件，同 TASK-1/TASK-2）
- [x] 规范符合性 + 代码质量审查通过（详见 review.md §九）
- [x] 范围合规：严格遵守"每次仅修问题描述本身"规则，未触碰 EmplaceModuleLib（L173-180）等同类点
- [x] 行为变更（重复 key 由"不覆盖+泄漏"改为"覆盖+释放"）已评估为可接受

---

## TASK-4 实现（问题 #4：GetNativeModulePath operator[] 默认插入 nullptr）

- [x] RED 阶段：在 `module_manager_test.cpp:2022-2073` 追加 `GetNativeModulePath_WhenAppLibPathValueIsNullptrShouldNotCrash`，3 段断言（AC-4.1 nullptr 值不崩溃+不污染；AC-4.2 不存在 key 不污染；AC-4.3 非空值进入 if 内层）
- [ ] RED 运行验证：未运行（同 TASK-1 环境问题）
- [x] GREEN 阶段：`native_module_manager.cpp:1057-1099` 已重构（外层 `if (isAppModule)` + `lock_guard + find` 取值；内层 `if (prefix != nullptr)` 包装原 `#ifdef ANDROID_PLATFORM` 块；原 `else` fallback 块原样保留）
- [ ] GREEN 运行验证：未运行
- [ ] 回归运行验证：未运行
- [x] REFACTOR：审视完成。原 `#ifdef ANDROID_PLATFORM` 块内 `if (prefix == nullptr) return false;` 已成死代码，按最小变更原则保留不删
- [x] 实际修改文件未超出 Task/Plan（仅 2 个文件，同 TASK-1/TASK-2/TASK-3）
- [x] 规范符合性 + 代码质量审查通过（详见 review.md §十）
- [x] 范围合规：未触碰 `IsExistedPath` 实现、其他 `operator[]` 使用点（L1112 IOS_PLATFORM `appLibPathMap_["default"]`）、其他 `IsExistedPath` 调用点（按用户规则显式排除）
- [x] 行为变更（key 不存在/值为 nullptr 由"崩溃"改为"走 fallback"）已评估为可接受

---

## TASK-5 实现（问题 #5：SetAppLibPath 空字符串 back() UB）

- [x] RED 阶段：在 `module_manager_test.cpp:2076-2117` 追加 `SetAppLibPath_WhenAppLibPathIsEmptyShouldNotCrash`，3 段断言（AC-5.1 空 vector 不崩溃；AC-5.2 全空字符串不崩溃；AC-5.3 非空元素入库值正确）
- [ ] RED 运行验证：未运行（同 TASK-1 环境问题）
- [x] GREEN 阶段：`native_module_manager.cpp:758` 已改为 `if (!tmpPath.empty() && tmpPath.back() == ':')`
- [ ] GREEN 运行验证：未运行
- [ ] 回归运行验证：未运行
- [x] REFACTOR：审视完成，无进一步重构空间（最小变更，1 行）
- [x] 实际修改文件未超出 Task/Plan（仅 2 个文件，同 TASK-1-#4）
- [x] 规范符合性 + 代码质量审查通过（详见 review.md §十一）
- [x] 范围合规：未加函数入口校验、未触碰其他 back()/front() 使用点、未触碰 L762/769/770 其他 UB/operator[] 点
- [x] 行为变更（空 vector / 全空元素由"崩溃"改为"安全入库空串"）已评估为可接受

---

## TASK-6 实现（问题 #6：LoadNativeModule relativePath 路径穿越校验加固）

- [x] RED 阶段：在 `module_manager_test.cpp:2120-2164` 追加 `IsSafeRelativePath_PathValidationTest`，22 个断言覆盖 AC-6.1（"my..pkg" 接受）/6.2（"/etc/passwd" 拒绝）/6.3（"a/./b" 等拒绝）/6.4（"a;b" 等拒绝）/6.5（合法路径接受）6 类场景
- [ ] RED 运行验证：未运行（同 TASK-1 环境问题，且 `IsSafeRelativePath` 未定义时编译失败即证明测试有效）
- [x] GREEN 阶段：
  - `native_module_manager.h:168` 新增 `static bool IsSafeRelativePath(const std::string& p);` 声明
  - `native_module_manager.cpp:1688-1709` 实现 helper（empty→true；前导 `/`→false；分段空/`.`/`..`→false；字符白名单 alnum/_/./）
  - `native_module_manager.cpp:864-868` `LoadNativeModule` 调用点替换（`find("..")` → `!IsSafeRelativePath(...)`）
  - `native_module_manager.cpp:18` 新增 `#include <cctype>`
- [ ] GREEN 运行验证：未运行
- [ ] 回归运行验证：未运行
- [x] REFACTOR：审视完成，helper 简洁（21 行），调用点最小替换，无进一步重构空间
- [x] 实际修改文件未超出 Task/Plan（3 个文件：`native_module_manager.h` + `native_module_manager.cpp` + `module_manager_test.cpp`）
- [x] 规范符合性 + 代码质量审查通过（详见 review.md §十二）
- [x] 范围合规：未加 realpath + 前缀校验、未加 URL 解码/符号链接检测、未触碰其他路径校验点、未修改 `LoadNativeModule` 其他校验逻辑
- [x] 行为变更双向（放宽 "my..pkg" + 收紧 "/etc/passwd" 等）已评估为可接受

---

## TASK-7 实现（问题 #7：greylist.json lib 名严格字符集校验）

- [x] RED 阶段：在 `module_manager_test.cpp:2167-2207` 追加 `IsValidLibNameStrict_LibNameValidationTest`，19 个断言覆盖 AC-7.1（`liba:b.so` 拒绝）/7.2（含空格/斜杠等拒绝）/7.3（`lib...so` 拒绝）/7.4（非 .so 拒绝）/7.5（空/超长拒绝）/7.6（合法名接受）7 类场景
- [ ] RED 运行验证：未运行（同 TASK-1 环境问题）
- [x] GREEN 阶段：
  - `native_module_manager.h:169` 新增 `static bool IsValidLibNameStrict(const std::string& libName);` 声明
  - `native_module_manager.cpp:1711-1725` 实现 helper（empty/size≥NAME_MAX→false；不以 `.so` 结尾→false；字符白名单 alnum/_/-/.；含 `...`→false）
  - `native_module_manager.cpp:516` `LoadGreylistConfig` 调用点替换（`IsValidLibName` → `NativeModuleManager::IsValidLibNameStrict`）
  - `native_module_manager.cpp:20` 新增 `#include <climits>`
- [ ] GREEN 运行验证：未运行
- [ ] 回归运行验证：未运行
- [x] REFACTOR：审视完成，helper 简洁（14 行），调用点最小替换，无进一步重构空间
- [x] 实际修改文件未超出 Task/Plan（3 个文件：`native_module_manager.h` + `native_module_manager.cpp` + `module_manager_test.cpp`）
- [x] 规范符合性 + 代码质量审查通过（详见 review.md §十三）
- [x] 范围合规：未替换为 JSON 解析器、未加配置完整性校验、未触碰 JSON 解析逻辑（L483-521）/`sharedLibsSonames_` 拼接（L598-617）/`dlns_inherit` 调用点（L679/687）/原 `IsValidLibName`（L442-456 保留为孤立死代码）
- [x] 行为变更（含危险字符 lib 名由"接受"改为"拒绝"）已评估为可接受

---

## TASK-8 实现（问题 #8：UnloadNativeModule TOCTOU 竞态）

- [x] RED 阶段：在 `module_manager_test.cpp:2212-2226` 追加 `UnloadNativeModule_WhenModuleNotInLibMapShouldReturnFalseSafely`，空 libMap → EXPECT_FALSE（覆盖 AC-8.1 早退路径）
- [ ] RED 运行验证：未运行（同 TASK-1 环境问题）
- [x] GREEN 阶段：
  - `native_module_manager.h:170-171` 新增 2 个私有声明（`RemoveNativeModuleByCacheLocked` + `RemoveNativeModuleLocked`）
  - `native_module_manager.cpp:271-295` 重构 `UnloadNativeModule` 为双锁临界区 + 锁外 `dlclose`（按用户建议代码）
  - `native_module_manager.cpp:261-270` 新增 `RemoveNativeModuleLocked`（调用方持双锁 + libMap 已擦除；内部仅调 `RemoveModuleBuffer` + `RemoveNativeModuleByCacheLocked`）
  - `native_module_manager.cpp:1649-1711` 新增 `RemoveNativeModuleByCacheLocked`（与 `RemoveNativeModuleByCache` 同体，移除 lock_guard 行）
- [ ] GREEN 运行验证：未运行
- [ ] 回归运行验证：未运行
- [ ] 并发压力验证（AC-8.3）：待 CI
- [x] REFACTOR：审视完成。双锁临界区最小化（dlclose 在锁外）；2 个 Locked helper 单一职责；既有 4 个 Remove* 函数保留不动
- [x] 实际修改文件未超出 Task/Plan（3 个文件：`native_module_manager.h` + `native_module_manager.cpp` + `module_manager_test.cpp`，同 TASK-6/7）
- [x] 规范符合性 + 代码质量审查通过（详见 review.md §十四）
- [x] **锁顺序静态审查 PASS**：grep 全仓 14 处 lock_guard，唯一双锁嵌套在 L278-279（`lk1(moduleLibMutex_)` → `lk2(nativeModuleListMutex_)`），无反向持有路径 → 无死锁风险（AC-8.6）
- [x] **AC-8.4/8.5 静态审查 PASS**：`RemoveNativeModuleLocked`（L261-270）函数体无 `moduleLibMutex_`/`nativeModuleListMutex_` lock_guard；`RemoveNativeModuleByCacheLocked`（L1649-1711）首行无 lock_guard（仅注释）
- [x] 范围合规：未采用引用计数方案、未触碰 `CloseModuleLibrary`/其他锁粒度问题/`dlclose` 失败处理；既有 4 个 Remove* 函数全部保留不动
- [x] 行为变更（cache 不一致时 handle 泄漏优于 TOCTOU heap corruption）已评估为可接受

---

## TASK-9 实现（问题 #9：EmplaceModuleLib 重复 key 句柄泄漏）

- [x] RED 阶段：在 `module_manager_test.cpp:2228-2256` 追加 2 个测试用例
  - `EmplaceModuleLib_ShouldNotEmplaceWhenLibIsNullptr`（覆盖 AC-9.1 nullptr 早退）
  - `EmplaceModuleLib_ShouldEmplaceWhenKeyNotExists`（覆盖 AC-9.2 首次入库，sentinel 指针 + 手动 erase 清理）
- [ ] RED 运行验证：未运行（同 TASK-1 环境问题）
- [x] GREEN 阶段：`native_module_manager.cpp:175-187` 重构 `EmplaceModuleLib` 为三分支逻辑（nullptr 早退 / find 重复 → `UnloadModuleLibrary(lib)` / find 未命中 → emplace），与用户参考代码逐字一致
- [ ] GREEN 运行验证：未运行
- [ ] 回归运行验证：未运行
- [ ] 重复路径运行验证（AC-9.3/9.4）：待 CI（dlclose 真实句柄单测无法安全模拟）
- [x] REFACTOR：审视完成，函数体从 4 行扩为 10 行，无进一步重构空间
- [x] 实际修改文件未超出 Task/Plan（2 个文件：`native_module_manager.cpp` + `module_manager_test.cpp`，仅 .cpp）
- [x] 规范符合性 + 代码质量审查通过（详见 review.md §十五）
- [x] 范围合规：未采用 `insert_or_assign` 方案、未触碰其他 `emplace` 调用点/`LoadModuleLibrary` 调用方逻辑/dlclose 失败处理/锁外 dlclose 改造
- [x] 行为变更（重复 key 由"不覆盖+泄漏新句柄"改为"不覆盖+dlclose 新句柄"）已评估为可接受
