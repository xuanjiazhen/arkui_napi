# LoadGreylistConfig 使用 nlohmann::json 重构说明

**修改日期**: 2026-02-06
**修改文件**:
- `module_manager/native_module_manager.cpp`
- `BUILD.gn`

---

## 1. 修改概述

将 `LoadGreylistConfig` 函数从手动解析JSON字符串改为使用 `nlohmann::json` 库进行解析，提高代码健壮性和可维护性。

**参考实现**: `F:\Develop\code\ability_ability_runtime\frameworks\native\ability\native\extension_config_mgr.cpp`

---

## 2. 修改内容

### 2.1 头文件修改（平台宏保护）

**文件**: `module_manager/native_module_manager.cpp`
**修改位置**: Line 16-33

**修改前**:
```cpp
#include "native_module_manager.h"

#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <unistd.h>

#ifdef ENABLE_HITRACE
#include "hitrace_meter.h"
#endif
#include "module_load_checker.h"
#include "native_engine/native_engine.h"
#include "securec.h"
#include "utils/log.h"
```

**修改后**:
```cpp
#include "native_module_manager.h"

#include <cstring>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <unistd.h>

#if !defined(WINDOWS_PLATFORM) && !defined(MAC_PLATFORM) && !defined(__BIONIC__) && !defined(IOS_PLATFORM) && \
    !defined(LINUX_PLATFORM)
#include <nlohmann/json.hpp>  // ✅ 新增，平台宏保护
#endif

#ifdef ENABLE_HITRACE
#include "hitrace_meter.h"
#endif
#include "module_load_checker.h"
#include "native_engine/native_engine.h"
#include "securec.h"
#include "utils/log.h"
```

**关键点**:
- ✅ 使用与 `CreateSharedLibsSonames` 相同的平台宏保护
- ✅ 只在 OHOS 平台下编译 JSON 解析代码
- ✅ 避免在其他平台引入不必要的依赖

---

### 2.2 IsValidLibName 函数简化

**文件**: `module_manager/native_module_manager.cpp`
**修改位置**: Line 397-409

**修改前**（过度校验）:
```cpp
static bool IsValidLibName(const std::string& libName)
{
    if (libName.empty() || libName.length() > 256) {
        MODULEMNG_HILOG_ERROR("invalid lib name length: %{public}zu", libName.length());
        return false;
    }

    // Check if it ends with .so
    if (libName.size() < 3 || libName.substr(libName.size() - 3) != ".so") {
        MODULEMNG_HILOG_ERROR("lib name must end with .so: %{public}s", libName.c_str());
        return false;
    }

    // Check for valid characters: alphanumeric, underscore, dot, hyphen
    for (char c : libName) {
        if (!std::isalnum(c) && c != '_' && c != '.' && c != '-') {
            MODULEMNG_HILOG_ERROR("invalid character in lib name: %{public}s", libName.c_str());
            return false;
        }
    }

    // Check for path traversal attempts
    if (libName.find("..") != std::string::npos) {
        MODULEMNG_HILOG_ERROR("path traversal not allowed in lib name: %{public}s", libName.c_str());
        return false;
    }

    return true;
}
```

**修改后**（只校验 .so 结尾）:
```cpp
static bool IsValidLibName(const std::string& libName)
{
    if (libName.empty()) {
        return false;
    }

    // Check if it ends with .so
    if (libName.size() < 3 || libName.substr(libName.size() - 3) != ".so") {
        MODULEMNG_HILOG_WARN("lib name must end with .so: %{public}s", libName.c_str());
        return false;
    }

    return true;
}
```

**改进点**:
- ✅ 移除长度限制（之前限制 256 字符）
- ✅ 移除字符集检查（字母、数字、下划线等）
- ✅ 移除路径遍历检查（`..` 检查）
- ✅ **只校验是否以 `.so` 结尾**
- ✅ 日志级别从 ERROR 降为 WARN（更合理）

---

### 2.3 LoadGreylistConfig 函数重构

