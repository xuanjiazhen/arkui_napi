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

#include "ark_interop_scope.h"

/**
 * @file ark_interop_scope.cpp
 * @brief C++ implementation of `ARKTS_Scope`: wraps `panda::LocalScope` so Locals do not leak across native calls.
 */

namespace CJ {
/**
 * @brief Opens a `LocalScope` on `vm`; returns opaque `ARKTS_Scope` (really `LocalScope*`).
 * @param vm Target `EcmaVM*`.
 */
ARKTS_Scope ARKTS_ScopeManager::OpenScope(panda::ecmascript::EcmaVM* vm)
{
    auto scopeId = new panda::LocalScope(vm);
    return P_CAST(scopeId, ARKTS_Scope);
}

/**
 * @brief Closes and destroys `LocalScope*`; pair with `OpenScope`.
 * @return Always `true` in the current implementation.
 */
bool ARKTS_ScopeManager::CloseScope(ARKTS_Scope target)
{
    // LocalScope destructor pops VM roots close()d in this scope; must match OpenScope's `new`.
    auto targetScope = P_CAST(target, panda::LocalScope*);
    delete targetScope;
    return true;
}
}