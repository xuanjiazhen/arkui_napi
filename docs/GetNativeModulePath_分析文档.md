# NativeModuleManager::GetNativeModulePath 函数分析文档

## 1. 函数概述

### 1.1 函数签名

```cpp
bool NativeModuleManager::GetNativeModulePath(
    const char* moduleName,          // 输入：模块名称
    const char* path,                // 输入：路径前缀（用于 app module）
    const char* relativePath,        // 输入：相对路径
    bool isAppModule,                // 输入：是否为应用模块
    char nativeModulePath[][NAPI_PATH_MAX],  // 输出：3个候选路径
    int32_t pathLength)              // 输入：路径缓冲区长度
```

### 1.2 输出数组含义

`nativeModulePath` 是一个包含 3 个元素的数组，每个元素最大长度为 `NAPI_PATH_MAX`：

| 索引 | 用途 | 优先级 | 说明 |
|------|------|--------|------|
| `[0]` | 主要动态库路径 | 最高 | 首先尝试加载的 .so/.dll/.dylib 文件 |
| `[1]` | 次要动态库路径 | 次之 | NAPI 变体或备用路径（`libxxx_napi.z.so`） |
| `[2]` | ABC 文件路径 | 最低 | ArkTS 字节码文件（.abc） |

加载顺序：`nativeModulePath[0]` → `nativeModulePath[1]` → `nativeModulePath[2]`

---

## 2. 关键变量详解

### 2.1 变量汇总表

| 变量名 | 类型 | 含义 | 来源 | 示例值（标准 OHOS） |
|--------|------|------|------|---------------------|
| **`prefix`** | `const char*` | 库文件的基础路径前缀 | • `isAppModule && IsExistedPath(path)` → `appLibPathMap_[path]`<br>• 否则 → `sysPrefix` 或 `sysPrefix + "/" + relativePath` | • `/system/lib/module`<br>• `/system/lib/module/relpath`<br>• `/data/app/el2/100/base/com.example/app/libs` |
| **`sysPrefix`** | `std::string` | 系统模块默认路径 | 编译时常量（line 924） | `/system/lib/module` |
| **`sysAbcPrefix`** | `std::string` | ABC 文件默认路径 | 编译时常量（line 926） | `/system/etc/abc` |
| **`zfix`** | `const char*` | 中间后缀（分阶段编译） | 编译时常量（line 923） | `.z` |
| **`soPostfix`** | `const char*` | 动态库文件后缀 | 编译时常量（line 922） | `.so` |
| **`abcfix`** | `const char*` | ABC 文件后缀 | 编译时常量（line 925） | `.abc` |
| **`part1`, `part2`** | `std::string` | Android 双路径（冒号分隔） | `prefixStr` 按 `:` 分割 | `part1="/data/.../libs1"`<br>`part2="/data/.../libs2"` |

### 2.2 `prefix` 变量的推导逻辑

```cpp
// 代码位置：line 939-995
const char* prefix = nullptr;

if (isAppModule && IsExistedPath(path)) {
    // 场景 A：App 模块且有路径配置
    appLibPathMapMutex_.lock();
    prefix = appLibPathMap_[path];  // 从 map 获取
    appLibPathMapMutex_.unlock();
} else {
    // 场景 B：系统模块
    if (relativePath[0]) {
        if (previewSearchPath_.empty()) {
            sysPrefix = sysPrefix + "/" + relativePath;  // 追加相对路径
        } else {
            sysPrefix = previewSearchPath_ + "/module";  // 预览模式
        }
    }
    prefix = sysPrefix.c_str();
}
```

**`prefix` 的可能值：**

| 场景 | 条件 | prefix 值 |
|------|------|-----------|
| 系统模块 | `isAppModule=false`, `relativePath=""` | `/system/lib/module` |
| 系统模块 + 相对路径 | `isAppModule=false`, `relativePath="ext"` | `/system/lib/module/ext` |
| App 模块（default） | `isAppModule=true`, `path="default"` | `appLibPathMap_["default"]` 的值 |
| App 模块（custom） | `isAppModule=true`, `path="feature"` | `appLibPathMap_["feature"]` 的值 |

