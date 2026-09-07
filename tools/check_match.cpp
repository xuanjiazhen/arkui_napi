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

#include "check_match.h"

#include <string>
#include <vector>

namespace ohos::check {
namespace {

// 在名称索引命中的候选合法条目中寻找匹配
bool MatchInCandidates(const ApiMeta& exposed, const std::vector<const ApiMeta*>& candidates,
                       const std::map<std::string, std::string>& inheritMap)
{
    for (const ApiMeta* legal : candidates) {
        if (Match(exposed, *legal, inheritMap)) {
            return true;
        }
    }
    return false;
}

} // namespace

CheckErrorCode CheckIllegalExposed(
    const LegalIndex& legalIndex,
    const std::vector<ApiMeta> &exposedList,
    const std::map<std::string, std::string> &inheritMap,
    std::vector<ApiMeta> &illegalList
) {
    CheckErrorCode ret = CHECK_OK;
    for (const auto& exposed : exposedList) {
        // 名称索引：合法条目在解析期已归一化出"可匹配名"（方法名/类型名/字段名），
        // 暴露项的属性名直接命中候选集，避免 O(暴露项×白名单) 全量扫描
        // （ArkUI kit 白名单 2 万条 × 79 模块，全量扫描在设备上不可接受）。
        bool found = false;
        auto it = legalIndex.find(exposed.apiText);
        if (it != legalIndex.end()) {
            found = MatchInCandidates(exposed, it->second, inheritMap);
        }

        if (!found) {
            LOG_ERROR(" [CheckAPI] 非法暴露API: " <<
                      exposed.apiText << " | apiType: " << exposed.apiType <<
                      " | className: " << exposed.className <<
                      " | file: " << exposed.file << " | kit: " << exposed.kit);
            illegalList.push_back(exposed);
            ret = ERROR_ILLEGAL_API_EXPOSED;
        }
    }

    return ret;
}

}
