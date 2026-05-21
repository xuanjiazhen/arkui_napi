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
#ifndef CHECK_HELPER_H
#define CHECK_HELPER_H

#include <string>
#include <vector>

#include "api_meta.h"
#include "napi/native_api.h"
 
namespace ohos::check {

napi_env CreateArkEnv();

void DestroyArkEnv(napi_env env);

// 忽略内置属性名单
bool IsIgnoredProperty(const char* name);

// 从NAPI值获取类型
std::string GetApiType(napi_env env, napi_value value);

// 递归遍历导出对象
void TraverseExports(
    napi_env env,
    napi_value exports,
    const std::string& parent_class,
    std::vector<ApiMeta>& out_meta
);

bool StartsWith(const std::string& str, const std::string& prefix);
bool EndsWith(const std::string& str, const std::string& suffix);

}

#endif