**文件**: `module_manager/native_module_manager.cpp`
**函数位置**: Line 427-491

#### 2.3.1 修改前（手动解析）

```cpp
bool NativeModuleManager::LoadGreylistConfig(std::vector<std::string>& greylistLibs)
{
    MODULEMNG_HILOG_DEBUG("enter");
    greylistLibs.clear();

    if (access(GREYLIST_CONFIG_PATH, F_OK) != 0) {
        MODULEMNG_HILOG_DEBUG("greylist config file not found: %{public}s", GREYLIST_CONFIG_PATH);
        return false;
    }

    std::ifstream configFile(GREYLIST_CONFIG_PATH);
    if (!configFile.is_open()) {
        MODULEMNG_HILOG_ERROR("failed to open greylist config file: %{public}s", GREYLIST_CONFIG_PATH);
        return false;
    }

    std::string content((std::istreambuf_iterator<char>(configFile)),
                        std::istreambuf_iterator<char>());
    configFile.close();

    // ❌ 手动解析 JSON，容易出错
    size_t pos = 0;
    while ((pos = content.find("\"", pos)) != std::string::npos) {
        pos++; // Skip opening quote
        size_t endPos = content.find("\"", pos);
        if (endPos == std::string::npos) {
            break;
        }
        std::string libName = content.substr(pos, endPos - pos);

        if (!libName.empty() && IsValidLibName(libName)) {
            greylistLibs.push_back(libName);
            MODULEMNG_HILOG_DEBUG("added greylist lib: %{public}s", libName.c_str());
        } else if (!libName.empty()) {
            MODULEMNG_HILOG_WARN("invalid lib name in greylist: %{public}s, skipping", libName.c_str());
        }
        pos = endPos + 1;
    }

    MODULEMNG_HILOG_DEBUG("loaded %{public}zu libs from greylist", greylistLibs.size());
    return !greylistLibs.empty();
}
```

**问题**:
1. 手动字符串查找，无法处理转义字符、复杂JSON结构
2. 无法验证JSON格式正确性
3. 错误处理不完善
4. 代码可读性差

#### 2.3.2 修改后（nlohmann::json）

```cpp
bool NativeModuleManager::LoadGreylistConfig(std::vector<std::string>& greylistLibs)
{
    MODULEMNG_HILOG_DEBUG("enter");
    greylistLibs.clear();

    if (access(GREYLIST_CONFIG_PATH, F_OK) != 0) {
        MODULEMNG_HILOG_DEBUG("greylist config file not found: %{public}s", GREYLIST_CONFIG_PATH);
        return false;
    }

    std::ifstream configFile(GREYLIST_CONFIG_PATH);
    if (!configFile.is_open()) {
        MODULEMNG_HILOG_ERROR("failed to open greylist config file: %{public}s", GREYLIST_CONFIG_PATH);
        return false;
    }

    // ✅ 使用 nlohmann::json 解析
    nlohmann::json greylistConfig;
    try {
        configFile >> greylistConfig;
        configFile.close();

        if (greylistConfig.is_discarded()) {
            MODULEMNG_HILOG_ERROR("greylist config json discarded error");
            return false;
        }

        // ✅ 验证 JSON 结构（应该是数组）
        if (!greylistConfig.is_array()) {
            MODULEMNG_HILOG_ERROR("greylist config is not a valid JSON array");
            return false;
        }

        // ✅ 遍历数组
        for (const auto& item : greylistConfig) {
            if (!item.is_string()) {
                MODULEMNG_HILOG_WARN("greylist config contains non-string item, skipping");
                continue;
            }

            std::string libName = item.get<std::string>();

            // ✅ 只校验 .so 结尾
            if (!libName.empty() && IsValidLibName(libName)) {
                greylistLibs.push_back(libName);
                MODULEMNG_HILOG_DEBUG("added greylist lib: %{public}s", libName.c_str());
            } else if (!libName.empty()) {
                MODULEMNG_HILOG_WARN("invalid lib name in greylist: %{public}s, skipping", libName.c_str());
            }
        }

        MODULEMNG_HILOG_DEBUG("loaded %{public}zu libs from greylist", greylistLibs.size());
        return !greylistLibs.empty();
    } catch (const nlohmann::json::exception& e) {
        MODULEMNG_HILOG_ERROR("failed to parse greylist config json: %{public}s", e.what());
        configFile.close();
        return false;
    } catch (const std::exception& e) {
        MODULEMNG_HILOG_ERROR("unexpected error parsing greylist config: %{public}s", e.what());
        configFile.close();
        return false;
    }
}
```

