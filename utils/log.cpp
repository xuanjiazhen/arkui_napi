/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#include "utils/log.h"
#include "base/log/log.h"


[[maybe_unused]] static void StripFormatString(const std::string& prefix, std::string& str)
{
    for (auto pos = str.find(prefix, 0); pos != std::string::npos; pos = str.find(prefix, pos)) {
        str.erase(pos, prefix.size());
    }
}

#if defined(ANDROID_PLATFORM) || defined(IOS_PLATFORM)

NAPI_EXPORT void PrintLog(LogLevel level, const char* fmt, ...)
{
    std::string newFmt(fmt);
    StripFormatString("{public}", newFmt);
    StripFormatString("{private}", newFmt);
    va_list args;
    va_start(args, fmt);
    OHOS::Ace::LogWrapper::PrintLog(OHOS::Ace::LogDomain::JS_APP, OHOS::Ace::LogLevel(level),OHOS::Ace::AceLogTag::ACE_DEFAULT_DOMAIN, newFmt.c_str(), args);
    va_end(args);
}

#elif defined(MAC_PLATFORM) || defined(WINDOWS_PLATFORM) || defined(LINUX_PLATFORM)

#include <securec.h>

constexpr uint32_t MAX_BUFFER_SIZE = 4096;

NAPI_EXPORT void PrintLog(LogLevel level, const char* fmt, ...)
{
    std::string newFmt(fmt);
    StripFormatString("{public}", newFmt);
    StripFormatString("{private}", newFmt);

    va_list args;
    va_start(args, fmt);

    char buf[MAX_BUFFER_SIZE] = { '\0' };
    int ret = vsnprintf_s(buf, sizeof(buf), sizeof(buf) - 1, newFmt.c_str(), args);
    va_end(args);
    if (ret < 0) {
        return;
    }

    printf("%s\r\n", buf);
    fflush(stdout);
}
#endif
