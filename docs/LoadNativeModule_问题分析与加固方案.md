# LoadNativeModule 问题分析与加固方案

## 1. 核心问题总结

### 1.1 问题描述

**现象**：`LoadNativeModule` 返回非预期的模块，或返回不完整的模块对象

**根本原因**：
1. `dlopen` 触发构造函数自动执行 → `napi_module_register` → `Register`
2. `Register` 中创建模块时，只设置 `name`，**不设置 `moduleName`
3. `FindNativeModuleByCache` 的查找 key 与模块的 `name` 不一致
4. `napi_onLoad` 可能再次注册，导致状态混乱

### 1.2 问题场景示例

#### 场景 A：模块已通过其他方式预加载

```
初始状态：
- 模块 "multimedia" 已被其他模块依赖加载
- dlopen 触发构造函数 → Register
- tailNativeModule_->name = "multimedia"
- tailNativeModule_->moduleName = nullptr  ⚠️

用户代码：
LoadNativeModule("multimedia", "default", true, ...)
  ↓
key = "default/multimedia"
  ↓
FindNativeModuleByCache("default/multimedia", ...)
  ↓
查找逻辑：
  if ((temp->moduleName && !strcmp(temp->moduleName, "default/multimedia"))  // ❌ 失败：moduleName 是 nullptr
      || !strcasecmp(temp->name, "multimedia"))  // ✅ 匹配！
  ↓
返回 tailNativeModule_
  ↓
问题：tailNativeModule_->moduleName == nullptr
后续代码访问 → 崩溃或错误
```

#### 场景 B：构造函数与 napi_onLoad 的双重注册

```
构造函数执行：
Register(nativeModule)
  → tailNativeModule_->name = "myModule"
  → tailNativeModule_->moduleName = nullptr

napi_onLoad 执行：
Register(nativeModule)  // 可能创建新对象或覆盖
  → tailNativeModule_->name = "myModule"
  → tailNativeModule_->moduleName = nullptr  // 再次为空

Line 1224：
tailNativeModule_->moduleName = strdup("default/myModule")
  ↓
如果 napi_onLoad 创建了新对象：
- 旧对象的 moduleName 永远为 nullptr
- 缓存中可能存在两个对象
- FindNativeModuleByCache 可能返回错误的那个
```

#### 场景 C：多线程并发加载

```
时间线    线程 A                               线程 B
─────────────────────────────────────────────────────
T1       LoadNativeModule("modA")
T2       FindNativeModuleByCache → 未找到
T3       pthread_mutex_lock
T4       g_isLoadingModule = true
T5       dlopen("libA.so")
T6       ├─ 构造函数 → Register
T7       │   └─ tail_->name = "A"
T8       │   └─ tail_->moduleName = nullptr
T9       [dlopen 返回，仍在锁内]
T10                                           LoadNativeModule("modA")
T11                                           FindNativeModuleByCache → 找到 tail_
T12                                           ❌ 返回 tail_（moduleName=nullptr）
T13      Napi_onLoadCallback
T14      Line 1224: 设置 moduleName
T15      pthread_mutex_unlock
T16      返回 tailNativeModule_          ❌ 线程 B 已返回错误
```

---

## 2. 当前代码的具体问题

### 2.1 Register 函数的问题

**代码位置：** `native_module_manager.cpp` Line 280-344

```cpp
void NativeModuleManager::Register(NativeModule* nativeModule)
{
    // ...
    char *moduleName = strdup(tmpName.c_str());  // tmpName 可能不完整
    // ...
    tailNativeModule_->name = moduleName;         // ✅ 设置
    tailNativeModule_->moduleName = nullptr;      // ❌ 问题：设为空
    // ...
}
```

**问题分析**：
1. `moduleName` 被显式设为 `nullptr`
2. `tmpName` 的生成逻辑可能不包含完整路径前缀（如 "default/"）
3. 依赖后续 `FindNativeModuleByDisk` 来设置 `moduleName`，存在时序窗口

### 2.2 FindNativeModuleByCache 的匹配逻辑问题

**代码位置：** `native_module_manager.cpp` Line 1371-1418

