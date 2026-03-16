/**
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

#ifndef SAMPLE_NATIVE_MODULE_CODECHECK_TEST2_H
#define SAMPLE_NATIVE_MODULE_CODECHECK_TEST2_H

#include <string>
#include "napi/native_api.h"
#include "napi/native_node_api.h"

constexpr size_t MAX_ALLOC_SIZE = 2 * 1024 * 1024;

int CalculateSum(int a, int b);
int GetMaxValue(int* arr, int size);
void ProcessData(const char* input, char* output, int maxLen);
void ProcessAllParameters(int parameterOne,
                          int parameterTwo,
                          int parameterThree,
                          int parameterFour);

#endif // SAMPLE_NATIVE_MODULE_CODECHECK_TEST2_H
