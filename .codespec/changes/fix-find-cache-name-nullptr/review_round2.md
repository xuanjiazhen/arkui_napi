# PR #2750 评审Round 2 修复映射文档

**对应 commit**: `5330e7bb` fix(module_manager): address SE review round 2 (dead code, UAF, style)
**评审时间**: 2026-07-14 ~ 2026-07-16
**评审人**: yzkp (yangzk)、SKY2001
**未解决评论数**: 8 条（yzkp 5 条 + SKY2001 3 条），全部在本次 commit 处理

---

## 修改文件清单

| 文件 | 行数变化 | 涉及评论 |
|---|---|---|
| `module_manager/native_module_manager.cpp` | +30 / -100 | #5 #6 #7 #8 #9 #10 #11 #12 |
| `module_manager/native_module_manager.h` | +2 / -2 | #5 #10 |
| `module_manager/test/unittest/module_manager_test/module_manager_test.cpp` | +40 / -224 | #5 #10 #11 |

---

## yzkp 评审意见

### #5 删除 `RemoveModuleLib` / `RemoveNativeModuleByCache` 公开版

**评审位置**: `native_module_manager.cpp` 原 L266-271（diff_comment id `7a60fe39...`）
**评审内容**:
> 这里移除 RemoveModuleLib、RemoveNativeModuleByCache 调用后，已经没有其他调用方了，确认下是不是可以删除

**核实结论**: ✅ 属实。`grep -rn` 全仓库扫描显示：
- `RemoveModuleLib` 仅 3 处引用：1 处定义 + 2 处单测调用，**生产代码 0 引用**
- `RemoveNativeModuleByCache` 仅 6 处引用：1 处定义 + 5 处单测调用，**生产代码 0 引用**

**代码修改**:

1. **`native_module_manager.cpp`** 删除函数实现：
   ```cpp
   // 已删除（原 L196-208）
   bool NativeModuleManager::RemoveModuleLib(const std::string moduleKey)
   {
       MODULEMNG_HILOG_DEBUG("module:'%{public}s'", moduleKey.c_str());
       bool deleted = false;
       std::lock_guard<std::mutex> lock(moduleLibMutex_);
       auto it = moduleLibMap_.find(moduleKey);
       if (it != moduleLibMap_.end()) {
           moduleLibMap_.erase(it);
           MODULEMNG_HILOG_DEBUG("module '%{public}s' erased", moduleKey.c_str());
           deleted = true;
       }
       return deleted;
   }
   ```

2. **`native_module_manager.cpp`** 删除 `RemoveNativeModuleByCache` 公开版（原 L1625-1680，整段 60 行，函数体与 Locked 版本重复）

3. **`native_module_manager.h`** 删除两处声明：
   ```cpp
   // 已删除
   bool RemoveModuleLib(const std::string moduleKey);                 // 原 L173
   bool RemoveNativeModuleByCache(const std::string& moduleKey);      // 原 L181
   ```

4. **`module_manager_test.cpp`** 删除 7 个测试用例：
   - `LoadNativeModuleTest_012`（RemoveModuleLib）
   - `LoadNativeModuleTest_014`（RemoveModuleLib + UnloadNativeModule）
   - `LoadNativeModuleTest_017`（RemoveNativeModuleByCache）
   - `RemoveNativeModuleByCache_ShouldHandleNullModuleName`
   - `RemoveNativeModuleByCache_ShouldFreeMemoryWhenRemoveHead`
   - `RemoveNativeModuleByCache_ShouldFreeMemoryWhenRemoveTail`
   - `RemoveNativeModuleByCache_ShouldFreeMemoryWhenRemoveMiddle`

---

### #6 合并去重 `RemoveNativeModuleByCacheLocked`

**评审位置**: `native_module_manager.cpp` 原 L1705（diff_comment id `6526f78c...`）
**评审内容**:
> 这段可以移到 RemoveNativeModuleByCache 的位置

**核实结论**: ✅ 属实。公开版 `RemoveNativeModuleByCache` 与 `RemoveNativeModuleByCacheLocked` 函数体逐行比对**完全相同**，仅相差一把 `nativeModuleListMutex_` 锁。

**代码修改**: 随 #5 删除公开版后，自动消除重复函数体。保留 `RemoveNativeModuleByCacheLocked` 不变，因为：
- 被 `RemoveNativeModuleLocked`（L262）在持锁状态调用
- Locked 后缀文档化"调用方持锁"契约，与同文件中 `RemoveNativeModuleLocked` 命名风格一致

