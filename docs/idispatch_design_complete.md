# 鸿蒙IDispatch机制设计文档

## 一、功能概述

鸿蒙IDispatch机制是参考Windows COM IDispatch接口实现的晚期绑定（Late Binding）调用框架，为鸿蒙系统提供运行时动态方法调用能力。

### 核心功能
1. **晚期绑定调用**：支持运行时通过方法名或DispID调用方法
2. **元数据查询**：提供类型信息查询接口（ITypeInfo）
3. **参数编解码**：支持多种数据类型的VARIANT参数传递
4. **Windows兼容**：API设计与Windows IDispatch保持一致，便于应用迁移
5. **动态扩展**：支持运行时动态注册新方法（插件系统）

### 应用场景
- 脚本引擎集成（JavaScript、Python等）
- 插件系统动态调用
- 跨语言互操作
- Windows应用迁移到鸿蒙

---

## 二、实现思路

### 总体思路
采用基于鸿蒙Extension能力的Proxy-Extension架构，通过IDL工具自动生成元数据和IPC通信代码，实现高性能的晚期绑定调用。

### 关键设计决策

| 决策点 | 方案选择 | 原因 |
|--------|----------|------|
| 架构模式 | Extension架构 | 符合鸿蒙应用模型，支持按需启动 |
| 元数据格式 | JSON | 可读性强，易于解析和扩展 |
| IPC通信方式 | Extension Proxy的SendRequest | 统一的IPC通信机制 |
| 方法标识 | DispID + IPC Code | DispID用于API兼容，IPC Code用于实际通信 |
| 参数传递 | VARIANT | 与Windows兼容，支持多种类型 |

### DispID机制
- **DispID**：32位整数标识符，客户端API层面使用
- **IPC Code**：由IDL工具自动分配，实际IPC通信使用
- **映射关系**：DispID → IPC Code（从JSON元数据读取）

---

## 三、实现设计

### 3.1 TLB JSON 元数据格式说明

#### 顶层结构
| 字段 | 类型 | 必填 | 说明 |
| -- | -- | -- | -- |
| version | string | 是 | TLB格式版本号，当前为"1.0" |
| package | string | 是 | IDL包名，格式如"com.example.dataservice" |
| enums | array | 否 | 枚举定义数组 |
| structs | array | 否 | 结构体定义数组 |
| interfaces | array | 否 | 接口定义数组 |

#### 枚举定义（enums）
每个枚举包含字段：
| 字段 | 类型 | 必填 | 说明 |
| -- | -- | -- | -- |
| name | string | 是 | 枚举名称 |
| values | array | 是 | 枚举值定义数组 |

##### 枚举值定义(values):
| 字段 | 类型 | 必填 | 说明 |
| -- | -- | -- | -- |
| name | string | 是 | 枚举值名称 |
| value | number | 是 | 枚举值数值 |

#### 结构体定义（structs）
每个结构体包含字段：
| 字段 | 类型 | 必填 | 说明 |
| -- | -- | -- | -- |
| name | string | 是 | 结构体名称 |
| fields | array | 是 | 字段定义数组 |

##### 字段定义(fields):
| 字段 | 类型 | 必填 | 说明 |
| -- | -- | -- | -- |
| name | string | 是 | 字段名称 |
| type_info | object | 是 | 类型信息对象 |

#### 接口定义（interfaces）
每个接口包含字段：
| 字段 | 类型 | 必填 | 说明 |
| -- | -- | -- | -- |
| name | string | 是 | 接口名称 |
| descriptor | string | 是 | 接口描述符，格式如"com.example.dataservice.ICalculator" |
| version | number | 是 | 接口版本号 |
| methods | array | 是 | 方法定义数组 |

##### 方法定义(methods):
| 字段 | 类型 | 必填 | 说明 |
| -- | -- | -- | -- |
| name | string | 是 | 方法名称 |
| code | number | 是 | 方法代码（命令码） |
| oneway | boolean | 否 | 是否为单向调用，默认false |
| parameters | array | 是 | 参数定义数组 |
| return_type | object | 是 | 返回类型信息对象 |

##### 参数定义(parameters):
| 字段 | 类型 | 必填 | 说明 |
| -- | -- | -- | -- |
| name | string | 是 | 参数名称 |
| type_info | object | 是 | 类型信息对象 |
| direction | string | 是 | 参数方向："in"或"out" |

##### 参数类型信息（type_info）:
| type值 | 是否需要idl_type | 说明 |
| -- | -- | -- |
| "i8" | 否 | 8位整数 |
| "i32" | 否 | 32位整数 |
| "i64" | 否 | 64位整数 |
| "f64" | 否 | 浮点数 |
| "String" | 否 | 字符串 |
| "void" | 否 | 空类型 |
| "enum" | 是 | 枚举类型，idl_type指定枚举名 |
| "interface" | 是 | 接口类型，idl_type指定接口名 |
| "struct" | 是 | 结构体类型，idl_type指定结构体名 |

#### 类型信息对象（type_info）
| 字段 | 类型 | 必填 | 说明 |
| -- | -- | -- | -- |
| type | string | 是 | 基础类型，如"i32"、"String"、"interface"等 |
| idl_type | string | 条件必填 | 当type为"interface"、"enum"或"struct"时必填，指定具体的IDL类型名称 |

#### 参数方向与序列化时机
| direction | 说明 | 序列化时机 |
| -- | -- | -- |
| "in" | 输入参数，从客户端传递到服务端 | Proxy写入data Parcel，Stub从data Parcel读取 |
| "out" | 输出参数，从服务端返回给客户端 | Stub写入reply Parcel，Proxy从reply Parcel读取 |

#### 完整TLB JSON示例
```json
{
    "version": "1.0",
    "package": "com.example.dataservice",
    "enums": [
        {
            "name": "StatusCode",
            "values": [
                { "name": "Success", "value": 0 },
                { "name": "InvalidParam", "value": 1 },
                { "name": "NotFound", "value": 2 },
                { "name": "InternalError", "value": 3 }
            ]
        }
    ],
    "structs": [
        {
            "name": "UserInfo",
            "fields": [
                { "name": "id", "type_info": { "type": "i32" } },
                { "name": "name", "type_info": { "type": "String" } }
            ]
        }
    ],
    "interfaces": [
        {
            "name": "ICalculator",
            "descriptor": "com.example.dataservice.ICalculator",
            "version": 1,
            "methods": [
                {
                    "name": "add",
                    "code": 43981,
                    "oneway": false,
                    "parameters": [
                        {
                            "name": "a",
                            "type_info": { "type": "i32" },
                            "direction": "in"
                        },
                        {
                            "name": "b",
                            "type_info": { "type": "i32" },
                            "direction": "in"
                        },
                        {
                            "name": "result",
                            "type_info": { "type": "i32" },
                            "direction": "out"
                        }
                    ],
                    "return_type": { "type": "i32" }
                }
            ]
        },
        {
            "name": "IDataService",
            "descriptor": "com.example.dataservice.IDataService",
            "version": 1,
            "methods": [
                {
                    "name": "getData",
                    "code": 4660,
                    "oneway": false,
                    "parameters": [
                        {
                            "name": "id",
                            "type_info": { "type": "i32" },
                            "direction": "in"
                        }
                    ],
                    "return_type": { "type": "String" }
                },
                {
                    "name": "GetCalculator",
                    "code": 4662,
                    "parameters": [
                        {
                            "name": "id",
                            "type_info": { "type": "interface", "idl_type": "ICalculator" },
                            "direction": "in"
                        }
                    ],
                    "return_type": { "type": "enum", "idl_type": "StatusCode" }
                }
            ]
        }
    ]
}
```

### 3.2 整体架构

```
┌─────────────────────────────────────────────────────────┐
│                    NDK Client Layer                      │
│  ┌──────────────────────────────────────────────────┐  │
│  │  IDispatch NDK Interface (libidispatch_ndkso)    │  │
│  │  - CoCreateInstance()                            │  │
│  │  - GetIDsOfNames()                              │  │
│  │  - Invoke()                                     │  │
│  │  - GetTypeInfo()                                │  │
│  └──────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
                             ↓
┌─────────────────────────────────────────────────────────┐
│                   IDispatch Proxy Layer                  │
│  ┌──────────────────────────────────────────────────┐  │
│  │  IDispatchProxy (C++)                             │  │
│  │  - JSON Metadata Parser                           │  │
│  │  - Parameter Marshaling                           │  │
│  │  - Name → IPC Code 映射缓存                     │  │
│  │  - 持有 OH_IPCRemoteProxy 对象                 │  │
│  │  - 从 RemoteProxy 获取 IRemoteObject            │  │
│  │  - 调用 OH_IPCRemoteProxy_SendRequest()         │  │
│  └──────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
                             ↓
┌─────────────────────────────────────────────────────────┐
│               Extension Connection Layer                 │
│  ┌──────────────────────────────────────────────────┐  │
│  │  connectExtension(bundleName, abilityName)        │  │
│  │  ↓                                              │  │
│  │  返回 OH_IPCRemoteProxy 对象                    │  │
│  │  ↓                                              │  │
│  │  OH_IPCRemoteProxy_GetObject() 获取 IRemoteObject│  │
│  └──────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
                             ↓ IPC通信
┌─────────────────────────────────────────────────────────┐
│              ServiceExtension (Ability)                 │
│  ┌──────────────────────────────────────────────────┐  │
│  │  ServiceExtension (继承ExtensionAbility)         │  │
│  │  - OnConnect(): 返回 IRemoteObject             │  │
│  │  - OnRemoteRequest(): 处理IPC请求              │  │
│  │  - Method Registry                                │  │
│  │  - Invocation Handler                             │  │
│  │  - IDL生成的Stub代码                              │  │
│  └──────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────┘
```