---

## 3. 平台相关的默认值

### 3.1 各平台编译时常量

| 平台 | `soPostfix` | `zfix` | `sysPrefix` | `sysAbcPrefix` | 代码行 |
|------|-------------|--------|-------------|----------------|--------|
| **Windows** | `.dll` | `""` | `./module` | `./module` | 898-902 |
| **macOS** | `.dylib` | `""` | `./module` | `./module` | 904-908 |
| **ARM64/Simulator** | `.so` | `.z` | `/system/lib64/module` | `/system/etc/abc` | 910-914 |
| **Linux** | `.so` | `""` | `./module` | `./module` | 916-920 |
| **标准 OHOS** | `.so` | `.z` | `/system/lib/module` | `/system/etc/abc` | 922-926 |

### 3.2 平台差异说明

- **zfix 存在与否**：反映是否支持分阶段编译（Staged Build）
  - OHOS 标准版：有 `.z` 中间后缀
  - Windows/macOS/Linux：无中间后缀（开发/预览平台）

- **路径前缀差异**：
  - 系统 OHOS：`/system/lib/module`（只读系统分区）
  - ARM64/Simulator：`/system/lib64/module`（64 位库路径）
  - 桌面平台：`./module`（相对路径，开发用）

---

## 4. 路径生成逻辑详细分析

### 4.1 决策树

```
输入：moduleName, path, relativePath, isAppModule
  │
  ├─ 模块名处理：转小写（line 986-988）
  │
  ├─ 分支 1：moduleName 已包含后缀？（line 998-1003）
  │   └─ 是 → 直接返回 {prefix}/{moduleName}
  │
  ├─ 分支 2：moduleName 是否包含点号？（line 1006）
  │   │
  │   ├─ 无点号（line 1007-1070）
  │   │   │
  │   │   ├─ 子分支 2a：!isAppModule || !IsExistedPath(path)
  │   │   │   └─ 生成标准路径（含 prefix、zfix）
  │   │   │
  │   │   └─ 子分支 2b：isAppModule && IsExistedPath(path)
  │   │       ├─ OHOS（line 1031-1033）：无前缀 libxxx.so
  │   │       ├─ Android（line 1034-1069）：双路径处理
  │   │       └─ 其他（line 1051-1054）：含前缀
  │   │
  │   └─ 有点号（line 1071-1121）
  │       │
  │       ├─ 处理：将所有 '.' 替换为 '/'（line 1078-1081）
  │       │
  │       ├─ 子分支 3a：!isAppModule || !IsExistedPath(path)
  │       │   ├─ 生成：{prefix}/{dirs}/lib{last}{zfix}.so
  │       │   └─ ABC：{sysAbcPrefix}/{dirs}/{last}.abc
  │       │
  │       └─ 子分支 3b：isAppModule && IsExistedPath(path)
  │           ├─ OHOS（line 1106-1108）：无前缀 lib{last}.so
  │           └─ 其他（line 1110-1120）：含前缀
  │
  └─ 返回：nativeModulePath[0/1/2]
```

### 4.2 详细场景表

