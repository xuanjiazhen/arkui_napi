# Thread-Safe Async 示例

## 概述

本示例演示了如何使用 NAPI 进行线程安全的异步操作，包括：
- 异步工作队列
- 线程安全函数
- 工作线程模式
- Promise 和回调模式

## 功能说明

### 1. runAsyncWork(input, callback?)

执行异步工作，支持 Promise 和回调两种模式。

**参数：**
- `input`: 输入值（数字）
- `callback`: 可选回调函数，如果不提供则返回 Promise

**返回值：**
- Promise: 如果没有提供回调，返回 Promise
- undefined: 如果提供了回调

**示例：**

Promise 模式：
```javascript
const threadsafeAsync = require('threadsafe_async');

threadsafeAsync.runAsyncWork(21).then((result) => {
    console.log('Async result:', result); // 输出: 42
});
```

回调模式：
```javascript
threadsafeAsync.runAsyncWork(21, (result) => {
    console.log('Async result:', result); // 输出: 42
});
```

### 2. createThreadsafeFunction(callback)

创建一个线程安全函数，可以从任何线程调用。

**参数：**
- `callback`: JavaScript 回调函数

**返回值：**
- int64: 线程安全函数句柄

**示例：**
```javascript
const handle = threadsafeAsync.createThreadsafeFunction((value) => {
    console.log('Received value:', value);
});
```

### 3. callThreadsafeFunction(handle, value)

调用线程安全函数，从任意线程向 JavaScript 线程发送数据。

**参数：**
- `handle`: 线程安全函数句柄
- `value`: 要传递的值（数字）

**返回值：**
- boolean: 是否成功调用

**示例：**
```javascript
const handle = threadsafeAsync.createThreadsafeFunction((value) => {
    console.log('Value from thread:', value);
});

const success = threadsafeAsync.callThreadsafeFunction(handle, 123);
console.log('Call successful:', success);
```

### 4. createWorkerThread(callback)

创建一个工作线程，可以接收和处理任务。

**参数：**
- `callback`: JavaScript 回调函数，用于处理工作线程的结果

**返回值：**
- int64: 工作线程句柄

**示例：**
```javascript
const worker = threadsafeAsync.createWorkerThread((result) => {
    console.log('Worker result:', result);
});
```

### 5. submitTaskToWorker(handle, taskValue)

向工作线程提交任务。

**参数：**
- `handle`: 工作线程句柄
- `taskValue`: 任务值（数字）

**返回值：**
- undefined

**示例：**
```javascript
const worker = threadsafeAsync.createWorkerThread((result) => {
    console.log('Task result:', result);
});

// 提交多个任务
threadsafeAsync.submitTaskToWorker(worker, 10);
threadsafeAsync.submitTaskToWorker(worker, 20);
threadsafeAsync.submitTaskToWorker(worker, 30);
```

### 6. stopWorkerThread(handle)

停止工作线程。

**参数：**
- `handle`: 工作线程句柄

**返回值：**
- undefined

**示例：**
```javascript
const worker = threadsafeAsync.createWorkerThread((result) => {
    console.log('Result:', result);
});

// 提交任务
threadsafeAsync.submitTaskToWorker(worker, 100);

// 停止工作线程
threadsafeAsync.stopWorkerThread(worker);
```

### 7. getWorkerStats(handle)

获取工作线程的统计信息。

**参数：**
- `handle`: 工作线程句柄

**返回值：**
- Object: 统计信息对象，包含以下属性：
  - `running`: 工作线程是否正在运行
  - `processedCount`: 已处理的任务数量
  - `queueSize`: 当前任务队列大小

**示例：**
```javascript
const worker = threadsafeAsync.createWorkerThread((result) => {
    console.log('Result:', result);
});

// 提交任务
for (let i = 0; i < 10; i++) {
    threadsafeAsync.submitTaskToWorker(worker, i);
}

// 获取统计信息
const stats = threadsafeAsync.getWorkerStats(worker);
console.log('Worker stats:', stats);
console.log('Queue size:', stats.queueSize);
console.log('Processed count:', stats.processedCount);
```

## 代码结构

```
threadsafe_async/
├── BUILD.gn                # GN 构建配置文件
├── threadsafe_async.cpp     # 线程安全异步实现
└── README.md              # 本文档
```

## 编译和使用

### 编译

在项目根目录执行：
```bash
./build.sh --product-name rk3568 --target-cpu arm64 --build-target threadsafe_async
```

### 使用

在 JavaScript 代码中引入模块：
```javascript
const threadsafeAsync = require('threadsafe_async');
```

## 使用场景

### 1. 异步文件操作

```javascript
// 使用 Promise 模式
async function readFileAsync() {
    const result = await threadsafeAsync.runAsyncWork(1);
    console.log('File read complete:', result);
}

// 使用回调模式
threadsafeAsync.runAsyncWork(1, (result) => {
    console.log('File read complete:', result);
});
```

### 2. 跨线程通信

```javascript
// 创建线程安全函数
const tsfn = threadsafeAsync.createThreadsafeFunction((data) => {
    console.log('Data from native thread:', data);
});

// 从 C++ 线程调用
// 在实际应用中，这会在另一个线程中执行
threadsafeAsync.callThreadsafeFunction(tsfn, 42);
```

### 3. 工作线程池

