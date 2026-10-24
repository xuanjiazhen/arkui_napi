# Spec

## 概述

| 属性 | 值 |
|------|-----|
| 特性名称 | NAPI 模块共享导出根（Shared Export Root） |
| 特性编号 | TBD（由特性管理流程分配） |
| 优先级 | P1 |
| 复杂度 | 复杂（引用 proposal.md 初始分级判断） |
| 目标版本 | 7.1 |

> **激活前置（关键）**：共享导出根分支由 `IsSharedModule(module)` 谓词门控，谓词读 `module->shared != 0`。`shared` 字段由**新增公共 API 注册函数** `napi_shared_module_register`（`interfaces/kits/napi/native_api.h`）在注册时置 `1`；老的 `napi_module_register` 路径不读 `napi_module` 任何新字段、`shared` 保持 `0`。模块作者在 `.so` constructor 调 `napi_shared_module_register(&mod)` 即声明共享；调 `napi_module_register` → `IsSharedModule` false → 走普通本地路径。二进制兼容由"调哪个注册函数"决定（老 `.so` 只调 `napi_module_register`），无需版本门控，不改 `napi_module` 结构体。下列"被声明为共享的模块"行为的 AC，其生产激活即经 `napi_shared_module_register` 路径；UT 用 `napi_shared_module_register` 注册共享模块验证（见「测试设计提示」）。

## 用户故事或场景

### US-1: 多 Worker 共享同一 NAPI 模块的原生导出

**作为** 应用/系统模块开发者，**我想要** 用 NAPI 编写的原生模块在多 Worker 间只初始化一次并共享同一导出根对象，**以便** 避免每个 Worker 重复初始化、重复占用原生状态，并获得跨 Worker 一致的导出对象身份。

**验收标准：**

> Then 仅凭公开/系统接口（napi 侧可观测的导出对象、init 次数、日志、崩溃与否）判定；内部状态机/两段锁/SendableGlobal 句柄归属移 design.md。

- **AC-1.1:** Given 一个被声明为共享的 NAPI 模块 M（经 `napi_shared_module_register` 注册，`module->shared == 1`）且进程内尚未初始化 M When 两个不同 Worker 的 VM 各自首次 import M Then 两个 VM 获得的导出根对象经 `napi_strict_equals` 判定为同一对象（true），且 M 的 registerCallback 在进程内仅被调用一次（可通过模块内置计数器或 HILOG 日志观测调用次数 == 1）。
- **AC-1.2:** Given 共享模块 M 已在 VM-A 完成初始化且导出根就绪 When VM-B（另一 Worker）import M Then VM-B 获得与 VM-A 经 `napi_strict_equals` 判定为同一的导出根对象，且不再触发 registerCallback（init 次数保持 == 1）。
- **AC-1.3:** Given 共享模块 M 的 registerCallback 返回一个非 Sendable 对象（普通 Object/Function） When 任一 Worker import M Then 该模块进入错误状态，import 返回的导出为空（undefined），HILOG_ERROR 记录 "callback returned non-sendable root"；所有 Worker 看到一致的错误结果（导出空）。
- **AC-1.4:** Given 共享模块 M 的 registerCallback 返回 null 或 undefined When Worker import M Then 模块进入错误状态，导出为空（undefined），HILOG_ERROR 记录 "callback returned null"。
- **AC-1.5:** Given 共享模块 M 的 registerCallback 执行期间抛出 JS 异常（napi_throw_error） When Worker import M Then 模块进入错误状态，导出为空（undefined），HILOG_ERROR 记录初始化失败；异常不向 import 调用方传播为未捕获异常。
- **AC-1.6:** Given 多个 Worker 同时并发首次 import 同一未初始化共享模块 M When 并发 import 全部完成 Then 无死锁或挂起（全部 Worker 在合理时间内返回），registerCallback 仅执行一次，所有 Worker 最终获得同一导出根对象或一致的错误状态。
- **AC-1.7:** Given 共享模块 M 的导出根已在 VM-A 创建 When VM-A 被销毁后 VM-B 调用导出根上的 Sendable 方法 Then 方法在 VM-B 上正确执行并返回正确结果，不发生崩溃。
- **AC-1.8:** Given 共享模块 M 的导出根仍存活 When 对 M 执行模块卸载（`UnloadNativeModule`）后调用导出根上的方法 Then 底层 .so 未被卸载（dlclose 被跳过），调用不发生函数地址悬空崩溃。
- **AC-1.9:** Given 一个普通 NAPI 模块 P（经 `napi_module_register` 注册，`shared == 0`） When 两个 Worker 各自 import P Then 各 VM 独立初始化 P、获得各自独立的导出对象（跨 VM `napi_strict_equals` 为 false），初始化与导出行为与变更前完全一致（allowlist、exports 赋值、VM 内缓存路径不变）。