```cpp
NativeModule* NativeModuleManager::FindNativeModuleByCache(
    const char* moduleName,  // 如 "default/multimedia"
    char nativeModulePath[][NAPI_PATH_MAX],
    NativeModule*& cacheNativeModule,
    NativeModuleHeadTailStruct& cacheHeadTailStruct,
    bool checkLoadingNativeModule)
{
    // ...
    for (NativeModule* temp = headNativeModule_; temp != nullptr; temp = temp->next) {
        // 比较逻辑：
        if ((temp->moduleName && !strcmp(temp->moduleName, moduleName))  // 完全匹配
            || !strcasecmp(temp->name, moduleName)) {  // 忽略大小写匹配 name
            // ...
            result = temp;
            break;
        }
    }
    // ...
}
```

**问题分析**：

| 匹配条件 | moduleName | 匹配结果 | 问题 |
|---------|-----------|---------|------|
| `strcmp(temp->moduleName, "default/multimedia")` | `nullptr` | ❌ 失败 | `nullptr` 不会匹配 |
| `strcasecmp(temp->name, "default/multimedia")` | `"multimedia"` | ❌ 失败 | 不包含前缀 |
| `strcasecmp(temp->name, "multimedia")` | `"multimedia"` | ✅ 成功 | 错误匹配！ |

**严重后果**：
- 使用 `name` 进行匹配，导致**前缀不匹配**的情况
- 可能返回不完整的模块对象（`moduleName = nullptr`）
- `systemFilePath` 可能也不匹配

### 2.3 模块加载状态不一致

**表格：模块完整性的时间线**

| 时间点 | name | moduleName | systemFilePath | moduleLoaded | 完整性 |
|--------|------|------------|----------------|---------------|--------|
| T1: Register 执行 | `"myModule"` | `nullptr` | `""` | `false` | ❌ 不完整 |
| T2: dlopen 返回 | `"myModule"` | `nullptr` | `""` | `false` | ❌ 不完整 |
| T3: napi_onLoad 执行 | `"myModule"` | `nullptr` | `""` | `false` | ❌ 不完整 |
| T4: Line 1215（加锁） | `"myModule"` | `nullptr` | `""` | `false` | ❌ 不完整 |
| T5: Line 1224 | `"myModule"` | `"default/myModule"` | `"/path/lib.so"` | `false` | ❌ 仍不完整 |
| T6: Line 1276 | `"myModule"` | `"default/myModule"` | `"/path/lib.so"` | `true` | ✅ 完整 |

**问题**：
- T1-T5 期间，模块处于**不完整状态**
- `FindNativeModuleByCache` 可能在任意时刻被调用
- 没有机制阻止返回不完整的模块

---

## 3. 详细问题分析

### 3.1 问题 1：key 与 name 的不匹配

**根源**：
- `LoadNativeModule` 生成 key：`"{prefix}/{moduleName}"`（如 "default/multimedia"）
- `Register` 设置 name：`moduleName`（如 "multimedia"，无前缀）
- 缓存查找使用 key，但对象中存储的是 name

**影响**：
```cpp
// 用户代码
LoadNativeModule("multimedia", "default", true, ...)
  → key = "default/multimedia"
  → FindNativeModuleByCache("default/multimedia", ...)

// Register 中（构造函数）
Register(nativeModule)
  → tailNativeModule_->name = "multimedia"  // 无前缀
  → tailNativeModule_->moduleName = nullptr

// 查找逻辑
if (temp->moduleName && strcmp(temp->moduleName, "default/multimedia"))  // 失败
if (!strcasecmp(temp->name, "default/multimedia"))  // 失败："multimedia" != "default/multimedia"
```

**结果**：
- 查找失败（正确）
- 但如果有其他模块的 name 恰好匹配（如 name="default/multimedia"），会错误返回

### 3.2 问题 2：nullptr 的 moduleName 被接受

**代码逻辑**：
```cpp
if ((temp->moduleName && !strcmp(temp->moduleName, moduleName))  // 如果 moduleName != nullptr
    || !strcasecmp(temp->name, moduleName))  // 或者 name 匹配
```

**问题**：
- 第二个条件 `!strcasecmp(temp->name, moduleName)` 不检查 `temp->moduleName` 是否为空
- 即使 `temp->moduleName == nullptr`，只要 `name` 匹配就会返回
- 返回的模块对象**不完整**（缺少 moduleName）

**实际案例**：
```cpp
// 模块 A 已加载（通过构造函数）
NativeModule moduleA = {
    .name = "multimedia",           // ✅ 有值
    .moduleName = nullptr,           // ❌ 为空
    .systemFilePath = "",            // ❌ 为空
    .moduleLoaded = false,           // ❌ 未完成
};

// 用户请求
LoadNativeModule("multimedia", ...)
  → key = "default/multimedia"
  → FindNativeModuleByCache

// 查找过程
strcmp(moduleA.moduleName, "default/multimedia")  // 失败：nullptr
strcasecmp(moduleA.name, "multimedia")  // ⚠️ 可能错误匹配
  // 如果恰好有 name="multimedia"，返回
  // 但用户期望的是 "default/multimedia"

// 返回不完整的模块 → 后续代码崩溃
```

