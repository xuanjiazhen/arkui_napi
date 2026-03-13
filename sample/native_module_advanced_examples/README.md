# NAPI 高级示例

## 概述

本目录包含多个 NAPI 高级功能示例，演示了 NAPI 的各种能力和最佳实践。每个示例都是独立的模块，展示了特定的 NAPI 功能和编程模式。

## 示例列表

### 1. Event Notification (事件通知)

演示 NAPI 的事件通知机制，包括：
- 发送普通事件
- 发送不同优先级的事件（高优先级、空闲优先级）
- 发送可取消的事件
- 使用 Vsync 屏障同步
- 批量发送事件

**目录：** `event_notification/`

**主要 API：**
- `sendEventWithCallback(callback, delay)` - 发送普通事件
- `sendHighPriorityEvent(callback)` - 发送高优先级事件
- `sendIdlePriorityEvent(callback)` - 发送空闲优先级事件
- `sendVsyncBarrierEvent(callback)` - 发送 Vsync 屏障事件
- `sendCancelableEvent(callback, delay)` - 发送可取消事件
- `cancelEvent(handleId)` - 取消事件
- `isEventCompleted(handleId)` - 检查事件是否完成
- `sendMultipleEvents(count, callback)` - 批量发送事件

**使用场景：**
- UI 事件处理
- 定时任务调度
- 优先级任务管理
- 动画同步

### 2. Memory Management (内存管理)

演示 NAPI 的内存管理功能，包括：
- 创建外部缓冲区
- 包装原生对象
- 创建类型化数组
- 共享缓冲区管理
- 外部内存调整
- 内存池管理
- 对齐缓冲区创建

**目录：** `memory_management/`

**主要 API：**
- `createExternalBuffer(size)` - 创建外部缓冲区
- `createWrappedObject(name, value)` - 创建包装对象
- `createTypedArray(length, type)` - 创建类型化数组
- `createSharedBuffer(size)` - 创建共享缓冲区
- `adjustExternalMemory(changeInBytes)` - 调整外部内存
- `createMemoryPool(blockSize, blockCount)` - 创建内存池
- `createAlignedBuffer(size, alignment)` - 创建对齐缓冲区
- `getMemoryInfo()` - 获取内存信息

**使用场景：**
- 大数据处理
- 图像/音频处理
- 性能敏感的内存操作
- 跨语言数据共享

### 3. Thread-Safe Async (线程安全异步)

演示 NAPI 的线程安全异步操作，包括：
- 异步工作队列
- 线程安全函数
- 工作线程模式
- Promise 和回调模式

**目录：** `threadsafe_async/`

**主要 API：**
- `runAsyncWork(input, callback?)` - 执行异步工作
- `createThreadsafeFunction(callback)` - 创建线程安全函数
- `callThreadsafeFunction(handle, value)` - 调用线程安全函数
- `createWorkerThread(callback)` - 创建工作线程
- `submitTaskToWorker(handle, taskValue)` - 提交任务到工作线程
- `stopWorkerThread(handle)` - 停止工作线程
- `getWorkerStats(handle)` - 获取工作线程统计信息

**使用场景：**
- 文件 I/O 操作
- 网络请求
- 耗时计算任务
- 跨线程通信

## 编译

### 前置要求

- OpenHarmony 开发环境
- GN 构建工具
- NDK 工具链

### 编译单个示例

在项目根目录执行：

```bash
# 编译事件通知示例
./build.sh --product-name rk3568 --target-cpu arm64 --build-target event_notification

# 编译内存管理示例
./build.sh --product-name rk3568 --target-cpu arm64 --build-target memory_management

# 编译线程安全异步示例
./build.sh --product-name rk3568 --target-cpu arm64 --build-target threadsafe_async
```

### 编译所有示例

```bash
./build.sh --product-name rk3568 --target-cpu arm64 --build-target native_module_advanced_examples
```

## 使用

### 引入模块