### 3.3 Extension连接机制说明

**重要说明**：
- IDispatch没有自己的Extension类型
- 服务端使用的是**ServiceExtension**（类似于ServiceExtension的Ability）
- 客户端通过`connectExtension()`连接服务端，返回**OH_IPCRemoteProxy对象**
- 从OH_IPCRemoteProxy对象中获取IRemoteObject
- IPC通信时调用`OH_IPCRemoteProxy_SendRequest()`接口
- 服务端的`ServiceExtension::OnRemoteRequest()`接收并处理请求

**连接流程**：
```
1. 客户端调用 connectExtension(bundleName, abilityName)
   ↓
2. 系统启动ServiceExtension进程（如未运行）
   ↓
3. ServiceExtension::OnConnect() 被调用
   ↓
4. 返回 OH_IPCRemoteProxy 对象
   ↓
5. 客户端调用 OH_IPCRemoteProxy_GetObject(proxy)
   ↓
6. 获取 IRemoteObject
   ↓
7. CoCreateInstance(remoteObject, &dispatch)
   ↓
8. IDispatchProxy持有 OH_IPCRemoteProxy 和 IRemoteObject
```

**IPC通信流程**：
```
IDispatchProxy
    ↓
OH_IPCRemoteProxy_SendRequest(proxy, code, data, reply, option)
    ↓
内部获取 IRemoteObject 并进行IPC传输
    ↓ IPC
ServiceExtension::OnRemoteRequest(code, data, reply)
    ↓
处理请求并返回结果
```

**关键API说明**：
```c
// connectExtension 返回 OH_IPCRemoteProxy 对象
OH_IPCRemoteProxy* proxy = connectExtension(bundleName, abilityName);

// 从 OH_IPCRemoteProxy 获取 IRemoteObject
IRemoteObject* remoteObject = OH_IPCRemoteProxy_GetObject(proxy);

// 使用 OH_IPCRemoteProxy_SendRequest 进行IPC通信
int32_t err = OH_IPCRemoteProxy_SendRequest(
    proxy,              // OH_IPCRemoteProxy 对象
    code,               // IPC code
    data,               // 请求数据
    reply,              // 响应数据
    option              // 选项
 );
 ```

### 3.4 目录结构

```
foundation/distributed/idispatch/
├── interfaces/
│   ├── innerkits/
│   │   ├── native/
│   │   │   └── include/
│   │   │       └── idispatch_interface.h      # NDK接口头文件（对外）
│   │   └── cpp/
│   │       ├── include/
│   │       │   ├── idispatch.h                # IDispatch类定义（内部）
│   │       │   ├── idispatch_struct.h          # 结构体操作类定义（内部）
│   │       │   ├── json_metadata_parser.h      # JSON元数据解析器
│   │       │   └── parameter_marshaller.h      # 参数编解码器
│   │       ├── src/
│   │       │   ├── idispatch.cpp               # IDispatch类实现
│   │       │   ├── idispatch_ndk.cpp           # NDK函数实现
│   │       │   ├── idispatch_struct.cpp        # 结构体操作实现
│   │       │   ├── json_metadata_parser.cpp    # JSON元数据解析实现
│   │       │   └── parameter_marshaller.cpp   # 参数编解码实现
│   └── kits/
│       └── ndk/
│           └── libidispatch_ndkso             # NDK动态库
├── services/
│   └── idispatch_service/
│       ├── include/
│       │   ├── idispatch_service_extension.h    # ServiceExtension头文件
│       │   ├── method_registry.h
│       │   └── struct_marshaller.h            # 结构体序列化器（IDL生成）
│       ├── src/
│       │   ├── idispatch_service_extension.cpp  # ServiceExtension实现
│       │   ├── method_registry.cpp
│       │   ├── invocation_handler.cpp
│       │   └── struct_marshaller.cpp            # 结构体序列化实现（IDL生成）
│       └── bundle.json                          # ServiceExtension配置
├── metadata/
│   └── type_library.json                      # 类型库JSON（IDL生成）
└── tests/
    ├── unittest/
    └── fuzztest/
```

**说明**：
- IDispatch没有自己的Extension类型
- 服务端使用**ServiceExtension**（继承ExtensionAbility）
- 客户端通过`connectExtension()`获取**OH_IPCRemoteProxy**对象
- 从OH_IPCRemoteProxy获取IRemoteObject
- IPC通信时调用`OH_IPCRemoteProxy_SendRequest()`接口

### 3.5 调用流程

```
1. connectExtension("com.ohos.idispatch", "IDispatchExtension")
    ↓
    系统启动ServiceExtension进程（如未运行）
    ↓
    ServiceExtension::OnConnect() 被调用
    ↓
    返回 OH_IPCRemoteProxy 对象
    ↓
    客户端调用 OH_IPCRemoteProxy_GetObject(proxy)
    ↓
    获取 IRemoteObject

2. CoCreateInstance(remoteObject, &dispatch)
    ↓
    创建 IDispatchProxy类实例（内部保存 OH_IPCRemoteProxy 和 IRemoteObject）
    ↓ 返回 IDispatch 对象（void*类型）

3. GetTypeInfo(dispatch, &typeInfo)
    ↓
    OH_IPCRemoteProxy_SendRequest(proxy, CMD_GET_TYPE_INFO, data, reply, option)
    ↓
    内部获取 IRemoteObject 并进行IPC传输
    ↓ IPC
    ↓
    ServiceExtension::OnRemoteRequest(CMD_GET_TYPE_INFO, data, reply)
    ↓
    ServiceExtension读取本地tlb.json文件
    ↓ 将tlb.json文件内容序列化到reply
    ↓ IPC
    ↓
    IDispatchProxy接收tlb.json字符串
    ↓ 解析JSON并缓存在本地内存
    ↓ 创建IDispatchTypeInfo对象
    ↓ 返回typeInfo对象

4. typeInfo->GetMethodCount(typeInfo, &count)
    ↓
    从本地缓存的tlb.json元数据中查询
    ↓ 返回count

5. typeInfo->GetMethodName(typeInfo, index, name, size)
    ↓
    从本地缓存的tlb.json元数据中查询
    ↓ 返回name

6. GetIDsOfNames(dispatch, &name, 1, &dispId)
    ↓
    从本地缓存的tlb.json元数据中查找 name
    ↓ 返回 dispId

7. Invoke(dispatch, dispId, name, cArgs, args, &result)
    ↓
    序列化参数到 MessageParcel
    ↓
    OH_IPCRemoteProxy_SendRequest(proxy, CMD_INVOKE, data, reply, option)
    ↓
    内部获取 IRemoteObject 并进行IPC传输
    ↓ IPC
    ↓
    ServiceExtension::OnRemoteRequest(CMD_INVOKE, data, reply)
    ↓
    ServiceExtension处理请求
    ↓ 序列化返回值到 reply
    ↓ IPC
    ↓
    IDispatchProxy反序列化返回值
```

---

## 四、功能FMEA分析

| 失败模式 | 失败原因 | 影响范围 | 检测方式 | 恢复措施 |
|----------|----------|----------|----------|----------|
| Extension连接失败 | Extension未安装或权限不足 | 无法创建IDispatch对象 | 连接返回错误码 | 提示用户安装Extension或检查权限 |
| 元数据解析失败 | JSON格式错误或版本不匹配 | 无法获取类型信息 | JSON解析错误 | 使用默认元数据或降级处理 |
| DispID查找失败 | 方法名不存在或元数据未加载 | 方法调用失败 | 返回METHOD_NOT_FOUND | 返回错误码给调用者 |
| IPC通信失败 | Extension进程崩溃或网络异常 | 调用超时或失败 | SendRequest返回错误 | 重试机制或提示用户 |
| 参数类型不匹配 | 传入参数类型与元数据不符 | 调用失败或数据错误 | 参数验证 | 返回TYPE_MISMATCH错误 |
| 内存不足 | VARIANT参数过大或频繁分配 | 系统性能下降 | 内存监控 | 限制参数大小，使用对象池 |
| 权限不足 | 调用者无权限调用该方法 | 调用被拒绝 | 权限检查 | 返回PERMISSION_DENIED |

---

## 五、接口设计

### 5.1 NDK C接口

