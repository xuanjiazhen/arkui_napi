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

#include "module_load_checker.h"

#include "app_module_checker.h"
#include "form_module_checker.h"
#include "utils/log.h"

ModuleLoadChecker::ModuleLoadChecker(ProcessType type)
{
    std::lock_guard<std::mutex> lock(checkerDelegateMutex_);
    moduleCheckerDelegate_ = CreateModuleCheckerDelegate(type);
}

std::unique_ptr<ModuleCheckerDelegate> ModuleLoadChecker::CreateModuleCheckerDelegate(ProcessType type)
{
    if (type == ProcessType::APPLICATION) {
        return std::make_unique<AppModuleChecker>();
    } else if (type == ProcessType::FORM) {
        return std::make_unique<FormModuleChecker>();
    }
    return nullptr;
}

void ModuleLoadChecker::SetModuleBlocklist(
    std::unordered_map<int32_t, std::unordered_set<std::string>>&& blocklist)
{
    std::lock_guard<std::mutex> lock(checkerDelegateMutex_);
    if (!moduleCheckerDelegate_) {
        HILOG_ERROR("SetModuleBlocklist failed, moduleCheckerDelegate_ is nullptr");
        return;
    }
    moduleCheckerDelegate_->SetModuleBlocklist(std::move(blocklist));
}

void ModuleLoadChecker::SetProcessExtensionType(int32_t extensionType)
{
    std::lock_guard<std::mutex> lock(checkerDelegateMutex_);
    if (!moduleCheckerDelegate_) {
        HILOG_ERROR("SetProcessExtensionType failed, moduleCheckerDelegate_ is nullptr");
        return;
    }
    moduleCheckerDelegate_->SetProcessExtensionType(extensionType);
}

int32_t ModuleLoadChecker::GetProcessExtensionType()
{
    std::lock_guard<std::mutex> lock(checkerDelegateMutex_);
    if (!moduleCheckerDelegate_) {
        HILOG_ERROR("GetProcessExtensionType failed, moduleCheckerDelegate_ is nullptr");
        return EXTENSION_TYPE_UNKNOWN;
    }
    return moduleCheckerDelegate_->GetProcessExtensionType();
}

bool ModuleLoadChecker::CheckModuleLoadable(const char* moduleName)
{
    std::lock_guard<std::mutex> lock(checkerDelegateMutex_);
    if (!moduleCheckerDelegate_) {
        HILOG_ERROR("CheckModuleLoadable failed, moduleCheckerDelegate_ is nullptr");
        return false;
    }
    return moduleCheckerDelegate_->CheckModuleLoadable(moduleName);
}