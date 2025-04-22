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

#ifndef FOUNDATION_ACE_NAPI_NATIVE_ENGINE_IMPL_ARK_ARK_NATIVE_FINALIZER_H
#define FOUNDATION_ACE_NAPI_NATIVE_ENGINE_IMPL_ARK_ARK_NATIVE_FINALIZER_H

#include <iostream>
#include <unordered_map>

#include "interfaces/inner_api/napi/native_node_api.h"

class NativeEngine;

class ArkNativeFinalizer {
public:
    ArkNativeFinalizer(NativeEngine* engine, napi_finalize finalize_cb, void* data, void* finalize_hint)
        : env_(reinterpret_cast<void*>(engine)), finalizeCb_(finalize_cb), data_(data), finalizeHint_(finalize_hint),
          isTriggered_(false), prev_(nullptr), next_(nullptr)
    {
        // insert ark native finalizer into ark native finalizer list on ark native engine
        InsertArkNativeFinalizer();
    }
    ~ArkNativeFinalizer() = default;

    struct Deleter {
        void operator()(ArkNativeFinalizer* nativeFinalizer)
        {
            delete nativeFinalizer;
        }
    };

    static void TestFinalizeCallback(void*, void* data, void* finalize_hint);

    void TriggerAllArkNativeFinalizer();

    void RemoveArkNativeFinalizer();

    inline void TriggerFinalizer()
    {
        if (isTriggered_) {
            // finalizer callback has been triggered, do nothing.
            return;
        }
        finalizeCb_(reinterpret_cast<napi_env>(env_), data_, finalizeHint_);
        isTriggered_ = true;
    }

private:
    void InsertArkNativeFinalizer();

    void* env_;
    napi_finalize finalizeCb_;
    void* data_;
    void* finalizeHint_;
    bool isTriggered_;

    ArkNativeFinalizer* prev_;
    ArkNativeFinalizer* next_;
};
#endif // FOUNDATION_ACE_NAPI_NATIVE_ENGINE_IMPL_ARK_ARK_NATIVE_FINALIZER_H