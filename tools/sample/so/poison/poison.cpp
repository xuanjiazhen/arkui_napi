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

// 抛异常的访问器 getter：遍历导出面对 "trap" 执行 napi_get_property 时被触发
// （napi_get_property 带 NAPI_PREAMBLE），在 env 上留下挂起异常。
// 该场景复现真机大面积失败的根因：修复前（遍历失败分支不清异常），
// "trap" 之后的属性被静默跳过（"after" 丢失，漏报），且挂起异常会污染
// 后续所有模块的 requireNapi 查找（napi_get_named_property 同样带
// PREAMBLE，直接短路成 "requireNapi not found"）。
static napi_value ThrowingGetter(napi_env env, napi_callback_info /* info */)
{
    napi_throw_error(env, nullptr, "poison getter");
    return nullptr;
}

// 模块初始化入口：属性按插入顺序枚举——safe、trap（抛异常）、after。
// after 位于 trap 之后，用于验证异常被清理后遍历继续（修复前会丢失）。
static napi_value ModuleInit(napi_env env, napi_value exports)
{
    // 导出普通字段 safe
    napi_value safe = nullptr;
    napi_create_uint32(env, 1, &safe);
    napi_set_named_property(env, exports, "safe", safe);

    // 导出访问器属性 trap：getter 抛异常
    napi_property_descriptor trapDesc[] = {
        {"trap", nullptr, nullptr, ThrowingGetter, nullptr, nullptr, napi_default, nullptr}
    };
    napi_define_properties(env, exports, 1, trapDesc);

    // 导出普通字段 after（位于 trap 之后）
    napi_value after = nullptr;
    napi_create_string_utf8(env, "after", NAPI_AUTO_LENGTH, &after);
    napi_set_named_property(env, exports, "after", after);

    return exports;
}

// 标准node-api模块声明
static napi_module poisonModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = ModuleInit,
    .nm_modname = "poison",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterCheckApiPoisonModule(void)
{
    napi_module_register(&poisonModule);
}
