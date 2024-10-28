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

#include "native_context_manager.h"

#include "native_engine/native_engine.h"
#include "utils/log.h"

NativeContextManager::NativeContextManager()
{
    // create two empty nodes to skip cmp when add
    first_ = new NativeContextNode(nullptr, nullptr, this);
    if (first_ == nullptr) {
        HILOG_FATAL("Failed to init NativeContextManager");
    }
    last_ = first_;
}

bool NativeContextManager::RegisterNode(NativeContextNode* current)
{
    if (current->GetCallback() == nullptr) {
        HILOG_ERROR("callback of node register into context manager cannot be empty");
        return false;
    }

    if (current->GetManager() != nullptr) {
        return false;
    }

    if (current->GetPrev() != nullptr && current->GetNext() != nullptr) {
        return false;
    }

    last_->SetNext(current);
    current->SetPrev(last_);

    return true;
}

bool NativeContextManager::UnregisterNode(NativeContextNode* current)
{
    if (current->GetManager() != this) {
        HILOG_ERROR("current is not registered in this context manager");
        return false;
    }

    NativeContextNode* prev = current->GetPrev();
    NativeContextNode* next = current->GetNext();

    // first element is protected
    if (prev == nullptr) {
        return false;
    }
    prev->SetNext(next);

    if (next != nullptr) {
        next->SetPrev(prev);
    }

    if (current == last_) {
        last_ = prev;
    }

    return true;
}

NativeContext::NativeContext(NativeEngine* engine, NativeContextFinalize callback, void* data)
{
    if (callback == nullptr) {
        return;
    }
    NativeContextManager* contextManager = engine->GetContextManager();
    if (contextManager != nullptr) {
        node_ = new NativeContextNode(callback, data, contextManager);
        if (node_ != nullptr && !contextManager->RegisterNode(node_)) {
            delete node_;
            node_ = nullptr;
        };
    }
};

NativeContextNode::~NativeContextNode()
{
    if (manager_ != nullptr) {
        manager_->UnregisterNode(this);
    }

    data_ = nullptr;
    callback_ = nullptr;

    manager_ = nullptr;
    prev_ = nullptr;
    next_ = nullptr;
}

NativeContextManager::~NativeContextManager()
{
    for (NativeContextNode* current = last_; current != first_; current = last_) {
        UnregisterNode(last_);
        current->TriggerCallback();
    };

    delete first_;
}