```c
// interfaces/innerkits/native/include/idispatch_interface.h

#ifndef ABILITY_RUNTIME_IDISPATCH_H
#define ABILITY_RUNTIME_IDISPATCH_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// 错误码定义
typedef enum {
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OK = 0,
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PARAM_INVALID = -1,
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_METHOD_NOT_FOUND = -2,
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_TYPE_MISMATCH = -3,
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_IPC_FAILED = -4,
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_OUT_OF_MEMORY = -5,
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_PERMISSION_DENIED = -6,
    ABILITY_RUNTIME_IDISPATCH_ERROR_CODE_CONTEXT_NOT_EXIST = -7
} AbilityRuntime_IDispatch_ErrorCode;

// 变体类型（与tlb.json中的type值对应）
typedef enum {
    ABILITY_RUNTIME_IDISPATCH_VT_I8 = 1,                   // 8位整数
    ABILITY_RUNTIME_IDISPATCH_VT_I32 = 2,                  // 32位整数
    ABILITY_RUNTIME_IDISPATCH_VT_I64 = 3,                  // 64位整数
    ABILITY_RUNTIME_IDISPATCH_VT_F64 = 4,                   // 浮点数
    ABILITY_RUNTIME_IDISPATCH_VT_STRING = 5,                // 字符串
    ABILITY_RUNTIME_IDISPATCH_VT_VOID = 6,                  // 空类型
    ABILITY_RUNTIME_IDISPATCH_VT_ENUM = 7,                  // 枚举类型
    ABILITY_RUNTIME_IDISPATCH_VT_INTERFACE = 8,             // 接口类型
    ABILITY_RUNTIME_IDISPATCH_VT_STRUCT = 9                 // 结构体类型
} AbilityRuntime_IDispatch_Vt;

// 结构体句柄（不透明指针，隐藏内部实现）
typedef void* AbilityRuntime_IDispatch_StructHandle;

// 变体结构（使用指针+类型标签实现ABI兼容性）
typedef struct {
    AbilityRuntime_IDispatch_Vt vt;  // 类型标签
    void* value;  // 指向实际值（不透明，内部实现可自由演进）
} AbilityRuntime_IDispatch_Variant;

// 参数信息结构
typedef struct {
    const char* name;
    AbilityRuntime_IDispatch_Vt vt;
    bool isIn;
    bool isOut;
} AbilityRuntime_IDispatch_ParamInfo;

// 方法信息结构
typedef struct {
    uint32_t dispId;
    const char* name;
    const char* returnType;
    AbilityRuntime_IDispatch_ParamInfo* params;
    uint32_t cParams;
} AbilityRuntime_IDispatch_MethodInfo;

// 类型信息接口
typedef struct AbilityRuntime_IDispatch_TypeInfo AbilityRuntime_IDispatch_TypeInfo;
struct AbilityRuntime_IDispatch_TypeInfo {
    // 获取接口名称
    int32_t (*GetInterfaceName)(AbilityRuntime_IDispatch_TypeInfo* self, char* name, uint32_t size);

    // 获取接口描述符
    int32_t (*GetInterfaceDescriptor)(AbilityRuntime_IDispatch_TypeInfo* self, char* descriptor, uint32_t size);

    // 获取接口版本
    int32_t (*GetInterfaceVersion)(AbilityRuntime_IDispatch_TypeInfo* self, uint32_t* version);

    // 获取方法数量
    int32_t (*GetMethodCount)(AbilityRuntime_IDispatch_TypeInfo* self, uint32_t* count);

    // 根据索引获取方法名称
    int32_t (*GetMethodName)(AbilityRuntime_IDispatch_TypeInfo* self, uint32_t index, char* name, uint32_t size);

    // 根据方法名获取DispID
    int32_t (*GetMethodDispId)(AbilityRuntime_IDispatch_TypeInfo* self, const char* name, uint32_t* dispId);

    // 根据DispID获取方法名称
    int32_t (*GetMethodNameByDispId)(AbilityRuntime_IDispatch_TypeInfo* self, uint32_t dispId, char* name, uint32_t size);

    // 获取方法参数数量
    int32_t (*GetMethodParamCount)(AbilityRuntime_IDispatch_TypeInfo* self, uint32_t dispId, uint32_t* count);

    // 获取方法参数名称
    int32_t (*GetMethodParamName)(AbilityRuntime_IDispatch_TypeInfo* self, uint32_t dispId, uint32_t paramIndex, char* name, uint32_t size);

    // 获取方法参数类型
    int32_t (*GetMethodParamType)(AbilityRuntime_IDispatch_TypeInfo* self, uint32_t dispId, uint32_t paramIndex, char* type, uint32_t size);

    // 获取方法参数方向
    int32_t (*GetMethodParamDirection)(AbilityRuntime_IDispatch_TypeInfo* self, uint32_t dispId, uint32_t paramIndex, char* direction, uint32_t size);

    // 获取方法返回类型
    int32_t (*GetMethodReturnType)(AbilityRuntime_IDispatch_TypeInfo* self, uint32_t dispId, char* type, uint32_t size);

    // 获取枚举方法数量
    int32_t (*GetEnumCount)(AbilityRuntime_IDispatch_TypeInfo* self, uint32_t* count);

    // 根据索引获取枚举名称
    int32_t (*GetEnumName)(AbilityRuntime_IDispatch_TypeInfo* self, uint32_t index, char* name, uint32_t size);

    // 获取枚举值数量
    int32_t (*GetEnumValueCount)(AbilityRuntime_IDispatch_TypeInfo* self, const char* enumName, uint32_t* count);

    // 获取枚举值名称
    int32_t (*GetEnumValueName)(AbilityRuntime_IDispatch_TypeInfo* self, const char* enumName, uint32_t valueIndex, char* name, uint32_t size);

    // 获取枚举值
    int32_t (*GetEnumValue)(AbilityRuntime_IDispatch_TypeInfo* self, const char* enumName, uint32_t valueIndex, int32_t* value);

    // 获取结构体数量
    int32_t (*GetStructCount)(AbilityRuntime_IDispatch_TypeInfo* self, uint32_t* count);

    // 根据索引获取结构体名称
    int32_t (*GetStructName)(AbilityRuntime_IDispatch_TypeInfo* self, uint32_t index, char* name, uint32_t size);

    // 获取结构体字段数量
    int32_t (*GetStructFieldCount)(AbilityRuntime_IDispatch_TypeInfo* self, const char* structName, uint32_t* count);

    // 获取结构体字段名称
    int32_t (*GetStructFieldName)(AbilityRuntime_IDispatch_TypeInfo* self, const char* structName, uint32_t fieldIndex, char* name, uint32_t size);

    // 获取结构体字段类型
    int32_t (*GetStructFieldType)(AbilityRuntime_IDispatch_TypeInfo* self, const char* structName, uint32_t fieldIndex, char* type, uint32_t size);

    // 释放类型信息对象
    void (*Release)(AbilityRuntime_IDispatch_TypeInfo* self);
};
} VARIANT;

// 类型信息接口
typedef struct IDispatchTypeInfo IDispatchTypeInfo;
struct IDispatchTypeInfo {
    // 获取接口名称
    int32_t (*GetInterfaceName)(IDispatchTypeInfo* self, char* name, uint32_t size);

    // 获取接口描述符
    int32_t (*GetInterfaceDescriptor)(IDispatchTypeInfo* self, char* descriptor, uint32_t size);

    // 获取接口版本
    int32_t (*GetInterfaceVersion)(IDispatchTypeInfo* self, uint32_t* version);

    // 获取方法数量
    int32_t (*GetMethodCount)(IDispatchTypeInfo* self, uint32_t* count);

    // 根据索引获取方法名称
    int32_t (*GetMethodName)(IDispatchTypeInfo* self, uint32_t index, char* name, uint32_t size);

    // 根据方法名获取DispID
    int32_t (*GetMethodDispId)(IDispatchTypeInfo* self, const char* name, uint32_t* dispId);

    // 根据DispID获取方法名称
    int32_t (*GetMethodNameByDispId)(IDispatchTypeInfo* self, uint32_t dispId, char* name, uint32_t size);

    // 获取方法参数数量
    int32_t (*GetMethodParamCount)(IDispatchTypeInfo* self, uint32_t dispId, uint32_t* count);

    // 获取方法参数名称
    int32_t (*GetMethodParamName)(IDispatchTypeInfo* self, uint32_t dispId, uint32_t paramIndex, char* name, uint32_t size);

    // 获取方法参数类型
    int32_t (*GetMethodParamType)(IDispatchTypeInfo* self, uint32_t dispId, uint32_t paramIndex, char* type, uint32_t size);

    // 获取方法参数方向
    int32_t (*GetMethodParamDirection)(IDispatchTypeInfo* self, uint32_t dispId, uint32_t paramIndex, char* direction, uint32_t size);

    // 获取方法返回类型
    int32_t (*GetMethodReturnType)(IDispatchTypeInfo* self, uint32_t dispId, char* type, uint32_t size);

    // 获取枚举方法数量
    int32_t (*GetEnumCount)(IDispatchTypeInfo* self, uint32_t* count);

    // 根据索引获取枚举名称
    int32_t (*GetEnumName)(IDispatchTypeInfo* self, uint32_t index, char* name, uint32_t size);

    // 获取枚举值数量
    int32_t (*GetEnumValueCount)(IDispatchTypeInfo* self, const char* enumName, uint32_t* count);

    // 获取枚举值名称
    int32_t (*GetEnumValueName)(IDispatchTypeInfo* self, const char* enumName, uint32_t valueIndex, char* name, uint32_t size);

    // 获取枚举值
    int32_t (*GetEnumValue)(IDispatchTypeInfo* self, const char* enumName, uint32_t valueIndex, int32_t* value);

    // 获取结构体数量
    int32_t (*GetStructCount)(IDispatchTypeInfo* self, uint32_t* count);

    // 根据索引获取结构体名称
    int32_t (*GetStructName)(IDispatchTypeInfo* self, uint32_t index, char* name, uint32_t size);

    // 获取结构体字段数量
    int32_t (*GetStructFieldCount)(IDispatchTypeInfo* self, const char* structName, uint32_t* count);

    // 获取结构体字段名称
    int32_t (*GetStructFieldName)(IDispatchTypeInfo* self, const char* structName, uint32_t fieldIndex, char* name, uint32_t size);

    // 获取结构体字段类型
    int32_t (*GetStructFieldType)(IDispatchTypeInfo* self, const char* structName, uint32_t fieldIndex, char* type, uint32_t size);

    // 释放类型信息对象
    void (*Release)(IDispatchTypeInfo* self);
};

// 核心接口
typedef void* IDispatch;
typedef void* OH_IPCRemoteProxyHandle;

// CoCreateInstance - 创建IDispatch对象
// 参数:
//   - ipcRemoteProxy: 通过connectExtension获取的OH_IPCRemoteProxy对象
//   - ppDispatch: 输出IDispatch对象指针（void*类型，内部为IDispatch类实例）
// 返回值:
//   - IDISPATCH_OK: 成功
//   - 其他: 失败
int32_t CoCreateInstance(OH_IPCRemoteProxyHandle ipcRemoteProxy, IDispatch* ppDispatch);

// 释放OH_IPCRemoteProxy对象
void ReleaseExtensionProxy(OH_IPCRemoteProxyHandle ipcRemoteProxy);

// GetIDsOfNames - 获取方法/属性的DispID
// 参数:
//   - dispatch: IDispatch对象
//   - rgszNames: 方法/属性名称数组
//   - cNames: 名称数量
//   - rgDispId: 输出DispID数组
// 返回值: IDISPATCH_OK成功，其他失败
int32_t GetIDsOfNames(IDispatch dispatch, const char** rgszNames, uint32_t cNames, uint32_t* rgDispId);

// Invoke - 调用方法或访问属性
// 参数:
//   - dispatch: IDispatch对象
//   - dispId: DispID（优先）或0
//   - lpszMemberName: 方法/属性名称（dispId为0时使用）
//   - cArgs: 参数数量
//   - pArgs: 参数数组
//   - pVarResult: 输出返回值
// 返回值: IDISPATCH_OK成功，其他失败
int32_t Invoke(IDispatch dispatch, uint32_t dispId, const char* lpszMemberName, uint32_t cArgs, VARIANT* pArgs, VARIANT* pVarResult);

// GetTypeInfo - 获取类型信息
// 参数:
//   - dispatch: IDispatch对象
//   - ppTypeInfo: 输出类型信息对象
// 返回值: IDISPATCH_OK成功，其他失败
int32_t GetTypeInfo(IDispatch dispatch, IDispatchTypeInfo** ppTypeInfo);

// Release - 释放IDispatch对象
// 参数:
//   - dispatch: IDispatch对象
void Release(IDispatch dispatch);

// CreateStruct - 创建结构体
// 参数:
//   - dispatch: IDispatch对象（用于获取元数据）
//   - structName: 结构体名称（从tlb.json中查询）
//   - pStruct: 输出结构体句柄
// 返回值: IDISPATCH_OK成功，其他失败
int32_t CreateStruct(IDispatch dispatch, const char* structName, IDispatchStruct* pStruct);

// SetStructField - 设置结构体字段值
// 参数:
//   - pStruct: 结构体句柄
//   - fieldName: 字段名称
//   - pValue: 字段值（VARIANT类型）
// 返回值: IDISPATCH_OK成功，其他失败
int32_t SetStructField(IDispatchStruct pStruct, const char* fieldName, const VARIANT* pValue);

// GetStructField - 获取结构体字段值
// 参数:
//   - pStruct: 结构体句柄
//   - fieldName: 字段名称
//   - pValue: 输出字段值（VARIANT类型）
'// 返回值: IDISPATCH_OK成功，其他失败
int32_t GetStructField(IDispatchStruct pStruct, const char* fieldName, VARIANT* pValue);

// ReleaseStruct - 释放结构体
// 参数:
//   - pStruct: 结构体句柄
void ReleaseStruct(IDispatchStruct pStruct);
```

