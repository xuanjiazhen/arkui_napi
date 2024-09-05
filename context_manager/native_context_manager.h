/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ARKUI_NAPI_CONTEXT_MANAGER_NATIVE_CONTEXT_MANAGER_H
#define ARKUI_NAPI_CONTEXT_MANAGER_NATIVE_CONTEXT_MANAGER_H

#include "native_engine/native_value.h"

typedef void (*NativeContextFinalize)(void* data);

class NativeContextManager;

class NativeContextNode {
public:
    NativeContextNode(NativeContextFinalize callback, void* data, NativeContextManager* manager)
        : manager_(manager), callback_(callback), data_(data) {};

    ~NativeContextNode();

    inline NativeContextFinalize GetCallback() const
    {
        return callback_;
    }

    inline void TriggerCallback()
    {
        if (callback_ != nullptr) {
            callback_(const_cast<void*>(data_));
        }
    }

    inline void SetPrev(NativeContextNode* prev)
    {
        prev_ = prev;
    }

    inline NativeContextNode* GetPrev() const
    {
        return prev_;
    }

    inline void SetNext(NativeContextNode* next)
    {
        next_ = next;
    }

    inline NativeContextNode* GetNext() const
    {
        return next_;
    }

    inline void SetManager(NativeContextManager* manager)
    {
        manager_ = manager;
    }

    inline NativeContextManager* GetManager() const
    {
        return manager_;
    }

private:
    NativeContextManager* manager_ {nullptr};

    NativeContextFinalize callback_ { nullptr};
    void* data_ = nullptr;

    NativeContextNode* prev_ = nullptr;
    NativeContextNode* next_ = nullptr;
};

class NativeContext {
public:
    NativeContext(NativeEngine* engine, NativeContextFinalize callback, void* data);
    ~NativeContext()
    {
        if (node_ != nullptr) {
            delete node_;
            node_ = nullptr;
        }
    }

    template<typename T>
    static void ContextFinalizeCallback(void* data)
    {
        delete reinterpret_cast<T*>(data);
    }

private:
    NativeContextNode* node_ { nullptr };
};

class NativeContextManager {
public:
    NativeContextManager();
    ~NativeContextManager();

    bool RegisterNode(NativeContextNode* current);
    bool UnregisterNode(NativeContextNode* current);

private:
    NativeContextNode* first_ = nullptr;
    NativeContextNode* last_ = nullptr;
};

#endif /* ARKUI_NAPI_CONTEXT_MANAGER_NATIVE_CONTEXT_MANAGER_H */
