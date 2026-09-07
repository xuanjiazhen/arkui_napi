/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef LOAD_ABC_H
#define LOAD_ABC_H

#include <set>
#include <string>
#include <vector>

#include "api_meta.h"
#include "error_codes.h"
#include "napi/native_api.h"

namespace ohos::check {

/**
 * @brief 在Ark引擎中执行abc文件并提取导出API元数据
 * @param kit 所属kit名
 * @param jsFilePath abc文件路径
 * @param exposedMeta 输出：apiText + apiType + className
 * @param expandableNames 需要展开成员的已声明命名空间/枚举名集合，可为 nullptr
 * @return 错误码
 */
CheckErrorCode LoadJSAndGetExposedMeta(
    napi_env env,
    const std::string& kit,
    const std::string& jsFilePath,
    std::vector<ApiMeta>& exposedMeta,
    const std::set<std::string>* expandableNames = nullptr
);

}

#endif
