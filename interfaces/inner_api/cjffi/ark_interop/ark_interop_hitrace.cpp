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

#include "ark_interop_hitrace.h"

#ifndef NO_HITRACE
#include "hitrace_meter.h"

/**
 * @file ark_interop_hitrace.cpp
 * @brief HiTrace async trace and counter wrappers (stubs when `NO_HITRACE` for builds without the service).
 */

/**
 * @brief Starts an async trace (`HITRACE_TAG_APP`).
 * @param name Task name (paired with `Finish`).
 * @param taskId Task id (disambiguates concurrent instances with the same name).
 */
DECL_START
void ARKTS_HiTraceStartTrace(const char* name, int32_t taskId)
{
    StartAsyncTrace(HITRACE_TAG_APP, name, taskId);
}

/** @brief Finishes the async trace started with the same name and id. */
void ARKTS_HiTraceFinishTrace(const char* name, int32_t taskId)
{
    FinishAsyncTrace(HITRACE_TAG_APP, name, taskId);
}

/** @brief Emits an instantaneous counter trace point (not an async span). */
void ARKTS_HiTraceCountTrace(const char* name, int64_t count)
{
    CountTrace(HITRACE_TAG_APP, name, count);
}

DECL_END
#else
/** @brief `NO_HITRACE` build: no-op stubs (no system profiling). */
void ARKTS_HiTraceStartTrace(const char* /* name */, int32_t /* taskId */) {}
void ARKTS_HiTraceFinishTrace(const char* /* name */, int32_t /* taskId */) {}
void ARKTS_HiTraceCountTrace(const char* /* name */, int64_t /* count */) {}
#endif