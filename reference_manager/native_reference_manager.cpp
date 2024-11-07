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
    for (auto handler = references_; handler != nullptr; handler = references_) {
        references_ = reinterpret_cast<ArkNativeReference*>(handler)->next_;
        references_->FinalizeCallback(FinalizerState::ENV_TEARDOWN);
    }
}

void NativeReferenceManager::CreateHandler(ArkNativeReference* reference)
{
    if (references_) {
        references_->prev_ = reference;
        reference->next_ = references_;
    }
    references_ = reference;
}

void NativeReferenceManager::ReleaseHandler(ArkNativeReference* reference)
{
    ArkNativeReference* prev =reference->prev_;
    ArkNativeReference* next =reference->next_;

    if (prev) {
        prev->next_ = next;
    }
    if (reference == references_) {
        // reference is the head node.
        references_ = next;
    }
    if (next) {
        next->prev_ = prev;
    }
}