在 JavaScript 代码中引入所需的示例模块：

```javascript
// 引入事件通知模块
const eventNotification = require('event_notification');

// 引入内存管理模块
const memoryManagement = require('memory_management');

// 引入线程安全异步模块
const threadsafeAsync = require('threadsafe_async');
```

### 基本使用示例

#### 事件通知示例

```javascript
const eventNotification = require('event_notification');

// 发送普通事件
const handle = eventNotification.sendEventWithCallback((result) => {
    console.log('Event executed:', result);
}, 1000);

// 发送高优先级事件
eventNotification.sendHighPriorityEvent(() => {
    console.log('High priority event');
});

// 批可发送事件
const handles = eventNotification.sendMultipleEvents(5, (result) => {
    console.log('Event:', result);
});
```

#### 内存管理示例

```javascript
const memoryManagement = require('memory_management');

// 创建外部缓冲区
const buffer = memoryManagement.createExternalBuffer(4096);
const view = new Uint8Array(buffer);

// 创建类型化数组
const int32Array = memoryManagement.createTypedArray(100, 4);

// 获取内存信息
const info = memoryManagement.getMemoryInfo();
console.log('Memory info:', info);
```

#### 线程安全异步示例

```javascript
const threadsafeAsync = require('threadsafe_async');

// Promise 模式
threadsafeAsync.runAsyncWork(21).then((result) => {
    console.log('Result:', result); // 42
});

// 回调模式
threadsafeAsync.runAsyncWork(21, (result) => {
    console.log('Result:', result); // 42
});

// 创建工作线程
const worker = threadsafeAsync.createWorkerThread((result) => {
    console.log('Task result:', result);
});

// 提交任务
threadsafeAsync.submitTaskToWorker(worker, 100);
```

## 编程规范

### 代码风格

1. **命名规范**
   - 函数名使用 PascalCase: `CreateExternalBuffer`
   - 变量名使用 camelCase: `bufferSize`
   - 常量使用 UPPER_SNAKE_CASE: `DEFAULT_BUFFER_SIZE`
   - 类名使用 PascalCase: `AsyncWorkContext`

2. **注释规范**
   - 每个函数都有清晰的注释说明功能、参数和返回值
   - 复杂逻辑添加行内注释
   - 使用 Doxygen 风格的注释格式

3. **错误处理**
   - 检查所有 NAPI API 的返回值
   - 使用 NAPI_ASSERT 进行参数验证
   - 提供清晰的错误信息

4. **内存管理**
   - 所有动态分配的内存都有对应的释放逻辑
   - 使用 finalizer 确保资源正确释放
   - 避免内存泄漏

5. **线程安全**
   - 使用适当的同步机制（互斥锁、条件变量等）
   - 线程安全函数正确使用 napi_threadsafe_function
   - 避免数据竞争

### 性能优化

1. **减少内存分配**
   - 重用缓冲区和对象
   - 使用内存池
   - 避免频繁的 new/delete

2. **异步操作**
   - 耗时操作使用异步工作队列
   - 合理使用事件优先级
   - 避免阻塞 JavaScript 线程

3. **批量处理**
   - 批量操作减少函数调用开销
   - 使用类型化数组处理大量数据
   - 合理设置缓冲区大小

### 安全性考虑

1. **边界检查**
   - 验证所有输入参数
   - 检查数组边界
   - 防止缓冲区溢出

2. **资源管理**
   - 使用 RAII 模式管理资源
   - 确保异常安全
   - 及时释放不再使用的资源

3. **错误恢复**
   - 提供清晰的错误信息
   - 支持优雅降级
   - 避免崩溃

## 最佳实践

### 1. 异步操作

