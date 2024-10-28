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

#include "native_engine/impl/ark/ark_native_reference.h"

NativeReferenceManager::~NativeReferenceManager()
{
    for (auto handler = head_; handler != nullptr; handler = head_) {
        head_ = handler->next_;
        handler->FinalizeCallback(FinalizerState::ENGINE_TEARDOWN);
    }
}

void NativeReferenceManager::CreateHandler(ArkNativeReference* reference)
{
    if (head_) {
        head_->prev_ = reference;
        reference->next_ = head_;
    }
    head_ = reference;
}

void NativeReferenceManager::ReleaseHandler(ArkNativeReference* reference)
{
    ArkNativeReference* prev = reference->prev_;
    ArkNativeReference* next = reference->next_;

    if (prev) {
        prev->next_ = next;
    } else {
        // reference is the head node.
        head_ = next;
    }
    if (next) {
        next->prev_ = prev;
    }
}
