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

// 访问器安全遍历回归样例：遍历导出面时访问器属性（getter/setter）不读值、
// 按 field 记录（ReadExportProperty，check_helper.cpp）。真机上类原型的原生
// getter 期望实例 receiver（内部 napi_unwrap(this) 后解引用），以原型为
// receiver 调用会空指针崩溃（@ohos.util 的 TextEncoder 原型 encoding getter
// 即此死法，SIGSEGV @0x0）。本模块导出四个属性：
//  - safe / after：普通数据字段；
//  - trapThrow：访问器，getter 被调用即抛异常（旧版挂起异常污染源——修复后
//    遍历不读访问器值，getter 永不被调用）；
//  - trapFunc：访问器，getter 返回函数。若遍历退化为"读值判型"，其类型会被
//    误判为 method，与白名单 field 条目不匹配 → 本用例变红，锁定该行为。
static napi_value ThrowingGetter(napi_env env, napi_callback_info /* info */)
{
    napi_throw_error(env, nullptr, "poison getter");
    return nullptr;
}

static napi_value FuncGetter(napi_env env, napi_callback_info /* info */)
{
    napi_value func = nullptr;
    napi_create_function(env, "trapFuncValue", NAPI_AUTO_LENGTH, ThrowingGetter, nullptr, &func);
    return func;
}

static napi_value ModuleInit(napi_env env, napi_value exports)
{
    napi_value safe = nullptr;
    napi_create_uint32(env, 1, &safe);
    napi_set_named_property(env, exports, "safe", safe);

    napi_value after = nullptr;
    napi_create_string_utf8(env, "after", NAPI_AUTO_LENGTH, &after);
    napi_set_named_property(env, exports, "after", after);

    napi_property_descriptor accessors[] = {
        {"trapThrow", nullptr, nullptr, ThrowingGetter, nullptr, nullptr, napi_default, nullptr},
        {"trapFunc", nullptr, nullptr, FuncGetter, nullptr, nullptr, napi_default, nullptr},
    };
    napi_define_properties(env, exports, sizeof(accessors) / sizeof(accessors[0]), accessors);
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