**改进点**:
1. ✅ 使用标准JSON库，自动处理转义字符
2. ✅ 类型安全：`is_array()`, `is_string()`, `get<std::string>()`
3. ✅ 异常处理：捕获 `nlohmann::json::exception`
4. ✅ 代码可读性更好
5. ✅ 支持复杂的JSON结构扩展

---

### 2.4 BUILD.gn 依赖添加

**文件**: `BUILD.gn`
**修改位置**: Line 154-166

**修改前**:
```gni
} else if (!is_mingw && !is_mac && !is_linux && is_ohos_standard_system) {
  external_deps += [
    "c_utils:utils",
    "eventhandler:libeventhandler",
    "ffrt:libffrt",
    "hilog:libhilog",
    "hitrace:hitrace_meter",
    "hitrace:libhitracechain",
    "hiview:libucollection_client",
    "init:libbegetutil",
    "resource_schedule_service:ressched_client",
    "samgr:samgr_proxy",
  ]
```

**修改后**:
```gni
} else if (!is_mingw && !is_mac && !is_linux && is_ohos_standard_system) {
  external_deps += [
    "c_utils:utils",
    "eventhandler:libeventhandler",
    "ffrt:libffrt",
    "hilog:libhilog",
    "hitrace:hitrace_meter",
    "hitrace:libhitracechain",
    "hiview:libucollection_client",
    "init:libbegetutil",
    "json:nlohmann_json_static",  // ✅ 新增
    "resource_schedule_service:ressched_client",
    "samgr:samgr_proxy",
  ]
```

**依赖说明**:
- `json:nlohmann_json_static`: OpenHarmony 系统中的 nlohmann::json 库
- 虽然是 header-only 库，但 OpenHarmony 构建系统需要显式声明依赖
- 参考了 `ability_ability_runtime` 的实现方式

---

## 3. 平台宏说明

### 3.1 使用的宏定义

```cpp
#if !defined(WINDOWS_PLATFORM) && !defined(MAC_PLATFORM) && !defined(__BIONIC__) && \
    !defined(IOS_PLATFORM) && !defined(LINUX_PLATFORM)
    // 只在 OHOS 平台编译
    #include <nlohmann/json.hpp>

    bool LoadGreylistConfig(std::vector<std::string>& greylistLibs);
    void CreateSharedLibsSonames();
#endif
```

### 3.2 平台映射

| 宏定义 | 平台 | 是否编译 JSON 代码 |
|--------|------|-------------------|
| `WINDOWS_PLATFORM` | Windows (mingw) | ❌ 否 |
| `MAC_PLATFORM` | macOS | ❌ 否 |
| `__BIONIC__` | Android | ❌ 否 |
| `IOS_PLATFORM` | iOS | ❌ 否 |
| `LINUX_PLATFORM` | Linux | ❌ 否 |
| **无以上宏** | **OHOS** | ✅ **是** |

### 3.3 为什么需要平台宏

1. **功能相关**: `LoadGreylistConfig` 和 `CreateSharedLibsSonames` 是 OHOS 特有的 namespace 功能
2. **避免依赖**: 其他平台不需要链接 nlohmann_json
3. **编译优化**: 减少不必要的代码编译

---

## 4. JSON 配置文件格式

**文件路径**: `/data/service/el0/public/musl_namespace_config/greylist.json`

