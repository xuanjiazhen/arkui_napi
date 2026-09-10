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

#include "napi/native_api.h"

#include "abctest.h"

// Mixed .so regression sample: exports come from the embedded ABC and napi_module_register
// is deliberately not called to force the ABC fallback path (see run_abcmixed).
static void GetAbcCode(const char** buf, int* len)
{
    *buf = reinterpret_cast<const char*>(ABC_DATA);
    *len = ABC_LEN;
}

// The symbol name is mandated by the engine's NAPI_<module>_GetABCCode lookup contract
// (module @ohos.ability.abcmixed -> requireNapi "ability.abcmixed"), so it cannot follow
// the CamelCase rule.
// NOLINTNEXTLINE
extern "C" __attribute__((visibility("default"))) void NAPI_ability_abcmixed_GetABCCode(const char** buf, int* len)
{
    GetAbcCode(buf, len);
}
