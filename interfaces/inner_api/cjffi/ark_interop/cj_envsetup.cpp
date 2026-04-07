/*
* Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "cj_envsetup.h"

#ifdef __LINUX__
#include <dlfcn.h>
/**
 * @brief POSIX: `dlopen` with `RTLD_NOW`.
 */
static void* OpenLib(const char* name)
{
    return dlopen(name, RTLD_NOW);
}
/**
 * @brief POSIX: `dlsym` by symbol name.
 */
static void* FindSym(void* handle, const char* name)
{
    return dlsym(handle, name);
}
#ifndef __OHOS__
#define LIB_NAME "libcj_environment.so"
#else
#define LIB_NAME "libcj_environment.z.so"
#endif
#elif defined(__WINDOWS__)
#include <libloaderapi.h>
/**
 * @brief Windows: `LoadLibraryA`.
 */
static void* OpenLib(const char* name)
{
    return LoadLibraryA(name);
}
/**
 * @brief Windows: `GetProcAddress`.
 */
static void* FindSym(void* handle, const char* name)
{
    return reinterpret_cast<void*>(GetProcAddress(reinterpret_cast<HMODULE>(handle), name));
}
#define LIB_NAME "libcj_environment.dll"
#else
#error "unsupported platform"
#endif
#define GET_ENV_INS_NAME "OHOS_GetCJEnvInstance"

namespace OHOS {
/**
 * @brief Loads the platform `libcj_environment` and calls `OHOS_GetCJEnvInstance` for the capability singleton.
 * @return `CJEnvMethods*` on success; `nullptr` if the library or symbol is missing.
 */
CJEnvMethods* CJEnv::LoadInstance()
{
    auto handle = OpenLib(LIB_NAME);
    if (!handle) {
        return nullptr;
    }
    auto symbol = FindSym(handle, GET_ENV_INS_NAME);
    if (!symbol) {
        return nullptr;
    }
    auto func = reinterpret_cast<CJEnvMethods* (*)()>(symbol);
    return func();
}

} // OHOS