### 3.3 问题 3：napi_onLoad 的不确定性

**模块开发者的选择**：

```cpp
// 选择 1：仅在构造函数注册
extern "C" __attribute__((constructor)) void Init() {
    napi_module_register(&module);  // ✅ 立即注册
}

extern "C" void napi_onLoad() {
    // 不注册，只做其他初始化
}

// 选择 2：仅在 napi_onLoad 注册
extern "C" __attribute__((constructor)) void Init() {
    // 不注册
}

extern "C" void napi_onLoad() {
    napi_module_register(&module);  // ⚠️ 延迟注册
}

// 选择 3：两次都注册
extern "C" __attribute__((constructor)) void Init() {
    napi_module_register(&module1);  // 第一次
}

extern "C" void napi_onLoad() {
    napi_module_register(&module2);  // ⚠️ 第二次，可能覆盖
}
```

**影响**：
- 注册时机不可预测
- 可能创建多个 `NativeModule` 对象
- `moduleName` 的设置时机不确定

### 3.4 问题 4：缓存快照的时效性问题

**代码位置：** Line 726-742

```cpp
NativeModule* nativeModule = FindNativeModuleByCache(...);  // 第一次查找
if (nativeModule == nullptr) {
    pthread_mutex_lock(&mutex_);
    if (CheckNativeListChanged(cacheHeadTailNativeModule)) {  // 检查是否变化
        nativeModule = FindNativeModuleByCache(...);  // 第二次查找
    }
    // ...
}
```

**问题**：
- `cacheHeadTailNativeModule` 是第一次查找时的快照
- 如果在第一次查找和加锁之间，其他线程加载了模块
- `CheckNativeListChanged` 能检测到变化
- 但第二次查找时，仍可能找到**不完整的模块**（刚被构造函数创建，但 napi_onLoad 未执行）

---

## 4. 加固方案

### 4.1 方案 1：在 Register 中立即设置 moduleName（推荐）

#### 实施步骤

**步骤 1：修改 Register 函数**

```cpp
void NativeModuleManager::Register(NativeModule* nativeModule)
{
    if (nativeModule == nullptr) {
        MODULEMNG_HILOG_ERROR("Module null");
        return;
    }

    MODULEMNG_HILOG_DEBUG("native module name is '%{public}s'", nativeModule->name);
    std::lock_guard<std::mutex> lock(nativeModuleListMutex_);

    const char *nativeModuleName = nativeModule->name == nullptr ? "" : nativeModule->name;
    std::string appName = prefix_ + "/" + nativeModuleName;
    std::string tmpName = isAppModule_ ? appName : nativeModuleName;

    char *moduleName = strdup(tmpName.c_str());
    if (moduleName == nullptr) {
        MODULEMNG_HILOG_ERROR("failed");
        return;
    }

    // ✅ 加固：立即设置 moduleName
    if (!CreateTailNativeModule()) {
        MODULEMNG_HILOG_ERROR("failed");
        free(moduleName);
        return;
    }

    SetLoadingNativeModuleKey(moduleName);  // ✅ 立即设置加载标志

    tailNativeModule_->version = nativeModule->version;
    tailNativeModule_->fileName = nativeModule->fileName;
    tailNativeModule_->isAppModule = isAppModule_;
    tailNativeModule_->name = moduleName;
    tailNativeModule_->moduleName = moduleName;  // ✅ 立即设置，不再为空
    tailNativeModule_->refCount = nativeModule->refCount;
    tailNativeModule_->registerCallback = nativeModule->registerCallback;
    tailNativeModule_->getJSCode = nativeModule->getJSCode;
    tailNativeModule_->getABCCode = nativeModule->getABCCode;
    tailNativeModule_->next = nullptr;
    tailNativeModule_->moduleLoaded = false;  // ⚠️ 注意：仍为 false
    tailNativeModule_->systemFilePath = "";

    MODULEMNG_HILOG_DEBUG("Module registered with moduleName='%{public}s'", moduleName);
}
```

**步骤 2：修改 FindNativeModuleByDisk 中的 moduleName 赋值**