| 场景编号 | 条件组合 | moduleName 示例 | nativeModulePath[0] | nativeModulePath[1] | nativeModulePath[2] | 代码行 |
|---------|---------|-----------------|---------------------|---------------------|---------------------|--------|
| **场景1** | 输入已包含后缀 | `"mylib.so"` | `"{prefix}/mylib.so"` | *(未设置)* | *(未设置)* | 998-1003 |
| **场景2** | 无点号 + 系统模块 | `"multimedia"` | `"{prefix}/libmultimedia{zfix}.so"` | `"{prefix}/libmultimedia_napi{zfix}.so"` | `"{sysAbcPrefix}/multimedia.abc"` | 1008-1027 |
| **场景2a** | 无点号 + Android App | `"multimedia"` | `"{part1}/libmultimedia.so"` | `"{part2}/libmultimedia.so"` | `"{sysAbcPrefix}/multimedia.abc"` | 1034-1069 |
| **场景2b** | 无点号 + OHOS App | `"multimedia"` | `"libmultimedia.so"` | *(空)* | *(空)* | 1031-1033 |
| **场景2c** | 无点号 + 其他 App | `"multimedia"` | `"{prefix}/libmultimedia.so"` | *(空)* | *(空)* | 1051-1054 |
| **场景3** | 有点号 + 系统模块 | `"multimedia.image"` | `"{prefix}/multimedia/libimage{zfix}.so"` | `"{prefix}/multimedia/libimage_napi{zfix}.so"` | `"{sysAbcPrefix}/multimedia/image.abc"` | 1083-1102 |
| **场景3a** | 有点号 + iOS | `"multimedia.image"` | `"{prefix}/multimedia/libimage{zfix}.so"` | `"{prefix}/multimedia/libimage_napi{zfix}.so"` | `"{sysAbcPrefix}/image.abc"` | 1093-1096 |
| **场景3b** | 有点号 + OHOS App | `"multimedia.image"` | `"libimage.so"` | *(空)* | *(空)* | 1106-1108 |
| **场景3c** | 有点号 + Android App | `"multimedia.image"` | `"{prefix}/multimedia/libimage.so"` | `"{prefix}/multimedia/libimage.so"` | *(空)* | 1116-1119 |

---

## 5. 标准 OHOS 平台完整示例

### 5.1 前提条件

- **平台**：标准 OHOS（非 ARM64/Simulator，非 Windows/Mac/Linux）
- **编译时常量**：
  - `zfix = ".z"`
  - `soPostfix = ".so"`
  - `abcfix = ".abc"`
  - `sysPrefix = "/system/lib/module"`
  - `sysAbcPrefix = "/system/etc/abc"`

### 5.2 系统模块示例

#### 场景 2.1：基础系统模块

| 输入 | 值 |
|------|-----|
| `moduleName` | `"multimedia"` |
| `path` | `nullptr` |
| `relativePath` | `""` |
| `isAppModule` | `false` |

**输出结果：**

```
nativeModulePath[0]: /system/lib/module/libmultimedia.z.so
nativeModulePath[1]: /system/lib/module/libmultimedia_napi.z.so
nativeModulePath[2]: /system/etc/abc/multimedia.abc
```

**代码执行路径：**
1. Line 986-988：转小写（如果需要）
2. Line 977-988：`prefix = sysPrefix`（系统模块）
3. Line 1007：`lastDot == nullptr`（无点号）
4. Line 1008：`!isAppModule` 条件成立
5. Line 1014-1019：生成三个路径

#### 场景 2.2：带相对路径的系统模块

| 输入 | 值 |
|------|-----|
| `moduleName` | `"multimedia"` |
| `path` | `nullptr` |
| `relativePath` | `"ext"` |
| `isAppModule` | `false` |

**输出结果：**

```
nativeModulePath[0]: /system/lib/module/ext/libmultimedia.z.so
nativeModulePath[1]: /system/lib/module/ext/libmultimedia_napi.z.so
nativeModulePath[2]: /system/etc/abc/multimedia.abc
```

**代码执行路径：**
1. Line 978-984：`sysPrefix` 追加 `relativePath` → `/system/lib/module/ext`
2. 其他同场景 2.1

#### 场景 3.1：系统子模块（有点号）

| 输入 | 值 |
|------|-----|
| `moduleName` | `"multimedia.image"` |
| `path` | `nullptr` |
| `relativePath` | `""` |
| `isAppModule` | `false` |

**处理过程：**
1. Line 1072：`afterDot = "image"`
2. Line 1076：`*lastDot = '\0'` → `dupModuleName = "multimedia"`
3. Line 1078-1081：将 `.` 替换为 `/`（无其他点号，不变）
4. Line 1084-1099：生成路径