**格式** (标准 JSON 数组):
```json
[
  "libexample1.so",
  "libexample2.so",
  "libexample3.z.so"
]
```

**校验规则**:
- ✅ 必须是 JSON 数组格式
- ✅ 数组元素必须是字符串
- ✅ 字符串必须以 `.so` 结尾（大小写敏感）
- ❌ **不校验**长度、字符集、路径等

**支持的特性**:
- ✅ 标准 JSON 数组格式
- ✅ 转义字符（自动处理）
- ✅ 空格、换行符（自动忽略）
- ✅ Unicode 支持

**示例（有效）**:
```json
[
  "libtest.so",
  "lib-test.so",
  "lib_test.so",
  "lib.test.so",
  "libtest.z.so",
  "lib非常长的库名称123.so"
]
```

**示例（无效）**:
```json
[
  "libtest.dll",     // ❌ 不以 .so 结尾
  "libtest",         // ❌ 无扩展名
  123,               // ❌ 不是字符串
  ["nested.so"]      // ❌ 不是数组元素
]
```

---

## 5. 改进对比

| 特性 | 修改前（手动解析） | 修改后（nlohmann::json） |
|------|-------------------|-------------------------|
| **解析方式** | 字符串查找 | 标准JSON库 |
| **错误处理** | 简单检查 | 异常捕获 |
| **类型检查** | 无 | `is_array()`, `is_string()` |
| **转义字符** | 不支持 | 自动处理 |
| **校验规则** | 严格（长度、字符等） | 宽松（仅 .so 结尾） |
| **代码行数** | ~30 行 | ~65 行（含异常处理） |
| **可维护性** | 低 | 高 |
| **扩展性** | 差 | 好 |
| **性能** | 略快 | 略慢（可忽略） |
| **依赖** | 无 | nlohmann_json_static |
| **平台宏** | 无 | OHOS 专用 |

---

## 6. 测试建议

### 6.1 单元测试

```cpp
// 测试正常JSON数组
TEST(NativeModuleManager, LoadGreylistConfig_ValidJson)
{
    // 创建测试文件
    std::ofstream testFile("/data/test_greylist.json");
    testFile << R"([
        "libtest1.so",
        "libtest2.z.so",
        "libtest3.so"
    ])";
    testFile.close();

    std::vector<std::string> greylistLibs;
    bool result = manager->LoadGreylistConfig(greylistLibs);

    EXPECT_TRUE(result);
    EXPECT_EQ(greylistLibs.size(), 3);
    EXPECT_EQ(greylistLibs[0], "libtest1.so");
}

// 测试无效JSON
TEST(NativeModuleManager, LoadGreylistConfig_InvalidJson)
{
    std::ofstream testFile("/data/test_greylist.json");
    testFile << R"([invalid json])";
    testFile.close();

    std::vector<std::string> greylistLibs;
    bool result = manager->LoadGreylistConfig(greylistLibs);

    EXPECT_FALSE(result);
}

// 测试非数组JSON
TEST(NativeModuleManager, LoadGreylistConfig_NotArray)
{
    std::ofstream testFile("/data/test_greylist.json");
    testFile << R"({"key": "value"})";
    testFile.close();

    std::vector<std::string> greylistLibs;
    bool result = manager->LoadGreylistConfig(greylistLibs);

    EXPECT_FALSE(result);
}

// 测试不以.so结尾的库名（应被过滤）
TEST(NativeModuleManager, LoadGreylistConfig_NotSoEnding)
{
    std::ofstream testFile("/data/test_greylist.json");
    testFile << R"([
        "libtest1.so",
        "libtest2.dll",
        "libtest3.so"
    ])";
    testFile.close();

    std::vector<std::string> greylistLibs;
    bool result = manager->LoadGreylistConfig(greylistLibs);

    EXPECT_TRUE(result);
    EXPECT_EQ(greylistLibs.size(), 2);  // libtest2.dll 被过滤
    EXPECT_EQ(greylistLibs[0], "libtest1.so");
    EXPECT_EQ(greylistLibs[1], "libtest3.so");
}

// 测试包含特殊字符的库名（应该接受）
TEST(NativeModuleManager, LoadGreylistConfig_SpecialCharacters)
{
    std::ofstream testFile("/data/test_greylist.json");
    testFile << R"([
        "lib-test.so",
        "lib_test.so",
        "lib.test.so",
        "lib测试.so",
        "lib very long name with spaces.so"
    ])";
    testFile.close();

    std::vector<std::string> greylistLibs;
    bool result = manager->LoadGreylistConfig(greylistLibs);

    EXPECT_TRUE(result);
    EXPECT_EQ(greylistLibs.size(), 5);  // 全部接受（只校验.so结尾）
}
```

