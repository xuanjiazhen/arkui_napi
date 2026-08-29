# Spec

## 概述

| 属性 | 值 |
|------|-----|
| 特性名称 | NAPI 共享导出模块（Sendable Shared Root，路径 B） |
| 特性编号 | NAPI-SSR-01 |
| 优先级 | P1 |
| 复杂度 | 复杂（引用 proposal.md 初始分级判断：跨仓、涉 Public API、涉安全/性能关键路径） |
| 目标版本 | 7.1 |

> Profile `arkruntime` 已应用：`additional_ac_categories`（GC 正确性、语言标准合规）+ `spec-append` fragment（见文末）+ `agent_instructions.specify`。Code fact check 已完成，关键事实见各 AC 与「假设/开放问题」。

## 用户故事或场景

### US-1: 多 Worker 共享 NAPI 模块导出

**作为** 应用/系统模块开发者，**我想要** 声明一个 NAPI 模块为共享导出，使多 Worker 导入时获得对象身份相同的导出根且原生初始化只执行一次，**以便** 避免每 Worker 重复初始化、降低 RSS/PSS 并保证跨 Worker 原生状态一致。

**验收标准：**

> Given/When/Then；Then 仅凭三层接口边界（public/system/inner API 返回值/回调/异常 + 终端可感知行为）可观测；内部数据结构/状态机/流程移 `design.md`。

- **AC-1.1:** Given 一个声明为共享的 NAPI 模块 M 已注册且对应 .so 可加载 When 应用在 ≥2 个 Worker（含主线程）中 import M Then 各 Worker 获得的导出根对象身份相同——可通过 `napi_is_sendable(env, root, &result)` 得 `result==true` 且跨 Worker 对导出根做严格相等比较为真来观测。
- **AC-1.2:** Given 共享模块 M 尚未初始化 When 多 Worker 并发首次 import M Then 模块 native init 回调在整个进程执行次数恰为 1（可通过回调内计数器观测），且并发导入不死锁、最终全部成功返回同一共享根。
- **AC-1.3:** Given 普通模块 N（未声明共享，flags==0）已注册 When 任一 Worker import N Then 加载与 exports 赋值语义与变更前逐项一致——loader 仍创建普通 exports 对象、init 回调仍按属性逐项赋值、缓存仍为本 VM 内 `loadedModules_` 命中。
- **AC-1.4:** Given 共享模块 M 的 init 回调返回非 Sendable 值（普通 Object / 普通 Function / undefined / null / 含 LocalHeap 引用的对象）When 加载 M Then 加载被拒绝：导入方观察到加载失败（抛出 JS 异常或导出为 undefined），模块进入一致错误状态，不产生可被任意 VM 使用的共享导出根；同一 M 在所有 VM 中可见相同的失败结果。
- **AC-1.5:** Given 共享模块 M 已加载、共享根存活 When 一个 Worker VM 销毁后，其余 Worker 调用 M 的导出函数 Then 不崩溃且行为正确——导出函数为 Sendable Function，在消费方 VM 动态获取当前 `napi_env` 执行（`napi_is_sendable(func, &r)` 得 `r==true`）。
- **AC-1.6:** Given 共享根存活且持有 native callback 地址 When 期间触发该 .so 的卸载路径（`dlclose`/模块卸载）Then 卸载被阻止或延迟至共享根释放，无函数地址悬空导致的崩溃。
- **AC-1.7:** Given 多 Worker 场景 When 各 Worker 导入同一共享模块 M Then 进程 RSS/PSS 相比「每 VM 独立加载」基线不升高，共享堆常驻增量有界（具体阈值在 PoC/实现阶段量化观测，本特性不设固定性能阈值）。

### US-2: Sendable 生态跨 Worker 复用 NAPI 能力

**作为** 上层业务（如 TaskPool / Sendable 业务），**我想要** 直接调用 NAPI 共享模块导出的 Sendable 原生方法，**以便** 让 NAPI 模块导出参与 Sendable 跨 Worker 传递生态而无需额外 ArkTS 包装。

