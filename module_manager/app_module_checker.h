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

#ifndef FOUNDATION_ACE_NAPI_MODULE_MANAGER_APP_MODULE_CHECKER_H
#define FOUNDATION_ACE_NAPI_MODULE_MANAGER_APP_MODULE_CHECKER_H

#include "module_checker_delegate.h"

/**
 * @brief Form module load checker. check whether module can be loaded in form
 * 
 */
class AppModuleChecker : public ModuleCheckerDelegate {
public:
    AppModuleChecker() = default;
    ~AppModuleChecker() override = default;

    bool CheckModuleLoadable(const char* moduleName) override;

    void SetModuleBlocklist(std::unordered_map<int32_t, std::unordered_set<std::string>>&& blocklist) override;

    void SetProcessExtensionType(int32_t extensionType) override;

    int32_t GetProcessExtensionType() override;

protected:
    std::unordered_map<int32_t, std::unordered_set<std::string>> moduleBlocklist_;
    int32_t processExtensionType_{EXTENSION_TYPE_UNKNOWN};
};

#endif /* FOUNDATION_ACE_NAPI_MODULE_MANAGER_APP_MODULE_CHECKER_H */