```javascript
// 创建工作线程
const worker = threadsafeAsync.createWorkerThread((result) => {
    console.log('Task completed:', result);
});

// 提交多个任务
const tasks = [10, 20, 30, 40, 50];
tasks.forEach(task => {
    threadsafeAsync.submitTaskToWorker(worker, task);
});

// 定期检查状态
setInterval(() => {
    const stats = threadsafeAsync.getWorkerStats(worker);
    console.log('Processed:', stats.processedCount, 'Queue:', stats.queueSize);
}, 1000);
```

### 4. 批量异步处理

```javascript
// 使用 Promise.all 批量处理
const inputs = [1, 2, 3, 4, 5];
const promises = inputs.map(input => threadsafeAsync.runAsyncWork(input));

Promise.all(promises).then(results => {
    console.log('All results:', results);
});
```

## 注意事项

1. **线程安全**
   - 线程安全函数可以从任何线程调用
   - 回调总是在 JavaScript 线程中执行
   - 避免在回调中执行耗时操作

2. **工作线程生命周期**
   - 创建工作线程后会启动一个原生线程
   - 必须在不再需要时调用 stopWorkerThread
   - 停止工作线程会等待当前任务完成

3. **任务队列**
   - 工作线程使用任务队列管理任务
   - 任务按提交顺序执行
   - 可以通过 getWorkerStats 查看队列状态

4. **内存管理**
   - 线程安全函数有适当的 finalizer
   - 工作线程停止时会释放资源
   - 避免创建过多的线程和函数

5. **错误处理**
   - 异步操作中的错误会在回调中传递
   - Promise 模式会通过 reject 传递错误
   - 建议添加适当的错误处理

## 性能优化建议

1. **合理使用线程**
   - 不要创建过多的工作线程
   - 一般 1-4 个工作线程足够
   - 考虑 CPU 核心数

2. **批量处理**
   - 将多个小任务合并为大任务
   - 减少线程切换开销
   - 使用 Promise.all 并行处理

3. **避免阻塞**
   - 不要在回调中执行耗时操作
   - 耗时操作应该在工作线程中完成
   - 回调只用于处理结果

4. **资源清理**
   - 及时停止不需要的工作线程
   - 释放不再使用的线程安全函数
   - 避免资源泄漏

## 错误处理

所有函数都可能抛出错误，建议添加适当的错误处理：

```javascript
// Promise 模式错误处理
threadsafeAsync.runAsyncWork(21)
    .then(result => {
        console.log('Result:', result);
    })
    .catch(error => {
        console.error('Error:', error);
    });

// 回调模式错误处理
try {
    const handle = threadsafeAsync.createThreadsafeFunction(callback);
} catch (error) {
    console.error('Failed to create threadsafe function:', error);
}

// 工作线程错误处理
try {
    const worker = threadsafeAsync.createWorkerThread(callback);
    threadsafeAsync.submitTaskToWorker(worker, task);
} catch (error) {
    console.error('Worker error:', error);
}
```

## 完整示例

### 示例 1: 异步数据处理

```javascript
const threadsafeAsync = require('threadsafe_async');

async function processData(data) {
    console.log('Processing data...');
    
    const results = [];
    for (const item of data) {
        const result = await threadsafeAsync.runAsyncWork(item);
        results.push(result);
    }
    
    console.log('Processing complete:', results);
    return results;
}

processData([1, 2, 3, 4, 5]);
```

### 示例 2: 工作线程模式

```javascript
const threadsafeAsync = require('threadsafe_async');

class WorkerPool {
    constructor(callback, size = 1) {
        this.workers = [];
        this.callback = callback;
        
        for (let i = 0; i < size; i++) {
            const worker = threadsafeAsync.createWorkerThread(callback);
            this.workers.push(worker);
        }
    }
    
    submit(task) {
        const worker = this.workers[0];
        threadsafeAsync.submitTaskToWorker(worker, task);
    }
    
    getStats() {
        return this.workers.map(worker => 
            threadsafeAsync.getWorkerStats(worker)
        );
    }
    
    stop() {
        this.workers.forEach(worker => {
            threadsafeAsync.stopWorkerThread(worker);
        });
        this.workers = [];
    }
}

// 使用工作线程池
const pool = new WorkerPool((result) => {
    console.log('Task result:', result);
}, 2);

// 提交任务
for (let i = 0; i < 10; i++) {
    pool.submit(i);
}

// 检查状态
setInterval(() => {
    const stats = pool.getStats();
    console.log('Pool stats:', stats);
}, 1000);

// 清理
setTimeout(() => {
    pool.stop();
}, 5000);
```

### 示例 3: 线程安全通信

```javascript
const threadsafeAsync = require('threadsafe_async');

// 创建线程安全函数
const communicator = threadsafeAsync.createThreadsafeFunction((message) => {
    console.log('Received message:', message);
    
    // 处理消息
    if (message.type === 'data') {
        console.log('Processing data:', message.value);
    } else if (message.type === 'control') {
        console.log('Control command:', message.command);
    }
});

// 模拟从不同线程发送消息
function sendMessageFromThread(type, value) {
    // 在实际应用中，这会在不同的线程中执行
    const message = { type, value };
    threadsafeAsync.callThreadsafeFunction(communicator, message);
}

// 发送消息
sendMessageFromThread('data', 42);
sendMessageFromThread('control', 'start');
sendMessageFromThread('data', 100);
```
