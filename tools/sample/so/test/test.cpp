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

#include <node_api.h>
#include <string.h>

/**
 * @brief 测试Class A
 */
static napi_value ClassAConstructor(napi_env env, napi_callback_info info) {
    napi_value jsthis;
    napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
    return jsthis;
}

/**
 * @brief 成员方法 foo
 */
static napi_value ClassAFoo(napi_env env, napi_callback_info info) {
    napi_value ret;
    napi_create_int32(env, 100, &ret);
    return ret;
}

/**
 * @brief 普通方法 bar
 */
static napi_value BarFunc(napi_env env, napi_callback_info info) {
    napi_value ret;
    napi_create_string_utf8(env, "bar", NAPI_AUTO_LENGTH, &ret);
    return ret;
}

/**
 * @brief 模块初始化入口（标准Node-API）
 */
static napi_value ModuleInit(napi_env env, napi_value exports) {
    // ================== 导出 class A ==================
    napi_value classA;
    napi_property_descriptor classDesc[] = {
        {"foo", nullptr, ClassAFoo, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    napi_define_class(env, "A", NAPI_AUTO_LENGTH, ClassAConstructor, nullptr, 1, classDesc, &classA);
    napi_set_named_property(env, exports, "A", classA);

    // ================== 导出 method bar ==================
    napi_value bar;
    napi_create_function(env, nullptr, 0, BarFunc, nullptr, &bar);
    napi_set_named_property(env, exports, "bar", bar);

    // ================== 导出 field version ==================
    napi_value version;
    napi_create_int32(env, 2, &version);
    napi_set_named_property(env, exports, "version", version);

    return exports;
}

// 标准Node-API模块声明
static napi_module testModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = ModuleInit,
    .nm_modname = "test",
    .nm_priv = ((void*)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterCheckApiTestModule(void)
{
    napi_module_register(&testModule);
}