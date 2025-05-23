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

#ifndef FOUNDATION_ACE_NAPI_UTILS_MACROS_H
#define FOUNDATION_ACE_NAPI_UTILS_MACROS_H

#ifndef NAPI_EXPORT
#ifdef WINDOWS_PLATFORM
#define NAPI_EXPORT __declspec(dllexport)
#else
#define NAPI_EXPORT __attribute__((visibility("default")))
#endif
#endif

#ifndef NAPI_DEFAULT_MOVE_CTOR
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NAPI_DEFAULT_MOVE_CTOR(TypeName)         \
    /* NOLINTNEXTLINE(misc-macro-parentheses) */ \
    TypeName(TypeName&&) = default
#endif

#ifndef NAPI_DEFAULT_MOVE_OPERATOR
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NAPI_DEFAULT_MOVE_OPERATOR(TypeName)     \
    /* NOLINTNEXTLINE(misc-macro-parentheses) */ \
    TypeName& operator=(TypeName&&) = default
#endif

#ifndef NAPI_DEFAULT_MOVE_SEMANTIC
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NAPI_DEFAULT_MOVE_SEMANTIC(TypeName) \
    NAPI_DEFAULT_MOVE_CTOR(TypeName);        \
    NAPI_DEFAULT_MOVE_OPERATOR(TypeName)
#endif

#ifndef NAPI_DEFAULT_COPY_CTOR
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NAPI_DEFAULT_COPY_CTOR(TypeName)         \
    /* NOLINTNEXTLINE(misc-macro-parentheses) */ \
    TypeName(const TypeName&) = default
#endif

#ifndef NAPI_DEFAULT_COPY_OPERATOR
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NAPI_DEFAULT_COPY_OPERATOR(TypeName)     \
    /* NOLINTNEXTLINE(misc-macro-parentheses) */ \
    TypeName& operator=(const TypeName&) = default
#endif

#ifndef NAPI_DEFAULT_COPY_SEMANTIC
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define NAPI_DEFAULT_COPY_SEMANTIC(TypeName) \
    NAPI_DEFAULT_COPY_CTOR(TypeName);        \
    NAPI_DEFAULT_COPY_OPERATOR(TypeName)
#endif

#endif /* FOUNDATION_ACE_NAPI_UTILS_MACROS_H */
