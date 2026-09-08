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

// 测试Class Sub 的 inherited 方法：白名单将其声明于逗号列表的第二个父类 Aux 名下
static napi_value ClassSubInherited(napi_env env, napi_callback_info /* info */)
{
    napi_value ret = nullptr;
    napi_get_undefined(env, &ret);
    return ret;
}

// 测试Class Sub 构造函数
static napi_value ClassSubConstructor(napi_env env, napi_callback_info info)
{
    napi_value jsthis = nullptr;
    napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
    return jsthis;
}

// 测试Class Sub2 的 qualifiedMember：白名单将其声明于限定名父类
// （"interface Sub2 extends ns.Qualified"）的末段 Qualified 名下
static napi_value ClassSub2QualifiedMember(napi_env env, napi_callback_info /* info */)
{
    napi_value ret = nullptr;
    napi_get_undefined(env, &ret);
    return ret;
}

// 测试Class Sub2 构造函数
static napi_value ClassSub2Constructor(napi_env env, napi_callback_info info)
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

// 测试B1：白名单以 "export function exportedFoo(...)" 声明（修饰词前缀）
static napi_value ExportedFooFunc(napi_env env, napi_callback_info /* info */)
{
    napi_value ret = nullptr;
    napi_get_undefined(env, &ret);
    return ret;
}

// 测试B2：白名单以 "optionalCallback?(...)" 声明（可选方法标记）
static napi_value OptionalCallbackFunc(napi_env env, napi_callback_info /* info */)
{
    napi_value ret = nullptr;
    napi_get_undefined(env, &ret);
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

    //导出enum Mode：白名单以移位表达式声明成员值（READ = 1 << 0），
    //覆盖 enum_instance 原文保留路径（值不剥 <>，成员按名匹配）
    napi_value mode = nullptr;
    napi_create_object(env, &mode);
    napi_value readVal = nullptr;
    napi_create_uint32(env, 1, &readVal);
    napi_set_named_property(env, mode, "READ", readVal);
    napi_value writeVal = nullptr;
    napi_create_uint32(env, 2, &writeVal);
    napi_set_named_property(env, mode, "WRITE", writeVal);
    napi_set_named_property(env, exports, "Mode", mode);

    //导出class Sub：成员 inherited 声明于白名单逗号多继承
    //"interface Sub extends Base, Aux" 的第二个父类 Aux 名下，
    //覆盖逗号多继承依赖提取（继承链 Sub→Aux）
    napi_value classSub = nullptr;
    napi_property_descriptor subDesc[] = {
        {"inherited", nullptr, ClassSubInherited, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    napi_define_class(env, "Sub", NAPI_AUTO_LENGTH, ClassSubConstructor, nullptr, 1, subDesc, &classSub);
    napi_set_named_property(env, exports, "Sub", classSub);

    //导出method exportedFoo / optionalCallback：白名单以 export function /
    //foo? 形态声明，覆盖 method 归一化（B1 修饰词前缀 / B2 可选标记）
    napi_value exportedFoo = nullptr;
    napi_create_function(env, "exportedFoo", NAPI_AUTO_LENGTH, ExportedFooFunc, nullptr, &exportedFoo);
    napi_set_named_property(env, exports, "exportedFoo", exportedFoo);

    napi_value optionalCallback = nullptr;
    napi_create_function(env, "optionalCallback", NAPI_AUTO_LENGTH, OptionalCallbackFunc, nullptr, &optionalCallback);
    napi_set_named_property(env, exports, "optionalCallback", optionalCallback);

    //导出class Sub2：成员 qualifiedMember 声明于限定名父类 ns.Qualified 的
    //末段 Qualified 名下，覆盖限定名父类坍缩（B3）
    napi_value classSub2 = nullptr;
    napi_property_descriptor sub2Desc[] = {
        {"qualifiedMember", nullptr, ClassSub2QualifiedMember, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    napi_define_class(env, "Sub2", NAPI_AUTO_LENGTH, ClassSub2Constructor, nullptr, 1, sub2Desc, &classSub2);
    napi_set_named_property(env, exports, "Sub2", classSub2);

    //导出常量 TEST_CONST：白名单以无类型标注的 const 形态声明（B5）
    napi_value testConst = nullptr;
    napi_create_uint32(env, 42, &testConst);
    napi_set_named_property(env, exports, "TEST_CONST", testConst);

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
