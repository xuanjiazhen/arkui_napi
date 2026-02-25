# NativeModuleManager 内存泄漏分析报告

**文件**: `module_manager/native_module_manager.cpp`
**分析日期**: 2026-02-06
**严重级别**: 高危

---

## 1. 概述

本文档详细分析了 `NativeModuleManager` 类中的内存泄漏问题，包括：
- 所有内存分配点（new, strdup, make_unique）
- 对应的释放点（delete, free）
- 已确认的内存泄漏问题
- 修复方案和代码建议

---

## 2. 内存分配与释放映射表

### 2.1 `new` 分配映射

| 行号 | 分配代码 | 变量/成员 | 释放位置 | 状态 |
|------|----------|-----------|----------|------|
| 121 | `new NativeModuleManager()` | instance_ | ❌ 未释放 | ⚠️ **单例泄漏** |
| 349 | `new NativeModule()` | headNativeModule_ | 85, 1436, 1459 | ✅ 正常 |
| 356 | `new NativeModule()` | head | 85, 1436, 1459 | ✅ 正常 |
| 372 | `new NativeModule()` | headNativeModule_ | 85, 1436, 1459 | ✅ 正常 |
| 379 | `new NativeModule()` | tail | 85, 1436, 1459 | ✅ 正常 |
| 545 | `new char[sharedLibsSonamesLength]` | sharedLibsSonames_ | 95, 549, 558 | ✅ 正常 |
| 1577 | `new char[resLdPermittedPath.size() + 1]` | tmpLdPermittedPath | 1580, 1588 | ⚠️ **部分泄漏** |

### 2.2 `strdup` 分配映射

| 行号 | 分配代码 | 变量/成员 | 释放位置 | 状态 |
|------|----------|-----------|----------|------|
| 296 | `strdup(tmpName.c_str())` | name | 74, 1426, 1449 | ✅ 正常 |
| 710 | `strdup(tmpPath.c_str())` | appLibPathMap_[moduleName] | 100, 718 | ✅ 正常 |
| 1316 | `strdup(moduleKey.c_str())` | tailNativeModule_->moduleName | 77, 1428, 1451 | ⚠️ **条件泄漏** |
| 1324 | `strdup(loadPath)` | tailNativeModule_->systemFilePath | 83, 1434, 1457 | ⚠️ **覆盖泄漏** |
| 1370 | `strdup(loadPath)` | tailNativeModule_->systemFilePath | 83, 1434, 1457 | ⚠️ **覆盖泄漏** |
| 1393 | `strdup(moduleKey.c_str())` | tailNativeModule_->moduleName | 77, 1428, 1451 | ✅ 正常 |
| 1399 | `strdup(moduleName)` | tailNativeModule_->name | 74, 1426, 1449 | ✅ 正常 |

### 2.3 `make_unique` 分配映射

| 行号 | 分配代码 | 变量 | 释放位置 | 状态 |
|------|----------|------|----------|------|
| 1207 | `std::make_unique<uint8_t[]>(len)` | buffer | 1211 (release) | ✅ 正常 |
| 1211 | `buffer.release()` | lib | ❌ 未释放 | ⚠️ **map泄漏** |

---

## 3. 已确认的内存泄漏问题

### 🔴🔴 问题10: 多处指针未判空导致空指针解引用崩溃

**严重性**: 🔴🔴 **极其严重** - 导致崩溃、未定义行为

**问题描述**:

代码中存在多处使用指针前未判空的情况，可能导致空指针解引用崩溃。

#### 场景1: loadPath 未判空（Line 1282-1286）

```cpp
// Line 1281-1286
char* loadPath = nativeModulePath[0];  // ⚠️ 未判空直接使用
MODULEMNG_HILOG_DEBUG("moduleName:%{public}s. path:%{public}s", moduleName, loadPath);  // ❌ 空指针崩溃
LIBHANDLE lib = LoadModuleLibrary(moduleKey, loadPath, path, isAppModule, errInfo, errReason0);  // ❌ 传递空指针
```

**修复**:
```cpp
char* loadPath = nativeModulePath[0];
if (loadPath == nullptr || loadPath[0] == '\0') {
    MODULEMNG_HILOG_ERROR("loadPath is null or empty");
    return nullptr;
}
```

#### 场景2: loadPath 用于 strdup 前未判空（Line 1333, 1379）

```cpp
tailNativeModule_->systemFilePath = strdup(loadPath);  // ❌ loadPath 可能为 nullptr
```

**修复**:
```cpp
if (loadPath != nullptr && loadPath[0] != '\0') {
    tailNativeModule_->systemFilePath = strdup(loadPath);
}
```

#### 场景3: prefix 未判空（Line 970, 1009, 1023-1024, 1028-1029, 1046, 1060）

```cpp
const char* prefix = nullptr;
if (isAppModule && IsExistedPath(path)) {
    appLibPathMapMutex_.lock();
    prefix = appLibPathMap_[path];  // ⚠️ 可能返回 nullptr
    appLibPathMapMutex_.unlock();
}

std::string prefixStr(prefix);  // ❌ nullptr 崩溃
sprintf_s(nativeModulePath[0], pathLength, "%s/%s", prefix, dupModuleName);  // ❌ 未定义行为
```

**修复**:
```cpp
if (prefix == nullptr || prefix[0] == '\0') {
    MODULEMNG_HILOG_ERROR("prefix is null or empty for path: %{public}s", path);
    return false;
}
```

#### 场景4: 成员指针未判空（Line 896-897）

```cpp
if (!cacheHeadNativeModule || !cacheTailNativeModule || !headNativeModule_ || !tailNativeModule_) {
    return true;
}
// ❌ 未检查 name 成员
if (strcmp(cacheHeadNativeModule->name, headNativeModule_->name) != 0 ||
    strcmp(cacheTailNativeModule->name, tailNativeModule_->name) != 0) {
    return true;
}
```

**修复**:
```cpp
if (!cacheHeadNativeModule || !cacheTailNativeModule || !headNativeModule_ || !tailNativeModule_ ||
    !cacheHeadNativeModule->name || !cacheTailNativeModule->name ||
    !headNativeModule_->name || !tailNativeModule_->name) {
    return true;
}
```

#### 场景5: RemoveNativeModuleByCache 中 moduleName 成员未判空（Line 1430, 1453）

```cpp
// Line 1430
if (!strcasecmp(nativeModule->moduleName, moduleKey.c_str())) {
    // ❌ nativeModule->moduleName 未判空
}

// Line 1453
if (!strcasecmp(curr->moduleName, moduleKey.c_str())) {
    // ❌ curr->moduleName 未判空
}
```

**风险分析**:
- `strcasecmp(nullptr, ...)` 会导致**立即崩溃**
- 函数检查了 `nativeModule` 和 `curr` 是否为空，但未检查其成员 `moduleName`
- 如果 `moduleName` 为 `nullptr`（例如模块注册失败或数据损坏），调用 `strcasecmp` 会触发段错误

**修复**:
```cpp
// Line 1430
if (nativeModule->moduleName != nullptr &&
    !strcasecmp(nativeModule->moduleName, moduleKey.c_str())) {
    // ...
}

// Line 1453
if (curr->moduleName != nullptr &&
    !strcasecmp(curr->moduleName, moduleKey.c_str())) {
    // ...
}
```

#### 场景6: strcmp 未检查 moduleName 参数（Line 307）

```cpp
// Line 301-307
char *moduleName = strdup(tmpName.c_str());  // 可能返回 nullptr
// ...
if (g_isLoadingModule || !strcmp(loadingModuleName_.c_str(), moduleName)) {
    // ❌ moduleName 未检查就直接传给 strcmp
}
```

**风险**: `strcmp(nullptr, ...)` 导致立即崩溃

**修复**:
```cpp
if (moduleName != nullptr &&
    (g_isLoadingModule || !strcmp(loadingModuleName_.c_str(), moduleName))) {
```

#### 场景7: dlns_get 返回的 name 字段未检查（Line 610, 617, 636, 642, 647, 651）