### 5.2 与Windows IDispatch对比

| Windows | 鸿蒙 | 说明 |
|---------|------|------|
| `CoCreateInstance(CLSID, IID, ppObj)` | `CoCreateInstance(ipcRemoteProxy, ppDispatch)` | 鸿蒙使用OH_IPCRemoteProxy对象 |
| `IDispatch::GetIDsOfNames` | `GetIDsOfNames(dispatch, ...)` | 鸿蒙为C函数，传递dispatch对象 |
| `IDispatch::Invoke` | `Invoke(dispatch, ...)` | 鸿蒙为C函数，传递dispatch对象 |
| `ITypeInfo` | `IDispatchTypeInfo` | 扩展版本，支持TLB JSON元数据查询 |
| `.tlb` 文件 | `.json` 文件 | 鸿蒙使用JSON格式元数据 |
| `DISPID` | `uint32_t dispId` | 保持一致 |
| `VARIANT` | `VARIANT` | 完整版本（参考Windows） |
| `BSTR` | `char*` | 鸿蒙使用UTF-8字符串（Windows为UTF-16） |

### 5.3 VARIANT类型到TLB JSON类型映射

| VARIANT类型 | TLB JSON type | 说明 |
|-------------|---------------|------|
| VT_I8 | i8 | 8位整数 |
| VT_I32 | i32 | 32位整数 |
| VT_I64 | i64 | 64位整数 |
| VT_F64 | f64 | 浮点数 |
| VT_STRING | String | 字符串（UTF-8） |
| VT_VOID | void | 空类型 |
| VT_ENUM | enum | 枚举类型，需要指定idl_type |
| VT_INTERFACE | interface | 接口类型，需要指定idl_type |
| VT_STRUCT | struct | 结构体类型，需要指定idl_type |

**注意**：
1. 支持的TLB JSON type值：i8、i32、i64、f64、String、void、enum、interface、struct
2. 当type为"interface"、"enum"或"struct"时，需要指定idl_type字段为具体的类型名称

### 5.4 ITypeInfo使用示例

#### 获取接口信息
```c
IDispatchTypeInfo* typeInfo = nullptr;
int32_t ret = GetTypeInfo(dispatch, &typeInfo);
if (ret == IDISPATCH_OK) {
    char charName[256];
    char descriptor[256];
    uint32_t version = 0;

    typeInfo->GetInterfaceName(typeInfo, charName, sizeof(charName));
    typeInfo->GetInterfaceDescriptor(typeInfo, descriptor, sizeof(descriptor));
    typeInfo->GetInterfaceVersion(typeInfo, &version);

    printf("Interface: %s, Descriptor: %s, Version: %u\n", charName, descriptor, version);
}
```

#### 遍历所有方法
```c
uint32_t methodCount = 0;
typeInfo->GetMethodCount(typeInfo, &methodCount);

for (uint32_t i = 0; i < methodCount; i++) {
    char methodName[256];
    uint32_t dispId = 0;
    uint32_t paramCount = 0;
    char returnType[256];

    typeInfo->GetMethodName(typeInfo, i, methodName, sizeof(methodName));
    typeInfo->GetMethodDispId(typeInfo, methodName, &dispId);
    typeInfo->GetMethodParamCount(typeInfo, dispId, &paramCount);
    typeInfo->GetMethodReturnType(typeInfo, dispId, returnType, sizeof(returnType));

    printf("Method[%u]: %s, DispID: %u, ReturnType: %s, Params: %u\n",
           i, methodName, dispId, returnType, paramCount);

    // 遍历参数
    for (uint32_t j = 0; j < paramCount; j++) {
        char paramName[256];
        char paramType[256];
        char direction[16];

        typeInfo->GetMethodParamName(typeInfo, dispId, j, paramName, sizeof(paramName));
        typeInfo->GetMethodParamType(typeInfo, dispId, j, paramType, sizeof(paramType));
        typeInfo->GetMethodParamDirection(typeInfo, dispId, j, direction, sizeof(direction));

        printf("  Param[%u]: %s, Type: %s, Direction: %s\n", j, paramName, paramType, direction);
    }
}
```

#### 获取枚举信息
```c
uint32_t enumCount = 0;
typeInfo->GetEnumCount(typeInfo, &enumCount);

for (uint32_t i = 0; i < enumCount; i++) {
    char enumName[256];
    uint32_t valueCount = 0;

    typeInfo->GetEnumName(typeInfo, i, enumName, sizeof(enumName));
    typeInfo->GetEnumValueCount(typeInfo, enumName, &valueCount);

    printf("Enum[%u]: %s, Values: %u\n", i, enumName, valueCount);

    // 遍历枚举值
    for (uint32_t j = 0; j < valueCount; j++) {
        char valueName[256];
        int32_t value = 0;

        typeInfo->GetEnumValueName(typeInfo, enumName, j, valueName, sizeof(valueName));
        typeInfo->GetEnumValue(typeInfo, enumName, j, &value);

        printf("  %s = %d\n", valueName, value);
    }
}
```