### GC 正确性（arkruntime profile 必填）

- **AC-G1:** Given 共享模块 M 的导出根 R 被进程级句柄持有 When 触发共享堆/CMC GC 标记遍历 Then R 不被回收；源 VM 销毁后 R 仍可被其他 VM 通过 `SendableGlobal` 重新具象化访问（对应 AC-1.7 的可观测前提）。
- **AC-G2:** Given 共享导出根 R 及其属性（Sendable Object/Array/Map/Class/TypedArray） When 多 Worker 长期并发调用 R 上方法（持续 1 小时） Then 无内存泄漏（共享堆占用无持续增长，冷启动基线 105% 以内；属 arkruntime 内存约束）。

### 语言标准合规（arkruntime profile 必填）

- **AC-L1:** Given 共享模块 M 的 registerCallback 通过 `napi_create_sendable_object_with_properties` 构造导出根 When 属性描述符中包含 LocalHeap（非 Sendable）值 Then 构造侧返回非 `napi_ok` 错误码（不静默成功），导出根不被创建，模块进入错误状态。
- **AC-L2:** Given ArkTS `"use shared"` 模块与 NAPI 共享模块并存 When NAPI 共享模块 M 被多 VM import Then NAPI 共享模块不破坏 ArkTS 标准模块加载与 Sendable 类型约束（模块记录保持本地 `UNSENDABLE_MODULE`，不触发 ets_runtime 共享模块导出校验）；现有 ArkTS 标准测试套件通过率不低于基线。

### 二进制兼容（arkruntime profile 追加）

- **AC-BC1:** Given 老 `.so`（只调 `napi_module_register`，不调 `napi_shared_module_register`） When 加载并注册 Then `napi_module_register` 不读 `napi_module` 任何新字段，`module.shared` 保持 `0`，不越界；模块按普通本地路径加载。共享判定由"调哪个注册函数"决定，无需版本门控。

## 业务规则

> 约束声明（非 AC）；内部实现展开移 design.md「状态归属与不变量」。

| 规则 ID | 规则描述 | 约束条件 | 关联 AC |
|---------|----------|----------|---------|
| BR-1 | 共享导出根由进程级句柄持有，跨 VM 共享对象身份 | 句柄独立于任何单 VM（GC 标记跨 VM 遍历） | AC-1.1, AC-1.7, AC-G1 |
| BR-2 | 共享模块的 native init 在进程内至多执行一次 | init 不持任何锁；init 结果对全体 VM 一致可见 | AC-1.1, AC-1.2 |
| BR-3 | 共享导出根必须是 Sendable 类型 | loader 侧校验返回值；非 Sendable 即 errored | AC-1.3, AC-L1 |
| BR-4 | 共享导出根存活期间底层 .so 不被卸载 | `.so` pin 后永不 unpin 直至最后 VM 销毁/进程结束 | AC-1.8 |
| BR-5 | 普通（非共享）模块加载路径不受影响 | 调 `napi_module_register`（`shared==0`）时 `IsSharedModule` false，走原本地路径 | AC-1.9 |
| BR-6 | 共享模块 Sendable 方法在消费方 VM 执行，运行时环境为当前 VM | 模块作者不得缓存创建 VM 专属的 napi_env/线程状态 | AC-1.7 |
| BR-7 | allowlist 拷贝过滤不适用于共享分支（拷贝产物为本地堆，破坏 sendable） | 共享分支 early return 天然绕过；API 暴露面由模块作者在构造时自决 | AC-1.9 |
| BR-8 | 共享模块 canonical key 首版仅用 moduleName | 跨 bundle/HSP 同名模块存在误共享风险（开放问题，后续加固） | AC-1.1（边界） |
| BR-9 | opt-in 经新增 inner API 函数 `napi_shared_module_register` | `napi_module_register` 不读 `napi_module` 新字段；老 `.so` 只调老函数→普通路径；二进制兼容由 API 选择保证，无需版本门控，不改 `napi_module` 结构体 | AC-BC1, AC-1.9 |