```cpp
// Line 1224 附近
std::lock_guard<std::mutex> lock(nativeModuleListMutex_);
if (tailNativeModule_ && !abcBuffer) {
    // ✅ 加固：如果已设置，不重复设置
    if (tailNativeModule_->moduleName == nullptr) {
        const char* moduleName = strdup(moduleKey.c_str());
        if (moduleName == nullptr) {
            MODULEMNG_HILOG_ERROR("strdup failed");
            SetLoadingNativeModuleKey("");
            return nullptr;
        }
        tailNativeModule_->moduleName = moduleName;
        MODULEMNG_HILOG_DEBUG("Set moduleName: %{public}s", moduleName);
    } else if (strcmp(tailNativeModule_->moduleName, moduleKey.c_str()) != 0) {
        // ⚠️ 警告：moduleName 不一致
        MODULEMNG_HILOG_WARN("ModuleName mismatch: existing='%{public}s', new='%{public}s'",
            tailNativeModule_->moduleName, moduleKey);
        // 更新为完整的 moduleKey
        free(const_cast<char*>(tailNativeModule_->moduleName));
        tailNativeModule_->moduleName = strdup(moduleKey.c_str());
    }

    tailNativeModule_->systemFilePath = strdup(loadPath);
    // ...
}
```

**步骤 3：添加完整性检查**

```cpp
// FindNativeModuleByCache 返回前检查
NativeModule* NativeModuleManager::FindNativeModuleByCache(...)
{
    // ...
    for (NativeModule* temp = headNativeModule_; temp != nullptr; temp = temp->next) {
        if ((temp->moduleName && !strcmp(temp->moduleName, moduleName))
            || !strcasecmp(temp->name, moduleName)) {

            // ✅ 加固：检查模块完整性
            if (temp->moduleName == nullptr) {
                MODULEMNG_HILOG_WARN("Found incomplete module (moduleName=nullptr): %{public}s", temp->name);
                continue;  // 跳过不完整的模块
            }

            int label = 0;
            while (label < NATIVE_PATH_NUMBER && strcmp(temp->systemFilePath, nativeModulePath[label])) {
                label++;
            }
            if (label < NATIVE_PATH_NUMBER || !strcmp(temp->systemFilePath, "")) {
                result = temp;
                break;
            } else {
                MODULEMNG_HILOG_WARN("Module path conflict: %{public}s", moduleName);
                cacheNativeModule = temp;
            }
        }
    }
    // ...
}
```

**优点**：
- ✅ 消除空值窗口期
- ✅ 模块始终有完整的 name 和 moduleName
- ✅ 允许 napi_onLoad 中的二次注册（更新或覆盖）
- ✅ 最小化代码改动

**缺点**：
- ⚠️ 如果构造函数中没有完整的 prefix 信息，moduleName 可能不准确
- ⚠️ 需要确保 `prefix_` 和 `isAppModule_` 已正确设置

### 4.2 方案 2：延迟暴露模块到缓存

#### 实施步骤

**步骤 1：添加初始化标志**

```cpp
struct NativeModule {
    // ...
    bool moduleFullyInitialized = false;  // 新增
};
```

**步骤 2：Register 中设置标志**

```cpp
void NativeModuleManager::Register(NativeModule* nativeModule)
{
    // ...
    tailNativeModule_->moduleName = strdup(moduleName);
    tailNativeModule_->moduleFullyInitialized = false;  // ⚠️ 标记为未完全初始化
}
```

**步骤 3：FindNativeModuleByDisk 中标记为已初始化**

```cpp
// Line 1276 附近
if (tailNativeModule_) {
    tailNativeModule_->moduleLoaded = true;
    tailNativeModule_->moduleFullyInitialized = true;  // ✅ 标记为已完全初始化
}
```

**步骤 4：FindNativeModuleByCache 中过滤未初始化模块**

```cpp
NativeModule* NativeModuleManager::FindNativeModuleByCache(...)
{
    // ...
    for (NativeModule* temp = headNativeModule_; temp != nullptr; temp = temp->next) {
        // ✅ 加固：跳过未完全初始化的模块
        if (!temp->moduleFullyInitialized) {
            MODULEMNG_HILOG_DEBUG("Skip uninitialized module: %{public}s",
                temp->name ? temp->name : "(null)");
            continue;
        }

        if ((temp->moduleName && !strcmp(temp->moduleName, moduleName))
            || !strcasecmp(temp->name, moduleName)) {
            // ... 匹配逻辑
        }
    }
    // ...
}
```