```cpp
// Line 610
if (strlen(ndk_ns.name) > 0) {  // ❌ ndk_ns.name 可能为 nullptr

// Line 617
if (dlns_get(PASS_THROUGH, &passthrough_ns) == 0 && strlen(passthrough_ns.name) > 0) {
    // ❌ 即使 dlns_get 成功，passthrough_ns.name 也可能为 nullptr

// Line 647
if (dlns_get(PASSTHROUGH_INDIRECT, &passthrough_indirect_ns) == 0 &&
    strlen(passthrough_indirect_ns.name) > 0) {  // ❌ 未检查 name 是否为 nullptr
```

**风险**: `dlns_get` 可能成功但返回的 `name` 字段为 `nullptr`，导致 `strlen` 崩溃

**修复**:
```cpp
// Line 610
if (dlns_get(NDK, &ndk_ns) == 0 && ndk_ns.name != nullptr && strlen(ndk_ns.name) > 0) {

// Line 617
Dl_namespace passthrough_ns;
if (dlns_get(PASS_THROUGH, &passthrough_ns) == 0 &&
    passthrough_ns.name != nullptr && strlen(passthrough_ns.name) > 0) {

// Line 647
Dl_namespace passthrough_indirect_ns;
if (dlns_get(PASSTHROUGH_INDIRECT, &passthrough_indirect_ns) == 0 &&
    passthrough_indirect_ns.name != nullptr && strlen(passthrough_indirect_ns.name) > 0) {
```

#### 场景8: strcmp 未检查完整字段（Line 1334）

```cpp
// Line 1334
if (tailNativeModule_->name && strcmp(tailNativeModule_->moduleName, tailNativeModule_->name)) {
    // ❌ 检查了 name 但未检查 moduleName
}
```

**风险**: `tailNativeModule_->moduleName` 为 `nullptr` 时崩溃

**修复**:
```cpp
if (tailNativeModule_->name && tailNativeModule_->moduleName &&
    strcmp(tailNativeModule_->moduleName, tailNativeModule_->name)) {
```

#### 场景9: FindNativeModuleByCache 中指针未检查（Line 1494-1495, 1498, 1502）

```cpp
// Line 1494-1495
if ((temp->moduleName && !strcmp(temp->moduleName, moduleName))
    || !strcasecmp(temp->name, moduleName)) {  // ❌ temp->name 未检查

// Line 1498
while (label < NATIVE_PATH_NUMBER && strcmp(temp->systemFilePath, nativeModulePath[label])) {
    // ❌ temp->systemFilePath 未检查

// Line 1502
if (label < NATIVE_PATH_NUMBER || !strcmp(temp->systemFilePath, "")) {
    // ❌ temp->systemFilePath 未检查
```

**修复**:
```cpp
// Line 1494-1495
if ((temp->moduleName != nullptr && !strcmp(temp->moduleName, moduleName)) ||
    (temp->name != nullptr && !strcasecmp(temp->name, moduleName))) {

// Line 1498
while (label < NATIVE_PATH_NUMBER &&
       (temp->systemFilePath == nullptr || strcmp(temp->systemFilePath, nativeModulePath[label]))) {
    label++;
}

// Line 1502
if (label < NATIVE_PATH_NUMBER ||
    (temp->systemFilePath != nullptr && !strcmp(temp->systemFilePath, ""))) {
```

#### 场景10: strlen 未检查函数参数（Line 941）

```cpp
// Line 941
int32_t lengthOfModuleName = strlen(moduleName);  // ❌ moduleName 未检查
```

**风险**: 调用方传递 `nullptr` 参数时崩溃

**修复**: 在函数入口处添加
```cpp
bool NativeModuleManager::GetNativeModulePath(const char* moduleName, ...)
{
    if (moduleName == nullptr) {
        MODULEMNG_HILOG_ERROR("moduleName is nullptr");
        return false;
    }
    int32_t lengthOfModuleName = strlen(moduleName);
    // ...
}
```

#### 场景11: relativePath 数组访问未检查（Line 987）

```cpp
// Line 987
if (relativePath[0]) {  // ❌ relativePath 未检查是否为 nullptr
```

**风险**: `relativePath` 为 `nullptr` 时访问 `[0]` 导致崩溃

**修复**:
```cpp
if (relativePath != nullptr && relativePath[0]) {
```

#### 场景12: strdup 返回值未检查（Line 1333, 1379）

```cpp
// Line 1333, 1379
tailNativeModule_->systemFilePath = strdup(loadPath);  // ❌ strdup 可能返回 nullptr
```

**风险**: 内存不足时 `strdup` 返回 `nullptr`，导致后续代码访问空指针

**修复**:
```cpp
char* filePath = strdup(loadPath);
if (filePath == nullptr) {
    MODULEMNG_HILOG_ERROR("strdup failed for systemFilePath");
    // 清理已分配的内存
    return nullptr;
}
tailNativeModule_->systemFilePath = filePath;
```

#### 场景13: 日志宏中的指针访问（Line 292）

```cpp
// Line 292
MODULEMNG_HILOG_DEBUG("native module name is '%{public}s'", nativeModule->name);
    // ❌ nativeModule->name 可能为 nullptr
```

**风险**: 某些日志实现可能无法正确处理 `nullptr`，导致崩溃或输出异常

**修复**:
```cpp
MODULEMNG_HILOG_DEBUG("native module name is '%{public}s'",
                       nativeModule->name ? nativeModule->name : "<null>");
```

**总修复位置**: **30+ 处** 指针未判空

---

### 🔴🔴 问题9: strdup/new[]/free/delete 严重配对错误（未定义行为）

**位置**: 多处 - 析构函数和删除函数

**严重性**: 🔴🔴 **极其严重** - 导致未定义行为、崩溃、堆损坏

**问题描述**:

代码中存在严重的内存管理配对错误：**使用 `strdup`（malloc）分配内存，但用 `delete[]` 释放**。

**分配点（全部使用 `strdup`）**:
```cpp
// Line 301 - Register()
char *moduleName = strdup(tmpName.c_str());  // ⚠️ malloc 分配
// ...
tailNativeModule_->name = moduleName;  // Line 317/339

// Line 1325 - FindNativeModuleByDisk()
const char* moduleName = strdup(moduleKey.c_str());  // ⚠️ malloc 分配
// ...
tailNativeModule_->moduleName = moduleName;  // Line 1332

// Line 1402 - RegisterByBuffer()
char *moduleName = strdup(moduleKey.c_str());  // ⚠️ malloc 分配
// ...
tailNativeModule_->moduleName = moduleName;  // Line 1407

// Line 1408 - RegisterByBuffer()
tailNativeModule_->name = strdup(moduleName);  // ⚠️ malloc 分配
```

**释放点（全部使用 `delete[]`）**:
```cpp
// 析构函数 Line 70-96
~NativeModuleManager() {
    // ...
    if (headNativeModule_->name) {
        delete[] headNativeModule_->name;  // ❌ 错误！应该用 free()
    }
    if (headNativeModule_->moduleName) {
        delete[] headNativeModule_->moduleName;  // ❌ 错误！应该用 free()
    }
    // ...
}

// RemoveNativeModuleByCache Line 1435-1437
delete[] nativeModule->name;  // ❌ 错误！应该用 free()
if (nativeModule->moduleName) {
    delete[] nativeModule->moduleName;  // ❌ 错误！应该用 free()
}

// Line 1458-1460
delete[] curr->name;  // ❌ 错误！应该用 free()
if (curr->moduleName) {
    delete[] curr->moduleName;  // ❌ 错误！应该用 free()
}
```

**错误分析**:

| 分配函数 | 内部实现 | 正确释放 | 当前释放 | 结果 |
|---------|---------|---------|---------|------|
| `strdup()` | `malloc()` | `free()` | `delete[]` | ❌ **未定义行为** |
| `malloc()` | 堆分配 | `free()` | `delete[]` | ❌ **未定义行为** |
| `new[]` | 堆分配 | `delete[]` | - | ✅ 正确 |

**影响**:

使用 `delete[]` 释放 `malloc` 分配的内存会导致：

1. **未定义行为（Undefined Behavior）**
   - 可能立即崩溃
   - 可能静默损坏堆
   - 可能偶尔工作（最危险的情况）

2. **堆损坏**
   - 堆管理结构被破坏
   - 后续内存操作失败
   - 难以调试的间歇性崩溃

3. **内存泄漏**
   - `delete[]` 不会调用 `free()`
   - `malloc` 分配的内存块永远不会被释放
   - 长期运行导致内存耗尽

4. **安全漏洞**
   - 堆喷射攻击面
   - 可能被利用执行任意代码

**为什么这是最严重的问题**:

- ❌ **影响所有 NativeModule 的 name 和 moduleName 字段**
- ❌ **每次模块加载/卸载都会触发**
- ❌ **未定义行为难以调试和复现**
- ❌ **可能导致整个进程崩溃**

**修复方案**:

**方案A**: 将所有释放改为 `free()`
```cpp
// 析构函数
~NativeModuleManager() {
    std::lock_guard<std::mutex> lock(nativeModuleListMutex_);
    NativeModule* nativeModule = headNativeModule_;
    while (nativeModule != nullptr) {
        nativeModule = nativeModule->next;
        if (headNativeModule_->name) {
            free(const_cast<char*>(headNativeModule_->name));  // ✅ 修复
        }
        if (headNativeModule_->moduleName) {
            free(const_cast<char*>(headNativeModule_->moduleName));  // ✅ 修复
        }
        // ... 其他清理
        delete headNativeModule_;
        headNativeModule_ = nativeModule;
    }
    // ...
}

// RemoveNativeModuleByCache
if (!strcasecmp(nativeModule->moduleName, moduleKey.c_str())) {
    // ...
    free(const_cast<char*>(nativeModule->name));  // ✅ 修复
    if (nativeModule->moduleName) {
        free(const_cast<char*>(nativeModule->moduleName));  // ✅ 修复
    }
    // ...
}

// 同样修复 Line 1458-1460
```

**方案B**: 将分配改为 `new[]`（不推荐）
```cpp
// 需要修改所有 strdup 为 new[]
char* moduleName = new char[strlen(tmpName.c_str()) + 1];
strcpy_s(moduleName, strlen(tmpName.c_str()) + 1, tmpName.c_str());
// ...
```

**推荐**: **方案A**，因为 `strdup` 已经广泛使用，只需要修改释放方式。

**修复位置**（共 8 处）:
1. Line 79: 析构函数中的 `headNativeModule_->name`
2. Line 82: 析构函数中的 `headNativeModule_->moduleName`
3. Line 105: 析构函数中的 `appLibPathMap_` 值
4. Line 727: `SetAppLibPath` 中的 `appLibPathMap_[moduleName]`
5. Line 1435: `RemoveNativeModuleByCache` 中的 `nativeModule->name`
6. Line 1437: `RemoveNativeModuleByCache` 中的 `nativeModule->moduleName`
7. Line 1458: 循环中的 `curr->name`
8. Line 1460: 循环中的 `curr->moduleName`

**测试建议**:
1. 使用 Valgrind 检测内存分配/释放配对
2. 使用 AddressSanitizer 检测无效释放
3. 压力测试模块加载/卸载循环

---

### 🔴 问题1: FindNativeModuleByDisk - moduleName 泄漏