**验收标准：**

- **AC-2.1:** Given 共享模块 M 的导出根上含方法 F（由 `napi_create_sendable_object_with_properties` 的 descriptor 创建）When Worker A（非共享根创建方 VM）调用 F Then F 在 A 的 VM 中以 A 的 `napi_env` 执行并返回正确结果（`napi_is_sendable(env, F, &r)` 得 `r==true`）。
- **AC-2.2:** Given 共享模块 M 的导出根由 `napi_create_sendable_object_with_properties`/`napi_define_sendable_class`/Sendable Array/Map/ArrayBuffer/TypedArray 构造 When 检查根与各属性 Then `napi_is_sendable` 均为 true（覆盖 Sendable Object、Class、Array、Map、ArrayBuffer、TypedArray）。

## 业务规则

> 约束声明（非 AC）；约束条件列可含简短内部线索作 design 线索；详细内部实现移 `design.md`「状态归属与不变量」。

| 规则 ID | 规则描述 | 约束条件 | 关联 AC |
|---------|----------|----------|---------|
| BR-1 | 共享能力为显式 opt-in | 未声明的模块走原普通本地模块路径；普通模块语义不变 | AC-1.3 |
| BR-2 | 共享导出根必须为 Sendable 类型 | 禁止 LocalHeap 对象进入共享根；构建与写入双重校验（NAPI 层 + 模块记录层 IsSharedType） | AC-1.4, AC-2.2 |
| BR-3 | .so 生命周期不短于共享根 | 共享根存活期间禁止 `dlclose`（强引用计数或进程驻留） | AC-1.6 |
| BR-4 | native callback 不假设线程亲和性 | callback 可在不同 Worker 线程执行；模块原生静态状态须线程安全；禁止保存创建 VM 专属 `napi_env`/普通 `napi_ref` | AC-1.5, AC-2.1 |
| BR-5 | 共享模块进程级 key 含隔离维度 | canonical key 须含 bundle/namespace/加载域，避免不同应用/HSP 同名模块误共享 | AC-1.1 |
| BR-6 | 不在 init 回调中执行重初始化/阻塞 | 沿用现有模块管理约束（注册回调在加载路径上） | AC-1.2 |
| BR-7 | 共享根在主 env context 创建 | `napi_create_sendable_object_with_properties` 要求 `IsMainEnvContext()`；Worker 仅消费共享根 | AC-1.2, AC-1.5 |

## 异常与边界规则

> 超时/跨进程字段须精确；性能/内存阈值本特性 PoC 量化。

| 编号 | 场景 | 触发条件 | 系统行为 | 关联 AC |
|------|------|----------|----------|---------|
| EX-1 | init 回调返回非 Sendable | 返回普通 Object/Function/undefined/null/含 LocalHeap 引用 | 拒绝加载，模块进入错误状态，导入方观察异常/undefined | AC-1.4 |
| EX-2 | init 回调抛异常 | callback 内 throw | 加载失败，错误状态在所有 VM 一致可见 | AC-1.4 |
| EX-3 | 并发首次 import | 多 Worker 同时 import 同一未初始化共享模块 | 模块级串行化，init 执行 1 次，其余等待后获同一根 | AC-1.2 |
| EX-4 | 一个 Worker 在并发初始化期间销毁 | init 期间某 Worker VM 销毁 | 不死锁、不崩溃；其余 Worker 仍获共享根 | AC-1.2, AC-1.5 |
| EX-5 | 循环/递归 import | 共享模块递归 import 自身或循环依赖 | 检测并按模块错误状态处理（路径 B 模块级状态仍以本 VM 为主，跨 VM 全局循环语义属非目标） | AC-1.4 |
| EX-6 | Worker 数量达上限 | 全局 80 / THREAD_WORKER 64 / LIMITED 16 / OLD 8（沿用现有 Worker 上限） | 沿用现有 Worker 创建失败行为，不影响共享语义 | AC-1.1 |
| EX-7 | 最后一个 VM 销毁 | 所有引用共享根的 VM 均已销毁 | 共享根与 callback 释放按设计释放，无泄漏 | AC-1.5, AC-1.6 |

