/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "native_reference_manager.h"

struct NativeReferenceHandler {
    NativeReference* reference = nullptr;
    NativeReferenceHandler* next = nullptr;
    NativeReferenceHandler* prev = nullptr;
};

NativeReferenceManager::NativeReferenceManager() : referenceHandlers_(nullptr) {}

NativeReferenceManager::~NativeReferenceManager()
{
    for (auto handler = referenceHandlers_; handler != nullptr; handler = referenceHandlers_) {
        delete handler->reference;
    }
}

NativeReferenceHandler* NativeReferenceManager::CreateHandler(NativeReference* reference)
{
    if (reference == nullptr) {
        return nullptr;
    }
    NativeReferenceHandler* temp = new NativeReferenceHandler;
    temp->reference = reference;
    temp->next = referenceHandlers_;
    if (referenceHandlers_) {
        referenceHandlers_->prev = temp;
    }
    referenceHandlers_ = temp;
    return temp;
}

void NativeReferenceManager::ReleaseHandler(NativeReferenceHandler* handler)
{
    if (handler == nullptr) {
        return;
    }
    NativeReferenceHandler* prev = handler->prev;
    NativeReferenceHandler* next = handler->next;
    if (prev) {
        prev->next = next;
    } else {
        referenceHandlers_ = next;
    }
    if (next) {
        next->prev = prev;
    }
    delete handler;
}