**优点**：
- ✅ 明确的初始化状态
- ✅ 防止返回不完整的模块
- ✅ 兼容现有逻辑

**缺点**：
- ⚠️ 需要修改结构体（可能影响 ABI）
- ⚠️ 需要确保所有路径都设置标志

### 4.3 方案 3：改进匹配逻辑，严格检查完整性

#### 实施步骤

**步骤 1：修改 FindNativeModuleByCache 的匹配条件**

```cpp
NativeModule* NativeModuleManager::FindNativeModuleByCache(
    const char* moduleName,
    char nativeModulePath[][NAPI_PATH_MAX],
    NativeModule*& cacheNativeModule,
    NativeModuleHeadTailStruct& cacheHeadTailStruct,
    bool checkLoadingNativeModule)
{
    NativeModule* result = nullptr;
    std::lock_guard<std::mutex> lock(nativeModuleListMutex_);
    cacheNativeModule = nullptr;

    if (!checkLoadingNativeModule && cacheHeadTailStruct.matchLoadingNativeModule) {
        MODULEMNG_HILOG_DEBUG("module: %{public}s match second", moduleName);
        return cacheHeadTailStruct.matchLoadingNativeModule;
    }

    for (NativeModule* temp = headNativeModule_; temp != nullptr; temp = temp->next) {
        // ✅ 加固 1：检查 moduleName 是否为空
        if (temp->moduleName == nullptr) {
            MODULEMNG_HILOG_DEBUG("Skip module with null moduleName: %{public}s",
                temp->name ? temp->name : "(null)");
            continue;
        }

        // ✅ 加固 2：优先使用 moduleName 精确匹配
        if (!strcmp(temp->moduleName, moduleName)) {
            // 精确匹配成功
            int label = 0;
            while (label < NATIVE_PATH_NUMBER && strcmp(temp->systemFilePath, nativeModulePath[label])) {
                label++;
            }

            if (label < NATIVE_PATH_NUMBER || !strcmp(temp->systemFilePath, "")) {
                result = temp;
                break;
            } else {
                MODULEMNG_HILOG_WARN("Module path conflict: %{public}s", moduleName);
                cacheNativeModule = temp;
            }
        }
    }

    // ✅ 加固 3：如果精确匹配失败，可以尝试 name 匹配，但必须检查完整性
    if (result == nullptr) {
        MODULEMNG_HILOG_DEBUG("Exact match failed, trying name match for: %{public}s", moduleName);

        for (NativeModule* temp = headNativeModule_; temp != nullptr; temp = temp->next) {
            // ⚠️ 降级匹配：使用 name
            if (!strcasecmp(temp->name, moduleName)) {
                MODULEMNG_HILOG_WARN("Degraded match using name: %{public}s (expected moduleName)", moduleName);

                // 检查完整性
                if (temp->moduleName == nullptr || temp->moduleLoaded == false) {
                    MODULEMNG_HILOG_WARN("Matched but incomplete, skipping: %{public}s", moduleName);
                    continue;
                }

                result = temp;
                break;
            }
        }
    }

    std::string loadingNativeModuleKey = GetLoadingNativeModuleKey();
    if (result && checkLoadingNativeModule && !strcasecmp(loadingNativeModuleKey.c_str(), moduleName)) {
        MODULEMNG_HILOG_DEBUG("Module is loading: %{public}s, not use", moduleName);
        result = nullptr;
    }

    cacheHeadTailStruct.headNativeModule = headNativeModule_;
    cacheHeadTailStruct.tailNativeModule = tailNativeModule_;

    return result;
}
```

**优点**：
- ✅ 强制检查完整性
- ✅ 优先精确匹配
- ✅ 对不完整模块记录日志
- ✅ 不修改结构体

**缺点**：
- ⚠️ 可能导致某些合法模块无法被找到
- ⚠️ 降级匹配可能掩盖真正的问题

### 4.4 方案 4：在 napi_onLoadCallback 前后设置保护

#### 实施步骤

**步骤 1：在 napi_onLoad 调用前设置临时标志**

