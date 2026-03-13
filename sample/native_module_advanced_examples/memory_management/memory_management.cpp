/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "hilog/log.h"

namespace {
constexpr int32_t HILOG_MODULE_ID = 0x0001;
constexpr uint32_t MODULE_VERSION = 1;
constexpr uint32_t NO_MODULE_FLAGS = 0;
constexpr size_t DEFAULT_BUFFER_SIZE = 1024;
constexpr size_t MAX_BUFFER_SIZE = 10 * 1024 * 1024;
constexpr uint32_t DEFAULT_ALIGNMENT = 16;

void LogInfo(const std::string& message)
{
    HiLogPrint(LOG_CORE, LOG_INFO, HILOG_MODULE_ID, "MemoryManagement", "%{public}s", message.c_str());
}

void LogError(const std::string& message)
{
    HiLogPrint(LOG_CORE, LOG_ERROR, HILOG_MODULE_ID, "MemoryManagement", "%{public}s", message.c_str());
}

struct BufferData {
    uint8_t* data;
    size_t size;
    size_t capacity;
    bool isExternal;
};

struct NativeObject {
    std::string name;
    int32_t value;
    std::vector<int32_t> array;
    std::shared_ptr<BufferData> buffer;
    size_t refCount;
};

std::vector<std::shared_ptr<NativeObject>> g_nativeObjects;

void BufferDataFinalizer(napi_env env, void* data, void* hint)
{
    (void)env;
    (void)hint;
    auto* bufferData = static_cast<BufferData*>(data);
    if (bufferData != nullptr) {
        if (bufferData->data != nullptr && bufferData->isExternal) {
            delete[] bufferData->data;
        }
        delete bufferData;
        LogInfo("Buffer data finalized");
    }
}

void NativeObjectFinalizer(napi_env env, void* data, void* hint)
{
    (void)env;
    (void)hint;
    auto* nativeObj = static_cast<NativeObject*>(data);
    if (nativeObj != nullptr) {
        LogInfo("Native object finalized: " + nativeObj->name);
        delete nativeObj;
    }
}

napi_value CreateExternalBuffer(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    size_t bufferSize = DEFAULT_BUFFER_SIZE;
    if (argc > 0) {
        napi_valuetype type = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[0], &type));
        if (type == napi_number) {
            double sizeValue = 0;
            NAPI_CALL(env, napi_get_value_double(env, argv[0], &sizeValue));
            bufferSize = static_cast<size_t>(sizeValue);
        }
    }

    if (bufferSize == 0 || bufferSize > MAX_BUFFER_SIZE) {
        napi_throw_error(env, nullptr, "Invalid buffer size");
        return nullptr;
    }

    auto* bufferData = new BufferData();
    bufferData->size = bufferSize;
    bufferData->capacity = bufferSize;
    bufferData->isExternal = true;
    bufferData->data = new uint8_t[bufferSize]();

    std::memset(bufferData->data, 0xAA, bufferSize);

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_external_arraybuffer(env, bufferData->data, bufferSize,
        BufferDataFinalizer, bufferData, &result));

    LogInfo("Created external buffer of size: " + std::to_string(bufferSize));
    return result;
}

napi_value CreateWrappedObject(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    auto* nativeObj = new NativeObject();
    nativeObj->name = "WrappedObject";
    nativeObj->value = 0;
    nativeObj->refCount = 1;

    if (argc > 0) {
        napi_valuetype type = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[0], &type));
        if (type == napi_string) {
            size_t strLength = 0;
            napi_get_value_string_utf8(env, argv[0], nullptr, 0, &strLength);
            if (strLength > 0) {
                std::vector<char> buffer(strLength + 1);
                napi_get_value_string_utf8(env, argv[0], buffer.data(), buffer.size(), &strLength);
                nativeObj->name = std::string(buffer.data(), strLength);
            }
        }
    }

    if (argc > 1) {
        napi_valuetype type = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &type));
        if (type == napi_number) {
            NAPI_CALL(env, napi_get_value_int32(env, argv[1], &nativeObj->value));
        }
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value nameValue = nullptr;
    NAPI_CALL(env, napi_create_string_utf8(env, nativeObj->name.c_str(), NAPI_AUTO_LENGTH, &nameValue));
    NAPI_CALL(env, napi_set_named_property(env, result, "name", nameValue));

    napi_value valueValue = nullptr;
    NAPI_CALL(env, napi_create_int32(env, nativeObj->value, &valueValue));
    NAPI_CALL(env, napi_set_named_property(env, result, "value", valueValue));

    napi_ref weakRef = nullptr;
    NAPI_CALL(env, napi_wrap(env, result, nativeObj, NativeObjectFinalizer, nullptr, &weakRef));

    LogInfo("Created wrapped object: " + nativeObj->name);
    return result;
}

