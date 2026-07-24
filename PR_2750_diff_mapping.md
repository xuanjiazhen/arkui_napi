# PR #2750 diff 逐处改动 → 问题映射

> **仓库**：`openharmony/arkui_napi`
> **PR**：https://gitcode.com/openharmony/arkui_napi/pull/2750
> **分支**：`fix/find-cache-name-nullptr`
> **基线**：`3119c6c6` (`!2746 merge addSym into master`)
> **HEAD**：`80e6d284`
> **关联 Issue**：#2157
> **改动统计**：4 个文件 / +939 −297

## 改动文件总览

| 文件 | 行数变更 |
|---|---|
| `module_manager/module_load_checker.h` | +1 −1 |
| `module_manager/native_module_manager.cpp` | +208 −77 |
| `module_manager/native_module_manager.h` | +8 −5 |
| `module_manager/test/unittest/module_manager_test/module_manager_test.cpp` | +722 −214 |

---

## 问题编号索引

| # | 问题摘要 | 严重性 | 状态 |
|---|---|---|---|
| #1 | `FindNativeModuleByCache` 对 `name` 字段 nullptr 解引用 | 崩溃 | ✅ 修复 |
| #2 | `FindNativeModuleByCache` 对 `systemFilePath` nullptr 解引用 | 崩溃 | ✅ 修复 |
| #3 | `EmplaceModuleBuffer` 重复 key 的内存模型 | UAF | ✅ 修复（重新定性为 no-op） |
| #4 | `GetNativeModulePath` 用 `operator[]` 默认插入 nullptr + 缺锁 | UB/数据竞争 | ✅ 修复 |
| #5 | `SetAppLibPath` 空字符串调用 `back()` UB | UB | ✅ 修复 |
| #6 | `LoadNativeModule` `relativePath` 路径穿越 | 安全 | ✅ 修复 |
| #7 | `LoadGreylistConfig` lib 名注入绕过 namespace 隔离 | 安全 | ✅ 修复 |
| #8 | `UnloadNativeModule` TOCTOU 竞态导致双重释放/悬空句柄 | 堆破坏 | ✅ 修复 |
| #9 | `EmplaceModuleLib` 重复 key 句柄泄漏 | 泄漏 | ⚠️ 部分回退（接受原泄漏） |
| #10 | `strdup(loadPath)` 返回值未校验导致 OOM 崩溃 | 崩溃 | ✅ 修复 |
| #11 | `GetFileBuffer` 未校验 `tellg`/读字节数 | UB | ✅ 修复（fd 释放部分回退） |
| #12 | `CreateHead/TailNativeModule` 用 `new` 不抛错版本 | 崩溃 | ✅ 修复 |
| #13 | 删除未实现的 `NativeModuleManager::Release` 声明 | 死代码 | ✅ 修复 |
| #14 | 删除孤儿函数 `RemoveNativeModule` | 死代码 | ✅ 修复 |
| #15 | `GetModuleFileName` 成功路径误报 ERROR 日志 | 日志噪音 | ✅ 修复 |
| #16 | `GetInstance` 数据竞争（原始指针 + 无原子） | 数据竞争 | ✅ 修复 |
| #17 | 析构函数 `appLibPathMap_`/`nativeEngineList_` 改 RAII | 异常安全/死锁 | ✅ 修复 |
| #19 | 注释/日志拼写错误（`moudle`/`latter`/`isisSystemApp`） | 文档 | ✅ 修复 |
| #20 | `ClassifyLoadError` 字符串前缀剥离脆弱 | 错误分类退化 | ❌ **已回退**（留待单独 PR） |
| #22 | `GetFileBuffer` IO + `napi_onLoad` 加 HiTrace 片段 | 可观测性 | ✅ 修复 |
| —  | SE 评审第 1 轮：path 空指针、greylist substr 下溢 | 评审反馈 | ✅ 修复 |
| —  | SE 评审第 2 轮：死代码、UAF、风格 | 评审反馈 | ✅ 修复 |
| —  | 中文跟踪注释清理 | 风格 | ✅ 修复 |

---

## `module_manager/module_load_checker.h`

### Hunk 1 — 拼写修正
**问题**：#19（注释/日志拼写错误）
```diff
-     * @brief Set the moudle checker delegate
+     * @brief Set the module checker delegate
```
**说明**：`moudle` → `module`。