**输出结果：**

```
nativeModulePath[0]: /system/lib/module/multimedia/libimage.z.so
nativeModulePath[1]: /system/lib/module/multimedia/libimage_napi.z.so
nativeModulePath[2]: /system/etc/abc/multimedia/image.abc
```

#### 场景 3.2：多层嵌套子模块

| 输入 | 值 |
|------|-----|
| `moduleName` | `"ohos.napi.z.utils"` |
| `path` | `nullptr` |
| `isAppModule` | `false` |

**处理过程：**
1. `lastDot` 指向 `.utils` 之前的点
2. `afterDot = "utils"`
3. `dupModuleName` 替换后：`ohos/napi/z`

**输出结果：**

```
nativeModulePath[0]: /system/lib/module/ohos/napi/z/libutils.z.so
nativeModulePath[1]: /system/lib/module/ohos/napi/z/libutils_napi.z.so
nativeModulePath[2]: /system/etc/abc/ohos/napi/z/utils.abc
```

### 5.3 App 模块示例

#### 场景 2b：基础 App 模块（OHOS 特殊处理）

| 输入 | 值 |
|------|-----|
| `moduleName` | `"myapp"` |
| `path` | `"default"` |
| `appLibPathMap_["default"]` | `"/data/app/el2/100/base/com.example.app/libs"` |
| `isAppModule` | `true` |

**输出结果（OHOS 平台）：**

```
nativeModulePath[0]: libmyapp.so
nativeModulePath[1]: (空)
nativeModulePath[2]: (空)
```

**代码执行路径：**
1. Line 940-943：`prefix = appLibPathMap_["default"]`
2. Line 1028：`isAppModule && IsExistedPath(path)` 条件成立
3. Line 1029-1033（OHOS 特定）：生成 **无前缀** 的库名

**⚠️ 注意**：OHOS 平台对于 App 模块生成的路径 **不带 prefix 前缀**，这是一个特殊处理！

#### 场景 3b：App 子模块（OHOS）

| 输入 | 值 |
|------|-----|
| `moduleName` | `"com.example.helper"` |
| `path` | `"default"` |
| `isAppModule` | `true` |

**处理过程：**
1. `lastDot` 指向 `.helper` 之前的点
2. `afterDot = "helper"`
3. `dupModuleName` 替换后：`com/example`

**输出结果：**

```
nativeModulePath[0]: libhelper.so
nativeModulePath[1]: (空)
nativeModulePath[2]: (空)
```

---

## 6. 特殊处理逻辑

### 6.1 moduleName 转小写

**代码位置：** Line 945-947（Android），Line 986-988（其他）

```cpp
for (int32_t i = 0; i < lengthOfModuleName; i++) {
    dupModuleName[i] = tolower(dupModuleName[i]);
}
```

**示例：**

| 输入 | 转换后 |
|------|--------|
| `"MyModule"` | `"mymodule"` |
| `"MULTIMEDIA"` | `"multimedia"` |
| `"Test.Module"` | `"test.module"` |

### 6.2 点号转路径分隔符

**代码位置：** Line 1078-1081

```cpp
for (int32_t i = 0; i < lengthOfModuleName; i++) {
    if (*(dupModuleName + i) == '.') {
        *(dupModuleName + i) = '/';
    }
}
```

**转换规则：**

| moduleName | 转换后 | lastDot 位置 | afterDot | 用途 |
|-----------|--------|--------------|----------|------|
| `"multimedia.image"` | `"multimedia/image"` | 最后一个 `.` | `"image"` | 库名 |
| `"a.b.c"` | `"a/b/c"` | 最后一个 `.` | `"c"` | 库名 |
| `"com.example.app"` | `"com/example/app"` | 最后一个 `.` | `"app"` | 库名 |

**应用场景：**
- `nativeModulePath[0]`：`{prefix}/{转换后的路径}/lib{afterDot}{zfix}.so`
- `nativeModulePath[2]`：`{sysAbcPrefix}/{转换后的路径}.abc`