napi_value CreateTypedArray(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    size_t arrayLength = DEFAULT_BUFFER_SIZE;
    napi_typedarray_type arrayType = napi_int8_array;

    if (argc > 0) {
        napi_valuetype type = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[0], &type));
        if (type == napi_number) {
            double lengthValue = 0;
            NAPI_CALL(env, napi_get_value_double(env, argv[0], &lengthValue));
            arrayLength = static_cast<size_t>(lengthValue);
        }
    }

    if (argc > 1) {
        napi_valuetype type = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &type));
        if (type == napi_number) {
            int32_t typeValue = 0;
            NAPI_CALL(env, napi_get_value_int32(env, argv[1], &typeValue));
            if (typeValue >= 0 && typeValue <= napi_float64_array) {
                arrayType = static_cast<napi_typedarray_type>(typeValue);
            }
        }
    }

    size_t elementSize = 1;
    switch (arrayType) {
        case napi_int8_array:
        case napi_uint8_array:
            elementSize = 1;
            break;
        case napi_int16_array:
        case napi_uint16_array:
            elementSize = 2;
            break;
        case napi_int32_array:
        case napi_uint32_array:
        case napi_float32_array:
            elementSize = 4;
            break;
        case napi_float64_array:
            elementSize = 8;
            break;
        default:
            elementSize = 1;
            break;
    }

    size_t byteLength = arrayLength * elementSize;
    void* data = nullptr;
    napi_value arrayBuffer = nullptr;
    NAPI_CALL(env, napi_create_arraybuffer(env, byteLength, &data, &arrayBuffer));

    std::memset(data, 0, byteLength);

    napi_value typedArray = nullptr;
    NAPI_CALL(env, napi_create_typedarray(env, arrayType, arrayLength, arrayBuffer, 0, &typedArray));

    LogInfo("Created typed array with " + std::to_string(arrayLength) + " elements");
    return typedArray;
}

napi_value CreateSharedBuffer(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    size_t bufferSize = DEFAULT_BUFFER_SIZE;
    if (argc > 0) {
        napi_valuetype type = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[0], &type));
        if (type == napi_number) {
            double sizeValue = 0;
            NAPI_CALL(env, napi_get_value_double(env, argv[0], &sizeValue));
            bufferSize = static_cast<size_t>(sizeValue);
        }
    }

    if (bufferSize == 0 || bufferSize > MAX_BUFFER_SIZE) {
        napi_throw_error(env, nullptr, "Invalid buffer size");
        return nullptr;
    }

    auto sharedBuffer = std::make_shared<BufferData>();
    sharedBuffer->size = bufferSize;
    sharedBuffer->capacity = bufferSize;
    sharedBuffer->isExternal = true;
    sharedBuffer->data = new uint8_t[bufferSize]();

    std::memset(sharedBuffer->data, 0xBB, bufferSize);

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_external_arraybuffer(env, sharedBuffer->data, bufferSize,
        [](napi_env env, void* data, void* hint) {
            (void)env;
            (void)hint;
            auto* bufferData = static_cast<BufferData*>(data);
            if (bufferData != nullptr) {
                if (bufferData->data != nullptr && bufferData->isExternal) {
                    delete[] bufferData->data;
                }
                delete bufferData;
            }
        }, sharedBuffer.get(), &result));

    LogInfo("Created shared buffer of size: " + std::to_string(bufferSize));
    return result;
}

