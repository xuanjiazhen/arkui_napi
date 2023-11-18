/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "native_reference.h"

#include "native_engine.h"
#include "native_engine/native_utils.h"
#include "utils/log.h"

NativeReference::NativeReference(NativeEngine* engine,
                                       napi_value value,
                                       uint32_t initialRefcount,
                                       bool deleteSelf,
                                       NapiNativeFinalize napiCallback,
                                       void* data,
                                       void* hint)
    : NativeReference::NativeReference(engine, LocalValueFromJsValue(value), initialRefcount, deleteSelf,
                                             napiCallback, data, hint)
{}

NativeReference::NativeReference(NativeEngine* engine,
                                       Local<JSValueRef> value,
                                       uint32_t initialRefcount,
                                       bool deleteSelf,
                                       NapiNativeFinalize napiCallback,
                                       void* data,
                                       void* hint)
    : engine_(engine),
      value_(Global<JSValueRef>(engine->GetEcmaVm(), JSValueRef::Undefined(engine->GetEcmaVm()))),
      refCount_(initialRefcount),
      deleteSelf_(deleteSelf),
      napiCallback_(napiCallback),
      data_(data),
      hint_(hint)
{
    auto vm = engine->GetEcmaVm();
    LocalScope scope(vm);
    Global<JSValueRef> newValue(vm, value);
    value_ = newValue;
    if (initialRefcount == 0) {
        value_.SetWeakCallback(reinterpret_cast<void*>(this), FreeGlobalCallBack, NativeFinalizeCallBack);
    }

    if (deleteSelf) {
        NativeReferenceManager* referenceManager = engine->GetReferenceManager();
        if (referenceManager != nullptr) {
            referenceManager->CreateHandler(this);
        }
    }
}

NativeReference::~NativeReference()
{
    if (deleteSelf_ && engine_->GetReferenceManager()) {
        engine_->GetReferenceManager()->ReleaseHandler(this);
    }
    if (value_.IsEmpty()) {
        return;
    }
    hasDelete_ = true;
    value_.FreeGlobalHandleAddr();
    FinalizeCallback();
}

uint32_t NativeReference::Ref()
{
    ++refCount_;
    if (refCount_ == 1) {
        value_.ClearWeak();
    }
    return refCount_;
}

uint32_t NativeReference::Unref()
{
    if (refCount_ == 0) {
        return refCount_;
    }
    --refCount_;
    if (value_.IsEmpty()) {
        return refCount_;
    }
    if (refCount_ == 0) {
        value_.SetWeakCallback(reinterpret_cast<void*>(this), FreeGlobalCallBack, NativeFinalizeCallBack);
    }
    return refCount_;
}

napi_value NativeReference::Get()
{
    if (value_.IsEmpty()) {
        return nullptr;
    }
    auto vm = engine_->GetEcmaVm();
    Local<JSValueRef> value = value_.ToLocal(vm);
    return JsValueFromLocalValue(value);
}

NativeReference::operator napi_value()
{
    return Get();
}

void* NativeReference::GetData()
{
    return data_;
}

void NativeReference::FinalizeCallback()
{
    if (napiCallback_ != nullptr) {
        napiCallback_(reinterpret_cast<napi_env>(engine_), data_, hint_);
    }
    napiCallback_ = nullptr;
    data_ = nullptr;
    hint_ = nullptr;
    finalRun_ = true;

    if (deleteSelf_ && !hasDelete_) {
        delete this;
    }
}

void NativeReference::FreeGlobalCallBack(void* ref)
{
    auto that = reinterpret_cast<NativeReference*>(ref);
    that->value_.FreeGlobalHandleAddr();
}

void NativeReference::NativeFinalizeCallBack(void* ref)
{
    auto that = reinterpret_cast<NativeReference*>(ref);
    that->FinalizeCallback();
}

void NativeReference::SetDeleteSelf()
{
    deleteSelf_ = true;
}

uint32_t NativeReference::GetRefCount()
{
    return refCount_;
}

bool NativeReference::GetFinalRun()
{
    return finalRun_;
}

napi_value NativeReference::GetNapiValue()
{
    return Get();
}