/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ark_native_set.h"

using panda::SetRef;

ArkNativeSet::ArkNativeSet(ArkNativeEngine* engine, Local<JSValueRef> value)
    : ArkNativeObject(engine, value) {}

ArkNativeSet::~ArkNativeSet() {}

void* ArkNativeSet::GetInterface(int interfaceId)
{
    return (NativeSet::INTERFACE_ID == interfaceId) ? (NativeSet*)this
        : ArkNativeObject::GetInterface(interfaceId);
}

void ArkNativeSet::Add(NativeValue* value)
{
    auto vm = engine_->GetEcmaVm();
    LocalScope scope(vm);
    Global<SetRef> obj = value_;
    Global<JSValueRef> val = *value;
    obj->Add(vm, val.ToLocal(vm));
}

bool ArkNativeSet::Delete(NativeValue* value)
{
    auto vm = engine_->GetEcmaVm();
    LocalScope scope(vm);
    Global<SetRef> obj = value_;
    Global<JSValueRef> val = *value;
    return obj->Delete(vm, val.ToLocal(vm));
}

NativeValue* ArkNativeSet::GetValue(int entry)
{
    auto vm = engine_->GetEcmaVm();
    LocalScope scope(vm);
    Global<SetRef> obj = value_;
    return ArkNativeEngine::ArkValueToNativeValue(engine_, obj->GetValue(vm, entry));
}