napi_value AdjustExternalMemory(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value argv[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    NAPI_ASSERT(env, argc >= 1, "Requires 1 argument: changeInBytes");

    napi_valuetype type = napi_undefined;
    NAPI_CALL(env, napi_typeof(env, argv[0], &type));
    NAPI_ASSERT(env, type == napi_number, "changeInBytes must be a number");

    int64_t changeInBytes = 0;
    NAPI_CALL(env, napi_get_value_int64(env, argv[0], &changeInBytes));

    int64_t adjustedValue = 0;
    napi_status status = napi_adjust_external_memory(env, changeInBytes, &adjustedValue);

    if (status != napi_ok) {
        napi_throw_error(env, nullptr, "Failed to adjust external memory");
        return nullptr;
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_int64(env, adjustedValue, &result));

    LogInfo("Adjusted external memory by: " + std::to_string(changeInBytes));
    return result;
}

napi_value CreateMemoryPool(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    size_t blockSize = DEFAULT_BUFFER_SIZE;
    size_t blockCount = 10;

    if (argc > 0) {
        napi_valuetype type = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[0], &type));
        if (type == napi_number) {
            double sizeValue = 0;
            NAPI_CALL(env, napi_get_value_double(env, argv[0], &sizeValue));
            blockSize = static_cast<size_t>(sizeValue);
        }
    }

    if (argc > 1) {
        napi_valuetype type = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &type));
        if (type == napi_number) {
            double countValue = 0;
            NAPI_CALL(env, napi_get_value_double(env, argv[1], &countValue));
            blockCount = static_cast<size_t>(countValue);
        }
    }

    struct MemoryPool {
        std::vector<std::unique_ptr<uint8_t[]>> blocks;
        size_t blockSize;
        size_t blockCount;
    };

    auto* pool = new MemoryPool();
    pool->blockSize = blockSize;
    pool->blockCount = blockCount;

    for (size_t i = 0; i < blockCount; i++) {
        pool->blocks.emplace_back(std::make_unique<uint8_t[]>(blockSize));
        std::memset(pool->blocks[i].get(), 0xCC, blockSize);
    }

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value blockSizeValue = nullptr;
    NAPI_CALL(env, napi_create_uint64(env, blockSize, &blockSizeValue));
    NAPI_CALL(env, napi_set_named_property(env, result, "blockSize", blockSizeValue));

    napi_value blockCountValue = nullptr;
    NAPI_CALL(env, napi_create_uint64(env, blockCount, &blockCountValue));
    NAPI_CALL(env, napi_set_named_property(env, result, "blockCount", blockCountValue));

    napi_value totalSizeValue = nullptr;
    NAPI_CALL(env, napi_create_uint64(env, blockSize * blockCount, &totalSizeValue));
    NAPI_CALL(env, napi_set_named_property(env, result, "totalSize", totalSizeValue));

    napi_ref weakRef = nullptr;
    NAPI_CALL(env, napi_wrap(env, result, pool, [](napi_env env, void* data, void* hint) {
        (void)env;
        (void)hint;
        auto* memoryPool = static_cast<MemoryPool*>(data);
        if (memoryPool != nullptr) {
            delete memoryPool;
            LogInfo("Memory pool finalized");
        }
    }, nullptr, &weakRef));

    LogInfo("Created memory pool with " + std::to_string(blockCount) + " blocks");
    return result;
}

