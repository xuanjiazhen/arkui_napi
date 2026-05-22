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

#include "ffi_remote_data.h"

#include <cinttypes>

#include "hilog/log_cpp.h"
#include "slab.h"

using namespace OHOS::FFI;
using namespace OHOS;
using namespace OHOS::HiviewDFX;

namespace {
constexpr HiLogLabel LABEL = { LOG_CORE, 0xD003F03, "CJ-FFIBindNative" };
MultSlabMT<sptr<FFIData>> ffiDataItems_;
} // namespace

RemoteData::RemoteData(int64_t id) : id_(id), isValid_(id != 0)
{
    HiLog::Debug(LABEL, "RemoteData constructed: %{public}" PRId64 ".", id_);
}

RemoteData::~RemoteData()
{
    HiLog::Debug(LABEL, "RemoteData destructed: %{public}" PRId64 ".", id_);
    auto cjFunc = CJFFIFnInvoker::GetInstance()->GetCJFuncs().atCOHOSFFIReleaseFFIData;
    if (!cjFunc) {
        HiLog::Error(LABEL, "Failed to invoke CJ function: FFIReleaseFFIData!");
        return;
    }
    cjFunc(GetID());
    isValid_ = false;
}

CJLambdaRemoteData::~CJLambdaRemoteData()
{
}

int64_t RemoteData::GetID() const
{
    if (!isValid_) {
        HiLog::Error(LABEL, "RemoteData::GetID error, remote data invalid: %{public}" PRId64 ".", id_);
    }
    return id_;
}

FFIData::~FFIData() {}

bool FFIData::Exist(int64_t id)
{
    return ffiDataItems_.Contains(id);
}

sptr<FFIData> FFIData::GetItem(int64_t id)
{
    auto result = ffiDataItems_.Get(id);
    if (result.has_value()) {
        return result.value();
    }
    return nullptr;
}

bool FFIData::Release(int64_t id)
{
    return ffiDataItems_.Remove(id).has_value();
}

void FFIData::AppendItem(const sptr<FFIData>& ref)
{
    ref->id_ = ffiDataItems_.Append(ref);
}
