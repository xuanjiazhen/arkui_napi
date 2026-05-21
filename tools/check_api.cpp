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

#include "check_api.h"
#include <iostream>
#include <algorithm>

namespace ohos::check {

CheckErrorCode CheckIllegalExposed(
    const std::vector<ApiMeta>& legalList,
    const std::vector<ApiMeta>& exposedList
) {
    CheckErrorCode ret = CHECK_OK;
    for (const auto& exposed : exposedList) {
        bool found = false;
        for (const auto& legal : legalList) {
            if (exposed == legal) {
                found = true;
                break;
            }
            if (exposed.apiText == legal.apiText) {
                std::cerr << "  current apiText: " << legal.apiText << std::endl;
                std::cerr << "  current apiType: " << legal.apiType << std::endl;
                std::cerr << "  current className: " << legal.className << std::endl;
            }
        }

        if (!found) {
            std::cerr << "[CheckAPI] 非法暴露API：" << std::endl;
            std::cerr << "  apiText: " << exposed.apiText << std::endl;
            std::cerr << "  apiType: " << exposed.apiType << std::endl;
            std::cerr << "  className: " << exposed.className << std::endl;
            std::cerr << "  ========================================  " << std::endl;
            ret = ERROR_ILLEGAL_API_EXPOSED;
        }
    }

    return ret;
}

}