**修改后调用链**:
```
UnloadNativeModule(key)                        // 加 moduleLibMutex_ + nativeModuleListMutex_
  └─ moduleLibMap_.erase(it)                   // 锁内
  └─ RemoveNativeModuleLocked(key)             // 锁内
       ├─ RemoveModuleBuffer(key)              // 锁内（非所有权索引清理）
       └─ RemoveNativeModuleByCacheLocked(key) // 锁内（链表节点 free）
  └─ UnloadModuleLibrary(handle)               // 锁外 dlclose
```

---

### #7 删除老版 `IsValidLibName` 死代码

**评审位置**: `native_module_manager.cpp` 原 L539（diff_comment id `230c36d2...`）
**评审内容**:
> 无需加类名，原有的 IsValidLibName 没人用可以删除

**核实结论**: ✅ 属实。`grep IsValidLibName\b` 全仓库扫描：
- 老版 `IsValidLibName`（L466，`static` 函数）：0 处调用
- 新版 `IsValidLibNameStrict`（L1843，类成员）：被 `LoadGreylistConfig` 调用 + 20+ 处单测调用

**代码修改**:

1. **`native_module_manager.cpp`** 删除函数 + 未使用常量：
   ```cpp
   // 已删除（原 L463-480）
   constexpr static int32_t SO_SUFFIX_LENGTH = 3;   // 未使用

   // Helper function to validate library name format
   static bool IsValidLibName(const std::string& libName)
   {
       if (libName.empty()) { return false; }
       if (libName.size() < SO_SUFFIX_LENGTH ||
           libName.substr(libName.size() - SO_SUFFIX_LENGTH) != ".so") {
           MODULEMNG_HILOG_WARN("lib name must end with .so: %{public}s", libName.c_str());
           return false;
       }
       return true;
   }
   ```

---

### #8 去掉 `NativeModuleManager::` 类名前缀

**评审位置**: `native_module_manager.cpp` 原 L888（diff_comment id `45496cf1...`）
**评审内容**:
> 无需加类名

**代码修改**:

```diff
-    if (!NativeModuleManager::IsSafeRelativePath(relativePathStr)) {
+    if (!IsSafeRelativePath(relativePathStr)) {
```

**说明**: `LoadNativeModule` 是 `NativeModuleManager` 的成员函数，同类内调用 `static` 方法无需类名限定。

---

### #9 `IsSafeRelativePath` 字符白名单加 `c != '-'`

**评审位置**: `native_module_manager.cpp` 原 L1835（diff_comment id `83d19483...`）
**评审内容**:
> 和下面的一样加上 c != '-' 吧

**核实结论**: ✅ 合理。下方 `IsValidLibNameStrict`（L1855）字符白名单允许 `-`，`IsSafeRelativePath` 不一致。

**代码修改**:

```diff
     for (char c : seg) {
-        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '.') {
+        if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_' && c != '.' && c != '-') {
             return false;
         }
     }
```

**安全性分析**:
- `-` 在路径段中无特殊语义
- 目录穿越防护未削弱：`/`（路径分隔符）、`..`（父目录）、空段 仍被 L1831 显式拒绝
- 允许合法的 dasherized 命名（如 `my-module/file.abc`）

---

## SKY2001 评审意见

### #10 `EmplaceModuleLib` 行为变更 — UAF 修复 🔴

**评审位置**: `native_module_manager.cpp` 原 L190（diff_comment id `a3406b32...`）
**评审内容**:
> 如果这里直接 UnloadModuleLibrary 掉重复 key 对应的 lib，NativeModuleManager::LoadModuleLibrary 中的返回值 lib 会不会存在行为变更

**核实结论**: ✅ **真实 UAF 风险**。父 commit `51de4694` 引入的重复 key 清理逻辑存在严重缺陷：

| 版本 | duplicate key 时 | `LoadModuleLibrary` 返回 | 调用方使用 |
|---|---|---|---|
| master | emplace no-op，新 lib 泄漏 | 新 lib（未在 map） | 调用方拿到有效但泄漏的 handle |
| 父 commit `51de4694` | dlclose 新 lib | 新 lib（**已 free**） | **UAF**：调用方后续 dlsym 已 free 的 handle |
| 本次 commit `5330e7bb` | dlclose 新 lib | map 中规范 handle（it->second） | 调用方拿到有效 handle ✅ |

**代码修改**:

