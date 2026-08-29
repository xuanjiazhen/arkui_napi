# Verification

## 验证执行记录

| 验证项 | 命令/证据 | 结果 |
|--------|----------|------|
| arm64 构建（TASK-1 字段+指针） | `./build.sh --product-name rk3568 --target-cpu arm64 --build-target ace_napi` | ✅ rk3568 build success，Cost 0:05:15，0 error |
| arm64 构建（TASK-2 apiVersion 门控） | 同上 | ✅ build success，Cost 0:01:05，0 error |
| arm64 构建（TASK-3 registry） | 同上 | ✅ build success，Cost 0:01:05，0 error |
| arm64 构建（TASK-4 共享分支+跨 VM 协调） | 同上 | ✅ build success，Cost 0:01:04~0:01:05，0 error |
| arm64 构建（TASK-6 .so pin guard） | 同上 | ✅ build success，Cost 0:01:05，0 error |
| 符号可见性（TASK-8） | `nm -D`/`readelf -s` libace_napi.z.so | ✅ AArch64；napi_create_sendable_object_with_properties/napi_is_sendable/napi_module_register/napi_send_event 均 GLOBAL DEFAULT |
| versionscript/napi_sources（TASK-8） | 通配 `global: *` + 无新 .cpp | ✅ 无需改 |
| runtime 多 Worker 行为（AC-1.1/1.2） | — | ❌ 未跑（无 ohos-sdk UT 套件 + 无设备）→ TASK-7 |
| 跨 VM 协调并发正确性 | — | ❌ 未跑（需多 Worker + 真机）→ TASK-7 |
| 普通 module 回归（AC-1.3） | — | ❌ 未跑 → TASK-7 |
| 异常用例（AC-1.4） | — | ❌ 未跑 → TASK-7 |
| 卸载/生命周期（AC-1.5/1.6） | — | ❌ 未跑 → TASK-7 |
| 内存基线（AC-1.7） | — | ❌ 未跑（需真机 RSS/PSS）→ TASK-7 |
| 类型覆盖（AC-2.2） | — | ❌ 未跑 → TASK-7 |

## 代码与规格一致性结论

代码实现了 spec/design 的机制链：DD-1（`napi_module.nm_shared` + apiVersion≥26 门控 + `napi_module*` 指针存储 + `LoadNativeModule` 门控解引用）→ TASK-4 共享分支（主 env `registerCallback` 取返回值 + `IsJSShared` 校验 + 进程级 `sharedRootMap_` + `MarkSharedSoPinned` + 跨 VM `napi_send_event`+condvar 协调）→ TASK-3 `SendableGlobal` 进程级句柄 → TASK-6 dlclose pin guard。

**一致项**：公共 API 签名/错误码未改（Sendable API 不变）；opt-in 兼容（普通模块走原路径）；arm64 构建 + 公共符号导出通过；versionscript/napi_sources 无需改。

**未一致/待闭合**：9 条 AC 中 7 条（1.1/1.2/1.4/1.5/1.6/1.7/2.1/2.2）需 runtime 验证（TASK-7 真机/多 Worker 未跑）；4 项设计偏差待补（canonical key 隔离 DD-5 / per-key 锁 / ERRORED+JS 异常 / allowlist 强制 TASK-5 / .so eager release TASK-6）。

**总体**：代码 + 构建层一致（build-pass + 符号 OK + 机制链实现 spec）；行为层 runtime 未验证，需 TASK-7 闭合后方可声称 AC 满足。当前为「代码+构建」交付评审状态，非行为闭合。