## 异常与边界规则

> 并发 init 等待为 notify 驱动，无固定超时阈值；此为本变更已锁定的设计决策，非"待定义"。

| 编号 | 场景 | 触发条件 | 系统行为 | 关联 AC |
|------|------|----------|----------|---------|
| EX-1 | 并发首次 import 死锁/挂起 | 多 Worker 同时 import 同一未初始化共享模块 | 两段锁协调：init 仅一次；等待方阻塞至 init 完成（success/error）或 VM 销毁后被唤醒；无固定超时；若 registerCallback 挂起则等待方无限阻塞（已知风险） | AC-1.6 |
| EX-2 | callback 返回非 Sendable | registerCallback 返回普通 Object/Function | 模块 errored；导出空（undefined）；HILOG_ERROR；所有 VM 一致 | AC-1.3 |
| EX-3 | callback 返回 null/undefined | registerCallback 返回空值 | 模块 errored；导出空；HILOG_ERROR | AC-1.4 |
| EX-4 | callback 抛异常 | registerCallback 内 napi_throw_error | 模块 errored；导出空；HILOG_ERROR；异常不向 import 调用方传播为未捕获 | AC-1.5 |
| EX-5 | 源 VM 销毁期间 init 进行中 | init 未完成时源 VM 被销毁 | SendableGlobal 跨 VM 存活不崩溃；init 结果对其他 VM 一致可见 | AC-1.7, AC-G1 |
| EX-6 | 同线程递归 import 自身 | 共享模块 M 的 registerCallback 内递归 import M | 首版返回空 exports + HILOG_ERROR（不深度处理，proposal §7.1） | —（开放问题） |
| EX-7 | .so 卸载尝试 | 共享根存活时 `UnloadNativeModule` | `IsSharedSoPinned` 命中，跳过 dlclose | AC-1.8 |
| EX-8 | canonical key 同名冲突 | 不同 bundle/HSP 同名共享模块 | 首版用 moduleName 作 key，存在误共享风险（proposal 开放问题） | AC-1.1（边界） |
| EX-9 | 模块未 opt-in | 模块调 `napi_module_register`（未调 `napi_shared_module_register`）→ `shared==0` | `IsSharedModule` false → 走普通本地模块路径，共享分支不进入 | AC-1.9, AC-BC1 |

## 错误码定义

> 本变更不引入新公共 napi 错误码。loader 侧失败以"空导出 + HILOG_ERROR"呈现，不向公共 API 调用方返回类型化错误码。

| 错误码 ID | 错误码值 | 含义 | 关联 AC |
|-----------|----------|------|---------|
| napi_ok | 0 | `napi_create_sendable_object_with_properties` 成功创建导出根 | AC-1.1, AC-L1 |
| napi_invalid_arg | 1 | 构造参数非法（property_count/properties 为空等） | AC-L1 |
| napi_generic_failure | 9 | 构造 sendable root 失败（属性含非 sendable 值等运行期失败） | AC-1.3, AC-L1 |
| napi_pending_exception | 10 | callback 执行期间存在 pending exception | AC-1.5 |
| 共享模块 init 失败（非 sendable/null/抛异常） | 无新错误码（design DD-3 决议） | 失败统一以 empty exports（undefined）+ HILOG_ERROR 呈现，错误状态对所有 VM 一致；不引入新类型化 napi 错误码 | AC-1.3/1.4/1.5 |