---

## `module_manager/native_module_manager.cpp`

### Hunk 1 — 头文件 include 新增（行 15-26）
**问题**：#6 / #7 / #11 / #12 / #16 配套
```diff
+#include <cctype>
+#include <climits>
+#include <new>
+// NAME_MAX 在 <climits> 中非可移植暴露，提供本地 fallback
+#ifndef NAME_MAX
+#define NAME_MAX 255
+#endif
```
**说明**：
- `<cctype>`：供 `IsSafeRelativePath` (#6) 与 `IsValidLibNameStrict` (#7) 用 `std::isalnum`
- `<climits>` + `NAME_MAX` fallback：供 `IsValidLibNameStrict` (#7) 长度上限（SE 评审第 2 轮指出 NAME_MAX 不可靠暴露）
- `<new>`：供 `new (std::nothrow)` (#12)

### Hunk 2 — `instance_` 类型改 atomic（行 97）
**问题**：#16
```diff
-NativeModuleManager* NativeModuleManager::instance_ = NULL;
+std::atomic<NativeModuleManager*> NativeModuleManager::instance_ { nullptr };
```

### Hunk 3 — 析构函数 RAII 化（行 140-160）
**问题**：#17
```diff
-    appLibPathMapMutex_.lock();
-    for (const auto& item : appLibPathMap_) { free(item.second); }
-    std::map<std::string, char*>().swap(appLibPathMap_);
-    appLibPathMapMutex_.unlock();
+    {
+        std::lock_guard<std::mutex> lock(appLibPathMapMutex_);
+        for (const auto& item : appLibPathMap_) { free(item.second); }
+        appLibPathMap_.clear();
+    }
+    // 持锁 swap 出，锁外销毁避免 NativeEngine 析构重入 nativeEngineListMutex_ 死锁
+    std::map<std::string, NativeEngine*> enginesToDestroy;
+    {
+        std::lock_guard<std::mutex> lock(nativeEngineListMutex_);
+        enginesToDestroy.swap(nativeEngineList_);
+    }
+    for (const auto& item : enginesToDestroy) {
+        if (item.second != nullptr) { delete item.second; }
+    }
```
**说明**：原代码 `lock/unlock` 裸调用 + `swap` 在锁内逐个 `delete`，存在异常安全与重入死锁问题。

### Hunk 4 — `GetInstance` 双检锁改 atomic（行 162-178）
**问题**：#16
```diff
-    if (instance_ == NULL) {
+    NativeModuleManager* ptr = instance_.load(std::memory_order_acquire);
+    if (ptr == nullptr) {
         std::lock_guard<std::mutex> lock(g_instanceMutex);
-        if (instance_ == NULL) {
-            instance_ = new NativeModuleManager();
+        ptr = instance_.load(std::memory_order_relaxed);
+        if (ptr == nullptr) {
+            ptr = new NativeModuleManager();
+            instance_.store(ptr, std::memory_order_release);
         }
     }
-    return instance_;
+    return ptr;
```
**说明**：acquire/release 配合 atomic 修复原 raw 指针 + 无同步的数据竞争。

### Hunk 5 — `EmplaceModuleLib` 重构（行 188-210）
**问题**：#9（⚠️ 部分回退）
```diff
-void NativeModuleManager::EmplaceModuleLib(std::string moduleKey, const LIBHANDLE lib)
+LIBHANDLE NativeModuleManager::EmplaceModuleLib(std::string moduleKey, const LIBHANDLE lib)
 {
     std::lock_guard<std::mutex> lock(moduleLibMutex_);
-    if (lib != nullptr) {
-        moduleLibMap_.emplace(moduleKey, lib);
+    if (lib == nullptr) {
+        return nullptr;
     }
-}
-/* RemoveModuleLib 整段删除（#14） */
+    auto it = moduleLibMap_.find(moduleKey);
+    if (it != moduleLibMap_.end()) {
+        MODULEMNG_HILOG_WARN(...);
+        // ⚠️ REVERT: 不再 dlclose 新句柄，保留原泄漏行为；LIBHANDLE 返回避免 UAF
+        return it->second;
+    }
+    moduleLibMap_.emplace(moduleKey, lib);
+    return lib;
+}
```
**说明**：
- 返回类型 `void → LIBHANDLE`：让 `LoadModuleLibrary` 调用方拿到规范 handle，避免 UAF（来自 SE 评审第 2 轮 #10）
- ⚠️ 原始 PR 在重复 key 分支调用了 `UnloadModuleLibrary(lib)`（dlclose），本次提交 `61746b9d` 已回退该 dlclose，恢复原泄漏行为（用户决策：风险优先于泄漏修复）

### Hunk 6 — `EmplaceModuleBuffer` 加注释（行 222-227）
**问题**：#3（重新定性）
```diff
+    // The ABC buffer's memory is owned by NativeModule::jsABCCode; moduleBufMap_ is just an
+    // index. std::map::emplace is a no-op on duplicate key, which matches the ownership
+    // model: replacing the map entry would free the still-in-use buffer of the first module
+    // (use-after-free via its jsABCCode pointer).
     if (lib != nullptr) {
         moduleBufMap_.emplace(moduleKey, lib);
     }
```
**说明**：SE 评审第 2 轮指出原 PR 想用 `delete[] + replace` 修泄漏，实际会造成 UAF（buffer 所有权在 `NativeModule::jsABCCode`，map 只是索引）。最终方案：保留 `emplace` no-op 行为 + 注释说明。

### Hunk 7 — `RemoveNativeModule` → `RemoveNativeModuleLocked`（行 257-296）
**问题**：#8（+ #14 删除孤儿函数）
```diff
-bool NativeModuleManager::RemoveNativeModule(const std::string& moduleKey)
+bool NativeModuleManager::RemoveNativeModuleLocked(const std::string& moduleKey)
 {
-    bool handleAbcRemoved = RemoveModuleBuffer(moduleKey);
-    bool handleRemoved = RemoveModuleLib(moduleKey);
-    bool moduleRemoved = RemoveNativeModuleByCache(moduleKey);
-    return ((handleRemoved || handleAbcRemoved) && moduleRemoved);
+    // Caller holds moduleLibMutex_ and nativeModuleListMutex_
+    bool bufferRemoved = RemoveModuleBuffer(moduleKey);
+    bool moduleRemoved = RemoveNativeModuleByCacheLocked(moduleKey);
+    return moduleRemoved;
 }

 bool NativeModuleManager::UnloadNativeModule(const std::string& moduleKey)
 {
-    LIBHANDLE handle = GetNativeModuleHandle(moduleKey);
-    if (handle == nullptr) { return false; }
-    if (RemoveNativeModule(moduleKey) == false) { return false; }
-    return UnloadModuleLibrary(handle);
+    LIBHANDLE handle = nullptr;
+    bool removed = false;
+    {
+        std::lock_guard<std::mutex> lk1(moduleLibMutex_);
+        std::lock_guard<std::mutex> lk2(nativeModuleListMutex_);
+        auto it = moduleLibMap_.find(moduleKey);
+        if (it == moduleLibMap_.end()) { return false; }
+        handle = it->second;
+        moduleLibMap_.erase(it);
+        removed = RemoveNativeModuleLocked(moduleKey);
+    }
+    if (!removed) { return false; }
+    return UnloadModuleLibrary(handle);   // dlclose 在锁外
 }
```
**说明**：
- 原 `UnloadNativeModule` 三步操作各自独立加锁，存在 TOCTOU 竞态 → 双重 dlclose / 悬空句柄
- 改为 `moduleLibMutex_ + nativeModuleListMutex_` 双锁临界区原子"查询-擦除-移除"，dlclose 在锁外
- `RemoveModuleLib` 整段删除（#14 无生产调用方）

### Hunk 8 — `GetModuleFileName` 日志级别修正（行 325）
**问题**：#15
```diff
-    MODULEMNG_HILOG_ERROR("failed");
+    MODULEMNG_HILOG_INFO("module:%{public}s, loadPath:%{public}s", moduleName, loadPath.c_str());
```
**说明**：成功返回路径误打 ERROR，改为 INFO 并带上现场信息。

### Hunk 9 — `Register` 注释拼写（行 375）
**问题**：#19
```diff
-        tailNativeModule_->moduleName = nullptr;  /* we update moduleName latter */
+        tailNativeModule_->moduleName = nullptr;  /* we update moduleName later */
```

### Hunk 10 — `CreateHeadNativeModule` 用 nothrow new（行 412-426）
**问题**：#12
```diff
-        headNativeModule_ = new NativeModule();
+        headNativeModule_ = new (std::nothrow) NativeModule();
         if (headNativeModule_ == nullptr) { ... }
-        auto head = new NativeModule();
+        auto head = new (std::nothrow) NativeModule();
```
**说明**：原本用 throw 版 `new`，下方 `if (head == nullptr)` 永远不成立（OOM 时直接抛异常）；改 `nothrow` 让判空分支有意义。

### Hunk 11 — `CreateTailNativeModule` 同上（行 435-449）
**问题**：#12（同 Hunk 10 对称改动）

### Hunk 12 — 删除孤儿函数 `IsValidLibName`（行 458-481）
**问题**：#7（替换为 `IsValidLibNameStrict`）+ SE 评审第 2 轮（删除未用 legacy helper）
```diff
-constexpr static int32_t SO_SUFFIX_LENGTH = 3;
-static bool IsValidLibName(const std::string& libName) { ... }
```
**说明**：原 `IsValidLibName` 只检查 `.so` 后缀且 `substr(size()-3)` 在 `size()<3` 时下溢；被新的 `IsValidLibNameStrict` 取代。

### Hunk 13 — `LoadGreylistConfig` 改调 `IsValidLibNameStrict`（行 516）
**问题**：#7
```diff
-        if (IsValidLibName(libName)) {
+        if (NativeModuleManager::IsValidLibNameStrict(libName)) {
```

### Hunk 14 — `SetAppLibPath` 日志拼写（行 750）
**问题**：#19
```diff
-    MODULEMNG_HILOG_DEBUG("moduleName is %{public}s, isisSystemApp is %{public}d", ...);
+    MODULEMNG_HILOG_DEBUG("moduleName is %{public}s, isSystemApp is %{public}d", ...);
```

### Hunk 15 — `SetAppLibPath` 空串保护（行 760）
**问题**：#5
```diff
-    if (tmpPath.back() == ':') {
+    if (!tmpPath.empty() && tmpPath.back() == ':') {
         tmpPath.pop_back();
     }
```
**说明**：`std::string::back()` 在空串上是 UB。

### Hunk 16 — `LoadNativeModule` 路径穿越校验（行 865-873）
**问题**：#6
```diff
-    if (relativePathStr.find("..") != std::string::npos) {
+    if (!IsSafeRelativePath(relativePathStr)) {
         SetLoadErrInfo(loadErrInfo, "invalid relativePath");
-        MODULEMNG_HILOG_ERROR("failed");
+        MODULEMNG_HILOG_ERROR("invalid relativePath: rejected by whitelist");
         return nullptr;
     }
```
**说明**：原 `find("..")` 误拦 `"my..pkg"` 等合法段名，且漏拦 `"/etc/passwd"`、`"a/./b"`；新白名单校验见 Hunk 24。

### Hunk 17 — `GetNativeModulePath` 改 `find` + 缺失锁（行 1060-1072）
**问题**：#4（+ SE 评审第 1 轮 path null-check）
```diff
-    if (isAppModule && IsExistedPath(path)) {
-        appLibPathMapMutex_.lock();
-        prefix = appLibPathMap_[path];
-        appLibPathMapMutex_.unlock();
+    if (isAppModule && path != nullptr) {
+        std::lock_guard<std::mutex> guard(appLibPathMapMutex_);
+        auto it = appLibPathMap_.find(path);
+        if (it != appLibPathMap_.end()) {
+            prefix = it->second;
+        }
+    }
+    if (prefix != nullptr) {
```
**说明**：
- 原代码 `appLibPathMap_[path]` 在 key 不存在时默认插入 nullptr（污染 map）
- 原代码 `lock/unlock` 裸调用，没有 RAII；改为 `lock_guard`
- SE 评审第 1 轮额外要求：`IsExistedPath(nullptr)` 内部 `std::string(nullptr)` 是 UB，故前置 `path != nullptr` 判空

### Hunk 18 — `LoadModuleLibrary` 返回规范 handle（行 1310-1320）
**问题**：#9（配套 Hunk 5）
```diff
-    EmplaceModuleLib(moduleKey, lib);
-    return lib;
+    // EmplaceModuleLib discards the duplicate handle (if any) and returns the canonical
+    // tracked handle; returning it here ensures callers never dereference a freed dlopen
+    // handle on duplicate-key paths.
+    return EmplaceModuleLib(moduleKey, lib);
```
**说明**：让 caller 拿到 map 内的规范 handle，避免重复 key 时拿到泄漏的新句柄。

### Hunk 19 — `GetFileBuffer` 加 `tellg`/size 校验（行 1325-1345）
**问题**：#11（⚠️ fd 释放部分已回退）
```diff
-    len = static_cast<size_t>(inFile.tellg());
+    std::streampos pos = inFile.tellg();
+    if (pos < 0) {
+        MODULEMNG_HILOG_ERROR("tellg failed, invalid file size: %{public}s", filePath.c_str());
+        len = 0;
+        return nullptr;
+    }
+    constexpr size_t maxAbcFileSize = 256 * 1024 * 1024; // 256MB
+    size_t fileSize = static_cast<size_t>(pos);
+    if (fileSize == 0 || fileSize > maxAbcFileSize) {
+        MODULEMNG_HILOG_ERROR("invalid abc file size: %{public}zu", fileSize);
+        len = 0;
+        return nullptr;
+    }
+    len = fileSize;
```
**说明**：⚠️ 原 PR 还在每个错误分支加了显式 `inFile.close()`，已被提交 `45f4803a` 回退（`std::ifstream` RAII 本就会关闭，显式 close 冗余）。

### Hunk 20 — `GetFileBuffer` 加读字节数校验 + HiTrace（行 1347-1373）
**问题**：#11（short read 校验）+ #22（HiTrace 片段）
```diff
-    std::unique_ptr<uint8_t[]> buffer = std::make_unique<uint8_t[]>(len);
-    inFile.seekg(0);
-    inFile.read(reinterpret_cast<char*>(buffer.get()), len);
+    std::unique_ptr<uint8_t[]> buffer;
+    {
+#ifdef ENABLE_HITRACE
+        StartTrace(HITRACE_TAG_ACE, "GetFileBuffer::read");
+#endif
+        buffer = std::make_unique<uint8_t[]>(len);
+        inFile.seekg(0);
+        inFile.read(reinterpret_cast<char*>(buffer.get()), len);
+#ifdef ENABLE_HITRACE
+        FinishTrace(HITRACE_TAG_ACE);
+#endif
+    }
+    if (static_cast<size_t>(inFile.gcount()) != len) {
+        MODULEMNG_HILOG_ERROR("short read: expected %{public}zu, got %{public}zu", ...);
+        len = 0;
+        return nullptr;
+    }
     inFile.close();
```
**说明**：
- HiTrace 片段：#22
- short read 校验（`gcount() != len`）：#11
- ⚠️ short read 分支原 PR 加的 `inFile.close()` 也已被 `45f4803a` 回退

### Hunk 21 — `Napi_onLoadCallback` 加 HiTrace（行 1404-1412）
**问题**：#22
```diff
     if (onLoadFunc != nullptr) {
+#ifdef ENABLE_HITRACE
+        StartTrace(HITRACE_TAG_ACE, "napi_onLoad");
+#endif
         onLoadFunc();
+#ifdef ENABLE_HITRACE
+        FinishTrace(HITRACE_TAG_ACE);
+#endif
     }
```

### Hunk 22 — `FindNativeModuleByDisk` `strdup` 返回值校验（行 1509-1517）
**问题**：#10
```diff
     tailNativeModule_->systemFilePath = strdup(loadPath);
+    if (tailNativeModule_->systemFilePath == nullptr) {
+        SetLoadErrInfo(loadErrInfo, "internal error: out of memory");
+        MODULEMNG_HILOG_ERROR("strdup systemFilePath failed");
+        free(const_cast<char*>(tailNativeModule_->moduleName));
+        tailNativeModule_->moduleName = nullptr;
+        SetLoadingNativeModuleKey("");
+        return nullptr;
+    }
```
**说明**：原代码 OOM 时 `systemFilePath` 保持 nullptr 继续往下走，可能 UB；新代码对称回滚 `moduleName` 并优雅降级返回。

### Hunk 23 — `RemoveNativeModuleByCache` → `*Locked`（行 1617-1623）
**问题**：#8
```diff
-bool NativeModuleManager::RemoveNativeModuleByCache(const std::string& moduleKey)
+bool NativeModuleManager::RemoveNativeModuleByCacheLocked(const std::string& moduleKey)
 {
-    std::lock_guard<std::mutex> lock(nativeModuleListMutex_);
+    // Caller holds nativeModuleListMutex_
```
**说明**：由 #8 引入的双锁临界区调用，避免重入死锁；公开 wrapper 在 SE 评审第 2 轮被删除（无生产调用方）。

### Hunk 24 — `FindNativeModuleByCache` nullptr 短路（行 1690-1705）
**问题**：#1（name）+ #2（systemFilePath）
```diff
     if ((temp->moduleName && !strcmp(temp->moduleName, moduleName))
-        || !strcasecmp(temp->name, moduleName)) {
+        || (temp->name != nullptr && !strcasecmp(temp->name, moduleName))) {
         int label = 0;
 #if !defined(ANDROID_PLATFORM) && !defined(IOS_PLATFORM)
-        while (label < NATIVE_PATH_NUMBER && strcmp(temp->systemFilePath, nativeModulePath[label])) {
+        while (label < NATIVE_PATH_NUMBER && temp->systemFilePath != nullptr
+               && strcmp(temp->systemFilePath, nativeModulePath[label])) {
             label++;
         }
 #endif
-        if (label < NATIVE_PATH_NUMBER || !strcmp(temp->systemFilePath, "")) {
+        if (label < NATIVE_PATH_NUMBER
+            || (temp->systemFilePath != nullptr && !strcmp(temp->systemFilePath, ""))) {
```
**说明**：两处 nullptr 解引用（`temp->name` / `temp->systemFilePath`）分别加短路。

### Hunk 25 — 新增 `IsSafeRelativePath`（行 1732-1756）
**问题**：#6
```cpp
static bool IsSafeRelativePath(const std::string& p) {
    if (p.empty()) return true;
    if (p.front() == '/') return false;            // 拒绝对路径
    std::stringstream ss(p);
    std::string seg;
    while (std::getline(ss, seg, '/')) {
        if (seg.empty() || seg == "." || seg == "..") return false;  // 拒绝 . / .. / //
        for (char c : seg) {
            if (!std::isalnum(...) && c != '_' && c != '.' && c != '-') return false;
        }
    }
    return true;
}
```
**说明**：白名单校验，拒绝绝对路径、`.`/`..` 段、空段、非白名单字符。SE 评审第 2 轮追加了 `c == '-'` 允许。

### Hunk 26 — 新增 `IsValidLibNameStrict`（行 1758-1779）
**问题**：#7
```cpp
static bool IsValidLibNameStrict(const std::string& libName) {
    if (libName.size() < 4 || libName.size() >= NAME_MAX) return false;
    if (libName.compare(libName.size() - 3, 3, ".so") != 0) return false;
    for (char c : libName) {
        if (!std::isalnum(...) && c != '.' && c != '_' && c != '-') return false;
    }
    return libName.find("...") == std::string::npos;
}
```
**说明**：
- 长度 < 4 直接拒绝（最短合法名 `"x.so"` = 4 字节），避免 `substr(size()-3)` 下溢
- 字符白名单拒绝含冒号/空格/斜杠等可绕过 namespace 隔离的字符

---

## `module_manager/native_module_manager.h`

### Hunk 1 — 加 `<atomic>` include（行 16）
**问题**：#16 配套
```diff
+#include <atomic>
```

### Hunk 2 — 删除 `static uint64_t Release()` 声明（行 83）
**问题**：#13
```diff
-    static uint64_t Release();
```
**说明**：声明存在但 cpp 中无实现，删除以避免误用。

### Hunk 3 — 私有成员声明重构（行 165-180）
**问题**：#6 / #7 / #8 / #9 / #14
```diff
-    void EmplaceModuleLib(const std::string moduleKey, LIBHANDLE lib);
-    bool RemoveModuleLib(const std::string moduleKey);
+    static bool IsSafeRelativePath(const std::string& p);
+    static bool IsValidLibNameStrict(const std::string& libName);
+    bool RemoveNativeModuleByCacheLocked(const std::string& moduleKey);
+    bool RemoveNativeModuleLocked(const std::string& moduleKey);
+    LIBHANDLE EmplaceModuleLib(const std::string moduleKey, LIBHANDLE lib);
...
-    bool RemoveNativeModuleByCache(const std::string& moduleKey);
-    bool RemoveNativeModule(const std::string& moduleKey);
```
**说明**：
- 删除 `RemoveModuleLib` / `RemoveNativeModuleByCache`（public wrapper）/ `RemoveNativeModule`（#14）
- 新增 `IsSafeRelativePath` / `IsValidLibNameStrict`（#6 / #7）
- 新增 `*Locked` 两个 helper（#8）
- `EmplaceModuleLib` 返回类型改 `LIBHANDLE`（#9）

### Hunk 4 — `instance_` 类型改 atomic（行 198）
**问题**：#16
```diff
-    static NativeModuleManager *instance_;
+    static std::atomic<NativeModuleManager*> instance_;
```

---

## `module_manager/test/unittest/module_manager_test/module_manager_test.cpp`

### 删除的旧测试用例（5 个）
**问题**：#14 配套（删除对应被删函数的测试）
- `LoadNativeModuleTest_012` / `_014` / `_017` — 依赖被删 API
- `RemoveNativeModuleByCache_ShouldHandleNullModuleName`
- `RemoveNativeModuleByCache_ShouldFreeMemoryWhenRemoveHead`
- `RemoveNativeModuleByCache_ShouldFreeMemoryWhenRemoveTail`
- `RemoveNativeModuleByCache_ShouldFreeMemoryWhenRemoveMiddle`
- `SetAppLibPath_ShouldFreeOldPathWhenSetTwice`

### 新增的测试用例（22 个）

| 测试用例 | 关联问题 |
|---|---|
| `FindNativeModuleByCache_WhenNodeNameIsNullptrShouldNotCrash` | #1 |
| `FindNativeModuleByCache_WhenNodeSystemFilePathIsNullptrShouldNotCrash` | #2 |
| `EmplaceModuleBuffer_ShouldKeepOriginalBufferOnDuplicateKey` | #3（重写为 no-op 断言） |
| `GetNativeModulePath_WhenAppLibPathValueIsNullptrShouldNotCrash` | #4 |
| `SetAppLibPath_WhenAppLibPathIsEmptyShouldNotCrash` | #5 |
| `IsSafeRelativePath_PathValidationTest` | #6 |
| `IsValidLibNameStrict_LibNameValidationTest` | #7 |
| `UnloadNativeModule_WhenModuleNotInLibMapShouldReturnFalseSafely` | #8 |
| `EmplaceModuleLib_ShouldNotEmplaceWhenLibIsNullptr` | #9 |
| `EmplaceModuleLib_ShouldEmplaceWhenKeyNotExists` | #9 |
| `EmplaceModuleLib_ShouldReturnCanonicalHandleOnDuplicateKey` | #9（重写：断言拿到规范 handle，无 UAF） |
| `GetFileBuffer_ShouldReturnBufferAndMatchLenForNormalFile` | #11 |
| `GetFileBuffer_ShouldReturnSamePointerOnSecondCallFromCache` | #11 |
| `GetFileBuffer_ShouldReturnNullptrWhenFileDoesNotExist` | #11 |
| `GetFileBuffer_ShouldReturnNullptrForEmptyFile` | #11 |
| `CreateHeadNativeModule_ShouldInitHeadAndTailWhenListEmpty` | #12 |
| `CreateHeadNativeModule_ShouldPrependNewHeadWhenListExists` | #12 |
| `CreateTailNativeModule_ShouldInitHeadAndTailWhenListEmpty` | #12 |
| `CreateTailNativeModule_ShouldAppendNewTailWhenListExists` | #12 |
| `GetInstance_ShouldReturnStablePointerAcrossCalls` | #16 |
| `GetInstance_ShouldReturnSamePointerUnderConcurrency` | #16 |
| `Destructor_ShouldClearNativeEngineListSafely` | #17 |

### 修改的测试用例（1 个）
| 用例 | 改动 | 关联问题 |
|---|---|---|
| `FindNativeModuleByDisk_ErrInfo_DlopenFailed` | #20 本加了一条 `EXPECT_EQ(loadErrInfo.find("dlopen failed: failed "), std::string::npos)`，已随 #20 回退删除 | — |

---

## 已回退的改动（最终 diff 中**不存在**，仅作记录）

| 回退提交 | 回退对象 | 原因 |
|---|---|---|
| `45f4803a` | `GetFileBuffer` 3 处错误分支的显式 `inFile.close()` | `std::ifstream` RAII 已自动关闭，显式 close 冗余；优先零风险 |
| `61746b9d` | `EmplaceModuleLib` 重复 key 路径的 `UnloadModuleLibrary(lib)`（dlclose） | dlclose 影响 refcount 有风险；接受原泄漏（保留 LIBHANDLE 返回避免 UAF） |
| `80e6d284` | `ClassifyLoadError` 结构化 raw error 修复（commit `5f40d01b`，问题 #20） | 该问题留待单独 PR 处理；恢复 `find("failed ")==0` 前缀剥离原行为 |

> ⚠️ 注意：问题 #20 在最终 diff 中**未修复**，行为与基线一致。后续将作为独立 PR 提交。

---

## commit → 问题映射表

| Commit | 类型 | 关联问题 |
|---|---|---|
| `ea3f4bc3` | fix（基线 commit） | #1 #2 #3 #4 #5 #6 #7 |
| `7b1b4033` | fix | #8 |
| `25abb290` | fix | #9 |
| `1e961b9f` | fix | #10 |
| `b76e138a` | fix | #11 |
| `4599b272` | fix | #12 |
| `3ba6dc68` | fix | #13 |
| `50952e34` | fix | #14 |
| `165d5a19` | fix | #15 |
| `47c88050` | fix | #16 |
| `0adb0d91` | fix | #17 |
| `a14a6e0e` | docs | #19 |
| `5f40d01b` | fix | #20（⚠️ 已被 `80e6d284` 回退） |
| `cf4a8e40` | perf | #22 |
| `3bf09446` | chore | 中文跟踪注释清理 |
| `66a4d309` | fix | SE 评审第 1 轮（path null-check / greylist substr 下溢） |
| `c2444add` | fix | SE 评审第 2 轮（死代码、UAF、NAME_MAX、风格） |
| `45f4803a` | revert | 回退 #11 的显式 close |
| `61746b9d` | revert | 回退 #9 的 dlclose |
| `80e6d284` | revert | 回退 #20 全部 |

---

## 行为变更摘要（对外可见）

| 改动 | 行为影响 |
|---|---|
| #1 / #2 | 原崩溃入参现在安全返回，行为兼容 |
| #3 | `EmplaceModuleBuffer` 重复 key：no-op（与原一致），加注释说明所有权模型 |
| #4 | `GetNativeModulePath` 不再默认插入 nullptr 到 map，加锁 |
| #5 | `SetAppLibPath` 空入参不再 UB |
| #6 | 路径校验双向调整：放宽 `"my..pkg"` 等合法段名；收紧 `"/etc/passwd"` / `"a/./b"` |
| #7 | lib 名接受规则收紧（拒绝含冒号/空格/斜杠等非白名单字符） |
| #8 | `UnloadNativeModule` 改双锁临界区 + 锁外 dlclose；cache 不一致时 handle 泄漏优于 TOCTOU heap corruption |
| #9 ⚠️ | 重复 key 时**保留原泄漏行为**（接受），但 caller 拿到规范 handle（避免 UAF） |
| #10 | OOM 路径优雅降级（回滚 + 返回 nullptr） |
| #11 ⚠️ | 错误检查保留（tellg/size/short-read），但**移除错误分支显式 close**（依赖 RAII） |
| #12 | `nothrow new` 让 OOM 判空分支真正生效 |
| #13 / #14 | 删除未用 API |
| #15 | 日志级别修正 |
| #16 | atomic 双检锁修复数据竞争 |
| #17 | 析构 RAII 化 + 锁外销毁避免死锁 |
| #19 | 拼写修正 |
| #20 ❌ | **未修复**（已回退，单独 PR） |
| #22 | 加 HiTrace 片段提升可观测性 |
