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

/*
 * dlsym Mock Implementation
 *
 * Principle:
 * 1. Use dlsym(RTLD_NEXT, "func_name") to get the real function pointer
 * 2. Define same-name functions to override original functions
 * 3. Decide at runtime whether to call mock logic or real function
 *
 * Notes:
 * - std::ifstream uses open() system call internally, not fopen()
 * - Must mock open/read/close/fstat to intercept ifstream operations
 */

#ifndef FOUNDATION_ACE_NAPI_DLSYM_MOCK_H
#define FOUNDATION_ACE_NAPI_DLSYM_MOCK_H

#include <stddef.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct stat;

/*
 * ============================================================================
 * Function Pointer Type Definitions
 * ============================================================================
 */

typedef int (*dlsym_access_func_t)(const char*, int);
typedef int (*dlsym_open_func_t)(const char*, int, ...);
typedef ssize_t (*dlsym_read_func_t)(int, void*, size_t);
typedef ssize_t (*dlsym_write_func_t)(int, const void*, size_t);
typedef int (*dlsym_close_func_t)(int);
typedef off_t (*dlsym_lseek_func_t)(int, off_t, int);
typedef int (*dlsym_stat_func_t)(const char*, struct stat*);
typedef int (*dlsym_fstat_func_t)(int, struct stat*);
typedef FILE* (*dlsym_fopen_func_t)(const char*, const char*);

/*
 * ============================================================================
 * Mock Control Interface
 * ============================================================================
 */

void DlsymMock_Enable(void);
void DlsymMock_Disable(void);
void DlsymMock_SetFileContent(const char* path, const char* content);
void DlsymMock_SetFileExists(const char* path, int exists);
void DlsymMock_ClearAll(void);
int DlsymMock_IsEnabled(void);

/*
 * ============================================================================
 * Debug Interface
 * ============================================================================
 */

void DlsymMock_SetDebugMode(int enable);

#ifdef __cplusplus
}
#endif

#endif /* FOUNDATION_ACE_NAPI_DLSYM_MOCK_H */