> 注：napi_status 取值范围 0-21（标准）+ Ark 专有 22-24（见 `docs/knowledge/napi-differences.md`）。本变更未触及 Ark 专有码，相关精确值未在仓内核实（属 SDK 头文件），如需引用须在 design 阶段从 SDK 头文件复核。

## 接口变更分析

> 开放级别：**Public**（面向三方应用）/ **System**（@systemapi）/ **Inner**（InnerAPI，部件间内部接口）。

### 新增接口

| 接口名称 | 开放级别 | 参数概要 | 返回值 | 错误码 | 关联 AC |
|----------|----------|----------|--------|--------|---------|
| `napi_shared_module_register` | Public | `(napi_module* mod)` | `void` | — | BR-9, AC-BC1, AC-1.1~1.9 |
| `NativeModule.shared`（字段） | Inner | `unsigned int shared = 0` | — | — | BR-1/BR-9（由 `napi_shared_module_register` 置 1） |
| `NativeModuleManager::MarkSharedSoPinned` | Inner | `(const std::string& moduleKey)` | `void` | — | AC-1.8, BR-4 |
| `NativeModuleManager::IsSharedSoPinned` | Inner | `(const std::string& moduleKey)` | `bool` | — | AC-1.8, EX-7 |
| `ArkNativeEngine::LoadSharedNativeModule` | Inner | `(NativeModuleManager*, NativeModule*, const std::string& key)` | `Local<JSValueRef>`（导出根或空） | — | AC-1.1~1.7 |
| `ArkNativeEngine::CreateSendableRoot` | Inner | `(NativeModule* module)` | `Local<JSValueRef>`（sendable root 或空） | — | AC-1.3, AC-L1 |
| `ArkNativeEngine::IsSharedModule` | Inner | `(NativeModule* module) const` | `bool`（`module->shared != 0`） | — | AC-1.9, EX-9 |

### 变更/废弃接口

| 接口名称 | 开放级别 | 变更类型 | 影响场景 | 迁移指引 | 关联 AC |
|------|----------|----------|----------|----------|---------|
| `napi_module_register`（native_node_api.cpp:35） | Inner | 行为不变（不读 `napi_module` 新字段；`shared` 保持 0） | 模块注册路径 | 老 .so 二进制兼容：老 .so 只调本函数，不读新字段，`shared=0` 走普通路径 | BR-9, AC-BC1 |
| `ArkNativeEngine::LoadNativeModule`（ark_native_engine.cpp:838） | Inner | 行为扩展（插入 sendable 分支：`IsSharedModule` 为 true 时 early return 至 `LoadSharedNativeModule`） | 模块加载路径 | 普通模块（调 `napi_module_register`）路径不变 | AC-1.1~1.9 |
| `NativeModuleManager::Register`（复制 shared） | Inner | 行为扩展（head/tail 节点复制时复制 `shared`） | 模块链表注册 | 无破坏性 | BR-1/BR-9 |

> `napi_module` 结构体不改、不读新字段；`napi_shared_module_register` 为新增**公共 API** 函数（`interfaces/kits/napi/native_api.h`，面向三方应用），`ace_napi.versionscript` 为 `global: *` 通配导出新符号，无需改。`napi_module_with_js_register` 不改动（首版不支持混合模块共享声明）。

## 兼容性声明

- **已有 API 行为变更:** 是——`LoadNativeModule` 加 sendable 分支、`napi_module_register` 行为不变（不读新字段）、`Register` 复制 `shared`；但普通模块（调 `napi_module_register`）行为完全不变。
- **配置文件格式变更:** 否
- **数据存储格式变更:** 否