## 错误码定义

> 沿用标准 `napi_status`（0-21）+ Ark 专有（22-24，见 `js_native_api_types.h`）；不新增 napi_status 值。模块加载失败以 JS 异常/undefined 导出形式可观测。

| 错误码 ID | 错误码值 | 含义 | 关联 AC |
|-----------|----------|------|---------|
| napi_ok | 0 | 操作成功 | AC-1.1~1.3, AC-2.x |
| napi_invalid_arg | 标准值（见 `js_native_api_types.h`） | 参数无效；Sendable 创建在非主 env context 返回此码（现有 `napi_create_sendable_object_with_properties` 行为） | AC-1.4, AC-2.2 |
| napi_generic_failure | 标准值 | 通用失败（init 回调异常/共享根校验失败时 loader 沿用） | AC-1.4, EX-1~2 |
| napi_pending_exception | 标准值 | `NAPI_PREAMBLE` 检测到 pending 异常时短路返回（exception > status） | EX-2 |
| 共享模块加载失败（非新增 napi_status） | 已定（design）：不新增 napi_status 值 | init 返回非 Sendable/抛异常→模块进入 ERRORED 一致状态，加载以 JS 异常/undefined 导出形式失败（见 design 状态归属-Lifecycle） | AC-1.4, EX-1~2 |

## 接口变更分析

> 开放级别：Public（三方应用）/ System（@systemapi）/ Inner（部件间内部接口）。

### 新增接口

无新增接口——复用现有 `napi_module_register`，通过 `napi_module` 新增尾部字段 `nm_shared` 标识共享模块，并由 `LoadNativeModule` 经 `apiVersion` 门控安全读取（见「变更/废弃接口」）。

### 变更/废弃接口

| 接口名称 | 开放级别 | 变更类型 | 影响场景 | 迁移指引 | 关联 AC |
|----------|----------|----------|----------|----------|---------|
| `napi_module`（结构体） | Public | 新增尾部字段 `nm_shared`（置于 reserved 之后，见 design DD-1） | `nm_shared` 标识共享模块；`LoadNativeModule` 经 `apiVersion` 门控读取 | app 目标 SDK<7.1→门控不读→旧 .so 二进制兼容无需重编译 | AC-1.1, AC-1.2, AC-1.3 |
| `napi_module_register` | Public | 行为增强（不破坏签名） | 存 `napi_module*` 指针入 NativeModule（不解引用 nm_shared）；分流在 `LoadNativeModule` 经 apiVersion 门控 | 普通模块（app 目标<7.1 不读 nm_shared）零改动 | AC-1.1, AC-1.2, AC-1.3 |
| `napi_module_with_js_register` | Inner | 待定（见 OQ-5） | 是否同样新增该字段以支持混合模块共享 | 现有调用零改动 | AC-1.3 |

## 兼容性声明

- **已有 API 行为变更:** 是——`napi_module` 新增尾部字段 `nm_shared`（改变 struct 布局）；旧 .so 无此字段，靠 `apiVersion`（app 目标 SDK）门控在 `LoadNativeModule` 跳过读取（`napi_module_register` 只存指针、不解引用）→旧 .so 二进制兼容无需重编译；普通模块（app 目标<7.1 或 nm_shared=0）行为不变（opt-in）；`napi_create_sendable_object_with_properties` 等 Sendable API 不改签名。
- **配置文件格式变更:** 否
- **数据存储格式变更:** 否

## 验证映射