1. **`native_module_manager.h`** 签名变更：
   ```diff
   -    void EmplaceModuleLib(const std::string moduleKey, LIBHANDLE lib);
   +    LIBHANDLE EmplaceModuleLib(const std::string moduleKey, LIBHANDLE lib);
   ```

2. **`native_module_manager.cpp`** 实现改返回值：
   ```cpp
   LIBHANDLE NativeModuleManager::EmplaceModuleLib(std::string moduleKey, const LIBHANDLE lib)
   {
       MODULEMNG_HILOG_DEBUG("module:'%{public}s'", moduleKey.c_str());
       std::lock_guard<std::mutex> lock(moduleLibMutex_);
       if (lib == nullptr) {
           return nullptr;
       }
       auto it = moduleLibMap_.find(moduleKey);
       if (it != moduleLibMap_.end()) {
           MODULEMNG_HILOG_WARN("module '%{public}s' already exists, discard duplicate handle",
               moduleKey.c_str());
           UnloadModuleLibrary(lib);  // discard duplicate handle, refcount -1
           // Return the canonical handle tracked in the map so LoadModuleLibrary's caller
           // never observes a freed handle (use-after-free in subsequent dlsym).
           return it->second;
       }
       moduleLibMap_.emplace(moduleKey, lib);
       return lib;
   }
   ```

3. **`native_module_manager.cpp`** 调用方 `LoadModuleLibrary` 改用规范返回值：
   ```diff
   -    EmplaceModuleLib(moduleKey, lib);
   -
   -    return lib;
   +    // EmplaceModuleLib discards the duplicate handle (if any) and returns the canonical
   +    // tracked handle; returning it here ensures callers never dereference a freed dlopen
   +    // handle on duplicate-key paths.
   +    return EmplaceModuleLib(moduleKey, lib);
   ```

4. **`module_manager_test.cpp`** 新增回归测试：
   ```cpp
   HWTEST_F(ModuleManagerTest, EmplaceModuleLib_ShouldReturnCanonicalHandleOnDuplicateKey,
       TestSize.Level1)
   {
       NativeModuleManager& mgr = NativeModuleManager::GetInstance();
       LIBHANDLE first = reinterpret_cast<LIBHANDLE>(0xCAFEBABE);
       LIBHANDLE second = reinterpret_cast<LIBHANDLE>(0xBADDCAFE);
       const std::string key = "test_emplace_dup_key_9";

       mgr.EmplaceModuleLib(key, first);
       ASSERT_EQ(mgr.moduleLibMap_.count(key), 1u);

       LIBHANDLE returned = mgr.EmplaceModuleLib(key, second);
       EXPECT_EQ(returned, first);            // 必须返回 canonical handle
       EXPECT_EQ(mgr.moduleLibMap_[key], first);

       mgr.moduleLibMap_.erase(key);
   }
   ```

**⚠️ 已知问题（来自后续 review）**: 该测试会真实触发 `dlclose(0xBADDCAFE)`，是 UB。已记录在 `review_round2.md` 的 review 报告 High-1 中，下一版修复。

---

### #11 `EmplaceModuleBuffer` 行为变更 — 回退为 master 行为 🟡

**评审位置**: `native_module_manager.cpp` 原 L230（diff_comment id `1d8ea6d4...`）
**评审内容**:
> 这里是使用传入的 lib 覆盖掉相同 key 的 lib；原本的行为是如果有相同 key 就不做操作也不会覆盖，这里修改需要确认下是否有行为变更

**核实结论**: ✅ **真实 UAF 风险**。父 commit `db98cf0f`（PR 描述中的"问题 #3"）误判 `moduleBufMap_` 为所有权容器，实际它只是非所有权索引。

**所有权模型分析**:
```
GetFileBuffer 调用链：
  GetFileBuffer(key, path)
    └─ new uint8_t[len]                  ← 分配 buffer
    └─ EmplaceModuleBuffer(key, lib)     ← 写入 map（索引）
    └─ return lib                         ← 返回给调用方
  FindNativeModuleByDisk:
    └─ tailNativeModule_->jsABCCode = lib ← buffer 所有权转移到 NativeModule

卸载时（RemoveNativeModuleByCacheLocked）：
  └─ delete[] nativeModule->jsABCCode     ← NativeModule 释放 buffer
  （map 中的索引条目由 RemoveModuleBuffer 提前 erase）
```

**父 commit 的 bug**: 在 duplicate key 时 `delete[] it->second` 会释放**首个模块的 jsABCCode**，但首个 NativeModule 仍持有该指针 → UAF。