## 验证映射

| AC | 关联规则 | 验证方式 | 证据 |
|----|----------|----------|------|
| AC-1.1 | BR-1, BR-2 | 多 Worker import 同一共享模块后 `napi_strict_equals` 比对导出根 + init 计数器/日志断言 == 1 | UT（`napi_shared_module_register` 注册）+ 板侧多 Worker |
| AC-1.2 | BR-2 | VM-A init 后 VM-B import，断言 strict_equals + init 次数保持 1 | UT + 板侧 |
| AC-1.3 | BR-3 | callback 返回普通 Object，断言导出 undefined + HILOG_ERROR + 多 VM 一致 | UT |
| AC-1.4 | BR-3 | callback 返回 null/undefined，断言导出 undefined + HILOG_ERROR | UT |
| AC-1.5 | BR-3 | callback 内 napi_throw_error，断言导出 undefined + 无未捕获异常传播 | UT |
| AC-1.6 | BR-2, EX-1 | 并发首次 import，断言无挂起 + init == 1 + 结果一致 | UT（并发用例）+ 板侧 |
| AC-1.7 | BR-1, BR-6 | 销毁源 VM 后其他 VM 调用导出方法，断言正确返回不崩溃 | UT + 板侧 |
| AC-1.8 | BR-4, EX-7 | 共享根存活时 UnloadNativeModule 后调用，断言不崩溃 | UT |
| AC-1.9 | BR-5, BR-7 | 普通（调 `napi_module_register`）模块多 Worker import，断言各 VM 独立导出 + 行为不变（回归） | UT + 回归套件 |
| AC-BC1 | BR-9 | 老 .so（只调 `napi_module_register`）加载，断言不读新字段、`shared=0`、走普通路径 | UT |
| AC-G1 | BR-1 | 触发 GC 后断言导出根可访问；源 VM 销毁后其他 VM 可访问 | UT + 板侧 |
| AC-G2 | BR-1 | 多 Worker 1h 并发调用，共享堆无持续增长 | 板侧长稳 |
| AC-L1 | BR-3 | 构造含 LocalHeap 值，断言返回非 napi_ok + 导出根未创建 | UT（test_sendable_napi） |
| AC-L2 | BR-5 | 不触发 ets_runtime 共享校验；ArkTS 标准测试套件通过率 ≥ 基线 | UT + 标准套件 |

## 测试设计提示

> AC → 实现文件 + Task + 验证状态映射见 `execution-plan.md`。生产激活经 `napi_shared_module_register` 注册（公共 API）；UT 在 `.so` constructor 调 `napi_shared_module_register(&mod)` 注册共享模块验证机制，Red 条件说明实现前为何失败。