#### 获取结构体信息
```c
uint32_t structCount = 0;
typeInfo->GetStructCount(typeInfo, &structCount);

for (uint32_t i = 0; i < structCount; i++) {
    char structName[256];
    uint32_t fieldCount = 0;

    typeInfo->GetStructName(typeInfo, i, structName, sizeof(structName));
    typeInfo->GetStructFieldCount(typeInfo, structName, &fieldCount);

    printf("Struct[%u]: %s, Fields: %u\n", i, structName, fieldCount);

    // 遍历字段
    for (uint32_t j = 0; j < fieldCount; j++) {
        char fieldName[256];
        char fieldType[256];

        typeInfo->GetStructFieldName(typeInfo, structName, j, fieldName, sizeof(fieldName));
        typeInfo->GetStructFieldType(typeInfo, structName, j, fieldType, sizeof(fieldType));

        printf("  Field[%u]: %s, Type: %s\n", j, fieldName, fieldType);
    }
}
```

### 5.5 结构体传递机制

#### 架构说明
```
服务端（IDL生成代码）
    ↓ 编译期间
知道结构体定义（UserInfo、Address等）
    ↓ 运行期间
直接使用结构体类型
    ↓ 序列化/反序列化
按照结构体字段顺序读写基本类型

IDispatch（NDK库）
    ↓ 运行期间
从tlb.json获取结构体定义
    ↓ 按照tlb.json中字段顺序
序列化/反序列化基本类型
    ↓ 支持嵌套结构体递归

客户端（应用代码）
    ↓ 运行期间
不知道结构体定义
    ↓ 通过IDispatch接口
CreateStruct、SetStructField、GetStructField
```

#### 序列化规则
1. **字段顺序**：按照tlb.json中结构体定义的字段顺序序列化
2. **默认值**：未设置的字段使用默认值（数值类型为0，字符串为空字符串）
3. **嵌套结构体**：支持递归序列化，先写非空标记，再写内容
4. **字符串格式**：先写长度（uint32_t），再写内容
5. **嵌套结构体格式**：先写非空标记（uint8_t），再写内容

#### 服务端实现（IDL生成代码）

```cpp
// IDL工具根据tlb.json生成结构体定义
struct UserInfo {
    int32_t id;
    char* name;
    int32_t status;  // StatusCode枚举
    Address* address; // 嵌套结构体
};

struct Address {
    char* city;
    char* zip;
};

// IDL工具生成序列化/反序列化代码
class StructMarshaller {
public:
    // 序列化UserInfo到MessageParcel
    static int32_t MarshalUserInfo(const UserInfo* user, MessageParcel& parcel) {
        // 按照结构体字段顺序写入
        parcel.WriteInt32(user->id);
        
        // 字符串：先写长度，再写内容
        uint32_t nameLen = strlen(user->name);
        parcel.WriteUint32(nameLen);
        parcel.WriteBuffer(user->name, nameLen);
        
        parcel.WriteInt32(user->status);
        
        // 嵌套结构体
        if (user->address) {
            parcel.WriteUint8(1);  // 非空标记
            MarshalAddress(user->address, parcel);
        } else {
            parcel.WriteUint8(0);  // 空标记
        }
        
        return ERR_NONE;
    }
    
    // 反序列化UserInfo
    static int32_t UnmarshalUserInfo(MessageParcel& parcel, UserInfo* user) {
        // 按照结构体字段顺序读取
        parcel.ReadInt32(&user->id);
        
        // 字符串
        uint32_t nameLen;
        parcel.ReadUint32(&nameLen);
        user->name = (char*)malloc(nameLen + 1);
        parcel.ReadBuffer(user->name, nameLen);
        user->name[nameLen] = '\0';
        
        parcel.ReadInt32(&user->status);
        
        // 嵌套结构体
        uint8_t hasAddress;
        parcel.ReadUint8(&hasAddress);
        if (hasAddress) {
            user->address = new Address();
            UnmarshalAddress(parcel, user->address);
        } else {
            user->address = nullptr;
        }
        
        return ERR_NONE;
    }
    
    // 序列化Address
    static int32_t MarshalAddress(const Address* addr, MessageParcel& parcel) {
        uint32_t cityLen = strlen(addr->city);
        parcel.WriteUint32(cityLen);
        parcel.WriteBuffer(addr->city, cityLen);
        
        uint32_t zipLen = strlen(addr->zip);
        parcel.WriteUint32(zipLen);
        parcel.WriteBuffer(addr->zip, zipLen);
        
        return ERR_NONE;
    }
    
    // 反序列化Address
    static int32_t UnmarshalAddress(MessageParcel& parcel, Address* addr) {
        uint32_t cityLen;
        parcel.ReadUint32(&cityLen);
        addr->city = (char*)malloc(cityLen + 1);
        parcel.ReadBuffer(addr->city, cityLen);
        addr->city[cityLen] = '\0';
        
        uint32_t zipLen;
        parcel.ReadUint32(&zipLen);
        addr->zip = (char*)malloc(zipLen + 1);
        parcel.ReadBuffer(addr->zip, zipLen);
        addr->zip[zipLen] = '\0';
        
        return ERR_NONE;
    }
    
    // 释放UserInfo
    static void FreeUserInfo(UserInfo* user) {
        if (user) {
            if (user->name) free(user->name);
            if (user->address) {
                FreeAddress(user->address);
                delete user->address;
            }
        }
    }
    
    // 释放Address
    static void FreeAddress(Address* addr) {
        if (addr) {
            if (addr->city) free(addr->city);
            if (addr->zip) free(addr->zip);
        }
    }
};
```

#### IDispatch结构体序列化实现

```cpp
class IDispatchStructImpl {
private:
    std::string structName_;
    std::shared_ptr<JsonMetadataParser> metadataParser_;
    std::unordered_map<std::string, VARIANT> fields_;
    
public:
    IDispatchStructImpl(const std::string& name, 
                       std::shared_ptr<JsonMetadataParser> parser)
        : structName_(name), metadataParser_(parser) {}
    
    int32_t SetField(const std::string& fieldName, const VARIANT& value) {
        // 从tlb.json验证字段是否存在
        if (!metadataParser_->ValidateField(structName_, fieldName)) {
            return IDISPATCH_ERR_METHOD_NOT_FOUND;
        }
        
        // 深拷贝VARIANT
        fields_[fieldName] = DeepCopyVariant(value);
        return IDISPATCH_OK;
    }
    
    int32_t GetField(const std::string& fieldName, VARIANT& value) {
        auto it = fields_.find(fieldName);
        if (it != fields_.end()) {
            value = DeepCopyVariant(it->second);
            return IDISPATCH_OK;
        }
        
        // 字段未设置，返回默认值
        std::string type = metadataParser_->GetFieldType(structName_, fieldName);
        value = GetDefaultValue(type);
        return IDISPATCH_OK;
    }
    
    // 序列化到MessageParcel（按照tlb.json中结构体定义的顺序）
    int32_t SerializeToParcel(MessageParcel& parcel) {
        // 从tlb.json获取结构体定义
        StructInfo structInfo = metadataParser_->GetStructInfo(structName_);
        
        // 按照tlb.json中结构体定义的顺序遍历字段
        for (auto& field : structInfo.fields) {
            auto it = fields_.find(field.name);
            VARIANT var;
            
            if (it != fields_.end()) {
                // 字段已设置，使用实际值
                var = it->second;
            } else {
                // 字段未设置，使用默认值
                var = GetDefaultValue(field.typeInfo.type);
            }
            
            // 写入MessageParcel
            WriteVariantToParcel(var, field.typeInfo, parcel);
        }
        
        return IDISPATCH_OK;
    }
    
    // 从MessageParcel反序列化
    int32_t DeserializeFromParcel(MessageParcel& parcel) {
        // 从tlb.json获取结构体定义
        StructInfo structInfo = metadataParser_->GetStructInfo(structName_);
        
        // 按照tlb.json中结构体定义的顺序遍历字段
        for (auto& field : structInfo.fields) {
            // 从MessageParcel读取
            VARIANT var;
            ReadVariantFromParcel(field.typeInfo, parcel, var);
            
            // 设置到字段
            fields_[field.name] = var;
        }
        
        return IDISPATCH_OK;
    }
    
private:
    VARIANT GetDefaultValue(const std::string& type) {
        VARIANT var;
        if (type == "i8") {
            var.vt = VT_I8;
            var.i8Val = 0;
        } else if (type == "i32") {
            var.vt = VT_I32;
            var.i32Val = 0;
        } else if (type == "i64") {
            var.vt = VT_I64;
            var.i64Val = 0;
        } else if (type == "f64") {
            var.vt = VT_F64;
            var.f64Val = 0.0;
        } else if (type == "String") {
            var.vt = VT_STRING;
            var.strVal = strdup("");  // 空字符串
        } else if (type == "enum") {
            var.vt = VT_ENUM;
            var.enumVal = 0;  // 第一个枚举值
        } else if (type == "struct") {
            var.vt = VT_STRUCT;
            var.structVal = nullptr;  // 空结构体
        }
        return var;
    }
    
    void WriteVariantToParcel(const VARIANT& var, const TypeInfo& typeInfo, 
                               MessageParcel& parcel) {
        if (typeInfo.type == "i8") {
            parcel.WriteInt8(var.i8Val);
        } else if (typeInfo.type == "i32") {
            parcel.WriteInt32(var.i32Val);
        } else if (typeInfo.type == "i64") {
            parcel.WriteInt64(var.i64Val);
        } else if (typeInfo.type == "f64") {
            parcel.WriteDouble(var.f64Val);
        } else if (typeInfo.type == "String") {
            uint32_t len = strlen(var.strVal);
            parcel.WriteUint32(len);
            parcel.WriteBuffer(var.strVal, len);
        } else if (typeInfo.type == "enum") {
            parcel.WriteInt32(var.enumVal);
        } else if (typeInfo.type == "struct") {
            // 嵌套结构体，递归序列化
            if (var.structVal) {
                parcel.WriteUint8(1);  // 非空标记
                IDispatchStructImpl* nestedStruct = (IDispatchStructImpl)var.structVal;
                nestedStruct->SerializeToParcel(parcel);
            } else {
                parcel.WriteUint8(0);  // 空标记
            }
        }
    }
    
    void ReadVariantFromParcel(const TypeInfo& typeInfo, MessageParcel& parcel, 
                               VARIANT& var) {
        if (typeInfo.type == "i8") {
            var.vt = VT_I8;
            parcel.ReadInt8(&var.i8Val);
        } else if (typeInfo.type == "i32") {
            var.vt = VT_I32;
            parcel.ReadInt32(&var.i32Val);
        } else if (typeInfo.type == "i64") {
            var.vt = VT_I64;
            parcel.ReadInt64(&var.i64Val);
        } else if (typeInfo.type == "f64") {
            var.vt = VT_F64;
            parcel.ReadDouble(&var.f64Var);
        } else if (typeInfo.type == "String") {
            var.vt = VT_STRING;
            uint32_t len;
            parcel.ReadUint32(&len);
            var.strVal = (char*)malloc(len + 1);
            parcel.ReadBuffer(var.strVal, len);
            var.strVal[len] = '\0';
        } else if (typeInfo.type == "enum") {
            var.vt = VT_ENUM;
            parcel.ReadInt32(&var.enumVal);
        } else if (typeInfo.type == "struct") {
            var.vt = VT_STRUCT;
            // 读取非空标记
            uint8_t hasStruct;
            parcel.ReadUint8(&hasStruct);
            if (hasStruct) {
                // 创建嵌套结构体并递归反序列化
                IDispatchStructImpl* nestedStruct = new IDispatchStructImpl(
                    typeInfo.idl_type, metadataParser_
                );
                nestedStruct->DeserializeFromParcel(parcel);
                var.structVal = nestedStruct;
            } else {
                var.structVal = nullptr;
            }
        }
    }
    
    VARIANT DeepCopyVariant(const VARIANT& var) {
        VARIANT copy = var;
        if (var.vt == VT_STRING && var.strVal) {
            copy.strVal = strdup(var.strVal);
        }
        return copy;
    }
};
```

