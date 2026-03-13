# Memory Management 示例

## 概述

本示例演示了如何使用 NAPI 进行内存管理，包括：
- 创建外部缓冲区
- 包装原生对象
- 创建类型化数组
- 共享缓冲区管理
- 外部内存调整
- 内存池管理
- 对齐缓冲区创建

## 功能说明

### 1. createExternalBuffer(size)

创建一个外部缓冲区，使用原生内存管理。

**参数：**
- `size`: 缓冲区大小（字节），可选，默认为 1024

**返回值：**
- ArrayBuffer: 外部缓冲区对象

**示例：**
```javascript
const memoryManagement = require('memory_management');

const buffer = memoryManagement.createExternalBuffer(2048);
console.log('Buffer size:', buffer.byteLength);
```

### 2. createWrappedObject(name, value)

创建一个包装了原生对象的 JavaScript 对象。

**参数：**
- `name`: 对象名称，可选
- `value`: 对象值，可选

**返回值：**
- Object: 包装后的对象，包含 name 和 value 属性

**示例：**
```javascript
const obj = memoryManagement.createWrappedObject('MyObject', 42);
console.log('Object name:', obj.name);
console.log('Object value:', obj.value);
```

### 3. createTypedArray(length, type)

创建一个类型化数组。

**参数：**
- `length`: 数组元素数量，可选，默认为 1024
- `type`: 数组类型，可选，默认为 0 (int8)
  - 0: int8
  - 1: uint8
  - 2: int16
  - 3: uint16
  - 4: int32
  - 5: uint32
  - 6: float32
  - 7: float64

**返回值：**
- TypedArray: 类型化数组对象

**示例：**
```javascript
const int32Array = memoryManagement.createTypedArray(100, 4); // int32 array
const float64Array = memoryManagement.createTypedArray(50, 7); // float64 array
```

### 4. createSharedBuffer(size)

创建一个共享缓冲区，可以被多个对象引用。

**参数：**
- `size`: 缓冲区大小（字节），可选，默认为 1024

**返回值：**
- ArrayBuffer: 共享缓冲区对象

**示例：**
```javascript
const sharedBuffer = memoryManagement.createSharedBuffer(4096);
// 可以将此缓冲区传递给多个视图
const view1 = new Uint8Array(sharedBuffer);
const view2 = new Int32Array(sharedBuffer);
```

### 5. adjustExternalMemory(changeInBytes)

调整外部内存使用量，通知 V8 引擎内存变化。

**参数：**
- `changeInBytes`: 内存变化量（字节），正数表示增加，负数表示减少

**返回值：**
- int64: 调整后的外部内存总量

**示例：**
```javascript
const adjusted = memoryManagement.adjustExternalMemory(1024);
console.log('Adjusted external memory:', adjusted);

const decreased = memoryManagement.adjustExternalMemory(-512);
console.log('Decreased external memory:', decreased);
```

### 6. createMemoryPool(blockSize, blockCount)

创建一个内存池，预分配多个固定大小的内存块。

**参数：**
- `blockSize`: 每个块的大小（字节），可选，默认为 1024
- `blockCount`: 块数量，可选，默认为 10

**返回值：**
- Object: 内存池对象，包含以下属性：
  - `blockSize`: 每个块的大小
  - `blockCount`: 块数量
  - `totalSize`: 总大小

**示例：**
```javascript
const pool = memoryManagement.createMemoryPool(2048, 20);
console.log('Pool block size:', pool.blockSize);
console.log('Pool block count:', pool.blockCount);
console.log('Pool total size:', pool.totalSize);
```

### 7. createAlignedBuffer(size, alignment)

创建一个对齐的缓冲区，确保内存地址满足对齐要求。

**参数：**
- `size`: 缓冲区大小（字节），可选，默认为 1024
- `alignment`: 对齐字节数，必须是 2 的幂，可选，默认为 16

**返回值：**
- ArrayBuffer: 对齐的缓冲区对象

**示例：**
```javascript
const alignedBuffer = memoryManagement.createAlignedBuffer(1024, 64);
console.log('Buffer aligned to 64 bytes');
```

