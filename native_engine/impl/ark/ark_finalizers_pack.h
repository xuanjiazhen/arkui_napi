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

#ifndef FOUNDATION_ACE_NAPI_NATIVE_ENGINE_ARK_FINALIZERS_PACK_H
#define FOUNDATION_ACE_NAPI_NATIVE_ENGINE_ARK_FINALIZERS_PACK_H

#include <cstdint>
#include "interfaces/inner_api/napi/native_node_api.h"
#include "securec.h"

#define NAPI_CALLBACK_TAG 'n'

class NativeEngine;

using RefFinalizer = std::pair<NapiNativeFinalize, std::tuple<NativeEngine*, void*, void*>>;
using ArkFinalizersPackFinishNotify = std::function<void(size_t totalNativeBindingSize)>;

class MarkedCallback {
public:
    MarkedCallback(NapiNativeFinalize callback, napi_env env, void* data, void* hint)
        : env_(env), callback_(callback), data_(data), hint_(hint)
    {
        memset_s(&tagStart_, sizeof(uintptr_t), NAPI_CALLBACK_TAG, sizeof(uintptr_t));
        markedCallback_ = tagStart_ | reinterpret_cast<uintptr_t>(callback);
        memset_s(&tagEnd_, sizeof(uintptr_t), NAPI_CALLBACK_TAG, sizeof(uintptr_t));
    }
    __attribute__((optnone)) ~MarkedCallback()
    {
        callback_(env_, data_, hint_);
    }

private:
    napi_env env_ { nullptr };
    uintptr_t tagStart_ { 0 };
    NapiNativeFinalize callback_ { nullptr };
    uintptr_t markedCallback_ { 0 };
    uintptr_t tagEnd_ { 0 };
    void* data_ { nullptr };
    void* hint_ { nullptr };
};

class ArkFinalizersPack {
public:
    ArkFinalizersPack() = default;
    ~ArkFinalizersPack() = default;
    DEFAULT_MOVE_SEMANTIC(ArkFinalizersPack);
    DEFAULT_COPY_SEMANTIC(ArkFinalizersPack);

    void Clear()
    {
        finalizers_.clear();
        totalNativeBindingSize_ = 0;
        notify_ = nullptr;
    }
    bool Empty() const
    {
        return finalizers_.empty();
    }
    void RegisterFinishNotify(ArkFinalizersPackFinishNotify notify)
    {
        notify_ = notify;
    }
    size_t GetNumFinalizers() const
    {
        return finalizers_.size();
    }
    __attribute__((optnone)) void ProcessAll() const
    {
        for (auto &iter : finalizers_) {
            auto &[p0, p1, p2] = iter.second;
            MarkedCallback(iter.first, reinterpret_cast<napi_env>(p0), p1, p2);
        }
        NotifyFinish();
    }
    size_t GetTotalNativeBindingSize() const
    {
        return totalNativeBindingSize_;
    }
    void AddFinalizer(RefFinalizer &finalizer, size_t nativeBindingSize)
    {
        finalizers_.emplace_back(finalizer);
        totalNativeBindingSize_ += nativeBindingSize;
    }
private:
    void NotifyFinish() const
    {
        if (notify_ != nullptr) {
            notify_(totalNativeBindingSize_);
        }
    }
    std::vector<RefFinalizer> finalizers_ {};
    size_t totalNativeBindingSize_ {0};
    ArkFinalizersPackFinishNotify notify_ {nullptr};
};

#endif /* FOUNDATION_ACE_NAPI_NATIVE_ENGINE_ARK_FINALIZERS_PACK_H */
