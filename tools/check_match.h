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

#ifndef CHECK_MATCH_H
#define CHECK_MATCH_H

#include <map>
#include <vector>
#include <string>

#include "api_meta.h"
#include "check_helper.h"
#include "error_codes.h"

namespace ohos::check {

//完整字段匹配检查：暴露项经名称索引定位候选合法条目后逐条 Match
CheckErrorCode CheckIllegalExposed(
    const LegalIndex& legalIndex,
    const std::vector<ApiMeta>& exposedList,
    const std::map<std::string, std::string> &inheritMap,
    std::vector<ApiMeta> &illegalList
);

}

#endif
