# Spec Compliance Review

## Verdict

- [ ] Approved
- [ ] Needs Changes
- [x] Blocked

> 代码 + arm64 构建就位；runtime ACs 未验证（TASK-7 待真机/多 Worker）。代码侧交付评审。

## 需求覆盖

| AC | Covered? | Evidence | Gap |
|----|----------|----------|-----|
| AC-1.1 共享身份 | Partial(代码+build) | TASK-4 LookupSharedRoot + 主 env 创建 + 进程级 map | runtime 多 Worker 身份比对未跑（TASK-7） |
| AC-1.2 单 init | Partial(代码+build) | TASK-4 主 env 创建 once + 模块锁 double-check + condvar 协调 Worker | 并发正确性 runtime 未验证（死锁/超时/丢唤醒）；TASK-7 |
| AC-1.3 向后兼容 | Partial(代码+build) | 门控：apiVersion<26 或 nm_shared=0 → 走原普通路径 | 普通 module 回归 runtime 未跑（TASK-7） |
| AC-1.4 非 Sendable 拒绝 | Partial(代码) | TASK-4 IsJSShared 校验失败→return exports | runtime 异常用例未跑；失败仅 return exports（非正式 ERRORED+JS 异常）；allowlist 强制 deferred（TASK-5） |
| AC-1.5 跨 VM 生命周期 | Partial(代码) | 根存进程级 map（跨 VM 存活）；.so pinned（不悬空） | 无 eager release / last-VM unpin（TASK-6 deferred）；runtime 未跑 |
| AC-1.6 .so 不悬空 | Partial(代码+build) | TASK-6 pin guard：UnloadNativeModule 检查 pinned→跳过 dlclose | runtime 卸载路径未跑（TASK-7） |
| AC-1.7 内存基线 | Not covered | — | 需真机 RSS/PSS 采集（TASK-7） |
| AC-2.1 跨 VM Sendable 方法 | Partial(代码) | 复用既有 env 切换（ark_native_engine.cpp:1217） | runtime 非-创建方 Worker 调用未跑（TASK-7） |
| AC-2.2 类型覆盖 | Not covered | — | Sendable Object/Class/Array/Map/ArrayBuffer/TypedArray 用例未写（TASK-7） |

## 额外实现

| Extra Behavior | File | Risk | Required Action |
|----------------|------|------|-----------------|
| 无超出 spec 的额外行为 | — | — | — |

## 理解偏差

| Topic | Spec Says | Implementation Does | Required Action |
|-------|-----------|---------------------|-----------------|
| canonical key（DD-5） | 含 bundle/namespace/加载域隔离 | TASK-4 仅用 strModuleName | 补完整隔离（防跨应用同名误共享） |
| 模块级锁粒度 | per canonical key | TASK-4 全局 sharedRootMutex_（coarse） | 改 per-key |
| 失败状态 | ERRORED 一致 + JS 异常 | TASK-4 仅 return exports | 补 ERRORED + JS 异常 |
| allowlist 强制 | 身份保留式过滤 | TASK-5 deferred（共享分支绕过 CheckArkApiAllowList，身份保留但未强制） | 补身份保留式 reject-if-disallowed |
| .so 生命周期 | 强引用计数 + last-VM release | TASK-6 pin-only（无 eager release） | 补 last-VM unpin |
| TASK-3 scope | 改 jsnapi_expo + ark | 仅改 ark（SendableGlobal 已足悦，未改 ets_runtime） | 可接受（scope 精简） |

## 结论

Blocked——代码实现 spec 机制（DD-1 nm_shared+apiVersion 门控 + 共享分支 + 进程级根 + .so pin + 跨 VM 协调），arm64 构建 + 公共符号导出通过；但 9 条 AC 中 7 条需 runtime 验证（TASK-7 真机/多 Worker 未跑），且 4 项设计偏差待补齐（canonical key 隔离 / per-key 锁 / ERRORED / allowlist 强制）。代码侧可评审，行为闭合待 TASK-7。
