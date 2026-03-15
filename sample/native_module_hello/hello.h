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

#ifndef SAMPLE_NATIVE_MODULE_HELLO_HELLO_H
#define SAMPLE_NATIVE_MODULE_HELLO_HELLO_H

#include <string>
#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace NapiHello {

/**
 * @brief Returns a greeting message
 * @param name The name to greet
 * @return Greeting string
 */
std::string Greet(const std::string &name);

/**
 * @brief Adds two integers
 * @param a First number
 * @param b Second number
 * @return Sum of a and b
 */
int Add(int a, int b);

} // namespace NapiHello

#endif // SAMPLE_NATIVE_MODULE_HELLO_HELLO_H
