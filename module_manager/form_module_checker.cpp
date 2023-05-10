/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "form_module_checker.h"

#include "utils/log.h"

namespace {
    constexpr char UNSUPPORT_MODULE_PREFIX[] = "@ohos";
}

bool FormModuleChecker::CheckModuleLoadable(const char* moduleName)
{
    if (!moduleName) {
        return false;
    }

    // check moudleName contains unsupport mudule prefix or not
    size_t unsupportPrefixLen = strlen(UNSUPPORT_MODULE_PREFIX);
    if (strlen(moduleName) < unsupportPrefixLen) {
        return true;
    }
    if (strncmp(moduleName, UNSUPPORT_MODULE_PREFIX, unsupportPrefixLen) == 0) {
        return false;
    }
    return true;
}