```javascript
// 推荐：使用 Promise 处理异步操作
async function processData() {
    try {
        const result = await threadsafeAsync.runAsyncWork(data);
        console.log('Result:', result);
    } catch (error) {
        console.error('Error:', error);
    }
}

// 避免：回调地狱
threadsafeAsync.runAsyncWork(data, (result) => {
    threadsafeAsync.runAsyncWork(result, (result2) => {
        threadsafeAsync.runAsyncWork(result2, (result3) => {
            // 嵌套过深
        });
    });
});
```

### 2. 内存管理

```javascript
// 推荐：重用缓冲区
const buffer = memoryManagement.createExternalBuffer(4096);
const view = new Uint8Array(buffer);

for (let i = 0; i < 1000; i++) {
    // 重用同一个缓冲区
    processBuffer(view);
}

// 避免：重复创建缓冲区
for (let i = 0; i < 1000; i++) {
    const buffer = memoryManagement.createExternalBuffer(4096);
    const view = new Uint8Array(buffer);
    processBuffer(view);
}
```

### 3. 事件处理

```javascript
// 推荐：使用可取消事件
const handle = eventNotification.sendCancelableEvent(() => {
    console.log('Event executed');
}, 5000);

// 如果不再需要，及时取消
if (shouldCancel) {
    eventNotification.cancelEvent(handle);
}

// 推荐：使用高优先级事件处理重要任务
eventNotification.sendHighPriorityEvent(() => {
    // 处理重要任务
});

// 推荐：使用空闲优先级事件处理非关键任务
eventNotification.sendIdlePriorityEvent(() => {
    // 处理非关键任务
});
```

## 故障排查

### 编译错误

1. **找不到头文件**
   - 检查 `deps` 配置是否正确
   - 确认 NAPI 路径设置正确

2. **链接错误**
   - 检查 `external_deps` 配置
   - 确认所有依赖库都已添加

### 运行时错误

1. **模块加载失败**
   - 检查模块是否正确编译
   - 确认模块路径正确

2. **函数调用失败**
   - 检查参数类型是否正确
   - 查看日志输出获取详细错误信息

3. **内存泄漏**
   - 使用 `getMemoryInfo()` 监控内存使用
   - 检查 finalizer 是否正确设置

## 性能测试

### 基准测试

```javascript
// 测试异步操作性能
async function benchmarkAsync() {
    const start = Date.now();
    const count = 1000;

    const promises = [];
    for (let i = 0; i < count; i++) {
        promises.push(threadsafeAsync.runAsyncWork(i));
    }

    await Promise.all(promises);

    const duration = Date.now() - start;
    console.log(`Processed ${count} operations in ${duration}ms`);
    console.log(`Average: ${duration / count}ms per operation`);
}

// 测试内存操作性能
function benchmarkMemory() {
    const start = Date.now();
    const count = 1000;
    const size = 4096;

    for (let i = 0; i < count; i++) {
        const buffer = memoryManagement.createExternalBuffer(size);
    }

    const duration = Date - start();
    console.log(`Created ${count} buffers of ${size} bytes in ${duration}ms`);
}
```

## 贡献指南

欢迎贡献新的示例和改进！

### 添加新示例

1. 在 `sample/native_module_advanced_examples/` 下创建新目录
2. 实现 C++ 代码，遵循编程规范
3. 创建 BUILD.gn 配置文件
4. 编写详细的 README.md 文档
5. 添加使用示例和测试用例

### 代码审查清单

- [ ] 代码符合编程规范
- [ ] 有清晰的注释和文档
- [ ] 正确处理错误情况
- [ ] 没有内存泄漏
- [ ] 线程安全
- [ ] 有完整的使用示例
- [ ] 通过编译和测试

## 参考资料

- [NAPI 官方文档](https://nodejs.org/api/n-api.html)
- [OpenHarmony NAPI 指南](https://gitee.com/openharmony/docs/blob/master/zh-cn/application-dev/napi/napi-guidelines.md)
- [V8 嵌入式指南](https://v8.dev/docs/embed)

## 许可证

Copyright (c) 2026 Huawei Device Co., Ltd.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