#### 客户端使用示例

```c
// 创建结构体
IDispatchStruct userStruct = nullptr;
CreateStruct(dispatch, "UserInfo", &userStruct);

// 设置部分字段
VARIANT idVar;
idVar.vt = VT_I32;
idVar.i32Val = 123;
SetStructField(userStruct, "id", &idVar);

VARIANT nameVar;
nameVar.vt = VT_STRING;
nameVar.strVal = "张三";
SetStructField(userStruct, "name", &nameVar);

// 创建嵌套结构体
IDispatchStruct addrStruct = nullptr;
CreateStruct(dispatch, "Address", &addrStruct);

VARIANT cityVar;
cityVar.vt = VT_STRING;
cityVar.strVal = "北京";
SetStructField(addrStruct, "city", &cityVar);

// 设置嵌套结构体
VARIANT addrVar;
addrVar.vt = VT_STRUCT;
addrVar.structVal = addrStruct;
SetStructField(userStruct, "address", &addrVar);

// 注意：status字段未设置，序列化时会使用默认值0

// 创建VARIANT并调用Invoke
VARIANT var;
var.vt = VT_STRUCT;
var.structVal = userStruct;

Invoke(dispatch, dispId, "setUser", 1, &var, &result);

// 释放资源
ReleaseStruct(addrStruct);
ReleaseStruct(userStruct);
```

### 5.6 IPC通信接口说明

#### OH_IPCRemoteProxy接口
```c
// OH_IPCRemoteProxy - connectExtension返回的对象类型
typedef void* OH_IPCRemoteProxyHandle;

// 从OH_IPCRemoteProxy获取IRemoteObject
IRemoteObject* OH_IPCRemoteProxy_GetObject(OH_IPCRemoteProxyHandle proxy);

// 使用OH_IPCRemoteProxy进行IPC通信
int32_t OH_IPCRemoteProxy_SendRequest(
    OH_IPCRemoteProxyHandle proxy,    // OH_IPCRemoteProxy对象
    uint32_t code,                   // IPC code
    MessageParcel* data,             // 请求数据
    MessageParcel* reply,            // 响应数据
    MessageOption* option             // 选项
);
```

#### 使用示例
```c
// 1. 连接Extension，获取OH_IPCRemoteProxy对象
OH_IPCRemoteProxyHandle ipcRemoteProxy = connectExtension(
    "com.ohos.idispatch",
    "IDispatchExtension"
);

// 2. 从OH_IPCRemoteProxy获取IRemoteObject
IRemoteObject* remoteObject = OH_IPCRemoteProxy_GetObject(ipcRemoteProxy);

// 3. 创建IDispatch对象
IDispatch dispatch = nullptr;
int32_t ret = CoCreateInstance(ipcRemoteProxy, &dispatch);

// 4. 使用IDispatch进行方法调用
// 内部会调用 OH_IPCRemoteProxy_SendRequest 进行IPC通信
GetIDsOfNames(dispatch, &name, 1, &dispId);
Invoke(dispatch, dispId, name, cArgs, args, &result);

// 5. 释放资源
Release(dispatch);
ReleaseExtensionProxy(ipcRemoteProxy);
```

---

## 六、约束

### 6.1 功能约束
1. **线程安全**：NDK接口不是线程安全的，多线程调用需要外部同步
2. **调用顺序**：必须先调用GetTypeInfo才能调用GetIDsOfNames和Invoke
3. **资源管理**：调用者必须负责释放IDispatch对象和Extension Proxy
4. **参数限制**：单个VARIANT参数大小不超过1MB，数组元素不超过10000个

### 6.2 性能约束
1. **IPC调用开销**：每次每次Invoke都涉及IPC通信，单次调用耗时约0.5-2ms
2. **元数据加载**：首次GetTypeInfo需要通过IPC从服务端获取JSON元数据，耗时约10-50ms
3. **元数据查询**：元数据加载后，所有查询操作都在本地内存中进行，性能极高
4. **DispID缓存**：DispID查找性能比方法名高约3倍

### 6.3 兼容性约束
1. **VARIANT类型**：支持Windows VARENUM类型，映射到tlb.json中的type值（i8、i32、i64、f64、String、void、enum、interface、struct）
2. **方法重载**：不支持方法重载，方法名必须唯一
3. **Locale支持**：不支持Locale参数，统一使用UTF-8编码
4. **BSTR编码**：Windows使用UTF-16编码，鸿蒙使用UTF-8编码

---

## 七、周边依赖关系

###7.1 依赖组件

| 组件 | 版本要求 | 依赖类型 | 用途 |
|------|----------|----------|------|
| ability_runtime | API 9+ | 强依赖 | Extension连接管理（connectExtension） |
| ipc:ipc_core | API 9+ | 强依赖 | IPC通信（OH_IPCRemoteProxy、IRemoteObject） |
| hiviewdfx_hilog_native | API 9+ | 强依赖 | 日志输出 |
| napi:ace_napi.z | API 9+ | 可选 | NAPI绑定（如需JS支持） |
| bundle_framework | API 9+ | 强依赖 | Bundle管理 |

### 7.2 依赖关系图

```
IDispatch NDK
    ↓ 依赖
├── ability_runtime (connectExtension)
├── ipc:ipc_core (OH_IPCRemoteProxy, OH_IPCRemoteProxy_SendRequest, IRemoteObject)
├── hiviewdfx_hilog_native (日志)
└── bundle_framework (Bundle管理)

IDispatch Extension (ServiceExtension)
    ↓ 依赖
├── ability_runtime (ExtensionAbility)
├── ipc:ipc_core (IRemoteBroker)
├── hiviewdfx_hilog_native (日志)
└── napi:ace_napi.z (可选，JS支持)
```

---

## 八、架构关键设计

### 8.1 DispID机制设计

#### DispID分配规则
```cpp
// 方法DispID: 1 ~ 999
static constexpr uint32_t METHOD_DISPID_START = 1;
static constexpr uint32_t METHOD_DISPID_END = 999;

// 属性DispID: 1000 ~ 1999
static constexpr uint32_t PROPERTY_DISPID_START = 1000;
static constexpr uint32_t PROPERTY_DISPID_END = 1999;

// 自定义DispID: 3000 ~ 0x7FFFFFFF
static constexpr uint32_t CUSTOM_DISPID_START = 3000;
```