### 6.2 集成测试

1. **编译测试**:
```bash
# 从 OpenHarmony 根目录
./build.sh --product-name <product> --build-target arkui_napi
```

2. **运行时测试**:
- 确保 `/data/service/el0/public/musl_namespace_config/greylist.json` 存在
- 验证模块加载时能正确读取灰名单配置
- 检查日志输出 `MODULEMNG_HILOG_DEBUG("loaded %{public}zu libs from greylist", ...)`

---

## 7. 兼容性说明

### 7.1 向后兼容

- ✅ JSON 文件格式保持不变（字符串数组）
- ✅ 函数签名不变
- ✅ 调用方式不变
- ✅ 返回值语义不变

### 7.2 校验规则变化

**旧实现**（严格）:
- 长度限制: 1-256 字符
- 字符集: 字母、数字、`_`、`.`、`-`
- 禁止: `..`（路径遍历）
- 必须以 `.so` 结尾

**新实现**（宽松）:
- ✅ 必须以 `.so` 结尾
- ✅ 非空
- ✅ 其他不限制

**影响**:
- 旧的灰名单文件仍然有效
- 新的灰名单文件可以包含任意字符（只要以 .so 结尾）
- 更灵活，允许特殊字符库名

---

## 8. 未来扩展

使用 `nlohmann::json` 后，可以轻松支持更复杂的配置格式：

### 8.1 对象格式（可选）

```json
{
  "greylist": [
    {
      "name": "libexample.so",
      "version": "1.0.0",
      "comment": "Example library"
    }
  ]
}
```

### 8.2 命名空间配置（可选）

```json
{
  "namespaces": {
    "default": {
      "permitted_paths": ["/system/lib", "/vendor/lib"],
      "greylist": ["lib1.so", "lib2.so"]
    }
  }
}
```

---

## 9. 修改总结

| 文件 | 修改类型 | 修改内容 |
|------|---------|---------|
| `native_module_manager.cpp` | 头文件引入 | 添加平台宏保护的 `#include <nlohmann/json.hpp>` |
| `native_module_manager.cpp` | 函数简化 | `IsValidLibName` 只校验 `.so` 结尾 |
| `native_module_manager.cpp` | 函数重构 | `LoadGreylistConfig` 使用 `nlohmann::json` |
| `BUILD.gn` | 依赖添加 | 添加 `json:nlohmann_json_static` |

**代码行数变化**:
- 原代码: ~75 行（含 IsValidLibName）
- 新代码: ~95 行（含异常处理和日志）
- 增加: ~20 行

**编译依赖**:
- 新增: `json:nlohmann_json_static` (仅 OHOS 平台)

**运行时依赖**:
- 无新增

**平台支持**:
- OHOS: ✅ 完整支持
- Windows/macOS/Linux/Android/iOS: ❌ 不编译（平台宏保护）

---

## 10. 参考

- [nlohmann/json 官方文档](https://json.nlohmann.me/)
- 参考实现: `ability_ability_runtime/frameworks/native/ability/native/extension_config_mgr.cpp`
- OpenHarmony nlohmann_json: `//third_party/json`

---

**修改完成日期**: 2026-02-06
**审核状态**: 待审核
**测试状态**: 待测试
