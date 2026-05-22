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

#ifndef OHOS_FFI_FFI_REMOTE_DATA_H
#define OHOS_FFI_FFI_REMOTE_DATA_H

#include "nocopyable.h"
#include "refbase.h"
#include "unordered_map"

#include <mutex>

#include "cj_fn_invoker.h"
#include "runtimetype.h"

namespace OHOS::FFI {

constexpr int64_t MAX_INT64 = 0x7fffffffffffffff;

class RemoteData;
class FFIData;

#define CJ_REMOTE_CLASS(className)                               \
    DECL_TYPE(className, RemoteData)                           \
public:                                                        \
    friend class RefBase;                                      \
    friend class RemoteData;                                   \
    DISALLOW_COPY_AND_MOVE(className);                         \
    className() = delete;                                      \
                                                               \
protected:                                                     \
    explicit className(int64_t id) : OHOS::FFI::RemoteData(id) \
    {}

/**
 * a reference of cj FFIData object.
 *  standard create procedure:
 *      1. cj: create FFIData
 *      2. cj: register to FFIDataManager
 *      3. cj: pass id to native
 *      4. native: RemoteData::Create
 *  standard destroy procedure:
 *      1. native: ~RemoteData
 *      2. cj: remove from FFIDataManager
 *      3. cj: FFIData::onDestroyed
 */
class FFI_EXPORT RemoteData : public TypeBase, public virtual RefBase {
    DECL_TYPE(RemoteData, TypeBase)
public:
    DISALLOW_COPY_AND_MOVE(RemoteData);

    template<class T>
    static sptr<T> Create(int64_t id)
    {
        auto rawRef = new (std::nothrow) T(id);
        if (!rawRef) {
            return nullptr;
        }
        sptr<T> ref(rawRef);
        return ref;
    }

    ~RemoteData() override;

protected:
    explicit RemoteData(int64_t id);

private:
    friend class RefBase;

public:
    int64_t GetID() const;

private:
    int64_t id_;
    bool isValid_;
};

class FFI_EXPORT CJLambdaRemoteData : public RemoteData {
    DECL_TYPE(CJLambdaRemoteData, RemoteData)
public:
    explicit CJLambdaRemoteData(int64_t id): RemoteData(id) {}

    ~CJLambdaRemoteData() override;
};

/**
 * object to referenced by cj.
 *  standard create procedure:
 *      0. native: Define class derived from FFIData with macro DECL_TYPE to support safe dynamic cast
 *      1. native: FFIData::Create
 *      2. native: register to FFIDataManager
 *      3. native: pass to cj side
 *      4. cj: create RemoteData
 *      5. cj: register to RemoteDataManager
 *  standard destroy procedure:
 *      1. cj: RemoteData.release
 *      2. native: remove from FFIDataManager
 *      3. native: ~FFIData
 *      4. cj: RemoteData.onDestroyed
 */
class FFI_EXPORT FFIData : public TypeBase, public RefBase {
    DECL_TYPE(FFIData, TypeBase)
public:
    DISALLOW_COPY_AND_MOVE(FFIData);
    FFIData() : id_(-1) {}
    ~FFIData() override;

    template<class T, class... Args>
    static sptr<T> Create(Args... args)
    {
        auto rawPtr = new (std::nothrow) T(std::forward<Args>(args)...);
        if (!rawPtr) {
            return nullptr;
        }
        sptr<T> ref = rawPtr;
        AppendItem(ref);
        return ref;
    }

    template<class T>
    static sptr<T> GetData(int64_t id)
    {
        auto data = GetItem(id);
        if (data != nullptr) {
            return sptr<T>(data.GetRefPtr()->template DynamicCast<T>());
        }
        return nullptr;
    }

    static bool Release(int64_t id);

    static bool Exist(int64_t id);

    int64_t GetID() const
    {
        return id_;
    }

private:
    int64_t id_;
    static void AppendItem(const sptr<FFIData>& ref);
    static sptr<FFIData> GetItem(int64_t id);
};

} // namespace OHOS::FFI

#endif // OHOS_FFI_FFI_REMOTE_DATA_H