| AC | 关联规则 | 验证方式 | 证据 |
|----|----------|----------|------|
| AC-1.1 | BR-1, BR-5 | ≥2 Worker 导入同一共享模块，比对导出根 `napi_is_sendable` 与跨 VM 严格相等 | 多 Worker 用例 |
| AC-1.2 | BR-6, BR-7 | 多 Worker 并发首次 import + init 回调计数==1 + 无死锁 | 并发用例 + 计数器 |
| AC-1.3 | BR-1 | 普通 NAPI 模块回归用例逐项比对加载/exports 语义 | 回归用例 |
| AC-1.4 | BR-2, EX-1~2 | 异常用例：返回普通 Object/Function/undefined/null/含 LocalHeap；抛异常 | 异常用例 |
| AC-1.5 | BR-4, EX-4,7 | 一 VM 销毁后其余 VM 调用导出函数不崩溃 | 生命周期用例 |
| AC-1.6 | BR-3, EX-7 | 共享根存活期间触发卸载路径，无悬空崩溃 | 生命周期用例 |
| AC-1.7 | — | 多 Worker RSS/PSS 采集对比基线 | 内存采集 |
| AC-2.1 | BR-4 | 非创建方 Worker 调用 Sendable 方法 + `napi_is_sendable` | 跨 VM 调用用例 |
| AC-2.2 | BR-2 | 覆盖 Sendable Object/Class/Array/Map/ArrayBuffer/TypedArray 的 `napi_is_sendable` | 类型覆盖用例 |

## 测试设计提示

> 每条 AC 至少一个测试入口；Red 条件说明实现前为何会失败。

| AC | 测试类型 | 测试文件 | 测试名称 | 输入/触发 | 期望输出/错误 | Red 条件 |
|----|----------|----------|----------|-----------|---------------|----------|
| AC-1.1 | UT | test/unittest/test_sendable_napi.cpp | SharedModuleRootIdentityAcrossWorkers | ≥2 Worker import 共享模块 M | 导出根 `napi_is_sendable`==true 且跨 VM 严格相等 | 共享加载分支不存在→各 Worker 独立加载、导出根身份不同 |
| AC-1.2 | UT | test/unittest/test_sendable_napi.cpp | SharedModuleInitOnceConcurrent | 多 Worker 并发首次 import M | init 回调计数==1，无死锁 | 无模块级串行化→init 多次执行或死锁 |
| AC-1.3 | UT | test/unittest/test_napi.cpp | NormalModuleBackwardCompat | 普通 flags==0 模块 import | exports 逐项与基线一致 | flags 持久化/分流改动误伤普通路径→行为偏差 |
| AC-1.4 | UT | test/unittest/test_napi_errorcode.cpp | SharedModuleNonSendableRejected | init 返回普通 Object/Function/undefined/null | 加载失败，异常/undefined | 无 Sendable 校验→非 Sendable 值被当作共享根 |
| AC-1.5 | UT | test/unittest/test_worker_manager.cpp | SharedModuleSurviveVmDestroy | 一 VM 销毁后其余 VM 调用导出函数 | 不崩溃，行为正确 | .so 生命周期/共享根释放语义未实现→悬空崩溃 |
| AC-1.6 | UT | test/unittest/test_napi_global_ref_track.cpp | SharedModuleSoNotUnloaded | 共享根存活期间触发卸载 | 无悬空崩溃 | 无 .so 生命周期保护→dlclose 后 callback 悬空 |
| AC-2.1 | UT | test/unittest/test_napi_threadsafe.cpp | SendableMethodCrossVmEnv | 非创建方 Worker 调用 F | 正确执行，`napi_is_sendable`==true | env 动态获取未生效→用创建方 env 崩溃 |
| AC-2.2 | UT | test/unittest/test_sendable_napi.cpp | SharedRootTypeCoverage | Sendable Object/Class/Array/Map/ArrayBuffer/TypedArray | `napi_is_sendable` 均 true | 类型构造未覆盖→部分类型非 Sendable |

## 语言运行时 — spec 补充验证场景

> Profile `arkruntime` spec-append fragment（建议 AC 方向，模块 owner 按需定制）：