### 8. getMemoryInfo()

获取当前内存管理信息。

**参数：**
- 无

**返回值：**
- Object: 内存信息对象，包含以下属性：
  - `nativeObjectCount`: 原生对象数量
  - `totalBufferMemory`: 缓冲区总内存（字节）

**示例：**
```javascript
const info = memoryManagement.getMemoryInfo();
console.log('Native object count:', info.nativeObjectCount);
console.log('Total buffer memory:', info.totalBufferMemory);
```

## 代码结构

```
memory_management/
├── BUILD.gn                   # GN 构建配置文件
├── memory_management.cpp      # 内存管理实现
└── README.md                 # 本文档
```

## 编译和使用

### 编译

在项目根目录执行：
```bash
./build.sh --product-name rk3568 --target-cpu arm64 --build-target memory_management
```

### 使用

在 JavaScript 代码中引入模块：
```javascript
const memoryManagement = require('memory_management');
```

## 注意事项

1. **内存泄漏预防**
   - 所有创建的缓冲区和对象都有适当的 finalizer
   - 对象被垃圾回收时会自动释放原生内存
   - 使用弱引用避免循环引用

2. **缓冲区大小限制**
   - 单个缓冲区最大大小为 10MB
   - 创建过大的缓冲区会抛出错误

3. **对齐要求**
   - 对齐值必须是 2 的幂（1, 2, 4, 8, 16, 32, 64...）
   - 不满足要求的对齐值会抛出错误

4. **外部内存调整**
   - adjustExternalMemory 用于通知 V8 引擎外部内存变化
   - 正确使用可以帮助 V8 更好地进行垃圾回收

5. **内存池**
   - 内存池预分配内存，减少运行时分配开销
   - 适合频繁创建和销毁相同大小对象的场景

## 性能优化建议

1. **重用缓冲区**
   - 对于频繁使用的缓冲区，考虑创建一次并重用
   - 避免在循环中重复创建和销毁缓冲区

2. **使用合适的类型化数组**
   - 根据数据类型选择合适的类型化数组
   - int8/uint8 适合字节数据
   - float32/float64 适合浮点数计算

3. **内存池**
   - 对于固定大小的频繁分配，使用内存池
   - 可以显著减少内存分配开销

4. **对齐缓冲区**
   - SIMD 操作需要特定对齐的内存
   - 使用 createAlignedBuffer 确保满足对齐要求

## 错误处理

所有函数都会进行参数验证，如果参数不正确会抛出错误：
- `TypeError`: 参数类型错误
- `Error`: 内存分配失败或其他错误

建议在使用时添加 try-catch 进行错误处理：
```javascript
try {
    const buffer = memoryManagement.createExternalBuffer(size);
} catch (error) {
    console.error('Failed to create buffer:', error);
}
```

## 内存管理最佳实践

1. **及时释放**
   - 不再需要的对象及时解除引用
   - 让垃圾回收器可以回收内存

2. **避免大对象**
   - 尽量避免创建过大的单个对象
   - 考虑将大数据分割为多个小对象

3. **监控内存使用**
   - 定期调用 getMemoryInfo 检查内存使用情况
   - 及时发现内存泄漏问题

4. **合理使用外部内存**
   - 使用 adjustExternalMemory 通知 V8 引擎
   - 帮助垃圾回收器做出更好的决策

## 示例代码

### 完整示例

```javascript
const memoryManagement = require('memory_management');

// 创建外部缓冲区
const buffer = memoryManagement.createExternalBuffer(4096);
const view = new Uint8Array(buffer);
view[0] = 42;
console.log('First byte:', view[0]);

// 创建包装对象
const obj = memoryManagement.createWrappedObject('Test', 123);
console.log('Object:', obj);

// 创建类型化数组
const int32Array = memoryManagement.createTypedArray(100, 4);
for (let i = 0; i < int32Array.length; i++) {
    int32Array[i] = i * 2;
}

// 调整外部内存
memoryManagement.adjustExternalMemory(1024);

// 获取内存信息
const info = memoryManagement.getMemoryInfo();
console.log('Memory info:', info);
```