**代码修改**: 回退为 master 行为（`std::map::emplace` 重复 key no-op）：

```cpp
void NativeModuleManager::EmplaceModuleBuffer(const std::string moduleKey, const uint8_t* lib)
{
    MODULEMNG_HILOG_DEBUG("module:'%{public}s'", moduleKey.c_str());
    std::lock_guard<std::mutex> lock(moduleBufMutex_);
    // The ABC buffer's memory is owned by NativeModule::jsABCCode; moduleBufMap_ is just an
    // index. std::map::emplace is a no-op on duplicate key, which matches the ownership
    // model: replacing the map entry would free the still-in-use buffer of the first module
    // (use-after-free via its jsABCCode pointer). The caller (GetFileBuffer) independently
    // assigns the returned pointer to the current NativeModule.
    if (lib != nullptr) {
        moduleBufMap_.emplace(moduleKey, lib);
    }
}
```

**测试改写**: 原 `EmplaceModuleBuffer_ShouldReleaseOldBufferOnDuplicateKey`（验证错误行为）改写为 `EmplaceModuleBuffer_ShouldKeepOriginalBufferOnDuplicateKey`：

```cpp
// 第二次调用同一 key 时为 no-op
moduleManager.EmplaceModuleBuffer(key, lib1);
EXPECT_EQ(moduleManager.moduleBufMap_[key], lib1);

moduleManager.EmplaceModuleBuffer(key, lib2);
EXPECT_EQ(moduleManager.moduleBufMap_.size(), 1u);
EXPECT_EQ(moduleManager.moduleBufMap_[key], lib1);  // 仍是 lib1，未被覆盖

// 两个 buffer 都是测试自己分配的，map 不持有所有权
delete[] lib1;
delete[] lib2;
```

**⚠️ 影响 PR 描述中的"问题 #3"**: 原 PR 声称修复了"重复 key 内存泄漏"，经复核为**误判**（buffer 所有权本就在 NativeModule，map 为索引）。需在 PR 描述中更新 #3 状态。

---

### #12 `NAME_MAX` 编译可见性

**评审位置**: `native_module_manager.cpp` 原 L1848（diff_comment id `ef7ec785...`）
**评审内容**:
> 没找到 NAME_MAX 的定义，需要确认下

**核实结论**: ✅ 属实。当前 include 链 `<climits>` → `<limits.h>`，但 `NAME_MAX` 在 bionic/glibc 下需要 `_POSIX_C_SOURCE` 才会暴露，OH 部分工具链未必可见。

**代码修改**: 在 `<unistd.h>` include 之后加 fallback 定义：

```cpp
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>

// NAME_MAX (typically 255 on Linux/bionic) is not reliably exposed by <climits>
// across all OH build toolchains. Provide a local fallback so IsValidLibNameStrict
// compiles deterministically.
#ifndef NAME_MAX
#define NAME_MAX 255
#endif
```

**说明**:
- 值 255 是 POSIX 规定的 `NAME_MAX` 最小值
- `#ifndef` 守卫确保系统已定义时不覆盖，避免重定义警告
- 单测中的 `NAME_MAX` 引用（`module_manager_test.cpp:2191/2219/2221`）无需改动
- 定义放在 `.cpp` 而非 `.h`，因为只在 `IsValidLibNameStrict` 中使用

---

## 后续 review 发现的遗留问题（非本次评审范围）

### High-1（待修复）: 新增测试 UB

`EmplaceModuleLib_ShouldReturnCanonicalHandleOnDuplicateKey` 会真实调用 `dlclose(0xBADDCAFE)`，UB。注释错误地声称"bypass"了 dlclose。

**修复方案**（下一版）:
1. 拆分 `EmplaceModuleLib` 为 lookup + commit 两步，便于单测绕过 dlclose
2. 或使用 `RTLD_DEFAULT` 合法哨兵
3. 或删除测试，改为 PR 说明中描述 manual 验证步骤

### Medium-1（建议）: 缺少所有权约束注释

`RemoveNativeModuleLocked` 中 `RemoveModuleBuffer` 必须在 `RemoveNativeModuleByCacheLocked` **之前**调用的不变量缺少注释。当前代码顺序正确，但未来修改容易破坏。

---

## 参考

- PR: https://gitcode.com/openharmony/arkui_napi/pull/2750
- 关联 Issue: #2157
- 处理 commit: `5330e7bb`
- 前序 commit（已回复）: `a8781e79`
- 综合审查报告: 见本目录 `review_report.md`（如已生成）