```cpp
// FindNativeModuleByDisk 中
if (lib != nullptr) {
    // ✅ 在调用 napi_onLoad 前，标记模块为"加载中"
    if (tailNativeModule_) {
        tailNativeModule_->moduleLoaded = false;  // 标记为未完成
        MODULEMNG_HILOG_DEBUG("Before napi_onLoad: moduleLoaded=false");
    }

    Napi_onLoadCallback(lib, moduleName);

    // ✅ 在调用后验证
    if (tailNativeModule_ && tailNativeModule_->moduleName == nullptr) {
        MODULEMNG_HILOG_ERROR("napi_onLoad did not set moduleName, force set");
        tailNativeModule_->moduleName = strdup(moduleKey.c_str());
    }
}
```

**步骤 2：阻塞并发访问**

```cpp
// FindNativeModuleByCache 中
if (result && checkLoadingNativeModule) {
    // ✅ 如果模块正在加载，不要返回
    MODULEMNG_HILOG_DEBUG("Module is being loaded: %{public}s", moduleName);
    result = nullptr;
}
```

**优点**：
- ✅ 防止在 napi_onLoad 期间返回不完整模块
- ✅ 强制设置 moduleName

**缺点**：
- ⚠️ 可能影响加载性能
- ⚠️ 需要仔细处理边界情况

---

## 5. 推荐的综合加固方案

### 5.1 组合方案：方案 1 + 方案 3

#### 实施清单

**Phase 1：Register 函数改进**
- [ ] 在 Register 中立即设置 `moduleName`
- [ ] 添加日志记录模块注册状态
- [ ] 确保 moduleName 格式正确（包含前缀）

**Phase 2：FindNativeModuleByCache 改进**
- [ ] 添加 `moduleName == nullptr` 检查，跳过不完整模块
- [ ] 优先使用 `moduleName` 精确匹配
- [ ] 降级使用 `name` 匹配时，增加完整性检查
- [ ] 添加详细的日志记录

**Phase 3：FindNativeModuleByDisk 改进**
- [ ] 在 napi_onLoad 前后添加保护
- [ ] 如果 napi_onLoad 后 moduleName 仍为空，强制设置
- [ ] 添加二次注册检测（防止重复创建对象）

**Phase 4：测试验证**
- [ ] 单元测试：并发加载同一模块
- [ ] 单元测试：构造函数与 napi_onLoad 双重注册
- [ ] 单元测试：缓存查找的边界条件
- [ ] 集成测试：多模块依赖场景

### 5.2 关键代码片段

#### 片段 1：Register 函数核心修改

```cpp
void NativeModuleManager::Register(NativeModule* nativeModule)
{
    // ... 前置检查 ...

    std::lock_guard<std::mutex> lock(nativeModuleListMutex_);
    const char *nativeModuleName = nativeModule->name == nullptr ? "" : nativeModule->name;
    std::string appName = prefix_ + "/" + nativeModuleName;
    std::string tmpName = isAppModule_ ? appName : nativeModuleName;

    char *moduleName = strdup(tmpName.c_str());
    if (moduleName == nullptr) {
        MODULEMNG_HILOG_ERROR("strdup moduleName failed");
        return;
    }

    if (g_isLoadingModule || !strcmp(loadingModuleName_.c_str(), moduleName)) {
        if (!CreateTailNativeModule()) {
            MODULEMNG_HILOG_ERROR("CreateTailNativeModule failed");
            free(moduleName);
            return;
        }

        SetLoadingNativeModuleKey(moduleName);  // 立即设置

        tailNativeModule_->version = nativeModule->version;
        tailNativeModule_->fileName = nativeModule->fileName;
        tailNativeModule_->isAppModule = isAppModule_;
        tailNativeModule_->name = moduleName;
        tailNativeModule_->moduleName = moduleName;  // ✅ 立即设置

        tailNativeModule_->refCount = nativeModule->refCount;
        tailNativeModule_->registerCallback = nativeModule->registerCallback;
        tailNativeModule_->getJSCode = nativeModule->getJSCode;
        tailNativeModule_->getABCCode = nativeModule->getABCCode;
        tailNativeModule_->next = nullptr;
        tailNativeModule_->moduleLoaded = true;  // ✅ 立即标记为已加载
        tailNativeModule_->systemFilePath = "";  // ⚠️ 仍为空，待后续更新

        MODULEMNG_HILOG_INFO("Registered: name='%{public}s', moduleName='%{public}s'",
            tailNativeModule_->name, tailNativeModule_->moduleName);
    }
    // ...
}
```

#### 片段 2：FindNativeModuleByCache 完整性检查