### 6.3 Android 双路径处理

**代码位置：** Line 1034-1069

**触发条件：**
- 平台：`ANDROID_PLATFORM`
- `isAppModule && IsExistedPath(path)`
- `prefix`（从 `appLibPathMap_` 获取）包含冒号 `:`

**处理逻辑：**
```cpp
std::string prefixStr = std::string(prefix);  // 如 "/data/.../libs1:/data/.../libs2"
std::size_t pos = prefixStr.find(':');
if (pos != std::string::npos) {
    sprintfResult = sprintf_s(nativeModulePath[0], pathLength, "%s/lib%s%s",
        prefixStr.substr(0, pos).c_str(), dupModuleName, soPostfix);
    libPath = prefixStr.substr(pos + 1);  // 冒号后的部分
} else {
    sprintfResult = sprintf_s(nativeModulePath[0], pathLength, "lib%s%s", dupModuleName, soPostfix);
    libPath = prefixStr;
}

// nativeModulePath[1] 使用 libPath
sprintf_s(nativeModulePath[1], pathLength, "%s/lib%s%s", libPath.c_str(), dupModuleName, soPostfix);

// 如果 libPath 存在，交换 [0] 和 [1]
if (IsExistedPath(libPath.c_str())) {
    std::swap(nativeModulePath[0], nativeModulePath[1]);
}
```

**示例：**

| 输入 | prefix 值 | nativeModulePath[0] | nativeModulePath[1] |
|------|-----------|---------------------|---------------------|
| `"myapp"` | `"/libs1:/libs2"` | `/libs1/libmyapp.so` | `/libs2/libmyapp.so` |
| `"myapp"` | `"/libs1:/libs2"` 且 `/libs2` 存在 | `/libs2/libmyapp.so` | `/libs1/libmyapp.so` *(交换后)* |

### 6.4 Android ArkUI-X 特殊处理

**代码位置：** Line 951-975

**触发条件：**
- 平台：`ANDROID_PLATFORM`
- `prefix` 包含 `/files/arkui-x` 路径

**处理逻辑：**
```cpp
std::string prefixStr(prefix);
const std::string arkuiKey = "/files/arkui-x";
std::size_t arkuiPos = prefixStr.find(arkuiKey);
if (arkuiPos != std::string::npos) {
    std::size_t endPos = arkuiPos + arkuiKey.length();
    std::size_t delimiterPos = prefixStr.find(':');
    if (delimiterPos != std::string::npos) {
        sysAbcPrefix = (arkuiPos < delimiterPos)
                           ? prefixStr.substr(0, endPos) + "/systemres/abc"
                           : prefixStr.substr(delimiterPos + 1, endPos - (delimiterPos + 1)) +
                                "/systemres/abc";
    } else {
        sysAbcPrefix = prefixStr.substr(0, endPos) + "/systemres/abc";
    }
}
```

**目的：** 确保 ABC 格式文件放在 `/data/user/0/com.example.myapplication/files/arkui-x/systemres/abc`

**示例：**

| prefix 原值 | sysAbcPrefix 调整后 |
|-------------|---------------------|
| `/data/.../files/arkui-x/libs1:/libs2` | `/data/.../files/arkui-x/systemres/abc` |
| `/libs1:/data/.../files/arkui-x/libs2` | `/data/.../files/arkui-x/systemres/abc` |
| `/data/.../files/arkui-x/libs` | `/data/.../files/arkui-x/systemres/abc` |

### 6.5 Android 模块名含下划线

**代码位置：** Line 1063-1068

```cpp
char* lastUnderScore = strrchr(dupModuleName, '_');
const char* moduleNamePart = (lastUnderScore != nullptr) ? (lastUnderScore + 1) : dupModuleName;
sprintf_s(nativeModulePath[2], pathLength, "%s/%s%s", sysAbcPrefix.c_str(), moduleNamePart, abcfix);
```

