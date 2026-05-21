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

#include <iostream>
#include <string>
#include <vector>
#include "api_meta.h"
#include "error_codes.h"
#include "parse_json.h"
#include "load_abc.h"
#include "load_so.h"
#include "check_api.h"
#include "check_helper.h"

using namespace ohos::check;

int main(int argc, char* argv[])
{
    std::cout << "===== OpenHarmony API 非法暴露校验工具 =====" << std::endl;

    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " </path/test.abc or @ohos.xxx> <api.json>" << std::endl;
        return ERROR_PARAM_INVALID;
    }

    std::string checkFile = argv[1];
    std::string resultFile = argv[2];

    std::vector<ApiMeta> legalList;
    CheckErrorCode err = ParseLegalApiList(resultFile, legalList);
    if (err != CHECK_OK) {
        std::cout << "ParseLegalApiList failed" << std::endl;
        return err;
    }

    std::vector<ApiMeta> exposedList;
    if (EndsWith(checkFile, ".abc")) {
        err = LoadJSAndGetExposedMeta(checkFile, exposedList);
        if (err != CHECK_OK) {
            std::cout << "LoadJSAndGetExposedMeta failed" << std::endl;
            return err;
        }
#ifdef OHOS_PLATFORM
    } else if (StartsWith(checkFile, "@")) {
        err = LoadSoAndGetExposedMeta(checkFile, exposedList);
        if (err != CHECK_OK) {
            std::cout << "LoadSoAndGetExposedMeta failed" << std::endl;
            return err;
        }
#else
    } else if (EndsWith(checkFile, ".so")) {
        std::cout << "🏳️ 当前非ohos不支持检查so: " << checkFile << std::endl;
#endif
    } else {
        std::cout << "🏳️ 当前仅支持abc or so: " << checkFile << std::endl;
        return ERROR_FILE_OPEN_FAILED;
    }

    err = CheckIllegalExposed(legalList, exposedList);
    if (err != CHECK_OK) {
        std::cout << "❎ 检查不通过: 存在非法API暴露" << std::endl;
        return err;
    }

    std::cout << "☑️ 检查通过: 无非法API暴露: " << checkFile << std::endl;
    return CHECK_OK;
}