# Event Notification 示例

## 概述

本示例演示了如何使用 NAPI 的事件通知机制，包括：
- 发送普通事件
- 发送不同优先级的事件
- 发送可取消的事件
- 使用 Vsync 屏障同步
- 批量发送事件

## 功能说明

### 1. sendEventWithCallback(callback, delay)

发送一个普通优先级的事件，延迟指定时间后执行回调函数。

**参数：**
- `callback`: 回调函数，事件执行时会被调用
- `delay`: 延迟时间（毫秒），可选，默认为 1000ms

**返回值：**
- 事件句柄 ID (uint64_t)

**示例：**
```javascript
const eventNotification = require('event_notification');

const handle = eventNotification.sendEventWithCallback((result) => {
    console.log('Event executed with result:', result);
}, 2000);
```

### 2. sendHighPriorityEvent(callback)

发送一个高优先级事件，会优先于普通事件执行。

**参数：**
- `callback`: 回调函数

**返回值：**
- 事件句柄 ID (uint64_t)

**示例：**
```javascript
const handle = eventNotification.sendHighPriorityEvent((result) => {
    console.log('High priority event executed');
});
```

### 3. sendIdlePriorityEvent(callback)

发送一个空闲优先级事件，只在系统空闲时执行。

**参数：**
- `callback`: 回调函数

**返回值：**
- 事件句柄 ID (uint64_t)

**示例：**
```javascript
const handle = eventNotification.sendIdlePriorityEvent((result) => {
    console.log('Idle priority event executed');
});
```

### 4. sendVsyncBarrierEvent(callback)

发送一个 Vsync 屏障同步事件，等待下一个垂直同步信号后执行。

**参数：**
- `callback`: 回调函数

**返回值：**
- 事件句柄 ID (uint64_t)

**示例：**
```javascript
const handle = eventNotification.sendVsyncBarrierEvent((result) => {
    console.log('Vsync barrier event executed');
});
```

### 5. sendCancelableEvent(callback, delay)

发送一个可取消的事件。

**参数：**
- `callback`: 回调函数
- `delay`: 延迟时间（毫秒），可选，默认为 1000ms

**返回值：**
- 事件句柄 ID (uint64_t)

**示例：**
```javascript
const handle = eventNotification.sendCancelableEvent((result) => {
    console.log('This will not execute if cancelled');
}, 5000);

// 取消事件
eventNotification.cancelEvent(handle);
```

### 6. cancelEvent(handleId)

取消一个可取消的事件。

**参数：**
- `handleId`: 事件句柄 ID

**返回值：**
- boolean: 是否成功取消

**示例：**
```javascript
const handle = eventNotification.sendCancelableEvent(() => {}, 5000);
const cancelled = eventNotification.cancelEvent(handle);
console.log('Event cancelled:', cancelled);
```

### 7. isEventCompleted(handleId)

检查事件是否已完成。

**参数：**
- `handleId`: 事件句柄 ID

**返回值：**
- boolean: 事件是否已完成

**示例：**
```javascript
const handle = eventNotification.sendEventWithCallback(() => {});
const completed = eventNotification.isEventCompleted(handle);
console.log('Event completed:', completed);
```

### 8. sendMultipleEvents(count, callback)

批量发送多个事件。

**参数：**
- `count`: 事件数量（1-100）
- `callback`: 回调函数，每个事件执行时都会被调用

**返回值：**
- Array: 事件句柄 ID 数组

**示例：**
```javascript
const handles = eventNotification.sendMultipleEvents(5, (result) => {
    console.log('Event executed:', result);
});
console.log('Sent', handles.length, 'events');
```

## 代码结构

```
event_notification/
├── BUILD.gn                 # GN 构建配置文件
├── event_notification.cpp    # 事件通知实现
└── README.md               # 本文档
```

## 编译和使用

### 编译

在项目根目录执行：
```bash
./build.sh --product-name rk3568 --target-cpu arm64 --build-target event_notification
```

### 使用

在 JavaScript 代码中引入模块：
```javascript
const eventNotification = require('event_notification');
```

## 注意事项

1. 事件句柄 ID 是唯一的，用于标识和操作特定事件
2. 高优先级事件会优先于普通事件执行
3. 空闲优先级事件只在系统空闲时执行，可能不会立即执行
4. Vsync 屏障事件会等待下一个垂直同步信号，适合 UI 相关操作
5. 可取消事件必须在执行前取消，取消后回调不会被调用
6. 批量发送事件时，每个事件都会独立执行
7. 所有事件都在 JS 线程中执行回调函数

## 性能考虑

1. 避免在高频场景下创建过多事件
2. 合理使用事件优先级，不要滥用高优先级事件
3. 及时取消不需要的事件，避免资源浪费
4. 批量操作时考虑使用 sendMultipleEvents 提高效率

## 错误处理

所有函数都会进行参数验证，如果参数类型不正确会抛出错误：
- `TypeError`: 参数类型错误
- `Error`: 其他错误情况

建议在使用时添加 try-catch 进行错误处理：
```javascript
try {
    const handle = eventNotification.sendEventWithCallback(callback);
} catch (error) {
    console.error('Failed to send event:', error);
}
```