**示例：**

| moduleName | moduleNamePart | nativeModulePath[2] |
|-----------|----------------|---------------------|
| `"myapp_utils"` | `"utils"` | `{sysAbcPrefix}/utils.abc` |
| `"myapp"` | `"myapp"` | `{sysAbcPrefix}/myapp.abc` |

---

## 7. 路径生成规则总结

### 7.1 公式化表达

| 模块类型 | 路径结构 | 公式 | 示例 |
|---------|---------|------|------|
| **系统模块（无点）** | `{prefix}/lib{module}{zfix}.so` | 完整路径 | `/system/lib/module/libmultimedia.z.so` |
| **系统模块（有点）** | `{prefix}/{dirs}/lib{last}{zfix}.so` | 嵌套路径 | `/system/lib/module/multimedia/libimage.z.so` |
| **系统 NAPI 变体** | `{prefix}/lib{module}_napi{zfix}.so` | 备选实现 | `/system/lib/module/libmultimedia_napi.z.so` |
| **系统 ABC** | `{sysAbcPrefix}/{module}.abc` | 字节码 | `/system/etc/abc/multimedia.abc` |
| **系统 ABC（有点）** | `{sysAbcPrefix}/{dirs}/{last}.abc` | 嵌套字节码 | `/system/etc/abc/multimedia/image.abc` |
| **App 模块（OHOS）** | `lib{module}.so` | *无路径前缀* | `libmyapp.so` |
| **App 子模块（OHOS）** | `lib{last}.so` | *无路径前缀* | `libhelper.so` |
| **App 模块（其他）** | `{prefix}/lib{module}.so` | 含前缀 | `/data/app/.../libs/libmyapp.so` |

### 7.2 关键差异对比

#### 系统 vs App 模块（OHOS）

| 特性 | 系统模块 | App 模块（OHOS） |
|------|---------|-----------------|
| 路径前缀 | ✅ 有（`/system/lib/module`） | ❌ 无 |
| zfix 中间后缀 | ✅ 有 `.z` | ❌ 无 |
| NAPI 变体 | ✅ 生成 `libxxx_napi.z.so` | ❌ 不生成 |
| ABC 路径 | ✅ 生成 | ❌ 不生成 |
| 点号处理 | ✅ 转为路径嵌套 | ✅ 仅取最后部分 |

#### 平台差异

| 特性 | OHOS | Android | iOS | Windows/Mac/Linux |
|------|------|---------|-----|-------------------|
| App 模块路径 | 无前缀 | 双路径（冒号分隔） | 含前缀 | 含前缀 |
| ABC 路径（有点号） | 完整嵌套 | 简化（仅文件名） | 简化（仅文件名） | 完整嵌套 |
| zfix 后缀 | `.z` | 无 | 无 | 无 |

---

## 8. 潜在问题与注意事项

### 8.1 已知问题

| 问题 | 严重性 | 位置 | 描述 |
|------|--------|------|------|
| **缓冲区溢出风险** | 🟡 中 | 多处 `sprintf_s` | 虽然使用了 `sprintf_s`，但 `pathLength` 传参需确保正确 |
| **路径不一致** | 🟡 中 | iOS vs 其他 | iOS 的 ABC 路径不含子目录（line 1093-1096），其他平台含 |
| **App 模块路径前缀缺失** | 🟢 低 | OHOS Line 1031-1033 | 可能导致加载时找不到文件（依赖工作目录） |
| **相对路径覆盖** | 🟢 低 | Line 978-984 | `previewSearchPath_` 可完全覆盖 `sysPrefix` |

### 8.2 使用注意事项

1. ** moduleName 大小写**
   - 所有 moduleName 在使用前都会被转为小写
   - 建议传入时就使用小写，避免混淆

2. **App 模块的 path 键**
   - 必须先通过 `SetAppLibPath()` 注册到 `appLibPathMap_`
   - 常用键：`"default"`, 自定义名称