```cpp
NativeModule* NativeModuleManager::FindNativeModuleByCache(
    const char* moduleName,
    char nativeModulePath[][NAPI_PATH_MAX],
    NativeModule*& cacheNativeModule,
    NativeModuleHeadTailStruct& cacheHeadTailStruct,
    bool checkLoadingNativeModule)
{
    NativeModule* result = nullptr;
    std::lock_guard<std::mutex> lock(nativeModuleListMutex_);
    cacheNativeModule = nullptr;

    if (!checkLoadingNativeModule && cacheHeadTailStruct.matchLoadingNativeModule) {
        MODULEMNG_HILOG_DEBUG("module: %{public}s match loading module", moduleName);
        return cacheHeadTailStruct.matchLoadingNativeModule;
    }

    for (NativeModule* temp = headNativeModule_; temp != nullptr; temp = temp->next) {
        // ✅ 关键检查 1：moduleName 必须不为空
        if (temp->moduleName == nullptr) {
            MODULEMNG_HILOG_WARN("Incomplete module (null moduleName): name='%{public}s', skipping",
                temp->name ? temp->name : "(null)");
            continue;
        }

        // ✅ 关键检查 2：moduleLoaded 必须为 true
        if (!temp->moduleLoaded) {
            MODULEMNG_HILOG_WARN("Incomplete module (not loaded): moduleName='%{public}s', skipping",
                temp->moduleName);
            continue;
        }

        // ✅ 优先精确匹配 moduleName
        if (!strcmp(temp->moduleName, moduleName)) {
            int label = 0;
#if !defined(ANDROID_PLATFORM) && !defined(IOS_PLATFORM)
            while (label < NATIVE_PATH_NUMBER && strcmp(temp->systemFilePath, nativeModulePath[label])) {
                label++;
            }
#endif
            if (label < NATIVE_PATH_NUMBER || !strcmp(temp->systemFilePath, "")) {
                result = temp;
                MODULEMNG_HILOG_DEBUG("Found module by exact match: %{public}s", moduleName);
                break;
            } else {
                MODULEMNG_HILOG_WARN("Module path conflict: %{public}s", moduleName);
                cacheNativeModule = temp;
            }
        }
    }

    std::string loadingNativeModuleKey = GetLoadingNativeModuleKey();
    if (result && checkLoadingNativeModule && !strcasecmp(loadingNativeModuleKey.c_str(), moduleName)) {
        MODULEMNG_HILOG_DEBUG("Module is loading: %{public}s, not use", moduleName);
#if defined(ANDROID_PLATFORM)
        cacheHeadTailStruct.matchLoadingNativeModule = cacheHeadTailStruct.matchLoadingNativeModule ?
            cacheHeadTailStruct.matchLoadingNativeModule : result;
#else
        cacheHeadTailStruct.matchLoadingNativeModule = result;
#endif
        result = nullptr;
    }

    cacheHeadTailStruct.headNativeModule = headNativeModule_;
    cacheHeadTailStruct.tailNativeModule = tailNativeModule_;

    return result;
}
```

#### 片段 3：FindNativeModuleByDisk 保护

```cpp
// 在 napi_onLoad 调用前后添加保护
if (lib != nullptr) {
    // ✅ 保护 1：标记状态
    if (tailNativeModule_) {
        MODULEMNG_HILOG_DEBUG("Before napi_onLoad: moduleName='%{public}s', loaded=%d",
            tailNativeModule_->moduleName ? tailNativeModule_->moduleName : "(null)",
            tailNativeModule_->moduleLoaded);
    }

    Napi_onLoadCallback(lib, moduleName);

    // ✅ 保护 2：验证和修复
    std::lock_guard<std::mutex> lock(nativeModuleListMutex_);
    if (tailNativeModule_) {
        if (tailNativeModule_->moduleName == nullptr) {
            MODULEMNG_HILOG_ERROR("napi_onLoad did not set moduleName, force set");
            tailNativeModule_->moduleName = strdup(moduleKey.c_str());
        }

        if (!tailNativeModule_->moduleLoaded) {
            MODULEMNG_HILOG_WARN("Module not marked as loaded after napi_onLoad, fixing");
            tailNativeModule_->moduleLoaded = true;
        }

        MODULEMNG_HILOG_DEBUG("After napi_onLoad: moduleName='%{public}s', loaded=%d",
            tailNativeModule_->moduleName,
            tailNativeModule_->moduleLoaded);
    }
}
```

---

## 6. 测试用例建议

### 6.1 单元测试