napi_value CreateAlignedBuffer(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value argv[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, argv, nullptr, nullptr));

    size_t bufferSize = DEFAULT_BUFFER_SIZE;
    uint32_t alignment = DEFAULT_ALIGNMENT;

    if (argc > 0) {
        napi_valuetype type = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[0], &type));
        if (type == napi_number) {
            double sizeValue = 0;
            NAPI_CALL(env, napi_get_value_double(env, argv[0], &sizeValue));
            bufferSize = static_cast<size_t>(sizeValue);
        }
    }

    if (argc > 1) {
        napi_valuetype type = napi_undefined;
        NAPI_CALL(env, napi_typeof(env, argv[1], &type));
        if (type == napi_number) {
            double alignmentValue = 0;
            NAPI_CALL(env, napi_get_value_double(env, argv[1], &alignmentValue));
            alignment = static_cast<uint32_t>(alignmentValue);
        }
    }

    if (bufferSize == 0 || bufferSize > MAX_BUFFER_SIZE) {
        napi_throw_error(env, nullptr, "Invalid buffer size");
        return nullptr;
    }

    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        napi_throw_error(env, nullptr, "Alignment must be a power of 2");
        return nullptr;
    }

    size_t totalSize = bufferSize + alignment - 1;
    auto* rawBuffer = new uint8_t[totalSize];
    uintptr_t alignedAddress = reinterpret_cast<uintptr_t>(rawBuffer);
    uintptr_t alignedPtr = (alignedAddress + alignment - 1) & ~(alignment - 1);
    uint8_t* alignedBuffer = reinterpret_cast<uint8_t*>(alignedPtr);

    std::memset(alignedBuffer, 0xDD, bufferSize);

    struct AlignedBufferData {
        uint8_t* rawBuffer;
        uint8_t* alignedBuffer;
        size_t size;
    };

    auto* alignedData = new AlignedBufferData();
    alignedData->rawBuffer = rawBuffer;
    alignedData->alignedBuffer = alignedBuffer;
    alignedData->size = bufferSize;

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_external_arraybuffer(env, alignedBuffer, bufferSize,
        [](napi_env env, void* data, void* hint) {
            (void)env;
            (void)hint;
            auto* alignedBufferData = static_cast<AlignedBufferData*>(data);
            if (alignedBufferData != nullptr) {
                if (alignedBufferData->rawBuffer != nullptr) {
                    delete[] alignedBufferData->rawBuffer;
                }
                delete alignedBufferData;
                LogInfo("Aligned buffer finalized");
            }
        }, alignedData, &result));

    LogInfo("Created aligned buffer of size: " + std::to_string(bufferSize) + " with alignment: " + std::to_string(alignment));
    return result;
}

napi_value GetMemoryInfo(napi_env env, napi_callback_info info)
{
    (void)info;

    napi_value result = nullptr;
    NAPI_CALL(env, napi_create_object(env, &result));

    napi_value objectCount = nullptr;
    NAPI_CALL(env, napi_create_uint32(env, g_nativeObjects.size(), &objectCount));
    NAPI_CALL(env, napi_set_named_property(env, result, "nativeObjectCount", objectCount));

    size_t totalMemory = 0;
    for (const auto& obj : g_nativeObjects) {
        if (obj->buffer) {
            totalMemory += obj->buffer->capacity;
        }
    }

    napi_value totalMemoryValue = nullptr;
    NAPI_CALL(env, napi_create_uint64(env, totalMemory, &totalMemoryValue));
    NAPI_CALL(env, napi_set_named_property(env, result, "totalBufferMemory", totalMemoryValue));

    return result;
}

}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("createExternalBuffer", CreateExternalBuffer),
        DECLARE_NAPI_FUNCTION("createWrappedObject", CreateWrappedObject),
        DECLARE_NAPI_FUNCTION("createTypedArray", CreateTypedArray),
        DECLARE_NAPI_FUNCTION("createSharedBuffer", CreateSharedBuffer),
        DECLARE_NAPI_FUNCTION("adjustExternalMemory", AdjustExternalMemory),
        DECLARE_NAPI_FUNCTION("createMemoryPool", CreateMemoryPool),
        DECLARE_NAPI_FUNCTION("createAlignedBuffer", CreateAlignedBuffer),
        DECLARE_NAPI_FUNCTION("getMemoryInfo", GetMemoryInfo),
    };
    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

static napi_module g_memoryManagementModule = {
    .nm_version = MODULE_VERSION,
    .nm_flags = NO_MODULE_FLAGS,
    .nm_filename = nullptr,
    .nm_register_func = Init,
    .nm_modname = "memory_management",
    .nm_priv = nullptr,
};

extern "C" __attribute__((constructor)) void RegisterMemoryManagementModule(void)
{
    napi_module_register(&g_memoryManagementModule);
}
