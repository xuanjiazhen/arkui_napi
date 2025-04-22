/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <memory>

#include "native_engine/native_engine.h"
#include "native_engine/impl/ark/ark_native_finalizer.h"
#include "utils/log.h"

void ArkNativeFinalizer::TestFinalizeCallback(void*, void* data, void* finalize_hint)
{
    if (finalize_hint == nullptr) {
        return;
    }
    std::unique_ptr<ArkNativeFinalizer, ArkNativeFinalizer::Deleter> nativeFinalizer(
        reinterpret_cast<ArkNativeFinalizer*>(finalize_hint));
    nativeFinalizer->RemoveArkNativeFinalizer();
    nativeFinalizer->TriggerFinalizer();
}

void ArkNativeFinalizer::InsertArkNativeFinalizer()
{
    auto engine = reinterpret_cast<NativeEngine*>(this->env_);
    if (engine == nullptr) {
        return;
    }
    ArkNativeFinalizer* listHead = engine->GetArkNativeFinalizerListHead();
    if (listHead == nullptr) {
        // this is the list head node
        engine->SetArkNativeFinalizerListHead(this);
        return;
    }
    listHead->prev_ = this;
    next_ = listHead;
    listHead = this;
    engine->SetArkNativeFinalizerListHead(listHead);
}

void ArkNativeFinalizer::RemoveArkNativeFinalizer()
{
    if(isTriggered_) {
        // finalizer callback has been triggered, do nothing.
        return;
    }
    if (this->prev_) {
        this->prev_->next_ = this->next_;
    }

    if (this->next_) {
        this->next_->prev_ = this->prev_;
    }

    auto engine = reinterpret_cast<NativeEngine*>(this->env_);
    if (!engine) {
        return;
    }
    ArkNativeFinalizer* listHead = engine->GetArkNativeFinalizerListHead();
    if (listHead == this) {
        if (this->next_) {
            this->next_->prev_ = nullptr;
        }
        engine->SetArkNativeFinalizerListHead(this->next_);
    }
}

void ArkNativeFinalizer::TriggerAllArkNativeFinalizer()
{
    auto startNode = this;
    while (startNode != nullptr) {
        startNode->TriggerFinalizer();
        startNode = startNode->next_;
    }
}