| AC | 测试类型 | 测试文件 | 测试名称 | 输入/触发 | 期望输出/错误 | Red 条件 |
|----|----------|----------|----------|-----------|---------------|----------|
| AC-1.1 | UT | test/unittest/test_sendable_napi.cpp | SharedModule_InitOnce_SameRoot | 两 VM（经 `napi_shared_module_register` 注册）import 同一共享模块 | strict_equals true；init 计数 == 1 | 实现前 LoadNativeModule 无 sendable 分支，各 VM 独立 init、导出对象不同（strict_equals false） |
| AC-1.2 | UT | test/unittest/test_sendable_napi.cpp | SharedModule_SecondVMAfterInit | VM-A init 完成后 VM-B import | strict_equals true；init 保持 1 | 实现前无共享根缓存，VM-B 重复 init |
| AC-1.3 | UT | test/unittest/test_sendable_napi.cpp | SharedModule_NonSendableRoot_errored | callback 返回普通 Object | 导出 undefined；HILOG_ERROR；多 VM 一致 | 实现前 callback 返回值被忽略，导出为本地空对象非一致错误态 |
| AC-1.4 | UT | test/unittest/test_sendable_napi.cpp | SharedModule_NullRoot_errored | callback 返回 null | 导出 undefined；HILOG_ERROR | 同上 |
| AC-1.5 | UT | test/unittest/test_sendable_napi.cpp | SharedModule_ThrownException_errored | callback 内 napi_throw_error | 导出 undefined；无未捕获传播 | 同上 |
| AC-1.6 | UT | test/unittest/test_sendable_napi.cpp | SharedModule_ConcurrentInit_NoDeadlock | 多线程并发首次 import | 无挂起；init == 1；结果一致 | 实现前无两段锁，并发可能重复 init/竞争 |
| AC-1.7 | UT | test/unittest/test_napi_context.cpp | SharedModule_SurviveSourceVM | 销毁源 VM 后其他 VM 调用导出方法 | 正确返回；不崩溃 | 实现前用 Global（VM-local），源 VM 销毁后句柄失效崩溃 |
| AC-1.8 | UT | module_manager/test/unittest/module_manager_test/ | SharedModule_SoNotUnloaded | 共享根存活时 UnloadNativeModule | dlclose 跳过；调用不崩溃 | 实现前无 pin，Unload 后 dlclose 致函数地址悬空崩溃 |
| AC-1.9 | UT | test/unittest/test_napi.cpp、test_ark_api_allowlist.cpp | NormalModule_BehaviorUnchanged | 普通（调 `napi_module_register`）模块多 Worker import | 各 VM 独立导出；行为不变 | 实现前/后均应通过（回归保护） |
| AC-BC1 | UT | module_manager/test/unittest/module_manager_test/ | OldSo_NoOobRead | 老 .so（只调 `napi_module_register`）加载 | 不读新字段；`shared=0`；走普通路径 | —（二进制兼容由 API 选择保证，回归保护） |
| AC-G1 | UT | test/unittest/test_napi_global_ref_track.cpp | SharedModule_RootSurvivesGC | 触发 GC 后访问导出根 | 可访问；不崩溃 | 实现前无进程级句柄，GC 回收后访问崩溃 |
| AC-G2 | 板侧长稳 | —（板侧） | SharedModule_LongRun_NoLeak | 多 Worker 1h 并发调用 | 共享堆无持续增长 | —（板侧证据） |
| AC-L1 | UT | test/unittest/test_sendable_napi.cpp | SendableObject_NonSendableProp_Rejected | 构造含 LocalHeap 值的属性 | 返回非 napi_ok；导出根未创建 | 构造侧既有行为（napi_create_sendable_object_with_properties 已拦），回归保护 |
| AC-L2 | UT | test/unittest/test_napi.cpp | SharedModule_NoEtsRuntimeSharedCheck | NAPI 共享模块多 VM import | 不触发 ets_runtime 共享校验；标准套件通过 | 实现前模块记录本地 UNSENDABLE_MODULE 不触发校验（回归保护） |

> Fuzz：`test/fuzztest/loadarkmodule_fuzzer/` 应覆盖共享模块加载路径（经 `napi_shared_module_register` 注册）。

## 语言运行时 — spec 补充验证场景

> arkruntime profile 追加（fragment: spec-append.md，append 策略）。模块 owner 按需定制。

- **GC 正确性**：共享导出根无内存泄漏；GC 停顿 ≤ 10ms(P95)（本变更不引入新 GC 路径，须确认共享堆标记遍历不退化停顿）；被回收对象不可再访问（对应 AC-G1）。
- **语言标准合规**：ArkTS 标准测试套件通过率不低于基线；编译错误不静默失败（对应 AC-L1/L2）。
- **二进制兼容**：老 `.so`（只调 `napi_module_register`）加载不读 `napi_module` 新字段（对应 AC-BC1）；由 API 选择保证，无需版本门控。
- **编译正确性**：本变更不涉 JIT/AOT 与字节码格式，不适用（模块记录保持本地，无字节码/AOT 格式变更）。
- **内存约束**：冷启动内存不超基线 105%（共享导出根 + `.so` 常驻增量须观测）；1 小时运行无持续增长（对应 AC-G2）。