**位置**: [native_module_manager.cpp:1316-1323](native_module_manager.cpp#L1316-L1323)

**问题描述**:
```cpp
// Line 1316: 分配内存
const char* moduleName = strdup(moduleKey.c_str());

// Line 1317: 检查是否成功
if (moduleName == nullptr) {
    MODULEMNG_HILOG_ERROR("strdup faied");
    SetLoadingNativeModuleKey("");
    return nullptr;  // ✅ 这里正确处理了 null 情况
}

// Line 1323: 使用 moduleName
tailNativeModule_->moduleName = moduleName;
```

**泄漏场景**:
如果 `tailNativeModule_` 为 `nullptr`（理论上不应发生，但代码未断言），则 `moduleName` 会泄漏。

**严重性**: 🟡 中等（理论上存在，实际罕见）

**修复方案**:
```cpp
// 在 Line 1314 后添加断言
std::lock_guard<std::mutex> lock(nativeModuleListMutex_);
if (!tailNativeModule_) {
    MODULEMNG_HILOG_ERROR("tailNativeModule_ is null");
    SetLoadingNativeModuleKey("");
    return nullptr;
}
if (!abcBuffer) {
    const char* moduleName = strdup(moduleKey.c_str());
    if (moduleName == nullptr) {
        MODULEMNG_HILOG_ERROR("strdup faied");
        SetLoadingNativeModuleKey("");
        return nullptr;
    }
    // ... 继续正常流程
}
```

---

### 🔴 问题2: FindNativeModuleByDisk - systemFilePath 覆盖泄漏

**位置**: [native_module_manager.cpp:1314-1370](native_module_manager.cpp#L1314-L1370)

**问题描述**:
```cpp
// Line 1314: 加锁
std::lock_guard<std::mutex> lock(nativeModuleListMutex_);

// Line 1315: 检查条件
if (tailNativeModule_ && !abcBuffer) {
    // Line 1324: 第一次分配
    tailNativeModule_->systemFilePath = strdup(loadPath);
    // ...
}

// ... 中间代码

// Line 1368-1371: 另一个分支
} else {
    RegisterByBuffer(moduleKey, abcBuffer, len);
    tailNativeModule_->systemFilePath = strdup(loadPath);  // ⚠️ 可能覆盖旧值！
}
```

**泄漏场景**:
1. 如果进入 Line 1324 分支，分配了 `systemFilePath`
2. 后续代码再次调用 `strdup(loadPath)` 覆盖该值
3. 原有的 `systemFilePath` 内存未释放

**严重性**: 🔴 高危（多线程环境下可能触发）

**修复方案**:
```cpp
// 在分配前检查并释放旧值
if (tailNativeModule_->systemFilePath && tailNativeModule_->systemFilePath[0] != '\0') {
    free(const_cast<char*>(tailNativeModule_->systemFilePath));
}
tailNativeModule_->systemFilePath = strdup(loadPath);
if (tailNativeModule_->systemFilePath == nullptr) {
    MODULEMNG_HILOG_ERROR("strdup failed for systemFilePath");
    SetLoadingNativeModuleKey("");
    return nullptr;
}
```

需要修复的位置：
- Line 1324
- Line 1370

---

### 🔴 问题3: RegisterByBuffer - name 覆盖泄漏

**位置**: [native_module_manager.cpp:1398-1399](native_module_manager.cpp#L1398-L1399)

**问题描述**:
```cpp
// Line 1393: 分配 moduleName
char *moduleName = strdup(moduleKey.c_str());

// Line 1398-1399: 覆盖 name
tailNativeModule_->name = strdup(moduleName);
```

**分析**:
- `tailNativeModule_` 是新创建的，`name` 初始为 `nullptr`
- Line 1399 分配新值前未检查 `name` 是否已有值
- 虽然当前流程下 `name` 为空，但代码不健壮

**严重性**: 🟡 中等（当前不会泄漏，但代码不健壮）

**修复方案**:
```cpp
if (tailNativeModule_->name) {
    delete[] tailNativeModule_->name;
}
tailNativeModule_->name = strdup(moduleName);
if (tailNativeModule_->name == nullptr) {
    MODULEMNG_HILOG_ERROR("strdup failed for name");
    free(moduleName);
    tailNativeModule_->moduleName = nullptr;
    return;
}
```

---

### 🔴 问题4: RemoveNativeModuleByCache - jsABCCode 泄漏

**位置**: [native_module_manager.cpp:1430-1431, 1453-1454](native_module_manager.cpp#L1430-L1431)

**问题描述**:
```cpp
// Line 1430-1431: 删除 jsABCCode
if (nativeModule->jsABCCode) {
    delete[] nativeModule->jsABCCode;  // ⚠️ 释放内存
}

// 但是 moduleBufMap_ 中的同一块内存未释放！
// Line 172: EmplaceModuleBuffer 保存了相同的指针
// moduleBufMap_[moduleKey] = lib;  // lib 指向 jsABCCode

// Line 182-192: RemoveModuleBuffer 只 erase，未 delete
bool NativeModuleManager::RemoveModuleBuffer(const std::string moduleKey)
{
    // ...
    auto it = moduleBufMap_.find(moduleKey);
    if (it != moduleBufMap_.end()) {
        moduleBufMap_.erase(it);  // ❌ 只删除 map entry，未释放内存！
        deleted = true;
    }
    return deleted;
}
```

**所有权问题**:
- `NativeModule::jsABCCode` 和 `moduleBufMap_[key]` 指向**同一块内存**
- Line 1431 释放了 `jsABCCode`
- 但 `moduleBufMap_` 中仍保留该指针（已变为悬空指针）
- 如果后续访问 `moduleBufMap_`，会使用已释放的内存（**悬空指针**）
- 或者如果未释放 `jsABCCode`，则内存泄漏

**严重性**: 🔴 **高危**（双重释放或内存泄漏）

**修复方案**:

**方案A**: 明确所有权归 NativeModule，修改 RemoveModuleBuffer
```cpp
bool NativeModuleManager::RemoveModuleBuffer(const std::string moduleKey)
{
    std::lock_guard<std::mutex> lock(moduleBufMutex_);
    auto it = moduleBufMap_.find(moduleKey);
    if (it != moduleBufMap_.end()) {
        // ❌ 不要在这里释放，所有权归 NativeModule
        // delete[] it->second;
        moduleBufMap_.erase(it);
        deleted = true;
    }
    return deleted;
}
```

**方案B**: 明确所有权归 moduleBufMap_，修改 RemoveNativeModuleByCache
```cpp
bool NativeModuleManager::RemoveNativeModuleByCache(const std::string& moduleKey)
{
    // ...
    if (nativeModule->jsABCCode) {
        // ❌ 不要在这里释放，由 RemoveModuleBuffer 处理
        // delete[] nativeModule->jsABCCode;
        nativeModule->jsABCCode = nullptr;  // ✅ 仅置空
    }
    // 调用 RemoveModuleBuffer 释放内存
    RemoveModuleBuffer(moduleKey);
    // ...
}
```

**推荐**: **方案B**，因为 moduleBufMap_ 设计用于管理缓冲区生命周期。

同时修改 EmplaceModuleBuffer：
```cpp
void NativeModuleManager::EmplaceModuleBuffer(const std::string moduleKey, const uint8_t* lib)
{
    std::lock_guard<std::mutex> lock(moduleBufMutex_);
    // 检查是否已存在，避免泄漏
    auto it = moduleBufMap_.find(moduleKey);
    if (it != moduleBufMap_.end() && it->second != lib) {
        delete[] it->second;  // 释放旧缓冲区
    }
    if (lib != nullptr) {
        moduleBufMap_.emplace(moduleKey, lib);
    }
}
```

---

### 🔴 问题5: SetLdPermittedPathsForNamespace - 错误路径泄漏

**位置**: [native_module_manager.cpp:1577-1587](native_module_manager.cpp#L1577-L1587)

**问题描述**:
```cpp
// Line 1577: 分配内存
char* tmpLdPermittedPath = new char[resLdPermittedPath.size() + 1];
if (strcpy_s(tmpLdPermittedPath, resLdPermittedPath.size() + 1, resLdPermittedPath.c_str()) != 0) {
    MODULEMNG_HILOG_ERROR("strcpy_s ldPermittedPath '%{public}s' failed", resLdPermittedPath.c_str());
    delete[] tmpLdPermittedPath;  // ✅ 这里正确释放了
    return;
}

// Line 1583: 调用 API
if (dlns_set_ld_permitted_path(tmpLdPermittedPath, &ns) != 0) {
    MODULEMNG_HILOG_ERROR(
        "dlns_set_ld_permitted_path failed, ldPermittedPath = '%{public}s', nsName = '%{public}s'",
        tmpLdPermittedPath, nsName.c_str());
    // ❌ 泄漏！未释放 tmpLdPermittedPath
}
delete[] tmpLdPermittedPath;  // ✅ 成功路径正确释放
```

**严重性**: 🔴 高危（API 失败时泄漏）

**修复方案**:
```cpp
if (dlns_set_ld_permitted_path(tmpLdPermittedPath, &ns) != 0) {
    MODULEMNG_HILOG_ERROR(
        "dlns_set_ld_permitted_path failed, ldPermittedPath = '%{public}s', nsName = '%{public}s'",
        tmpLdPermittedPath, nsName.c_str());
    delete[] tmpLdPermittedPath;  // ✅ 修复：添加释放
    return;  // ✅ 添加 return
}
```

---

### 🟡 问题6: 单例 instance_ 未释放

**位置**: [native_module_manager.cpp:116-126](native_module_manager.cpp#L116-L126)

**问题描述**:
```cpp
NativeModuleManager* NativeModuleManager::GetInstance()
{
    if (instance_ == NULL) {
        std::lock_guard<std::mutex> lock(g_instanceMutex);
        if (instance_ == NULL) {
            instance_ = new NativeModuleManager();  // Line 121: 分配
            MODULEMNG_HILOG_DEBUG("create native module manager instance");
        }
    }
    return instance_;
}
```

**问题**:
- `instance_` 是单例，进程生命周期内一直存在
- **声明了 `Release()` 函数**（Line 85 头文件），但**未实现**
- 单例通常不释放，这是设计意图
- 但如果调用 `Release()`，会泄漏 `instance_`

**严重性**: 🟢 低（设计意图，但文档应说明）

**建议**:
1. **实现 `Release()` 函数**：
```cpp
uint64_t NativeModuleManager::Release()
{
    std::lock_guard<std::mutex> lock(g_instanceMutex);
    if (instance_ != nullptr) {
        delete instance_;
        instance_ = nullptr;
        return 0;  // 返回引用计数或其他指标
    }
    return UINT64_MAX;
}
```

2. **或者在文档中明确说明**：单例进程生命周期不释放，`Release()` 仅用于测试场景

---

## 4. 修复优先级和实施方案

### 优先级 P0（立即修复 - 致命错误）

| 问题编号 | 问题描述 | 风险 | 工作量 |
|---------|---------|------|--------|
| **问题9** | **strdup/delete[] 配对错误** | **未定义行为/崩溃** | **1小时** |
| **问题10** | **指针未判空** | **空指针崩溃** | **2小时** |

### 优先级 P1（立即修复 - 严重内存泄漏）

| 问题编号 | 问题描述 | 风险 | 工作量 |
|---------|---------|------|--------|
| 问题8 | sprintf_s 错误路径泄漏 | 严重内存泄漏 | 1小时 |
| 问题4 | jsABCCode 双重所有权 | 悬空指针/崩溃 | 2小时 |
| 问题5 | tmpLdPermittedPath 错误路径泄漏 | 内存泄漏 | 10分钟 |
| 问题2 | systemFilePath 覆盖泄漏 | 内存泄漏 | 30分钟 |
| Line 1379 | RegisterByBuffer 后的 systemFilePath 覆盖 | 内存泄漏 | 20分钟 |

### 优先级 P2（尽快修复）

| 问题编号 | 问题描述 | 风险 | 工作量 |
|---------|---------|------|--------|
| 问题1 | moduleName 条件泄漏 | 内存泄漏（罕见） | 30分钟 |
| 问题3 | name 覆盖泄漏 | 代码健壮性 | 20分钟 |
| 问题7 | getJSCode buf 所有权不明确 | 潜在泄漏 | 2小时（需调查） |

### 优先级 P3（建议修复）

| 问题编号 | 问题描述 | 风险 | 工作量 |
|---------|---------|------|--------|
| 问题6 | 单例 Release 未实现 | 设计完整性 | 1小时 |

---

## 5. 完整修复代码

### 5.0 修复问题9: strdup/delete[] 配对错误（最优先）

```cpp
// ========== 修复析构函数 ==========
// 文件: native_module_manager.cpp
// 位置: Line 70-96

NativeModuleManager::~NativeModuleManager()
{
    MODULEMNG_HILOG_INFO("enter");
    {
        std::lock_guard<std::mutex> lock(nativeModuleListMutex_);
        NativeModule* nativeModule = headNativeModule_;
        while (nativeModule != nullptr) {
            nativeModule = nativeModule->next;
            if (headNativeModule_->name) {
                free(const_cast<char*>(headNativeModule_->name));  // ✅ 修复
            }
            if (headNativeModule_->moduleName) {
                free(const_cast<char*>(headNativeModule_->moduleName));  // ✅ 修复
            }
            if (headNativeModule_->jsABCCode) {
                delete[] headNativeModule_->jsABCCode;
            }
            if (headNativeModule_->systemFilePath && headNativeModule_->systemFilePath[0] != '\0') {
                free(const_cast<char*>(headNativeModule_->systemFilePath));
            }
            delete headNativeModule_;
            headNativeModule_ = nativeModule;
        }
        headNativeModule_ = nullptr;
        tailNativeModule_ = nullptr;
    }

#if !defined(WINDOWS_PLATFORM) && !defined(MAC_PLATFORM) && !defined(__BIONIC__) && !defined(IOS_PLATFORM) && \
    !defined(LINUX_PLATFORM)
    if (sharedLibsSonames_) {
        delete[] sharedLibsSonames_;
    }
#endif
    appLibPathMapMutex_.lock();
    for (const auto& item : appLibPathMap_) {
        free(item.second);  // ✅ 修复：strdup 分配的用 free 释放
    }
    std::map<std::string, char*>().swap(appLibPathMap_);
    appLibPathMapMutex_.unlock();

    while (nativeEngineList_.size() > 0) {
        NativeEngine* wraper = nativeEngineList_.begin()->second;
        if (wraper != nullptr) {
            delete wraper;
            wraper = nullptr;
        }
        nativeEngineList_.erase(nativeEngineList_.begin());
    }
    pthread_mutex_destroy(&mutex_);
}

// ========== 修复 SetAppLibPath ==========
// 位置: Line 702-732

void NativeModuleManager::SetAppLibPath(const std::string& moduleName, const std::vector<std::string>& appLibPath,
                                        const bool& isSystemApp)
{
    MODULEMNG_HILOG_DEBUG("moduleName is %{public}s, isisSystemApp is %{public}d", moduleName.c_str(), isSystemApp);

    std::string tmpPath = "";
    for (size_t i = 0; i < appLibPath.size(); i++) {
        if (appLibPath[i].empty()) {
            continue;
        }
        tmpPath += appLibPath[i];
        tmpPath += ":";
    }
    if (tmpPath.back() == ':') {
        tmpPath.pop_back();
    }

    char *tmp = strdup(tmpPath.c_str());
    if (tmp == nullptr) {
        MODULEMNG_HILOG_ERROR("strdup failed. tmpPath is %{public}s", tmpPath.c_str());
        return;
    }

    std::lock_guard<std::mutex> guard(appLibPathMapMutex_);
    if (appLibPathMap_[moduleName] != nullptr) {
        free(appLibPathMap_[moduleName]);  // ✅ 修复：使用 free() 释放 strdup 分配的内存
    }
    appLibPathMap_[moduleName] = tmp;
    CreateLdNamespace(moduleName, tmp, isSystemApp);
    MODULEMNG_HILOG_DEBUG("path: %{public}s", appLibPathMap_[moduleName]);
}

// ========== 修复 RemoveNativeModuleByCache ==========
// 位置: Line 1413-1468

bool NativeModuleManager::RemoveNativeModuleByCache(const std::string& moduleKey)
{
    std::lock_guard<std::mutex> lock(nativeModuleListMutex_);
    if (headNativeModule_ == nullptr) {
        MODULEMNG_HILOG_WARN("Module list empty");
        return false;
    }
    NativeModule* nativeModule = headNativeModule_;
    if (!strcasecmp(nativeModule->moduleName, moduleKey.c_str())) {
        if (headNativeModule_ == tailNativeModule_) {
            tailNativeModule_ = nullptr;
        }
        headNativeModule_ = headNativeModule_->next;

        // ✅ 修复：使用 free() 释放 strdup 分配的内存
        if (nativeModule->name) {
            free(const_cast<char*>(nativeModule->name));
        }
        if (nativeModule->moduleName) {
            free(const_cast<char*>(nativeModule->moduleName));
        }

        // jsABCCode 由 moduleBufMap_ 管理（问题4的修复）
        if (nativeModule->jsABCCode) {
            RemoveModuleBuffer(moduleKey);
        }

        if (nativeModule->systemFilePath && nativeModule->systemFilePath[0] != '\0') {
            free(const_cast<char*>(nativeModule->systemFilePath));
        }
        delete nativeModule;
        MODULEMNG_HILOG_DEBUG("module %{public}s deleted from cache", moduleKey.c_str());
        return true;
    }

    bool moduleDeleted = false;
    NativeModule* prev = headNativeModule_;
    NativeModule* curr = prev->next;
    while (curr != nullptr) {
        if (!strcasecmp(curr->moduleName, moduleKey.c_str())) {
            if (curr == tailNativeModule_) {
                tailNativeModule_ = prev;
            }
            prev->next = curr->next;

            // ✅ 修复：使用 free() 释放 strdup 分配的内存
            if (curr->name) {
                free(const_cast<char*>(curr->name));
            }
            if (curr->moduleName) {
                free(const_cast<char*>(curr->moduleName));
            }

            // jsABCCode 由 moduleBufMap_ 管理
            if (curr->jsABCCode) {
                RemoveModuleBuffer(moduleKey);
            }

            if (curr->systemFilePath && curr->systemFilePath[0] != '\0') {
                free(const_cast<char*>(curr->systemFilePath));
            }

            delete curr;
            MODULEMNG_HILOG_DEBUG("module %{public}s deleted from cache", moduleKey.c_str());
            moduleDeleted = true;
            break;
        }
        prev = prev->next;
        curr = prev->next;
    }
    return moduleDeleted;
}
```

**注意**: `appLibPathMap_` 的值也有同样的问题！

**appLibPathMap_ 分配点**:
```cpp
// Line 719 - SetAppLibPath()
char *tmp = strdup(tmpPath.c_str());  // ⚠️ strdup 分配
// ...
if (appLibPathMap_[moduleName] != nullptr) {
    delete[] appLibPathMap_[moduleName];  // ❌ 错误：这里应该用 free()
}
appLibPathMap_[moduleName] = tmp;
```

**appLibPathMap_ 释放点**:
```cpp
// 析构函数 Line 103-106
appLibPathMapMutex_.lock();
for (const auto& item : appLibPathMap_) {
    delete[] item.second;  // ❌ 错误！应该用 free()
}
```

**完整修复需要包括**:
1. 析构函数 Line 105: `delete[] item.second;` → `free(item.second);`
2. SetAppLibPath Line 727: `delete[] appLibPathMap_[moduleName];` → `free(appLibPathMap_[moduleName]);`

---

## 5. 完整修复代码

### 5.1 修复问题2: systemFilePath 覆盖泄漏

**文件**: [native_module_manager.cpp](native_module_manager.cpp)

```cpp
// ========== 修复 FindNativeModuleByDisk ==========
// 位置: Line 1314-1330

std::lock_guard<std::mutex> lock(nativeModuleListMutex_);

// ✅ 添加空指针检查
if (tailNativeModule_ == nullptr) {
    MODULEMNG_HILOG_ERROR("tailNativeModule_ is null");
    SetLoadingNativeModuleKey("");
    return nullptr;
}

if (tailNativeModule_ && !abcBuffer) {
    const char* moduleName = strdup(moduleKey.c_str());
    if (moduleName == nullptr) {
        MODULEMNG_HILOG_ERROR("strdup faied");
        SetLoadingNativeModuleKey("");
        return nullptr;
    }

    tailNativeModule_->moduleName = moduleName;

    // ✅ 修复 systemFilePath 覆盖泄漏
    if (tailNativeModule_->systemFilePath && tailNativeModule_->systemFilePath[0] != '\0') {
        free(const_cast<char*>(tailNativeModule_->systemFilePath));
    }
    tailNativeModule_->systemFilePath = strdup(loadPath);
    if (tailNativeModule_->systemFilePath == nullptr) {
        MODULEMNG_HILOG_ERROR("strdup failed for systemFilePath");
        // 清理已分配的 moduleName
        free(const_cast<char*>(tailNativeModule_->moduleName));
        tailNativeModule_->moduleName = nullptr;
        SetLoadingNativeModuleKey("");
        return nullptr;
    }

    if (tailNativeModule_->name && strcmp(tailNativeModule_->moduleName, tailNativeModule_->name)) {
        MODULEMNG_HILOG_WARN("%{public}s Name mismatch: %{public}s != %{public}s",
            isAppModule ? "app module:" : "", tailNativeModule_->moduleName, tailNativeModule_->name);
        MODULEMNG_HILOG_DEBUG("keep .nm_modname match moduleName");
    }
}

// ... 中间代码 ...

// ✅ 修复 Line 1368-1371 分支
if (!internal) {
    // ...
    } else {
        RegisterByBuffer(moduleKey, abcBuffer, len);

        // ✅ 同样处理 systemFilePath 覆盖
        if (tailNativeModule_->systemFilePath && tailNativeModule_->systemFilePath[0] != '\0') {
            free(const_cast<char*>(tailNativeModule_->systemFilePath));
        }
        tailNativeModule_->systemFilePath = strdup(loadPath);
        if (tailNativeModule_->systemFilePath == nullptr) {
            MODULEMNG_HILOG_ERROR("strdup failed for systemFilePath in RegisterByBuffer path");
            SetLoadingNativeModuleKey("");
            return nullptr;
        }
    }
}
```

### 5.2 修复问题3: RegisterByBuffer name 覆盖

```cpp
// ========== 修复 RegisterByBuffer ==========
// 位置: Line 1385-1411

void NativeModuleManager::RegisterByBuffer(const std::string& moduleKey, const uint8_t* abcBuffer, size_t len)
{
    MODULEMNG_HILOG_DEBUG("native module name is '%{public}s'", moduleKey.c_str());
    if (!CreateTailNativeModule()) {
        MODULEMNG_HILOG_ERROR("failed");
        return;
    }

    char *moduleName = strdup(moduleKey.c_str());
    if (moduleName == nullptr) {
        MODULEMNG_HILOG_ERROR("failed");
        return;
    }
    tailNativeModule_->moduleName = moduleName;

    // ✅ 修复 name 覆盖泄漏
    if (tailNativeModule_->name) {
        delete[] tailNativeModule_->name;
        tailNativeModule_->name = nullptr;
    }
    tailNativeModule_->name = strdup(moduleName);
    if (tailNativeModule_->name == nullptr) {
        MODULEMNG_HILOG_ERROR("strdup failed for name");
        free(moduleName);
        tailNativeModule_->moduleName = nullptr;
        return;
    }

    tailNativeModule_->jsABCCode = abcBuffer;
    tailNativeModule_->jsCodeLen = static_cast<int32_t>(len);
    tailNativeModule_->next = nullptr;

    MODULEMNG_HILOG_INFO("Module:%{public}s", tailNativeModule_->moduleName);
}
```

### 5.3 修复问题4: jsABCCode 双重所有权

```cpp
// ========== 修复 RemoveNativeModuleByCache ==========
// 位置: Line 1413-1468

bool NativeModuleManager::RemoveNativeModuleByCache(const std::string& moduleKey)
{
    std::lock_guard<std::mutex> lock(nativeModuleListMutex_);
    if (headNativeModule_ == nullptr) {
        MODULEMNG_HILOG_WARN("Module list empty");
        return false;
    }
    NativeModule* nativeModule = headNativeModule_;
    if (!strcasecmp(nativeModule->moduleName, moduleKey.c_str())) {
        if (headNativeModule_ == tailNativeModule_) {
            tailNativeModule_ = nullptr;
        }
        headNativeModule_ = headNativeModule_->next;
        delete[] nativeModule->name;
        if (nativeModule->moduleName) {
            delete[] nativeModule->moduleName;
        }
        // ✅ 不在这里释放 jsABCCode，由 moduleBufMap_ 管理
        if (nativeModule->jsABCCode) {
            // ✅ 仅从 moduleBufMap_ 中移除
            RemoveModuleBuffer(moduleKey);
        }
        if (nativeModule->systemFilePath && nativeModule->systemFilePath[0] != '\0') {
            free(const_cast<char *>(nativeModule->systemFilePath));
        }
        delete nativeModule;
        MODULEMNG_HILOG_DEBUG("module %{public}s deleted from cache", moduleKey.c_str());
        return true;
    }
    bool moduleDeleted = false;
    NativeModule* prev = headNativeModule_;
    NativeModule* curr = prev->next;
    while (curr != nullptr) {
        if (!strcasecmp(curr->moduleName, moduleKey.c_str())) {
            if (curr == tailNativeModule_) {
                tailNativeModule_ = prev;
            }
            prev->next = curr->next;
            delete[] curr->name;
            if (curr->moduleName) {
                delete[] curr->moduleName;
            }
            // ✅ 同样处理
            if (curr->jsABCCode) {
                RemoveModuleBuffer(moduleKey);
            }
            if (curr->systemFilePath && curr->systemFilePath[0] != '\0') {
                free(const_cast<char *>(curr->systemFilePath));
            }
            delete curr;
            MODULEMNG_HILOG_DEBUG("module %{public}s deleted from cache", moduleKey.c_str());
            moduleDeleted = true;
            break;
        }
        prev = prev->next;
        curr = prev->next;
    }
    return moduleDeleted;
}

// ========== 修改 RemoveModuleBuffer 释放内存 ==========
// 位置: Line 181-193

bool NativeModuleManager::RemoveModuleBuffer(const std::string moduleKey)
{
    MODULEMNG_HILOG_DEBUG("module:'%{public}s'", moduleKey.c_str());
    bool deleted = false;
    std::lock_guard<std::mutex> lock(moduleBufMutex_);
    auto it = moduleBufMap_.find(moduleKey);
    if (it != moduleBufMap_.end()) {
        // ✅ 释放缓冲区内存
        delete[] it->second;
        moduleBufMap_.erase(it);
        MODULEMNG_HILOG_DEBUG("module '%{public}s' erased", moduleKey.c_str());
        deleted = true;
    }
    return deleted;
}

// ========== 修改 EmplaceModuleBuffer 防止重复插入泄漏 ==========
// 位置: Line 172-179

void NativeModuleManager::EmplaceModuleBuffer(const std::string moduleKey, const uint8_t* lib)
{
    MODULEMNG_HILOG_DEBUG("module:'%{public}s'", moduleKey.c_str());
    std::lock_guard<std::mutex> lock(moduleBufMutex_);
    if (lib == nullptr) {
        return;
    }

    // ✅ 检查是否已存在
    auto it = moduleBufMap_.find(moduleKey);
    if (it != moduleBufMap_.end()) {
        if (it->second != lib) {
            // 释放旧缓冲区
            delete[] it->second;
        } else {
            // 相同指针，无需重复插入
            MODULEMNG_HILOG_DEBUG("module '%{public}s' already exists with same buffer", moduleKey.c_str());
            return;
        }
    }

    moduleBufMap_[moduleKey] = lib;
}
```

### 5.4 修复问题5: SetLdPermittedPathsForNamespace

```cpp
// ========== 修复 SetLdPermittedPathsForNamespace ==========
// 位置: Line 1577-1588

void NativeModuleManager::SetLdPermittedPathsForNamespace(const std::string& nsName,
    const std::string& ldPermittedPath)
{
#if !defined(WINDOWS_PLATFORM) && !defined(MAC_PLATFORM) && !defined(__BONIC__) && !defined(IOS_PLATFORM) && \
    !defined(LINUX_PLATFORM)
    // ... 前面代码不变 ...

    char* tmpLdPermittedPath = new char[resLdPermittedPath.size() + 1];
    if (strcpy_s(tmpLdPermittedPath, resLdPermittedPath.size() + 1, resLdPermittedPath.c_str()) != 0) {
        MODULEMNG_HILOG_ERROR("strcpy_s ldPermittedPath '%{public}s' failed", resLdPermittedPath.c_str());
        delete[] tmpLdPermittedPath;
        return;
    }
    if (dlns_set_ld_permitted_path(tmpLdPermittedPath, &ns) != 0) {
        MODULEMNG_HILOG_ERROR(
            "dlns_set_ld_permitted_path failed, ldPermittedPath = '%{public}s', nsName = '%{public}s'",
            tmpLdPermittedPath, nsName.c_str());
        delete[] tmpLdPermittedPath;  // ✅ 修复：添加释放
        return;  // ✅ 修复：添加 return
    }
    delete[] tmpLdPermittedPath;
#endif
}
```

### 5.5 修复问题8: sprintf_s 错误路径内存泄漏

```cpp
// ========== 修复 FindNativeModuleByDisk 错误路径 ==========
// 位置: Line 1341-1350

// ✅ 在 sprintf_s 错误处理中清理已分配的内存
if (!internal) {
    char symbol[NAPI_PATH_MAX] = { 0 };
    if (sprintf_s(symbol, sizeof(symbol), "NAPI_%s_GetABCCode", moduleKey.c_str()) == -1) {
        if (lib != nullptr) {
            LIBFREE(lib);
        }

        // ✅ 修复：清理 Line 1325 和 1333 分配的内存
        if (tailNativeModule_) {
            if (tailNativeModule_->moduleName) {
                free(const_cast<char*>(tailNativeModule_->moduleName));
                tailNativeModule_->moduleName = nullptr;
            }
            if (tailNativeModule_->systemFilePath &&
                tailNativeModule_->systemFilePath[0] != '\0') {
                free(const_cast<char*>(tailNativeModule_->systemFilePath));
                tailNativeModule_->systemFilePath = nullptr;
            }
        }

        errInfo = "sprintf symbol NAPI_" + moduleKey + "_GetABCCode failed";
        MODULEMNG_HILOG_ERROR("%{public}s", errInfo.c_str());
        SetLoadingNativeModuleKey("");
        return nullptr;
    }

    // replace '.' and '/' with '_'
    for (char* p = strchr(symbol, '.'); p != nullptr; p = strchr(p + 1, '.')) {
        *p = '_';
    }
    for (char* p = strchr(symbol, '/'); p != nullptr; p = strchr(p + 1, '/')) {
        *p = '_';
    }

    // ... 继续后续代码
}
```

### 5.6 修复 Line 1379: RegisterByBuffer 后的 systemFilePath 覆盖

```cpp
// ========== 修复 FindNativeModuleByDisk RegisterByBuffer 分支 ==========
// 位置: Line 1377-1380

} else {
    RegisterByBuffer(moduleKey, abcBuffer, len);

    // ✅ 修复：检查并释放旧的 systemFilePath
    if (tailNativeModule_->systemFilePath &&
        tailNativeModule_->systemFilePath[0] != '\0') {
        free(const_cast<char*>(tailNativeModule_->systemFilePath));
    }
    tailNativeModule_->systemFilePath = strdup(loadPath);
    if (tailNativeModule_->systemFilePath == nullptr) {
        MODULEMNG_HILOG_ERROR("strdup failed for systemFilePath after RegisterByBuffer");
        SetLoadingNativeModuleKey("");
        return nullptr;
    }
}
```

### 5.7 实现 Release 函数

```cpp
// ========== 实现 NativeModuleManager::Release ==========
// 文件: native_module_manager.cpp

uint64_t NativeModuleManager::Release()
{
    std::lock_guard<std::mutex> lock(g_instanceMutex);
    if (instance_ != nullptr) {
        delete instance_;
        instance_ = nullptr;
        MODULEMNG_HILOG_INFO("native module manager instance released");
        return 0;
    }
    MODULEMNG_HILOG_WARN("native module manager instance already null");
    return UINT64_MAX;
}
```

---

## 6. 测试建议

### 6.1 内存泄漏检测

使用 AddressSanitizer 或 Valgrind 检测：

```bash
# 使用 ASan 编译
export CFLAGS="-fsanitize=address -g"
export CXXFLAGS="-fsanitize=address -g"
export LDFLAGS="-fsanitize=address"

./build.sh --product-name <product> --build-target arkui_napi

# 运行测试
./out/xxx/tests/unittest/native_module_manager_test
```

### 6.2 单元测试用例

```cpp
// 测试 systemFilePath 覆盖
TEST(NativeModuleManager, SystemFilePathOverrideLeak)
{
    auto manager = NativeModuleManager::GetInstance();
    std::string errInfo;

    // 第一次加载
    auto module1 = manager->LoadNativeModule("test_module", "default", true, errInfo);
    ASSERT_NE(module1, nullptr);

    // 模拟第二次加载（触发覆盖）
    auto module2 = manager->LoadNativeModule("test_module", "default", true, errInfo);
    ASSERT_NE(module2, nullptr);

    // 验证没有泄漏
    // ASan 会自动检测
}

// 测试 moduleBufMap_ 所有权
TEST(NativeModuleManager, ModuleBufMapOwnership)
{
    auto manager = NativeModuleManager::GetInstance();

    // 手动插入 buffer
    uint8_t* buffer = new uint8_t[100];
    manager->EmplaceModuleBuffer("test", buffer);

    // 移除模块
    bool removed = manager->RemoveNativeModule("test");
    EXPECT_TRUE(removed);

    // 验证 buffer 已释放（ASan 会检测）
}

// 测试错误路径
TEST(NativeModuleManager, SetLdPermittedPathsErrorPath)
{
    auto manager = NativeModuleManager::GetInstance();

    // 构造会失败的参数
    std::string invalidNs = "";  // 无效命名空间

    // 调用应该失败但不泄漏
    manager->SetLdPermittedPathsForNamespace(invalidNs, "/invalid/path");

    // ASan 会验证 tmpLdPermittedPath 是否释放
}
```

---

## 7. 总结

### 7.1 问题统计

| 严重性 | 数量 | 问题编号 |
|--------|------|---------|
| 🔴🔴 致命 | 2 | 问题9 (strdup/delete[] 配对错误), 问题10 (指针未判空，30+处) |
| 🔴 高危 | 6 | 问题2, 问题4, 问题5, 问题7, 问题8, Line 1379 |
| 🟡 中等 | 2 | 问题1, 问题3 |
| 🟢 低 | 1 | 问题6 |

**总计：11 个问题，其中 Problem 10 包含 13 个场景**

### 7.2 关键发现

1. **strdup/delete[] 配对错误**（问题9）**最致命** - 导致未定义行为、崩溃、堆损坏
2. **指针未判空**（问题10）**极其危险** - 30+ 处空指针解引用风险，包括：
   - 字符串函数（strcmp, strcasecmp, strlen）参数未检查
   - dlns_get 返回值字段未检查
   - 成员指针访问前未验证
   - strdup 返回值未检查
   - 数组访问未检查数组本身
3. **错误路径泄漏**（问题8）严重 - sprintf_s 失败后已分配内存未清理
4. **双重所有权问题**（问题4）严重，可能导致悬空指针或双重释放
5. **错误路径泄漏**（问题5）常见但易忽略的问题
6. **覆盖泄漏**（问题2、Line 1379）需要在使用前检查并释放旧值
7. **getJSCode 所有权不明**（问题7）需要深入调查其实现和约定

### 7.3 修复工作量估算

| 任务 | 工作量 |
|------|--------|
| 代码修复（问题9：strdup/delete[] 配对，8处） | 1 小时 |
| 代码修复（问题10：指针未判空，30+ 处，13个场景） | 4 小时 |
| 代码修复（其他9个问题） | 8-10 小时 |
| 单元测试编写 | 4-5 小时 |
| 问题7 调查（getJSCode 所有权） | 2 小时 |
| ASan/Valgrind 测试验证 | 2 小时 |
| 代码审查 | 1 小时 |
| **总计** | **22-25 小时** |

---

## 附录A: 内存管理最佳实践

### A.1 使用 RAII

```cpp
// ❌ 不推荐：手动管理
char* path = strdup(loadPath);
if (error) {
    free(path);  // 容易忘记
    return;
}
// 使用 path
free(path);

// ✅ 推荐：使用智能指针
std::unique_ptr<char[], decltype(&free)> path(strdup(loadPath), &free);
if (error) {
    return;  // 自动释放
}
// 使用 path
```

### A.2 明确所有权

```cpp
// 在注释中明确说明所有权
// moduleBufMap_[key] 拥有 jsABCCode 的所有权
// NativeModule::jsABCCode 仅是观察指针，不应释放
```

### A.3 统一分配/释放方式

```cpp
// ❌ 混用 new/malloc + delete/free
char* p = (char*)malloc(100);
delete p;  // ❌ 未定义行为

// ✅ 统一使用 new[]/delete[]
char* p = new char[100];
delete[] p;
```

---

## 🔴 问题8: sprintf_s 错误路径 - moduleName 和 systemFilePath 泄漏

**位置**: [native_module_manager.cpp:1324-1350](native_module_manager.cpp#L1324-L1350)

**问题描述**:
```cpp
// Line 1324-1333: 分配内存
std::lock_guard<std::mutex> lock(nativeModuleListMutex_);
if (tailNativeModule_ && !abcBuffer) {
    const char* moduleName = strdup(moduleKey.c_str());  // ⚠️ 分配1
    if (moduleName == nullptr) {
        MODULEMNG_HILOG_ERROR("strdup faied");
        SetLoadingNativeModuleKey("");
        return nullptr;
    }

    tailNativeModule_->moduleName = moduleName;  // Line 1332: 赋值给成员
    tailNativeModule_->systemFilePath = strdup(loadPath);  // Line 1333: ⚠️ 分配2
    // ...
}

// Line 1343-1350: 错误路径
if (!internal) {
    char symbol[NAPI_PATH_MAX] = { 0 };
    if (sprintf_s(symbol, sizeof(symbol), "NAPI_%s_GetABCCode", moduleKey.c_str()) == -1) {
        if (lib != nullptr) {
            LIBFREE(lib);  // ✅ 释放 lib
        }
        errInfo = "sprintf symbol NAPI_" + moduleKey + "_GetABCCode failed";
        MODULEMNG_HILOG_ERROR("%{public}s", errInfo.c_str());
        SetLoadingNativeModuleKey("");
        return nullptr;  // ❌ 泄漏！Line 1325 和 1333 分配的内存未释放
    }
}
```

**泄漏场景**:
1. Line 1325: `strdup(moduleKey.c_str())` 分配 `moduleName`
2. Line 1332: `tailNativeModule_->moduleName = moduleName` 保存指针
3. Line 1333: `strdup(loadPath)` 分配 `systemFilePath`
4. Line 1343: `sprintf_s` 失败，进入错误处理
5. Line 1350: 返回 `nullptr`
6. **调用方收到 nullptr，认为加载失败**
7. **但是 `tailNativeModule_` 仍然持有这些内存**：
   - `tailNativeModule_->moduleName`
   - `tailNativeModule_->systemFilePath`
8. **这些内存永远不会被释放**，因为调用方不知道它们的存在

**严重性**: 🔴 **严重高危**

**为什么这是严重问题**:
- 内存泄漏在每次 `sprintf_s` 失败时都会发生
- 调用方无法清理这些泄漏（返回 nullptr）
- 长期运行会导致内存耗尽

**修复方案**:

**方案A**: 在错误路径中清理已分配的内存
```cpp
if (sprintf_s(symbol, sizeof(symbol), "NAPI_%s_GetABCCode", moduleKey.c_str()) == -1) {
    if (lib != nullptr) {
        LIBFREE(lib);
    }

    // ✅ 修复：清理 tailNativeModule_ 中已分配的内存
    if (tailNativeModule_) {
        if (tailNativeModule_->moduleName) {
            free(const_cast<char*>(tailNativeModule_->moduleName));
            tailNativeModule_->moduleName = nullptr;
        }
        if (tailNativeModule_->systemFilePath &&
            tailNativeModule_->systemFilePath[0] != '\0') {
            free(const_cast<char*>(tailNativeModule_->systemFilePath));
            tailNativeModule_->systemFilePath = nullptr;
        }
    }

    errInfo = "sprintf symbol NAPI_" + moduleKey + "_GetABCCode failed";
    MODULEMNG_HILOG_ERROR("%{public}s", errInfo.c_str());
    SetLoadingNativeModuleKey("");
    return nullptr;
}
```

**方案B**: 延迟内存分配
```cpp
// 先检查 sprintf_s 是否成功
if (!internal) {
    char symbol[NAPI_PATH_MAX] = { 0 };
    if (sprintf_s(symbol, sizeof(symbol), "NAPI_%s_GetABCCode", moduleKey.c_str()) == -1) {
        if (lib != nullptr) {
            LIBFREE(lib);
        }
        errInfo = "sprintf symbol NAPI_" + moduleKey + "_GetABCCode failed";
        MODULEMNG_HILOG_ERROR("%{public}s", errInfo.c_str());
        SetLoadingNativeModuleKey("");
        return nullptr;
    }

    // sprintf_s 成功后，再分配内存
    if (tailNativeModule_ && !abcBuffer) {
        const char* moduleName = strdup(moduleKey.c_str());
        // ...
    }
}
```

**推荐**: **方案A**，因为代码更清晰，且容易维护。

---

## 🔴 问题7: getJSCode 返回的 buf 所有权不明确 + 可能泄漏

**位置**: [native_module_manager.cpp:1369-1376](native_module_manager.cpp#L1369-L1376)

**问题描述**:
```cpp
// Line 1369-1376
const char* buf = nullptr;
int bufLen = 0;
getJSCode(&buf, &bufLen);  // ⚠️ 谁分配 buf？谁负责释放？
if (tailNativeModule_) {
    tailNativeModule_->jsCode = buf;  // ⚠️ 直接赋值
    tailNativeModule_->jsCodeLen = bufLen;
}
```

**分析**:
- `getJSCode` 是回调函数，签名：`void (*)(const char**, int*)`
- `buf` 是输出参数，由 `getJSCode` 分配内存并让 `buf` 指向它
- `tailNativeModule_->jsCode` 保存这个指针（类型：`const char*`）
- **整个代码中没有任何地方释放 `jsCode`**（除了析构函数）

**所有权问题**:
1. **方案A**：`getJSCode` 返回的内存需要由调用方释放
   - 当前代码：`tailNativeModule_->jsCode = buf;` 直接保存
   - 谁应该在何时释放？找不到释放点

2. **方案B**：`getJSCode` 返回的内存所有权归 NativeModule
   - 需要记录这个分配，并在析构时释放

**严重性**: 🔴 **高危**

**现状验证**:
- ✅ 析构函数 Line 85: `if (nativeModule->jsCode) { delete[] nativeModule->jsCode; }`
- ⚠️ 但是 `getJSCode` 返回的 `buf` 类型是 `const char*`，不是数组
- ⚠️ 应该使用 `delete jsCode` 而不是 `delete[] jsCode`（但不是泄漏的主因）
- 🔴 **主要问题**：不清楚 `getJSCode` 返回内存的生命周期和所有权

**需要确认**:
1. 查看 `getJSCode` 的实现，确认它如何分配 `buf` 内存
2. 确认是否有其他地方释放 `jsCode`（除析构外）
3. 如果所有权归 NativeModule，需要明确注释说明

---

**报告结束**
