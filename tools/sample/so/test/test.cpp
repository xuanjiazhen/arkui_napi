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

// 测试Class A 的 foo 方法
static napi_value ClassAFoo(napi_env env, napi_callback_info /* info */)
{
    napi_value ret = nullptr;
    napi_get_undefined(env, &ret);
    return ret;
}

// 测试Class A 构造函数
static napi_value ClassAConstructor(napi_env env, napi_callback_info info)
{
    napi_value jsthis = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
    return jsthis;
}

// 普通方法bar
static napi_value BarFunc(napi_env env, napi_callback_info /* info */)
{
    napi_value ret = nullptr;
    napi_create_string_utf8(env, "bar", NAPI_AUTO_LENGTH, &ret);
    return ret;
}

// 模块初始化入口
static napi_value ModuleInit(napi_env env, napi_value exports)
{
    //导出class A
    napi_value classA = nullptr;
    napi_property_descriptor classDesc[] = {
        {"foo", nullptr, ClassAFoo, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    napi_define_class(env, "A", NAPI_AUTO_LENGTH, ClassAConstructor, nullptr, 1, classDesc, &classA);
    napi_set_named_property(env, exports, "A", classA);

    //导出method bar
    napi_value bar = nullptr;
    napi_create_function(env, "bar", NAPI_AUTO_LENGTH, BarFunc, nullptr, &bar);
    napi_set_named_property(env, exports, "bar", bar);

    return exports;
}

// 标准node-api模块声明
static napi_module testModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = ModuleInit,
    .nm_modname = "test",
    .nm_priv = nullptr,
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterCheckApiTestModule(void)
{
    napi_module_register(&testModule);
}