3. **relativePath 的影响**
   - 仅影响系统模块
   - 会追加到 `sysPrefix` 后面
   - 不影响 App 模块

4. **ABC 文件的作用**
   - 当 .so 文件不存在时，回退到加载 .abc 文件
   - .abc 是 ArkTS 字节码，由虚拟机直接执行

5. **加载优先级**
   ```
   nativeModulePath[0] → nativeModulePath[1] → nativeModulePath[2]
   （主要 .so）  →（NAPI 变体）  →（ABC 字节码）
   ```

---

## 9. 代码位置索引

| 功能 | 起始行 | 结束行 | 说明 |
|------|--------|--------|------|
| 平台常量定义 | 897 | 927 | `soPostfix`, `zfix`, `sysPrefix` 等 |
| prefix 推导逻辑 | 939 | 995 | 根据 `isAppModule` 和 `path` 确定 |
| 后缀检测（已含 .so） | 997 | 1003 | 快速路径，直接返回 |
| 无点号路径生成 | 1006 | 1070 | 系统模块和 App 模块分支 |
| 有点号路径生成 | 1071 | 1121 | 点号转路径分隔符 |
| Android 特殊处理 | 1034 | 1069 | 双路径和 ABC 文件名提取 |
| iOS 特殊处理 | 1092 | 1096 | ABC 路径简化 |

---

## 10. 附录：完整代码流程图

```
函数入口：GetNativeModulePath()
    │
    ├─[1] 确定平台相关常量（line 897-927）
    │   └─ soPostfix, zfix, sysPrefix, sysAbcPrefix
    │
    ├─[2] 复制并转小写 moduleName（line 932-937, 945-947, 986-988）
    │   └─ dupModuleName[]
    │
    ├─[3] 确定 prefix 值（line 939-995）
    │   ├─ isAppModule && IsExistedPath(path) → appLibPathMap_[path]
    │   └─ 否则 → sysPrefix [+ relativePath]
    │
    ├─[4] Android ArkUI-X 特殊处理（line 951-975）
    │   └─ 调整 sysAbcPrefix
    │
    ├─[5] 检测是否已含后缀（line 997-1003）
    │   └─ 是 → 返回 {prefix}/{moduleName}
    │
    ├─[6] 查找最后一个点号（line 1006）
    │   │
    │   ├─[6A] 无点号（line 1007-1070）
    │   │   │
    │   │   ├─ !isAppModule || !IsExistedPath(path)
    │   │   │   ├─[0] {prefix}/lib{module}{zfix}.so
    │   │   │   ├─[1] {prefix}/lib{module}_napi{zfix}.so
    │   │   │   └─[2] {sysAbcPrefix}/{module}.abc
    │   │   │
    │   │   └─ isAppModule && IsExistedPath(path)
    │   │       ├─ OHOS: lib{module}.so (无前缀)
    │   │       ├─ Android: 双路径处理
    │   │       └─ 其他: {prefix}/lib{module}.so
    │   │
    │   └─[6B] 有点号（line 1071-1121）
    │       ├─ 提取 afterDot（点后部分）
    │       ├─ 将所有 '.' 替换为 '/'
    │       │
    │       ├─ !isAppModule || !IsExistedPath(path)
    │       │   ├─[0] {prefix}/{dirs}/lib{afterDot}{zfix}.so
    │       │   ├─[1] {prefix}/{dirs}/lib{afterDot}_napi{zfix}.so
    │       │   └─[2] {sysAbcPrefix}/{dirs}/{afterDot}.abc
    │       │
    │       └─ isAppModule && IsExistedPath(path)
    │           ├─ OHOS: lib{afterDot}.so (无前缀)
    │           └─ 其他: {prefix}/{dirs}/lib{afterDot}.so
    │
    └─[7] 返回 true
```

---

**文档版本：** 1.0
**最后更新：** 2025-01-23
**适用代码版本：** OpenHarmony arkui_napi
**作者：** 代码分析自动生成
