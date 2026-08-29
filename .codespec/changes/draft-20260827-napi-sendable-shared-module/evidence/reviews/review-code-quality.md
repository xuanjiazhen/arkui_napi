# Code Quality Review

## Verdict

- [ ] Approved
- [x] Needs Changes
- [ ] Blocked

## Strengths

- 复用既有模式：`SendableGlobal<T>`（jsnapi_expo.h:553）/`napi_send_event`/`EscapeLocalScope`/`HILOG_ERROR`/`napi_module_register` 拷贝链/`Register` head-tail 拷贝；未发明不一致抽象。
- apiVersion 门控（DD-1）保证旧 .so 二进制兼容（app 目标<26 不读 nm_shared→无 OOB）。
- opt-in：nm_shared=0 → 走原普通路径，行为逐项不变。
- arm64 构建 5 次全绿（TASK-1~6+跨 VM）；公共 napi_ 符号 GLOBAL DEFAULT 导出。
- 进程级根存进程级 map（跨 VM 身份唯一基础）；.so pin 防悬空。

## Issues

| Severity | File:Line | Finding | Required Fix |
|----------|-----------|---------|--------------|
| High | ark_native_engine.cpp LoadNativeModule 共享分支 | 跨 VM 协调（condvar + napi_send_event）为并发代码，runtime 未验证；潜在死锁/超时/丢唤醒（5s 超时兜底但未证） | TASK-7 多 Worker + 真机验证并发正确性 |
| High | ark_native_engine.cpp（canonicalKey=strModuleName） | DD-5 canonical key 仅模块名，无 bundle/namespace/加载域隔离→跨应用同名模块可能误共享（安全） | 补完整 canonical key |
| Medium | ark_native_engine.cpp（失败 return exports） | 非 Sendable/异常仅 return exports，非正式 ERRORED + JS 异常（AC-1.4 一致错误状态未满） | 补 ERRORED 状态 + JS 异常 |
| Medium | ark_native_engine.cpp（共享分支绕过 CheckArkApiAllowList） | allowlist 对共享模块未强制（身份保留达成，但安全过滤缺失） | TASK-5 身份保留式 reject-if-disallowed |
| Medium | ark_native_engine.cpp（sharedRootMutex_ 全局） | init 锁为全局而非 per-key，串行化所有共享 init（perf） | 改 per-key 锁 |
| Medium | native_module_manager.cpp（pin 无 unmark） | TASK-6 pin-only，无 eager release / last-VM unpin→.so 进程期常驻 | 补 last-VM unpin + release |
| Low | ark_native_engine.cpp（`auto [insIt, inserted]`） | `inserted` 未使用（编译器未报，风格） | 用 `.first` 或 [[maybe_unused]] |
| Low | //third_party/node/src/node_api.h | nm_shared 加到 vendored 第三方头→上游同步维护成本 | 文档化 fork 偏差，sync 流程登记 |

## Recommendations

- 优先跑 TASK-7（多 Worker fixture + 真机）以验证并发正确性 + 行为 ACs。
- 补 DD-5 canonical key 隔离（安全相关，P1）。
- 失败路径补 ERRORED + JS 异常（AC-1.4）。
- allowlist 强制对共享模块（身份保留式）。
- per-key 锁 + .so eager release（last-VM unpin）。
- 提交前补单测：test_sendable_napi/test_napi_errorcode/test_worker_manager/test_napi_global_ref_track。

## Assessment

代码结构健全、复用既有模式、arm64 构建通过；但核心并发路径（跨 VM 协调）runtime 未验证，且 4 项 spec 偏差（canonical key/ERRORED/allowlist 强制/per-key 锁）+ .so eager release 待补。生产可用性取决于 TASK-7 真机验证结果。代码侧可评审，Needs Changes（行为 + 并发正确性待验/待补）。
