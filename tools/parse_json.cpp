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

#include "parse_json.h"
#include <fstream>
#include <iostream>
#include "cJSON.h"

namespace ohos::check {

CheckErrorCode ParseLegalApiList(const std::string& resultFile, std::vector<ApiMeta>& legalApiList)
{
    std::ifstream ifs(resultFile);
    if (!ifs.is_open()) {
        std::cerr << "[ParseLegalApiList] open file failed: " << resultFile << std::endl;
        return ERROR_FILE_OPEN_FAILED;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    cJSON* root = cJSON_Parse(content.c_str());

    if (!root || !cJSON_IsArray(root)) {
        std::cerr << "[ParseLegalApiList] json parse failed or not array" << std::endl;
        return ERROR_JSON_PARSE_FAILED;
    }

    int size = cJSON_GetArraySize(root);
    for (int i = 0; i < size; ++i) {
        cJSON* item = cJSON_GetArrayItem(root, i);
        if (!item || !cJSON_IsObject(item)) continue;

        ApiMeta meta{};

        // 读取三个关键字段
        cJSON* apiText = cJSON_GetObjectItem(item, "apiText");
        cJSON* apiType = cJSON_GetObjectItem(item, "apiType");
        cJSON* className = cJSON_GetObjectItem(item, "className");

        if (apiText && cJSON_IsString(apiText)) meta.apiText = apiText->valuestring;
        if (apiType && cJSON_IsString(apiType)) meta.apiType = apiType->valuestring;
        if (className && cJSON_IsString(className)) meta.className = className->valuestring;

        if (!meta.apiText.empty()) {
            legalApiList.push_back(meta);
        }
    }

    cJSON_Delete(root);
    return CHECK_OK;
}

}