```cpp
// test_native_module_manager.cpp

TEST(NativeModuleManagerTest, LoadModule_Concurrent) {
    // 测试并发加载同一模块
    const char* moduleName = "test_module";
    const char* path = "default";

    // 线程 A：加载模块
    std::thread threadA([this, moduleName, path]() {
        std::string errInfo;
        NativeModule* modA = LoadNativeModule(moduleName, path, true, errInfo);
        EXPECT_NE(modA, nullptr);
        EXPECT_NE(modA->moduleName, nullptr);
        EXPECT_STREQ(modA->moduleName, "default/test_module");
    });

    // 线程 B：同时加载同一模块
    std::thread threadB([this, moduleName, path]() {
        std::string errInfo;
        NativeModule* modB = LoadNativeModule(moduleName, path, true, errInfo);
        EXPECT_NE(modB, nullptr);
        EXPECT_NE(modB->moduleName, nullptr);
        EXPECT_STREQ(modB->moduleName, "default/test_module");
    });

    threadA.join();
    threadB.join();

    // 验证：两个线程应该获得相同的模块对象
    // 或者至少都能正确初始化 moduleName
}

TEST(NativeModuleManagerTest, LoadModule_Preloaded) {
    // 测试：模块已预加载（通过构造函数）的场景
    // 1. 模拟构造函数已执行，Register 已调用
    // 2. 调用 LoadNativeModule
    // 3. 验证返回的模块完整性
}

TEST(NativeModuleManagerTest, LoadModule_WithNapiOnLoad) {
    // 测试：napi_onLoad 中注册的场景
    // 1. 构造函数注册
    // 2. napi_onLoad 再次注册
    // 3. 验证最终状态正确
}

TEST(NativeModuleManagerTest, FindCache_IncompleteModule) {
    // 测试：缓存中存在不完整模块的场景
    // 1. 手动创建 moduleName=nullptr 的模块
    // 2. 调用 FindNativeModuleByCache
    // 3. 验证不返回不完整模块
}
```

### 6.2 压力测试

```cpp
TEST(NativeModuleManagerTest, Stress_ConcurrentLoad) {
    // 100 个线程同时加载 10 个不同的模块
    // 验证：
    // 1. 每个线程都获得有效的模块对象
    // 2. 所有模块的 moduleName 都不为空
    // 3. 没有内存泄漏或崩溃
}
```

---

## 7. 风险评估

### 7.1 当前风险等级

| 风险项 | 严重性 | 可能性 | 风险等级 | 影响 |
|--------|--------|--------|---------|------|
| 返回 moduleName=nullptr 的模块 | 🔴 高 | 🔴 高 | **严重** | 崩溃、数据损坏 |
| 缓存 key 与 name 不匹配 | 🟠 中 | 🟠 中 | **中等** | 返回错误模块 |
| napi_onLoad 重复注册 | 🟠 中 | 🟡 低 | **中等** | 状态不一致 |
| 并发加载竞态条件 | 🔴 高 | 🟠 中 | **严重** | 不确定行为 |

### 7.2 加固后的风险

| 加固措施 | 风险 | 缓解措施 |
|---------|------|---------|
| Register 中立即设置 moduleName | 可能与 napi_onLoad 冲突 | 在 FindNativeModuleByDisk 中检测并更新 |
| 严格检查 moduleLoaded | 可能导致模块无法找到 | 提供降级匹配机制 + 详细日志 |
| 跳过 moduleName=nullptr 的模块 | 可能降低性能 | 仅跳过极少数不完整模块 |

---

## 8. 总结

### 8.1 核心问题

1. **时序问题**：Register → dlopen 返回 → napi_onLoad → 设置 moduleName，存在多个空值窗口
2. **匹配问题**：缓存查找的 key 与模块的 name 不一致
3. **完整性问题**：不完整的模块被返回给调用者
4. **并发问题**：多线程同时加载时，可能访问未初始化的对象

### 8.2 推荐方案

**组合方案**：
1. Register 中立即设置 moduleName（消除空值窗口）
2. FindNativeModuleByCache 中强制检查完整性（防止返回不完整模块）
3. napi_onLoad 后验证和修复（确保状态一致）
4. 添加详细日志（便于排查问题）

### 8.3 预期效果

- ✅ 消除 `moduleName = nullptr` 的问题
- ✅ 保证返回的模块对象完整
- ✅ 支持构造函数和 napi_onLoad 两种注册模式
- ✅ 并发安全，不会返回错误的模块

---

**文档版本：** 1.0
**创建日期：** 2025-01-23
**作者：** 代码分析
