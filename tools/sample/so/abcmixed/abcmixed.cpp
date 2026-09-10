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

// 混合 so 回归样例：so 内嵌 ArkTS ABC（GetABCCode 符号），导出面由 ABC 提供
// 而非 napi 注册回调。requireNapi 走引擎 LoadArkModule 时因裸进程缺运行时
// 上下文可能失败——此时工具的 ABC 备选通道（直接提取+执行）应能恢复。
// 本模块刻意不调用 napi_module_register（nm_modname 不可用），逼设备上的
// requireNapi 走失败路径，从而锁定备选通道；宿主机上引擎路径可能直接
// 成功（同样验证 ABC 可执行）。
// ABC 内容（abctest.js）：
//   export class DemoAbc { value = 42 }
//   export function abcFunc() {}
//   export let abcConst = 7
static void GetAbcCode(const char** buf, int* len)
{
    *buf = reinterpret_cast<const char*>(g_abcData);
    *len = g_abcLen;
}

// 模块名 @ohos.ability.abcmixed -> requireNapi "ability.abcmixed"
// -> 符号 NAPI_ability_abcmixed_GetABCCode
extern "C" __attribute__((visibility("default")))
void NAPI_ability_abcmixed_GetABCCode(const char** buf, int* len)
{
    GetAbcCode(buf, len);
}
