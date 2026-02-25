# ability_runtime 代码问题修复指导手册

**生成日期**: 2026-02-06
**适用版本**: 当前主分支
**维护者**: Development Team

---

## 目录

1. [修复优先级总览](#一修复优先级总览)
2. [高优先级问题修复指导](#二高优先级问题修复指导)
3. [中优先级问题修复指导](#三中优先级问题修复指导)
4. [低优先级问题修复指导](#四低优先级问题修复指导)
5. [修复检查清单](#五修复检查清单)
6. [修复模板和最佳实践](#六修复模板和最佳实践)

---

## 一、修复优先级总览

### 问题统计汇总

| 优先级 | 类别 | 问题数 | 涉及文件 | 修复工作量 | 风险 |
|--------|------|--------|----------|-----------|------|
| **高** | thread_local变量直接赋值 | 25处 | 1个生产文件<br>7个测试文件 | 2-3天 | 高 |
| **中** | 指针未检查 | 30处 | 6个服务文件 | 3-5天 | 中 |
| **低** | C风格转换/malloc | 55+处 | 20+个文件 | 持续进行 | 低 |
| **总计** | - | **110+处** | **30+个文件** | **约2周** | - |

### 修复时间表

```
Week 1: 高优先级问题（2-3天）
  ├─ Day 1-2: 修复测试代码中的24处thread_local变量赋值
  └─ Day 3: 修复生产代码中的1处thread_local变量赋值

Week 2-3: 中优先级问题（3-5天）
  ├─ Day 1: 立即修复的2处高风险问题
  ├─ Day 2-3: GetScheduler/GetAbilityInfo检查（18处）
  └─ Day 4-5: 其他指针检查问题（10处）

Week 4+: 低优先级问题（持续进行）
  └─ C风格转换、malloc优化等
```

---

## 二、高优先级问题修复指导

### 问题1: thread_local 变量直接赋值（25处）

#### 问题描述
`StartAbilityUtils::startAbilityInfo` 等9个thread_local变量被多处直接赋值，缺少RAII封装。

#### 受影响的变量

```cpp
// services/abilitymgr/src/utils/start_ability_utils.h
thread_local std::shared_ptr<StartAbilityInfo> StartAbilityUtils::startAbilityInfo;
thread_local std::shared_ptr<StartAbilityInfo> StartAbilityUtils::callerAbilityInfo;
thread_local bool StartAbilityUtils::skipCrowTest;
thread_local bool StartAbilityUtils::skipStartOther;
thread_local bool StartAbilityUtils::skipErms;
thread_local int32_t StartAbilityUtils::ermsResultCode;
thread_local bool StartAbilityUtils::isWantWithAppCloneIndex;
thread_local bool StartAbilityUtils::ermsSupportBackToCallerFlag;
thread_local bool StartAbilityUtils::startSpecifiedBySCB;
```

#### 修复位置清单

##### 生产代码（1处）

| 序号 | 文件 | 行号 | 当前代码 | 修复方案 |
|------|------|------|---------|---------|
| 1 | `services/abilitymgr/src/interceptor/ecological_rule_interceptor.cpp` | 149 | `StartAbilityUtils::startAbilityInfo = ...` | 使用StartAbilityInfoWrap |

**详细修复**:

```cpp
// ❌ 错误代码（ecological_rule_interceptor.cpp:149）
StartAbilityUtils::startAbilityInfo = StartAbilityInfo::CreateStartAbilityInfo(
    want, userId, appIndex, nullptr);

// ✅ 正确代码
{
    StartAbilityInfoWrap threadLocalInfo(want, userId, appIndex, nullptr);
    // threadLocalInfo 作用域内，可以安全访问 thread_local 变量
    // 析构时自动清理
}
```

##### 测试代码（24处）

**文件清单**:

| 序号 | 文件 | 行号 | 数量 |
|------|------|------|------|
| 1 | `test/unittest/ability_jump_interceptor_test/ability_jump_interceptor_test.cpp` | 818, 849, 878, 919, 960 | 5 |
| 2 | `test/unittest/ability_interceptor_test/ability_interceptor_test.cpp` | 714 | 1 |
| 3 | `test/unittest/ability_permission_util_second_test/ability_permission_util_second_test.cpp` | 175, 199, 223 | 3 |
| 4 | `test/unittest/ability_manager_service_mock_test/ability_manager_service_mock_test.cpp` | 284, 311 | 2 |
| 5 | `test/unittest/start_ability_utils_test/start_ability_utils_test.cpp` | 107, 121, 140, 159, 178, 201, 248, 333, 339 | 9 |
| 6 | `test/unittest/screen_unlock_interceptor_test/screen_unlock_interceptor_test.cpp` | 76, 106, 184, 208 | 4 |
| 7 | `test/unittest/ability_jump_interceptor_test/mock/src/mock_my_flag.cpp` | 34 | 1 |

**测试代码修复模板**:

```cpp
// ❌ 错误模式
TEST_F(SomeTest, TestCaseName) {
    StartAbilityUtils::startAbilityInfo = std::make_shared<StartAbilityInfo>();

    // 测试代码...

    StartAbilityUtils::startAbilityInfo = nullptr;  // 需要手动清理
}

// ✅ 正确模式
TEST_F(SomeTest, TestCaseName) {
    {
        StartAbilityInfoWrap threadLocalInfo(want, userId, appIndex, callerToken);

        // 测试代码...
        // threadLocalInfo 会自动管理 thread_local 变量的生命周期
    }
    // 退出作用域时自动清理，不需要手动重置
}
```

**特殊情况 - mock文件**:

```cpp
// test/unittest/ability_jump_interceptor_test/mock/src/mock_my_flag.cpp:34
// ❌ 删除这行
std::shared_ptr<StartAbilityInfo> StartAbilityUtils::startAbilityInfo = nullptr;

// ✅ 应该依赖原始定义，不要在mock中重新定义
```

#### 修复步骤

1. **备份测试代码**
   ```bash
   git checkout -b fix/thread_local_variables
   ```

2. **修复生产代码**（1处）
   - 打开 `services/abilitymgr/src/interceptor/ecological_rule_interceptor.cpp`
   - 定位到149行
   - 使用 `StartAbilityInfoWrap` 封装

3. **修复测试代码**（24处）
   - 按文件清单逐个修复
   - 使用RAII模式替代直接赋值
   - 删除手动清理代码

4. **验证**
   ```bash
   # 编译
   ./build.sh --product-name <product> --build-target ability_runtime_test

   # 运行测试
   ./run_test.sh ability_runtime_unittest
   ```

---

## 三、中优先级问题修复指导

### 问题2: 指针未检查（30处）

#### 分类汇总

| 类型 | 数量 | 立即修复 | 计划修复 |
|------|------|---------|---------|
| GetScheduler()未检查 | 5 | 1 | 4 |
| GetAbilityInfo()未检查 | 16 | 1 | 15 |
| 容器访问未检查 | 4 | 1 | 3 |
| 智能指针.get()未检查 | 3 | 0 | 3 |
| thread_local直接读取 | 2 | 0 | 2 |
| **总计** | **30** | **3** | **27** |

### 立即修复（3处高风险）

#### 位置1: GetScheduler未检查

| 文件 | 行号 | 函数 | 风险 |
|------|------|------|------|
| `services/abilitymgr/src/ability_manager_service.cpp` | 13414 | 未知 | 高 |

```cpp
// ❌ 当前代码
targetRecord->GetScheduler()->OnExecuteIntent(abilityRequest.want);

// ✅ 修复后
auto scheduler = targetRecord->GetScheduler();
if (scheduler == nullptr) {
    TAG_LOGE(AAFwkTag::ABILITYMGR, "scheduler is null");
    return ERR_NULL_OBJECT;
}
scheduler->OnExecuteIntent(abilityRequest.want);
```

#### 位置2: 容器begin()未检查

| 文件 | 行号 | 函数 | 风险 |
|------|------|------|------|
| `services/abilitymgr/src/ability_manager_service.cpp` | 4268 | 未知 | 高 |

```cpp
// ❌ 当前代码
extensionSessionInfo->want.SetElementName(bundleInfo.name,
    bundleInfo.abilityInfos.begin()->name);

// ✅ 修复后
if (bundleInfo.abilityInfos.empty()) {
    TAG_LOGE(AAFwkTag::ABILITYMGR, "abilityInfos is empty");
    return ERR_INVALID_VALUE;
}
extensionSessionInfo->want.SetElementName(bundleInfo.name,
    bundleInfo.abilityInfos.begin()->name);
```

#### 位置3: GetAbilityInfo未检查

| 文件 | 行号 | 函数 | 风险 |
|------|------|------|------|
| `services/appmgr/src/module_running_record.cpp` | 194 | 未知 | 高 |

```cpp
// ❌ 当前代码
ability->GetAbilityInfo()->bundleName.c_str()

// ✅ 修复后
auto abilityInfo = ability->GetAbilityInfo();
if (abilityInfo == nullptr) {
    TAG_LOGE(AAFwkTag::APPMGR, "abilityInfo is null");
    return;
}
TAG_LOGI(AAFwkTag::APPMGR, "bundle: %{public}s", abilityInfo->bundleName.c_str());
```

### 计划修复（27处）

#### GetScheduler相关（4处）

| 文件 | 行号 | 当前检查状态 |
|------|------|-------------|
| `services/abilitymgr/src/data_ability/data_ability_manager.cpp` | 140 | 部分检查（安全）|
| `services/abilitymgr/src/data_ability/data_ability_manager.cpp` | 187 | 部分检查（安全）|
| `services/abilitymgr/src/data_ability/data_ability_manager.cpp` | 420 | 部分检查（安全）|
| `services/abilitymgr/src/data_ability/data_ability_manager.cpp` | 554 | 部分检查（安全）|

**评价**: 这些位置已有第一层检查，风险较低，可作为优化项。

#### GetAbilityInfo相关（15处）

**文件清单**:

| 文件 | 行号 | 优先级 |
|------|------|--------|
| `services/appmgr/src/module_running_record.cpp` | 261 | 中 |
| `services/appmgr/src/app_running_record.cpp` | 990 | 中 |
| `services/appmgr/src/app_running_record.cpp` | 991 | 中 |
| `services/appmgr/src/app_running_record.cpp` | 1020 | 低（已检查）|
| `services/appmgr/src/app_running_record.cpp` | 1047 | 低（已检查）|
| `services/appmgr/src/app_running_record.cpp` | 1124 | 中 |
| `services/appmgr/src/app_running_record.cpp` | 1976 | 中 |
| `services/appmgr/src/app_running_record.cpp` | 1977 | 中 |
| `services/appmgr/src/app_mgr_service_inner.cpp` | 3566 | 中 |
| `services/abilitymgr/src/ability_connect_manager.cpp` | 805 | 低 |
| `services/abilitymgr/src/ability_connect_manager.cpp` | 2076 | 低 |
| `services/abilitymgr/src/ability_connect_manager.cpp` | 2738 | 低 |
| `services/abilitymgr/src/ability_connect_manager.cpp` | 2752 | 低 |
| `services/abilitymgr/src/ability_connect_manager.cpp` | 2755 | 低 |
| `services/abilitymgr/src/ability_connect_manager.cpp` | 2764 | 低 |
| `services/abilitymgr/src/utils/start_ability_utils.cpp` | 58-60 | 中 |
| `services/abilitymgr/src/utils/start_ability_utils.cpp` | 82-83 | 中 |

**修复模板**:

```cpp
// 模式1: 直接访问
// ❌ 错误
auto type = record->GetAbilityInfo()->type;

// ✅ 正确
auto abilityInfo = record->GetAbilityInfo();
if (abilityInfo == nullptr) {
    TAG_LOGE(TAG, "abilityInfo is null");
    return ERR_NULL_OBJECT;
}
auto type = abilityInfo->type;

// 模式2: 多次访问（优化为一次调用）
// ❌ 低效
if (record->GetAbilityInfo() && record->GetAbilityInfo()->type == Type::PAGE)

// ✅ 高效
auto abilityInfo = record->GetAbilityInfo();
if (abilityInfo != nullptr && abilityInfo->type == Type::PAGE)
```

#### 容器访问（3处）

| 文件 | 行号 | 状态 |
|------|------|------|
| `services/appmgr/src/app_mgr_event.cpp` | 113 | 已检查（无需修复）|
| `services/appmgr/src/app_mgr_event.cpp` | 167 | 已检查（无需修复）|
| `services/abilitymgr/src/utils/start_options_utils.cpp` | 100 | 已检查（无需修复）|

#### 智能指针.get()（3处）

| 文件 | 行号 | 说明 |
|------|------|------|
| `test/mock/.../appmgr_test_service.cpp` | 92 | 测试代码 |
| `test/mock/.../appmgr_test_service.cpp` | 93 | 测试代码 |
| `test/mock/.../appmgr_test_service.cpp` | 127 | 测试代码 |

**评价**: 测试代码，低优先级。

---

## 四、低优先级问题修复指导

### 问题3: C风格类型转换（55+处）

#### 问题分类

**3.1 C风格指针转换（40+处）**

主要文件清单:

| 文件 | 行号 | 数量 | 优先级 |
|------|------|------|--------|
| `frameworks/js/napi/mission_manager/distributed_mission_manager.cpp` | 279, 286, 354, 360, 457, 1387, 1417 | 7 | 高 |
| `frameworks/cj/ffi/app/errormanager/cj_error_observer.cpp` | 28 | 1 | 高 |
| `frameworks/js/napi/inner/napi_ability_common/api_common_ability_execute_utils.cpp` | 1225 | 1 | 高 |
| `frameworks/js/napi/featureAbility/api_data_ability_observer.cpp` | 119 | 1 | 高 |
| `frameworks/js/napi/featureAbility/data_ability/include/api_async_proxy.h` | 134, 138 | 2 | 高 |
| `frameworks/ets/ani/ani_common/src/ets_application_context_utils.cpp` | 450, 459, 505 | 3 | 中 |
| `frameworks/native/appkit/ability_runtime/context/js_application_context_utils.cpp` | 1698 | 1 | 中 |
| `services/abilitymgr/src/mission/mission_data_storage.cpp` | 241 | 1 | 高 |
| `frameworks/native/ability/native/insight_intent_executor/ets_insight_intent_executor.cpp` | 501 | 1 | 高 |
| `cj_environment/frameworks/cj_environment/src/dynamic_loader_win.cpp` | 35 | 1 | 高 |

**修复模板**:

```cpp
// 模式1: 基本类型指针转换
// ❌ 错误
SyncRemoteMissionsContext* ctx = (SyncRemoteMissionsContext*)data;

// ✅ 正确
SyncRemoteMissionsContext* ctx = static_cast<SyncRemoteMissionsContext*>(data);

// 模式2: void* 转换
// ❌ 错误
Register((void *)callback)

// ✅ 正确
Register(static_cast<void *>(callback))

// 模式3: 不相关指针转换
// ❌ 错误
return (void*)GetProcAddress((HMODULE)so, symbol);

// ✅ 正确
return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(so), symbol));
```

**3.2 malloc使用（15+处）**

主要文件清单:

| 文件 | 行号 | 优先级 |
|------|------|--------|
| `services/abilitymgr/src/mission/mission_data_storage.cpp` | 241 | 高 |
| `services/appmgr/src/app_spawn_msg_wrapper.cpp` | 103 | 中 |
| `frameworks/c/ability_runtime/src/start_options_impl.cpp` | 232, 281 | 高 |
| `frameworks/cj/ffi/cj_want_ffi.cpp` | 63 | 高 |
| `frameworks/cj/ffi/cj_element_name_ffi.cpp` | 45 | 高 |
| `frameworks/cj/ffi/cj_ability_delegator_args.cpp` | 86, 91 | 中 |
| `frameworks/cj/ffi/application_context/src/cj_utils_ffi.cpp` | 29, 47 | 中 |
| `frameworks/cj/ffi/cj_application_context_ffi.cpp` | 53, 202 | 中 |
| `frameworks/cj/ffi/app/errormanager/cj_error_observer.cpp` | 28 | 高 |
| `frameworks/cj/ffi/app/app_manager/cj_app_manager.cpp` | 155 | 中 |
| `frameworks/native/appkit/ability_runtime/app/cj_ability_stage.cpp` | 33, 73 | 中 |
| `frameworks/native/ability/native/ability_runtime/cj_ability_object.cpp` | 63 | 中 |

**修复模板**:

```cpp
// 模式1: 基本类型数组
// ❌ 错误
uint8_t* data = (uint8_t*) malloc(dataLength);
if (data == nullptr) { /* ... */ }
free(data);

// ✅ 方案1: 使用vector
std::vector<uint8_t> data(dataLength);
// 使用 &data[0] 或 data.data()

// ✅ 方案2: 使用unique_ptr
auto data = std::make_unique<uint8_t[]>(dataLength);
// 自动释放，无需手动 free

// 模式2: 结构体
// ❌ 错误
CJWantParams* buffer = static_cast<CJWantParams*>(malloc(sizeof(CJWantParams)));
if (buffer == nullptr) { /* ... */ }
free(buffer);

// ✅ 正确
auto buffer = std::make_unique<CJWantParams>();
// 或者
auto buffer = new CJWantParams();
// 使用智能指针管理
```

**3.3 NULL vs nullptr（6+处）**

| 文件 | 行号 | 说明 |
|------|------|------|
| `frameworks/native/appkit/app/main_thread.cpp` | 3204 | sigaction参数 |
| `frameworks/native/ability/native/recovery/ability_recovery.cpp` | 256 | mmap参数 |
| `frameworks/native/ability/native/child_process_manager/child_process_manager.cpp` | 223 | waitpid参数 |
| `frameworks/js/napi/auto_fill_manager/js_auto_save_request_callback.cpp` | 105 | napi_call_function |
| `frameworks/js/napi/inner/napi_ability_common/napi_common_ability_execute_utils.cpp` | 1371, 1471 | napi_get_cb_info |

**修复模板**:

```cpp
// ❌ 错误
sigaction(MUSL_SIGNAL_JSHEAP, &sigAct, NULL);
napi_call_function(env, obj, func, 0, NULL, nullptr);

// ✅ 正确
sigaction(MUSL_SIGNAL_JSHEAP, &sigAct, nullptr);
napi_call_function(env, obj, func, 0, nullptr, nullptr);

// 注意: 日志字符串中的 "NULL" 可以保持不变
TAG_LOGE(TAG, "Remote is NULL");  // 这是文本，不是宏
```

**3.4 数组大小计算（10+处）**

| 文件 | 行号 | 数量 |
|------|------|------|
| `services/common/include/hilog_tag_wrapper.h` | 118, 129, 140, 151, 162, 174, 184, 194 | 8 |
| `services/common/src/event_report.cpp` | 893 | 1 |
| `cj_environment/frameworks/cj_environment/src/dynamic_loader_ohos.cpp` | 101 | 1 |

```cpp
// ❌ 错误
if (offset >= sizeof(tagNames) / sizeof(const char*)) {

// ✅ 正确（C++17）
#include <iterator>
if (offset >= std::size(tagNames)) {
```

---

## 五、修复检查清单

### 阶段1: 高优先级问题（Week 1）

- [ ] **P1-生产代码**: 修复 `services/abilitymgr/src/interceptor/ecological_rule_interceptor.cpp:149`
- [ ] **P1-测试-1**: `test/unittest/ability_jump_interceptor_test/...` (5处)
- [ ] **P1-测试-2**: `test/unittest/ability_interceptor_test/...` (1处)
- [ ] **P1-测试-3**: `test/unittest/ability_permission_util_second_test/...` (3处)
- [ ] **P1-测试-4**: `test/unittest/ability_manager_service_mock_test/...` (2处)
- [ ] **P1-测试-5**: `test/unittest/start_ability_utils_test/...` (9处)
- [ ] **P1-测试-6**: `test/unittest/screen_unlock_interceptor_test/...` (4处)
- [ ] **P1-mock**: 删除 `test/unittest/ability_jump_interceptor_test/mock/...` 中的重复定义
- [ ] **验证**: 运行所有单元测试确保无回归

### 阶段2: 中优先级问题（Week 2-3）

#### 立即修复（3处）

- [ ] **P2-立即-1**: `services/abilitymgr/src/ability_manager_service.cpp:13414` - GetScheduler
- [ ] **P2-立即-2**: `services/abilitymgr/src/ability_manager_service.cpp:4268` - 容器检查
- [ ] **P2-立即-3**: `services/appmgr/src/module_running_record.cpp:194` - GetAbilityInfo

#### 计划修复（27处）

**GetScheduler** (4处):
- [ ] **P2-Sched-1**: `data_ability_manager.cpp:140`
- [ ] **P2-Sched-2**: `data_ability_manager.cpp:187`
- [ ] **P2-Sched-3**: `data_ability_manager.cpp:420`
- [ ] **P2-Sched-4**: `data_ability_manager.cpp:554`

**GetAbilityInfo** (15处):
- [ ] **P2-Info-1**: `module_running_record.cpp:261`
- [ ] **P2-Info-2**: `app_running_record.cpp:990`
- [ ] **P2-Info-3**: `app_running_record.cpp:991`
- [ ] **P2-Info-4**: `app_running_record.cpp:1124`
- [ ] **P2-Info-5**: `app_running_record.cpp:1976`
- [ ] **P2-Info-6**: `app_running_record.cpp:1977`
- [ ] **P2-Info-7**: `app_mgr_service_inner.cpp:3566`
- [ ] **P2-Info-8**: `start_ability_utils.cpp:58-60`
- [ ] **P2-Info-9**: `start_ability_utils.cpp:82-83`
- [ ] 其他低风险项可选修复

### 阶段3: 低优先级问题（持续进行）

- [ ] **P3-Cast**: C风格类型转换（40+处）
- [ ] **P3-Malloc**: malloc优化（15+处）
- [ ] **P3-NULL**: NULL替换为nullptr（6+处）
- [ ] **P3-Size**: 数组大小计算（10+处）

---

## 六、修复模板和最佳实践

### 模板1: thread_local 变量封装

```cpp
// ✅ 正确模式
{
    StartAbilityInfoWrap threadLocalInfo(want, userId, appIndex, callerToken);

    // 在这个作用域内安全访问 thread_local 变量
    auto info = StartAbilityUtils::startAbilityInfo;

} // 自动清理

// ❌ 错误模式
StartAbilityUtils::startAbilityInfo = ...;
// ... 使用
StartAbilityUtils::startAbilityInfo = nullptr; // 需要手动清理，容易遗漏
```

### 模板2: 指针检查

```cpp
// ✅ 单步检查
auto ptr = obj->GetPtr();
if (ptr == nullptr) {
    TAG_LOGE(TAG, "ptr is null");
    return ERR_NULL_OBJECT;
}
ptr->Method();

// ✅ 链式检查（优化）
auto ptr1 = obj->GetPtr1();
if (ptr1 == nullptr) {
    return ERR_NULL_OBJECT;
}
auto ptr2 = ptr1->GetPtr2();
if (ptr2 == nullptr) {
    return ERR_NULL_OBJECT;
}
ptr2->Method();

// ❌ 错误的链式调用
obj->GetPtr1()->GetPtr2()->Method(); // 中间返回值未检查
```

### 模板3: 容器访问

```cpp
// ✅ 安全访问
if (container.empty()) {
    TAG_LOGE(TAG, "container is empty");
    return ERR_INVALID_VALUE;
}
auto& value = *container.begin();

// ✅ 使用 at()（自动检查）
try {
    auto value = container.at(0);
} catch (const std::out_of_range& e) {
    TAG_LOGE(TAG, "index out of range");
    return ERR_INVALID_VALUE;
}

// ❌ 不安全访问
auto value = *container.begin(); // 未检查是否为空
auto value = container[0];       // 未检查是否越界
```

### 模板4: 内存管理

```cpp
// ✅ 使用智能指针
auto data = std::make_unique<uint8_t[]>(length);
// 自动释放，无需手动 delete[]

// ✅ 使用容器
std::vector<uint8_t> data(length);
// 自动管理内存，支持STL算法

// ❌ 手动管理（不推荐）
uint8_t* data = static_cast<uint8_t*>(malloc(length));
if (data) {
    // ...
    free(data); // 容易遗漏或异常时未执行
}
```

### 模板5: 类型转换

```cpp
// ✅ 相关类型转换：static_cast
int i = 42;
void* p = &i;
int* pi = static_cast<int*>(p);

// ✅ 不相关类型转换：reinterpret_cast
int i = 42;
double* pd = reinterpret_cast<double*>(&i); // 明确表示危险操作

// ✅ const移除：const_cast
const int ci = 42;
int* pi = const_cast<int*>(&ci);

// ❌ 避免C风格转换
int* pi = (int*)p; // 不安全，难以检查
```

### 模板6: 现代C++最佳实践

```cpp
// ✅ 使用 nullptr 而非 NULL
if (ptr == nullptr) { }

// ✅ 使用 auto 简化类型
auto it = container.begin();

// ✅ 使用范围for循环
for (const auto& item : container) {
    // 处理 item
}

// ✅ 使用 enum class 而非 enum
enum class Color { Red, Green, Blue };

// ✅ 使用 constexpr 定义常量
constexpr int MAX_SIZE = 100;

// ✅ 使用 noexcept 标记不抛异常的函数
void SafeFunction() noexcept {
    // 不会抛出异常的代码
}
```

---

## 七、工具和脚本

### 7.1 自动化检测脚本

**查找所有 thread_local 变量直接赋值**:
```bash
grep -rn "StartAbilityUtils::startAbilityInfo\s*=" \
  --include="*.cpp" \
  --include="*.h" \
  f:/Develop/code/ability_ability_runtime/
```

**查找未检查的 GetScheduler()**:
```bash
grep -rn "GetScheduler()->" \
  --include="*.cpp" \
  f:/Develop/code/ability_ability_runtime/services/
```

**查找未检查的 GetAbilityInfo()**:
```bash
grep -rn "GetAbilityInfo()->" \
  --include="*.cpp" \
  f:/Develop/code/ability_ability_runtime/services/
```

**查找容器 begin() 未检查**:
```bash
grep -rn "\.begin()->" \
  --include="*.cpp" \
  f:/Develop/code/ability_ability_runtime/services/
```

**查找C风格类型转换**:
```bash
grep -rn "\(\w+\*\)\s*\w" \
  --include="*.cpp" \
  f:/Develop/code/ability_ability_runtime/
```

**查找 malloc 使用**:
```bash
grep -rn "malloc\(" \
  --include="*.cpp" \
  f:/Develop/code/ability_ability_runtime/
```

### 7.2 clang-tidy 配置

创建 `.clang-tidy` 文件:
```yaml
Checks: >
  -cppcoreguidelines-pro-bounds-array-to-pointer-decay
  -cppcoreguidelines-pro-type-cstyle-cast
  -cppcoreguidelines-pro-type-reinterpret-cast
  -cppcoreguidelines-avoid-malloc
  -hicpp-no-malloc
  -modernize-use-nullptr
  -modernize-use-auto
  -modernize-avoid-c-arrays
WarningsAsErrors: ''
HeaderFilterRegex: '.*'
FormatStyle: file
```

运行clang-tidy:
```bash
clang-tidy services/abilitymgr/src/*.cpp \
  -- -I./services/abilitymgr/include \
     -I./interfaces/kits/native
```

### 7.3 pre-commit 配置

创建 `.pre-commit-config.yaml`:
```yaml
repos:
  - repo: https://github.com/pocc/pre-commit-hooks
    rev: master
    hooks:
      - id: clang-tidy
        args: ['--checks=-*,cppcoreguidelines-pro-type-cstyle-cast']
      - id: cppcheck
        args: ['--enable=all']
```

---

## 八、验证和测试

### 8.1 编译验证

```bash
# 完整编译
./build.sh --product-name <product> --build-target ability_runtime

# 增量编译
./build.sh --product-name <product> --build-target ability_runtime --gn-target <specific_target>
```

### 8.2 单元测试

```bash
# 运行所有单元测试
./run_test.sh ability_runtime_unittest

# 运行特定测试
./run_test.sh ability_runtime_unittest --gtest_filter=MainThreadTest.*

# 运行测试并生成覆盖率报告
./run_test.sh ability_runtime_unittest --coverage
```

### 8.3 静态分析

```bash
# 使用 cppcheck
cppcheck --enable=all --inconclusive \
  --std=c++17 \
  --suppress=missingIncludeSystem \
  services/ frameworks/

# 使用 clang-static-analyzer
scan-build ./build.sh --product-name <product> --build-target ability_runtime
```

### 8.4 动态分析

```bash
# 使用 AddressSanitizer
export ASAN_OPTIONS=detect_leaks=1
./build.sh --product-name <product> --ccache --build-target ability_runtime \
  --gn-args="use_sanitizer=true"

# 使用 ThreadSanitizer
export TSAN_OPTIONS=second_deadlock_stack=1
./build.sh --product-name <product> --ccache --build-target ability_runtime \
  --gn-args="use_tsan=true"
```

---

## 九、常见问题和解决方案

### Q1: 修复后测试失败怎么办？

**A**:
1. 检查是否破坏了原有逻辑
2. 确认测试用例本身是否正确
3. 使用 git 对比修改内容
4. 逐个测试用例排查

### Q2: 如何判断是否需要添加检查？

**A**:
- 如果函数可能返回 nullptr，必须检查
- 如果是 getter 方法且对象保证非空，可不检查（但建议加 assert）
- 如果是跨线程/跨进程调用，必须检查
- 如果是容器访问，必须检查 empty() / size()

### Q3: 修复顺序有什么建议？

**A**:
1. 优先修复高风险、高影响的问题
2. 先修复生产代码，再修复测试代码
3. 先修复核心模块，再修复周边模块
4. 每修复一类问题后立即验证

### Q4: 如何避免引入新问题？

**A**:
1. 使用代码审查
2. 配置 pre-commit hooks
3. 定期运行静态分析
4. 编写单元测试覆盖修改
5. 使用 CI/CD 自动化验证

### Q5: 大规模重构如何管理？

**A**:
1. 创建专门的分支
2. 按模块分批修复
3. 每批修复后创建里程碑
4. 保持详细的变更日志
5. 定期合并到主分支

---

## 十、参考资源

### 10.1 编码规范

- [C++ Core Guidelines](https://isocpp.github.io/CppCoreGuidelines/)
- [OpenHarmony 编码规范](https://gitee.com/openharmony/coding-standards)
- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)

### 10.2 工具文档

- [clang-tidy](https://clang.llvm.org/extra/clang-tidy/)
- [cppcheck](https://cppcheck.sourceforge.io/)
- [AddressSanitizer](https://github.com/google/sanitizers/wiki/AddressSanitizer)

### 10.3 相关文档

- [SECURITY_VULNERABILITY_SCAN_REPORT.md](SECURITY_VULNERABILITY_SCAN_REPORT.md) - 完整扫描报告
- [POINTER_SAFETY_ANALYSIS_REPORT.md](POINTER_SAFETY_ANALYSIS_REPORT.md) - 指针安全分析
- [CODE_QUALITY_ANALYSIS_REPORT.md](CODE_QUALITY_ANALYSIS_REPORT.md) - 代码质量分析
- [ISSUES_TO_FIX.md](ISSUES_TO_FIX.md) - 问题汇总
- [MEDIUM_PRIORITY_ISSUES_DETAIL.md](MEDIUM_PRIORITY_ISSUES_DETAIL.md) - 中优先级问题详情

---

**文档版本**: v1.0
**最后更新**: 2026-02-06
**维护者**: Development Team
**反馈**: 请通过项目issue报告文档问题或建议
