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

#ifndef API_META_H
#define API_META_H

#include <string>

namespace ohos::check {

// 完整的API元数据（三字段联合校验）
struct ApiMeta {
    std::string apiText;
    std::string apiType;   // class / method / field
    std::string className; // 所属类（class类型可为空）

    // 重载 == 用于完整匹配校验
    bool operator==(const ApiMeta& other) const {
        return apiText == other.apiText &&
               apiType == other.apiType &&
               className == other.className;
    }
};

}

#endif // API_META_H