- **GC 正确性**：共享根生命周期内无泄漏；最后一个 VM 销毁时共享根与 callback 按设计释放；存活期间被引用对象不可再被访问后引用（关联 AC-1.5, AC-1.6, EX-7）。
- **语言标准合规**：共享 NAPI 模块与普通 ArkTS/普通 NAPI 模块共存时不破坏现有模块导入/导出/缓存标准行为（关联 AC-1.3）；编译错误不静默失败。
- **编译正确性**：路径 B 不改 ABC/字节码格式，无字节码版本变更（兼容性声明：配置/数据格式不变）。
- **内存约束**：多 Worker RSS/PSS 不超基线，1 小时运行无共享堆持续增长（关联 AC-1.7）。

## 假设与开放问题

### 假设（已由 code fact check 确认）
- [已确认] `napi_create_sendable_object_with_properties` 要求 `IsMainEnvContext()`，非主 env 返回 `napi_invalid_arg`——共享根 init 必须在主 env context 执行一次，Worker VM 仅消费共享根与 Sendable 方法（动态获取当前 env）。**此为对 proposal「callback 可在不同 Worker 线程执行」的精化**：创建在主 env，调用可跨 Worker。
- [已确认] `napi_module_register` 已将 `nm_flags` 拷入 `NativeModule.flags`（native_node_api.cpp:48），但 `NativeModuleManager::Register()` 仅以 `flags == IS_APP_MODULE_FLAGS(==100)` 特判 App Module 前缀，**链表节点未持久化 flags**。本特性**不沿用 nm_flags**（改用 `napi_module` 新增尾部字段 `nm_shared` + apiVersion 门控），故 flags 持久化问题不再构成前置阻塞；现有 flags==100 语义保持不变。
- [已确认] `napi_module_with_js_register` 未设置 `NativeModule.flags`——混合模块共享语义需补齐。
- [已确认] `ArkNativeEngine::LoadNativeModule` 当前：exports=`ObjectRef::New(vm_)`（普通 Object）；`registerCallback` 返回值被忽略；allowlist 生成 `exportCopy`（普通 Object 副本）；缓存 per-engine `loadedModules_`。
- [已确认] Sendable 引用无 refCount、无自动清理（Worker 退出须手动删除）。
- [已确认] `apiVersion_` = app/worker 的 `apiTargetVersion`（目标 SDK）（`native_engine.cpp:722` `SetApiVersion`；调用方 `ohos_js_environment_impl.cpp:199`；worker 继承 `ark_native_engine.cpp:2385`），per-engine，反映 .so 编译 SDK——可作 `nm_shared` 读取门控（app 目标 SDK ↔ .so 编译 SDK）。门控阈值=OH 7.1 对应 API level（TBD）。

### 开放问题
- **OQ-1（已定）**：共享声明接口形态——**不新增注册函数，不沿用 `nm_flags`**；在 `napi_module` 末尾（`reserved[4]` 之后）新增字段 `nm_shared`，`napi_module_register` 存 `napi_module*` 指针（不解引用），`LoadNativeModule` 经 `apiVersion`（app 目标 SDK）门控读取 `nm_shared` 区分 sendable module 并分流。旧 .so（app 目标<7.1）门控跳过读取→二进制兼容无需重编译。见 design DD-1。
- **OQ-2**：进程级 canonical module key 的隔离维度（bundle/namespace/加载域）——影响 BR-5 与 AC-1.1 隔离正确性，待 design 定。
- **OQ-3**：.so 生命周期语义——进程驻留 vs 强引用计数延迟卸载——影响 BR-3/AC-1.6，待 design 定。
- **OQ-4**：allowlist 兼容——当前 `CheckArkApiAllowList` 生成 LocalHeap `exportCopy` 破坏共享根，需改为构造前过滤或共享视图——影响 BR-2/AC-1.4，待 design 定。
- **OQ-5**：首期支持的模块类型范围（APP/OHOS/INTERNAL/STATIC + `napi_module_with_js`）——影响 spec AC 覆盖范围，待 design/prune 定。
- **OQ-6**：共享根存活期间触发卸载的具体阈值与观测方式——AC-1.7 内存阈值 PoC 量化。