#### DispID vs IPC Code关系
```
客户端API
    ↓
DispID (1)
    ↓ GetIDsOfNames
JSON元数据
    ↓ 查找
IPC Code (1)
    ↓ Invoke
SendRequest(ipcCode=1, data, reply)
    ↓ IPC
Extension Stub
    ↓ switch(code)
Add() 实现
```

### 8.2 结构体序列化机制

#### 序列化流程
```
客户端创建结构体
    ↓
SetStructField设置部分字段
    ↓
序列化时按照tlb.json中结构体定义的顺序遍历字段
    ↓
字段已设置 → 写入实际值
字段未设置 → 写入默认值
    ↓
嵌套结构体 → 递归序列化
    ↓
写入MessageParcel（基本类型）
    ↓ IPC传输
服务端
    ↓
按照tlb.json中结构体定义的顺序读取基本类型
    ↓
嵌套结构体 → 递归反序列化
    ↓
填充到结构体对象
```

#### 序列化规则
1. **字段顺序**：按照tlb.json中结构体定义的字段顺序序列化
2. **默认值**：未设置的字段使用默认值（数值类型为0，字符串为空字符串）
3. **嵌套结构体**：支持递归序列化，先写非空标记（uint8_t），再写内容
4. **字符串格式**：先写长度（uint32_t），再写内容
5. **嵌套结构体格式**：先写非空标记（uint8_t），再写内容

#### 元数据缓存机制

#### 服务端元数据管理
```cpp
class IDispatchServiceExtension {
private:
    // tlb.json文件路径
    std::string tlbJsonPath_;

public:
    int32_t OnRemoteRequest(uint32_t code, MessageParcel& data, MessageParcel& reply) {
        switch (code) {
            case CMD_GET_TYPE_INFO:
                // 读取tlb.json文件内容并返回
                std::ifstream file(tlbJsonPath_);
                std::string jsonContent((std::istreambuf_iterator<char>(file)),
                                        std::istreambuf_iterator<char>());
                reply.WriteString(jsonContent);
                break;
            case CMD_INVOKE:
                // 处理方法调用
                break;
        }
        return ERR_NONE;
    }
};
```

#### 客户端元数据缓存
```cpp
class IDispatchProxy {
private:
    // OH_IPCRemoteProxy - 通过connectExtension获取
    OH_IPCRemoteProxy* ipcRemoteProxy_;

    // IRemoteObject - 从OH_IPCRemoteProxy获取
    IRemoteObject* remoteObject_;

    // 从服务端获取的tlb.json文件内容（字符串形式）
    std::string tlbJsonContent_;

    // 元数据解析器（解析后的JSON对象）
    std::shared_ptr<JsonMetadataParser> metadataParser_;

    // 结构体解析器（用于结构体序列化/反序列化）
    std::shared_ptr<StructUnmarshaller> structUnmarshaller_;

    // 元数据是否已加载
    bool metadataLoaded_;

public:
    int32_t LoadMetadata() {
        if (metadataLoaded_) {
            return IDISPATCH_OK;
        }

        // 通过IPC从服务端获取tlb.json文件内容
        MessageParcel data, reply;
        MessageOption option;
        int32_t err = OH_IPCRemoteProxy_SendRequest(
            ipcRemoteProxy_, CMD_GET_TYPE_INFO, data, reply, option
        );

        if (err != ERR_NONE) {
            return IDISPATCH_ERR_IPC_FAILED;
        }

        // 读取tlb.json字符串
        reply.ReadString(tlbJsonContent_);

        // 解析tlb.json元数据
        metadataParser_ = std::make_shared<JsonMetadataParser>(tlbJsonContent_);
        
        // 创建结构体解析器
        structUnmarshaller_ = std::make_shared<StructUnmarshaller>(metadataParser_);
        
        metadataLoaded_ = true;

        return IDISPATCH_OK;
    }
    
    std::shared_ptr<StructUnmarshaller> GetStructUnmarshaller() {
        return structUnmarshaller_;
    }
};
```

#### IDispatchTypeInfo实现
```cpp
class IDispatchTypeInfoImpl {
private:
    // 持有JSON元数据解析器的引用
    std::shared_ptr<JsonMetadataParser> metadataParser_;

public:
    int32_t GetMethodCount(IDispatchTypeInfo* self, uint32_t* count) {
        *count = metadataParser_->GetMethodCount();
        return IDISPATCH_OK;
    }

    int32_t GetMethodName(IDispatchTypeInfo* self, uint32_t index, char* name, uint32_t size) {
        std::string methodName = metadataParser_->GetMethodName(index);
        strncpy(name, methodName.c_str(), size - 1);
        name[size - 1] = '\0';
        return IDISPATCH_OK;
    }

    int32_t GetMethodDispId(IDispatchTypeInfo* self, const char* name, uint32_t* dispId) {
        *dispId = metadataParser_->GetMethodDispId(name);
        return IDISPATCH_OK;
    }

    // ... 其他方法实现
};
```

#### 缓存策略
1. **服务端**：收到CMD_GET_TYPE_INFO请求时，读取本地tlb.json文件并返回文件内容
2. **客户端**：首次GetTypeInfo时通过IPC从服务端获取tlb.json文件内容并解析缓存
3. **生命周期**：元数据在IDispatch对象生命周期内有效
4. **Extension Proxy复用**：Extension连接后，Proxy对象在整个生命周期内复用

#### IPC通信实现
```cpp
// 使用 OH_IPCRemoteProxy_SendRequest 进行IPC通信
int32_t IDispatchProxy::SendRequest(uint32_t code, MessageParcel& data,
                                     MessageParcel& reply, MessageOption& option) {
    if (!ipcRemoteProxy_ || !remoteObject_) {
        return IDISPATCH_ERR_IPC_FAILED;
    }

    // 调用 OH_IPCRemoteProxy_SendRequest 接口
    int32_t err = OH_IPCRemoteProxy_SendRequest(
        ipcRemoteProxy_,  // OH_IPCRemoteProxy 对象
        code,             // IPC code
        data,             // 请求数据
        reply,            // 响应数据
        option            // 选项
    );

    if (err != ERR_NONE) {
        return IDISPATCH_ERR_IPC_FAILED;
    }

    return IDISPATCH_OK;
}
```

### 8.3 动态方法注册

#### 注册机制
```cpp
class MethodRegistry {
private:
    // DispID到方法的映射 - O(1)查找
    std::unordered_map<uint32_t, std::shared_ptr<Method>> dispIdToMethod_;

    // 方法名到DispID的映射 - 用于fallback
    std::unordered_map<std::string, uint32_t> nameToDispId_;

    uint32_t nextDispId_;  // 下一个可用的DispID
    std::mutex mutex_;     // 线程安全

public:
    // 动态注册方法 - 自动分配DispID
    int32_t RegisterMethod(const std::string& name,
                         std::function<int32_t(const std::vector<VARIANT>&, VARIANT&)> func,
                         uint32_t& outDispId);
};
```

---

## 九、开放能力设计和三方兼容性设计

### 9.1 Windows兼容性设计

#### API兼容性
| Windows API | 鸿蒙 API | 兼容性 |
|-------------|-----------|--------|
| `CoCreateInstance` | `CoCreateInstance` | ✅ 参数不同，语义一致 |
| `IDispatch::GetIDsOfNames` | `GetIDsOfNames` | ✅ C函数，需传递dispatch |
| `IDispatch::Invoke` | `Invoke` | ✅ C函数，需传递dispatch |
| `ITypeInfo` | `IDispatchTypeInfo` | ✅ 扩展版本，支持TLB JSON元数据查询 |
| `VARIANT` | `VARIANT` | ⚠️ 类型子集 |

#### ITypeInfo接口对比

| Windows ITypeInfo方法 | 鸿蒙 IDispatchTypeInfo方法 | 说明 |
|----------------------|---------------------------|------|
| `GetTypeAttr` | `GetInterfaceName`, `GetInterfaceDescriptor`, `GetInterfaceVersion` | 获取接口类型信息 |
| `GetFuncDesc` | `GetMethodNameByDispId`, `GetMethodReturnType`, `GetMethodParamCount` | 根据DispID获取方法信息 |
| `GetIDsOfNames` | `GetMethodDispId` | 根据方法名获取DispID |
| `GetFuncCount` | `GetMethodCount` | 获取方法数量 |
| `GetDocumentation` | - | 暂不支持文档信息 |
| - | `GetEnumCount`, `GetEnumName`, `GetEnumValueCount`, `GetEnumValueName`, `GetEnumValue` | 获取枚举信息（扩展） |
| - | `GetStructCount`, `GetStructName`, `GetStructFieldCount`, `GetStructFieldName`, `GetStructFieldType` | 获取结构体信息（扩展） |
| - | `GetMethodParamName`, `GetMethodParamType`, `GetMethodParamDirection` | 获取方法参数信息（（扩展） |
| `ReleaseTypeAttr` | - | 自动管理，无需手动释放 |
| `ReleaseFuncDesc` | - | 自动管理，无需手动释放 |
| `Release` | `Release` | 释放类型信息对象 |

**设计说明**：
- 所有接口只返回基本类型（char*, uint32_t, int32_t），不返回自定义结构体
- 字符串通过调用者提供的缓冲区返回，避免内存管理问题
- 元数据从服务端通过IPC通信获取，服务端解析tlb.json文件并缓存
- NDK层不暴露任何自定义结构体类型，保持接口简洁

