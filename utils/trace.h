/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef FOUNDATION_ACE_NAPI_UTILS_TRACE_H
#define FOUNDATION_ACE_NAPI_UTILS_TRACE_H

#ifdef ENABLE_HITRACE
#include "hitrace_meter.h"
#endif

#include <string>

class ArkHiTrace {
public:
    ArkHiTrace(std::string tag)
    {
#ifdef ENABLE_HITRACE
        StartTrace(HITRACE_TAG_ACE, tag.c_str());
#endif
    }

    ~ArkHiTrace()
    {
#ifdef ENABLE_HITRACE
        if (!stopped_) {
            FinishTrace(HITRACE_TAG_ACE);
        }
#endif
    }

    void Stop()
    {
#ifdef ENABLE_HITRACE
        if (stopped_) {
            return;
        }
        FinishTrace(HITRACE_TAG_ACE);
        stopped_ = true;
#endif
    }

private:
#ifdef ENABLE_HITRACE
    bool stopped_ { false };
#endif
};

#endif /* FOUNDATION_ACE_NAPI_UTILS_TRACE_H */
