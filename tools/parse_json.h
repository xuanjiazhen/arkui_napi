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

#ifndef PARSE_JSON_H
#define PARSE_JSON_H

#include <vector>
#include <string>
#include "error_codes.h"
#include "api_meta.h"

namespace ohos::check {

/**
 * @brief 解析完整合法API列表（apiText + apiType + className）
 */
CheckErrorCode ParseLegalApiList(
    const std::string& resultFile,
    std::vector<ApiMeta>& legalApiList
);

}

#endif