#### 类型映射
| Windows类型 | 鸿蒙类型 | 说明 |
|-------------|-----------|------|
| `DISPID` | `uint32_t` | 完全兼容 |
| `BSTR` | `char*` | Windows为UTF-16，鸿蒙为UTF-8 |
| `VARIANT` | `VARIANT` | 鸿蒙支持类型子集 |
| `HRESULT` | `int32_t` | 错误码映射 |

### 9.2 插件系统设计

#### 插件接口
```cpp
// 插件提供方
class IPlugin {
public:
    virtual std::vector<MethodInfo> GetMethods() = 0;
    virtual int32_t Initialize() = 0;
    virtual void Release() = 0;
};

// Extension加载插件
void LoadPlugins() {
    for (auto& plugin : pluginManager_->LoadPlugins()) {
        for (auto& method : plugin->GetMethods()) {
            uint32_t dispId;
            methodRegistry_->RegisterMethod(method.name, method.func, dispId);
        }
    }
}
```

### 9.3 脚本引擎集成

#### JavaScript集成示例
```cpp
class ScriptEngine {
public:
    void RegisterScriptMethod(const std::string& scriptName,
                           const std::string& scriptCode) {
        auto compiled = CompileScript(scriptCode);
        methodRegistry_->RegisterMethod(scriptName,
            [compiled](const std::vector<VARIANT>& args, VARIANT& result) {
                return ExecuteScript(compiled, args, result);
            },
            dispId
        );
    }
};
```

---

## 十、性能功耗设计

### 10.1 性能优化策略

| 优化项 | 优化方法 | 性能提升 |
|--------|----------|----------|
| 元数据缓存 | JSON解析结果缓存在Proxy端 | 避免重复解析 |
| DispID缓存 | 名称到DispID映射缓存 | 查找性能提升约3倍 |
| DispID索引 | Service端使用unordered_map实现O(1)查找 | 方法查找O(1) |
| Extension Proxy复用 | Extension连接后，Proxy对象复用 | 避免重复连接 |
| 参数池 | 重用VARIANT对象减少内存分配 | 减少内存分配开销 |
| 批量调用 | 支持一次IPC调用多个方法 | 减少IPC次数 |

### 10.2 性能数据

#### DispID vs 方法名性能对比
| 调用方式 | 10000次耗时 | 单次耗时 | 相对性能 |
|---------|------------|---------|---------|
| DispID | ~5ms | ~0.5μs | 3x |
| 方法名 | ~15ms | ~1.5μs | 1x |

#### IPC调用性能
| 操作 | 耗时 | 说明 |
|------|------|------|
| connectExtension | ~20-50ms | 首次连接Extension |
| GetTypeInfo | ~10-50ms | 首次加载JSON元数据 |
| GetIDsOfNames（缓存） | ~0.1μs | 从本地缓存查询 |
| Invoke | ~0.5-2ms | 单次IPC调用 |

### 10.3 功耗设计

#### 功耗优化
1. **Extension按需启动**：无连接时Extension进程休眠
2. **IPC异步调用**：支持异步调用减少等待功耗
3. **内存管理**：及时释放VARIANT参数，避免内存泄漏
4. **连接复用**：Extension连接复用，减少频繁连接的功耗

---

## 十一、数据库设计

本设计不涉及数据库存储，所有元数据通过JSON文件提供，运行时缓存在内存中。

---

## 十二、规格分析

### 12.1 支持的VARIANT类型

| 类型 | 值 | TLB JSON type | 说明 |
|------|-----|---------------|------|
| VT_I8 | 1 | i8 | 8位整数 |
| VT_I32 | 2 | i32 | 32位整数 |
| VT_I64 | 3 | i64 | 64位整数 |
| VT_F64 | 4 | f64 | 浮点数 |
| VT_STRING | 5 | String | 字符串（UTF-8） |
| VT_VOID | 6 | void | 空类型 |
| VT_ENUM | 7 | enum | 枚举类型，需要指定idl_type |
| VT_INTERFACE | 8 | interface | 接口类型，需要指定idl_type |
| VT_STRUCT | 9 | struct | 结构体类型，需要指定idl_type |

**说明**：
- 支持的TLB JSON type值：i8、i32、i64、f64、String、void、enum、interface、struct
- 当type为"interface"、"enum"或"struct"时，需要指定idl_type字段为具体的类型名称
- 字符串使用UTF-8编码

### 12.2 错误码定义

| 错误码 | 值 | 说明 |
|--------|-----|------|
| IDISPATCH_OK | 0 | 成功 |
| IDISPATCH_ERR_INVALID_PARAM | -1 | 参数无效 |
| IDISPATCH_ERR_METHOD_NOT_FOUND | -2 | 方法未找到 |
| IDISPATCH_ERR_TYPE_MISMATCH | -3 | 类型不匹配 |
| IDISPATCH_ERR_IPC_FAILED | -4 | IPC通信失败 |
| IDISPATCH_ERR_OUT_OF_MEMORY | -5 | 内存不足 |
| IDISPATCH_ERR_PERMISSION_DENIED | -6 | 权限不足 |

### 12.3 规格限制

| 限制项 | 限制值 | 说明 |
|--------|--------|------|
| 方法名长度 | 256字节 | 最大方法名长度 |
| 参数个数 | 32 | 单次调用最大参数个数 |
| 数组元素个数 | 10000 | 单个数组最大元素个数 |
| VARIANT大小 | 1MB | 单个VARIANT最大大小 |
| 字符串长度 | 1MB | 单个字符串最大长度 |

---

## 十三、DFX分析

### 13.1 日志设计

#### 日志级别
| 级别 | 用途 | 示例 |
|------|------|------|
| DEBUG | 调试信息 | 参数编解码详情 |
| INFO | 关键操作 | Extension连接、方法调用 |
| WARN | 警告信息 | 元数据解析警告 |
| ERROR | 错误信息 | IPC失败、方法未找到 |

#### 日志示例
```cpp
// Extension连接
LOGI("IDispatch: ConnectExtension success, bundleName=%s, abilityName=%s",
      bundleName.c_str(), abilityName.c_str());

// 方法调用
LOGI("IDispatch: Invoke method, dispId=%u, methodName=%s, cArgs=%u",
      dispId, memberName, cArgs);

// 错误信息
LOGE("IDispatch: IPC failed, error=%d", err);
```

### 13.2 测试策略

#### 单元测试
- IDispatchProxy测试：GetIDsOfNames、Invoke、GetTypeInfo
- ParameterMarshaller测试：各种VARIANT类型的编解码
- JsonMetadataParser测试：JSON解析和查询
- MethodRegistry测试：方法注册和查找

#### 集成测试
- Extension连接测试
- IPC通信测试
- 端到端调用测试

#### 模糊测试
- 参数边界测试
- 异常输入测试
- 并发调用测试

### 13.3 调试支持

#### 调试接口
```cpp
// 启用调试模式
void SetDebugMode(bool enable);

// 获取统计信息
struct Statistics {
    uint32_t totalInvokeCount;
    uint32_t cacheHitCount;
    uint32_t cacheMissCount;
    uint64_t totalInvokeTime;
};
Statistics GetStatistics();
```

---

## 十四、其他非功能分析

### 14.1 安全设计

#### 权限控制
```cpp
// Service端权限检查
int32_t IDispatchService::CheckPermission(uint32_t dispId) {
    uint32_t callingUid = IPCSkeleton::GetCallingUid();
    uint32_t callingPid = IPCSkeleton::GetCallingPid();

    // 检查调用者是否有权限调用该方法
    if (!PermissionChecker::Check(callingUid, callingPid, dispId)) {
        return IDISPATCH_ERR_PERMISSION_DENIED;
    }
    return IDISPATCH_OK;
}
```

#### 输入验证
- 参数类型检查
- 数组边界边界检查
- 字符串长度限制
- 递归深度限制（避免栈溢出）

#### 资源管理
- VARIANT内存自动释放
- IPC超时控制
- 对象生命周期管理

### 14.2 可维护性设计

#### 代码组织
- 清晰的模块划分（NDK、Proxy、Extension）
- 统一的错误处理
- 详细的代码注释

#### 配置管理
- JSON元数据文件
- Extension bundle.json配置
- 可配置的参数（超时、缓存大小等）

### 14.3 可扩展性设计

#### 扩展点
1. **自定义VARIANT类型**：支持扩展新的VARIANT类型
2. **自定义元数据格式**：支持元数据解析器插件
3. **自定义IPC传输**：支持自定义参数编解码器

#### 扩展示例
```cpp
// 自定义VARIANT类型编解码器
class CustomMarshaller : public ParameterMarshaller {
public:
    int32_t MarshalCustomType(const CustomType& value, MessageParcel& data) override;
    int32_t UnmarshalCustomType(MessageParcel& data, CustomType& value) override;
};
```

---

## 附录

### A. Windows IDispatch头文件参考
- oaidl.h：Windows IDispatch接口定义

### B. 鸿蒙IDispatch头文件
- idispatch_ndk.h：鸿蒙NDK接口定义
- idispatch_ndk_full.h：完整兼容版本（包含所有Windows类型）

### C. 参考资料
- Windows COM IDispatch规范
- 鸿蒙Extension开发指南
- 鸿蒙IPC通信文档
