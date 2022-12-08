/*
* Copyright (c) 2021 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License"); 
* you may not use this file except in compliance with the License. 
* You may obtain a copy of the License at 
*
* http://www.apache.org/licenses/LICENSE-2.0 
*
* Unless required by applicable law or agreed to in writing, software 
* distributed under the License is distributed on an "AS IS" BASIS, 
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. 
* See the License for the specific language governing permissions and 
* limitations under the License. 
*/
#include <cstring>
#include <string>
#include <memory>
#include <vector>
#include <node_api.h>
#include <any>
#include "tool_utility.h"
#include "napitest.h"

#define NUMBER_JS_2_C(napi_v, type, dest)      \
    if (typeid(type) == typeid(int32_t))  {    \
        dest = pxt->SwapJs2CInt32(napi_v);     \
    }                                          \
    else if (typeid(type) == typeid(uint32_t)){\
        dest = pxt->SwapJs2CUint32(napi_v);    \
    }                                          \
    else if (typeid(type) == typeid(int64_t)){ \
        dest = pxt->SwapJs2CInt64(napi_v);     \
    }                                          \
    else if (typeid(type) == typeid(double_t)){\
        dest = pxt->SwapJs2CDouble(napi_v);    \
    } 

#define NUMBER_JS_2_C_ENUM(napi_v, type, dest, enum_type)      \
    if (typeid(type) == typeid(int32_t))  {    \
        dest = static_cast<enum_type>(pxt->SwapJs2CInt32(napi_v));     \
    }                                          \
    else if (typeid(type) == typeid(uint32_t)){\
        dest = static_cast<enum_type>(pxt->SwapJs2CUint32(napi_v));    \
    }                                          \
    else if (typeid(type) == typeid(int64_t)){ \
        dest = static_cast<enum_type>(pxt->SwapJs2CInt64(napi_v));     \
    }                                          \
    else if (typeid(type) == typeid(double_t)){\
        dest = static_cast<enum_type>(pxt->SwapJs2CDouble(napi_v));    \
    } 
    
#define BOOLEAN_JS_2_C(napi_v, type, dest){    \
    dest = pxt->SwapC2JsBool(napi_v);          \
}

#define C_DELETE(p)  \
    if (p) {         \
        delete p;    \
    }

__attribute__((unused)) static napi_value number_c_to_js(XNapiTool *pxt, const std::type_info &n, void *num)
{
    if (n == typeid(int32_t))
        return pxt->SwapC2JsInt32(*(int32_t *)num);
    else if (n == typeid(uint32_t))
        return pxt->SwapC2JsUint32(*(uint32_t *)num);
    else if (n == typeid(int64_t))
        return pxt->SwapC2JsInt64(*(int64_t *)num);
    else if (n == typeid(double_t))
        return pxt->SwapC2JsDouble(*(double_t *)num);
    return nullptr;
}
#define NUMBER_C_2_JS(pxt, n) \
    number_c_to_js(pxt, typeid(n), &n)

namespace napitest {
class Test_middle {
public:
    static napi_value constructor(napi_env env, napi_callback_info info)
    {
        XNapiTool *pxt = new XNapiTool(env, info);

        Test *p = new Test();

        napi_value thisvar = pxt->WrapInstance(p, release);

        return thisvar;
    }
    static void release(void *p)
    {
        Test *p2 = (Test *)p;
        delete p2;
    }
    
    static napi_value getvalue_name(napi_env env, napi_callback_info info)
    {
        XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
        Test *p = (Test *)pxt->UnWarpInstance();
        napi_value result = nullptr;
        result = pxt->SwapC2JsUtf8(p->name.c_str());
        delete pxt;
        return result;
    }
    static napi_value setvalue_name(napi_env env, napi_callback_info info)
    {
        std::shared_ptr<XNapiTool> pxt = std::make_shared<XNapiTool>(env, info);
        Test *p = (Test *)pxt->UnWarpInstance();
        pxt->SwapJs2CUtf8(pxt->GetArgv(0), p->name);
        return nullptr;
    }

    static napi_value getvalue_age(napi_env env, napi_callback_info info)
    {
        XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
        Test *p = (Test *)pxt->UnWarpInstance();
        napi_value result = nullptr;
        result = NUMBER_C_2_JS(pxt, p->age);
        delete pxt;
        return result;
    }
    static napi_value setvalue_age(napi_env env, napi_callback_info info)
    {
        std::shared_ptr<XNapiTool> pxt = std::make_shared<XNapiTool>(env, info);
        Test *p = (Test *)pxt->UnWarpInstance();
        NUMBER_JS_2_C(pxt->GetArgv(0),NUMBER_TYPE_1,p->age);
        return nullptr;
    }

};
class Entry_middle {
public:
    static napi_value constructor(napi_env env, napi_callback_info info)
    {
        XNapiTool *pxt = new XNapiTool(env, info);

        Entry *p = new Entry();

        napi_value thisvar = pxt->WrapInstance(p, release);

        return thisvar;
    }
    static void release(void *p)
    {
        Entry *p2 = (Entry *)p;
        delete p2;
    }
    
    static napi_value getvalue_key(napi_env env, napi_callback_info info)
    {
        XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
        Entry *p = (Entry *)pxt->UnWarpInstance();
        napi_value result = nullptr;
        pxt->CreateArray(result);
    uint32_t outLen1 = p->key.size();
    for(uint32_t i = 0; i < outLen1; i++) {
        napi_value tnv1 = nullptr;
        tnv1 = NUMBER_C_2_JS(pxt,p->key[i]);
        pxt->SetArrayElement(result, i, tnv1);
    }
        delete pxt;
        return result;
    }
    static napi_value setvalue_key(napi_env env, napi_callback_info info)
    {
        std::shared_ptr<XNapiTool> pxt = std::make_shared<XNapiTool>(env, info);
        Entry *p = (Entry *)pxt->UnWarpInstance();
            uint32_t len1=pxt->GetArrayLength(pxt->GetArgv(0));
    for(uint32_t i1=0;i1<len1;i1++) {
        NUMBER_TYPE_2 tt1;
        NUMBER_JS_2_C(pxt->GetArrayElement(pxt->GetArgv(0),i1),NUMBER_TYPE_2,tt1);
        p->key.push_back(tt1);

    }

        return nullptr;
    }

    static napi_value getvalue_value(napi_env env, napi_callback_info info)
    {
        XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
        Entry *p = (Entry *)pxt->UnWarpInstance();
        napi_value result = nullptr;
        pxt->CreateArray(result);
    uint32_t outLen1 = p->value.size();
    for(uint32_t i = 0; i < outLen1; i++) {
        napi_value tnv1 = nullptr;
        tnv1 = pxt->SwapC2JsUtf8(p->value[i].c_str());
        pxt->SetArrayElement(result, i, tnv1);
    }
        delete pxt;
        return result;
    }
    static napi_value setvalue_value(napi_env env, napi_callback_info info)
    {
        std::shared_ptr<XNapiTool> pxt = std::make_shared<XNapiTool>(env, info);
        Entry *p = (Entry *)pxt->UnWarpInstance();
            uint32_t len2=pxt->GetArrayLength(pxt->GetArgv(0));
    for(uint32_t i2=0;i2<len2;i2++) {
        std::string tt2;
        pxt->SwapJs2CUtf8(pxt->GetArrayElement(pxt->GetArgv(0),i2), tt2);
        p->value.push_back(tt2);

    }

        return nullptr;
    }

    static napi_value getvalue_isExit(napi_env env, napi_callback_info info)
    {
        XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
        Entry *p = (Entry *)pxt->UnWarpInstance();
        napi_value result = nullptr;
        pxt->CreateArray(result);
    uint32_t outLen1 = p->isExit.size();
    for(uint32_t i = 0; i < outLen1; i++) {
        napi_value tnv1 = nullptr;
        tnv1 = pxt->SwapC2JsBool(p->isExit[i]);
        pxt->SetArrayElement(result, i, tnv1);
    }
        delete pxt;
        return result;
    }
    static napi_value setvalue_isExit(napi_env env, napi_callback_info info)
    {
        std::shared_ptr<XNapiTool> pxt = std::make_shared<XNapiTool>(env, info);
        Entry *p = (Entry *)pxt->UnWarpInstance();
            uint32_t len3=pxt->GetArrayLength(pxt->GetArgv(0));
    for(uint32_t i3=0;i3<len3;i3++) {
        bool tt3;
        tt3 = pxt->SwapJs2CBool(pxt->GetArrayElement(pxt->GetArgv(0),i3));
        p->isExit.push_back(tt3);

    }

        return nullptr;
    }

};
class Test1_middle {
public:
    static napi_value constructor(napi_env env, napi_callback_info info)
    {
        XNapiTool *pxt = new XNapiTool(env, info);

        Test1 *p = new Test1();

        napi_value thisvar = pxt->WrapInstance(p, release);

        return thisvar;
    }
    static void release(void *p)
    {
        Test1 *p2 = (Test1 *)p;
        delete p2;
    }
    
struct $listInputMethod_value_struct {
    std::string in0;
    
    
};

static napi_value $listInputMethod_middle(napi_env env, napi_callback_info info)
{
    XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
    if (pxt->IsFailed()) {
        napi_value err = pxt->GetError();
        delete pxt;
        return err;
    }
    Test1 *pInstance = (Test1 *)pxt->UnWarpInstance();

    struct $listInputMethod_value_struct *vio = new $listInputMethod_value_struct();
    
    pxt->SwapJs2CUtf8(pxt->GetArgv(0), vio->in0);
    

    pInstance->$listInputMethod(vio->in0);

    napi_value result = nullptr;
    result = pxt->UndefinedValue();

    

    delete vio;
    if (pxt->IsFailed()) {
        result = pxt->GetError();
    }
    
    delete pxt; // release
    return result;
}
};
class Person_middle {
public:
    static napi_value constructor(napi_env env, napi_callback_info info)
    {
        XNapiTool *pxt = new XNapiTool(env, info);

        Person *p = new Person();

        napi_value thisvar = pxt->WrapInstance(p, release);

        return thisvar;
    }
    static void release(void *p)
    {
        Person *p2 = (Person *)p;
        delete p2;
    }
    
struct $getMaxAge_value_struct {
    std::string in0;
    
    NUMBER_TYPE_7 out;
};

static napi_value $getMaxAge_middle(napi_env env, napi_callback_info info)
{
    XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
    if (pxt->IsFailed()) {
        napi_value err = pxt->GetError();
        delete pxt;
        return err;
    }
    Person *pInstance = (Person *)pxt->UnWarpInstance();

    struct $getMaxAge_value_struct *vio = new $getMaxAge_value_struct();
    
    pxt->SwapJs2CUtf8(pxt->GetArgv(0), vio->in0);
    

    pInstance->$getMaxAge(vio->in0, vio->out);

    napi_value result = nullptr;
    result = NUMBER_C_2_JS(pxt, vio->out);

    

    delete vio;
    if (pxt->IsFailed()) {
        result = pxt->GetError();
    }
    
    delete pxt; // release
    return result;
}
};
struct testArray_value_struct {
    std::vector<std::string> in0;
    
    std::vector<std::string> out;
};

napi_value testArray_middle(napi_env env, napi_callback_info info)
{
    XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
    if (pxt->IsFailed()) {
        napi_value err = pxt->GetError();
        delete pxt;
        return err;
    }
    

    struct testArray_value_struct *vio = new testArray_value_struct();
    
        uint32_t len4=pxt->GetArrayLength(pxt->GetArgv(0));
    for(uint32_t i4=0;i4<len4;i4++) {
        std::string tt4;
        pxt->SwapJs2CUtf8(pxt->GetArrayElement(pxt->GetArgv(0),i4), tt4);
        vio->in0.push_back(tt4);

    }


    testArray(vio->in0, vio->out);

    napi_value result = nullptr;
    pxt->CreateArray(result);
    uint32_t outLen1 = vio->out.size();
    for(uint32_t i = 0; i < outLen1; i++) {
        napi_value tnv1 = nullptr;
        tnv1 = pxt->SwapC2JsUtf8(vio->out[i].c_str());
        pxt->SetArrayElement(result, i, tnv1);
    }

    

    delete vio;
    if (pxt->IsFailed()) {
        result = pxt->GetError();
    }
    
    delete pxt; // release
    return result;
}
struct testArray1_value_struct {
    std::vector<NUMBER_TYPE_3> in0;
    
    std::vector<NUMBER_TYPE_4> out;
};

napi_value testArray1_middle(napi_env env, napi_callback_info info)
{
    XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
    if (pxt->IsFailed()) {
        napi_value err = pxt->GetError();
        delete pxt;
        return err;
    }
    

    struct testArray1_value_struct *vio = new testArray1_value_struct();
    
        uint32_t len5=pxt->GetArrayLength(pxt->GetArgv(0));
    for(uint32_t i5=0;i5<len5;i5++) {
        NUMBER_TYPE_3 tt5;
        NUMBER_JS_2_C(pxt->GetArrayElement(pxt->GetArgv(0),i5),NUMBER_TYPE_3,tt5);
        vio->in0.push_back(tt5);

    }


    testArray1(vio->in0, vio->out);

    napi_value result = nullptr;
    pxt->CreateArray(result);
    uint32_t outLen1 = vio->out.size();
    for(uint32_t i = 0; i < outLen1; i++) {
        napi_value tnv1 = nullptr;
        tnv1 = NUMBER_C_2_JS(pxt,vio->out[i]);
        pxt->SetArrayElement(result, i, tnv1);
    }

    

    delete vio;
    if (pxt->IsFailed()) {
        result = pxt->GetError();
    }
    
    delete pxt; // release
    return result;
}
struct testArray2_value_struct {
    std::vector<bool> in0;
    
    std::vector<bool> out;
};

napi_value testArray2_middle(napi_env env, napi_callback_info info)
{
    XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
    if (pxt->IsFailed()) {
        napi_value err = pxt->GetError();
        delete pxt;
        return err;
    }
    

    struct testArray2_value_struct *vio = new testArray2_value_struct();
    
        uint32_t len6=pxt->GetArrayLength(pxt->GetArgv(0));
    for(uint32_t i6=0;i6<len6;i6++) {
        bool tt6;
        tt6 = pxt->SwapJs2CBool(pxt->GetArrayElement(pxt->GetArgv(0),i6));
        vio->in0.push_back(tt6);

    }


    testArray2(vio->in0, vio->out);

    napi_value result = nullptr;
    pxt->CreateArray(result);
    uint32_t outLen1 = vio->out.size();
    for(uint32_t i = 0; i < outLen1; i++) {
        napi_value tnv1 = nullptr;
        tnv1 = pxt->SwapC2JsBool(vio->out[i]);
        pxt->SetArrayElement(result, i, tnv1);
    }

    

    delete vio;
    if (pxt->IsFailed()) {
        result = pxt->GetError();
    }
    
    delete pxt; // release
    return result;
}
struct fun1_value_struct {
    std::vector<std::string> in0;
    std::vector<Test> in1;
    
    std::vector<NUMBER_TYPE_5> out;
};

napi_value fun1_middle(napi_env env, napi_callback_info info)
{
    XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
    if (pxt->IsFailed()) {
        napi_value err = pxt->GetError();
        delete pxt;
        return err;
    }
    

    struct fun1_value_struct *vio = new fun1_value_struct();
    
        uint32_t len7=pxt->GetArrayLength(pxt->GetArgv(0));
    for(uint32_t i7=0;i7<len7;i7++) {
        std::string tt7;
        pxt->SwapJs2CUtf8(pxt->GetArrayElement(pxt->GetArgv(0),i7), tt7);
        vio->in0.push_back(tt7);

    }
    uint32_t len8=pxt->GetArrayLength(pxt->GetArgv(1));
    for(uint32_t i8=0;i8<len8;i8++) {
        Test tt8;
        napi_value tnv9 = pxt->GetValueProperty(pxt->GetArrayElement(pxt->GetArgv(1),i8), "name");
    if(tnv9!=nullptr){pxt->SwapJs2CUtf8(tnv9,tt8.name);}napi_value tnv10 = pxt->GetValueProperty(pxt->GetArrayElement(pxt->GetArgv(1),i8), "age");
    if(tnv10!=nullptr){NUMBER_JS_2_C(tnv10,NUMBER_TYPE_1,tt8.age);}
        vio->in1.push_back(tt8);

    }


    fun1(vio->in0, vio->in1, vio->out);

    napi_value result = nullptr;
    pxt->CreateArray(result);
    uint32_t outLen1 = vio->out.size();
    for(uint32_t i = 0; i < outLen1; i++) {
        napi_value tnv1 = nullptr;
        tnv1 = NUMBER_C_2_JS(pxt,vio->out[i]);
        pxt->SetArrayElement(result, i, tnv1);
    }

    

    delete vio;
    if (pxt->IsFailed()) {
        result = pxt->GetError();
    }
    
    delete pxt; // release
    return result;
}
struct fun2_value_struct {
    std::vector<NUMBER_TYPE_6> in0;
    std::vector<Test> in1;
    
    std::vector<std::string> out;
};

napi_value fun2_middle(napi_env env, napi_callback_info info)
{
    XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
    if (pxt->IsFailed()) {
        napi_value err = pxt->GetError();
        delete pxt;
        return err;
    }
    

    struct fun2_value_struct *vio = new fun2_value_struct();
    
        uint32_t len11=pxt->GetArrayLength(pxt->GetArgv(0));
    for(uint32_t i11=0;i11<len11;i11++) {
        NUMBER_TYPE_6 tt11;
        NUMBER_JS_2_C(pxt->GetArrayElement(pxt->GetArgv(0),i11),NUMBER_TYPE_6,tt11);
        vio->in0.push_back(tt11);

    }
    uint32_t len12=pxt->GetArrayLength(pxt->GetArgv(1));
    for(uint32_t i12=0;i12<len12;i12++) {
        Test tt12;
        napi_value tnv13 = pxt->GetValueProperty(pxt->GetArrayElement(pxt->GetArgv(1),i12), "name");
    if(tnv13!=nullptr){pxt->SwapJs2CUtf8(tnv13,tt12.name);}napi_value tnv14 = pxt->GetValueProperty(pxt->GetArrayElement(pxt->GetArgv(1),i12), "age");
    if(tnv14!=nullptr){NUMBER_JS_2_C(tnv14,NUMBER_TYPE_1,tt12.age);}
        vio->in1.push_back(tt12);

    }


    fun2(vio->in0, vio->in1, vio->out);

    napi_value result = nullptr;
    pxt->CreateArray(result);
    uint32_t outLen1 = vio->out.size();
    for(uint32_t i = 0; i < outLen1; i++) {
        napi_value tnv1 = nullptr;
        tnv1 = pxt->SwapC2JsUtf8(vio->out[i].c_str());
        pxt->SetArrayElement(result, i, tnv1);
    }

    

    delete vio;
    if (pxt->IsFailed()) {
        result = pxt->GetError();
    }
    
    delete pxt; // release
    return result;
}
struct fun3_value_struct {
    std::vector<bool> in0;
    std::vector<Test> in1;
    
    std::vector<bool> out;
};

napi_value fun3_middle(napi_env env, napi_callback_info info)
{
    XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
    if (pxt->IsFailed()) {
        napi_value err = pxt->GetError();
        delete pxt;
        return err;
    }
    

    struct fun3_value_struct *vio = new fun3_value_struct();
    
        uint32_t len15=pxt->GetArrayLength(pxt->GetArgv(0));
    for(uint32_t i15=0;i15<len15;i15++) {
        bool tt15;
        tt15 = pxt->SwapJs2CBool(pxt->GetArrayElement(pxt->GetArgv(0),i15));
        vio->in0.push_back(tt15);

    }
    uint32_t len16=pxt->GetArrayLength(pxt->GetArgv(1));
    for(uint32_t i16=0;i16<len16;i16++) {
        Test tt16;
        napi_value tnv17 = pxt->GetValueProperty(pxt->GetArrayElement(pxt->GetArgv(1),i16), "name");
    if(tnv17!=nullptr){pxt->SwapJs2CUtf8(tnv17,tt16.name);}napi_value tnv18 = pxt->GetValueProperty(pxt->GetArrayElement(pxt->GetArgv(1),i16), "age");
    if(tnv18!=nullptr){NUMBER_JS_2_C(tnv18,NUMBER_TYPE_1,tt16.age);}
        vio->in1.push_back(tt16);

    }


    fun3(vio->in0, vio->in1, vio->out);

    napi_value result = nullptr;
    pxt->CreateArray(result);
    uint32_t outLen1 = vio->out.size();
    for(uint32_t i = 0; i < outLen1; i++) {
        napi_value tnv1 = nullptr;
        tnv1 = pxt->SwapC2JsBool(vio->out[i]);
        pxt->SetArrayElement(result, i, tnv1);
    }

    

    delete vio;
    if (pxt->IsFailed()) {
        result = pxt->GetError();
    }
    
    delete pxt; // release
    return result;
}
struct fun4_value_struct {
    std::string in0;
    std::vector<Entry> in1;
    
    std::vector<Entry> out;
};

napi_value fun4_middle(napi_env env, napi_callback_info info)
{
    XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
    if (pxt->IsFailed()) {
        napi_value err = pxt->GetError();
        delete pxt;
        return err;
    }
    

    struct fun4_value_struct *vio = new fun4_value_struct();
    
    pxt->SwapJs2CUtf8(pxt->GetArgv(0), vio->in0);
        uint32_t len19=pxt->GetArrayLength(pxt->GetArgv(1));
    for(uint32_t i19=0;i19<len19;i19++) {
        Entry tt19;
            uint32_t len20=pxt->GetArrayLength(pxt->GetValueProperty(pxt->GetArrayElement(pxt->GetArgv(1),i19), "key"));
    for(uint32_t i20=0;i20<len20;i20++) {
        NUMBER_TYPE_2 tt20;
        NUMBER_JS_2_C(pxt->GetArrayElement(pxt->GetValueProperty(pxt->GetArrayElement(pxt->GetArgv(1),i19), "key"),i20),NUMBER_TYPE_2,tt20);
        tt19.key.push_back(tt20);

    }
    uint32_t len21=pxt->GetArrayLength(pxt->GetValueProperty(pxt->GetArrayElement(pxt->GetArgv(1),i19), "value"));
    for(uint32_t i21=0;i21<len21;i21++) {
        std::string tt21;
        pxt->SwapJs2CUtf8(pxt->GetArrayElement(pxt->GetValueProperty(pxt->GetArrayElement(pxt->GetArgv(1),i19), "value"),i21), tt21);
        tt19.value.push_back(tt21);

    }
    uint32_t len22=pxt->GetArrayLength(pxt->GetValueProperty(pxt->GetArrayElement(pxt->GetArgv(1),i19), "isExit"));
    for(uint32_t i22=0;i22<len22;i22++) {
        bool tt22;
        tt22 = pxt->SwapJs2CBool(pxt->GetArrayElement(pxt->GetValueProperty(pxt->GetArrayElement(pxt->GetArgv(1),i19), "isExit"),i22));
        tt19.isExit.push_back(tt22);

    }

        vio->in1.push_back(tt19);

    }


    fun4(vio->in0, vio->in1, vio->out);

    napi_value result = nullptr;
    pxt->CreateArray(result);
    uint32_t outLen1 = vio->out.size();
    for(uint32_t i = 0; i < outLen1; i++) {
        napi_value tnv1 = nullptr;
        {
napi_value tnv2 = nullptr;
pxt->CreateArray(tnv2);
    uint32_t outLen3 = vio->out[i].key.size();
    for(uint32_t i = 0; i < outLen3; i++) {
        napi_value tnv3 = nullptr;
        tnv3 = NUMBER_C_2_JS(pxt,vio->out[i].key[i]);
        pxt->SetArrayElement(tnv2, i, tnv3);
    }
pxt->SetValueProperty(tnv1,"key",tnv2);
}
{
napi_value tnv2 = nullptr;
pxt->CreateArray(tnv2);
    uint32_t outLen3 = vio->out[i].value.size();
    for(uint32_t i = 0; i < outLen3; i++) {
        napi_value tnv3 = nullptr;
        tnv3 = pxt->SwapC2JsUtf8(vio->out[i].value[i].c_str());
        pxt->SetArrayElement(tnv2, i, tnv3);
    }
pxt->SetValueProperty(tnv1,"value",tnv2);
}
{
napi_value tnv2 = nullptr;
pxt->CreateArray(tnv2);
    uint32_t outLen3 = vio->out[i].isExit.size();
    for(uint32_t i = 0; i < outLen3; i++) {
        napi_value tnv3 = nullptr;
        tnv3 = pxt->SwapC2JsBool(vio->out[i].isExit[i]);
        pxt->SetArrayElement(tnv2, i, tnv3);
    }
pxt->SetValueProperty(tnv1,"isExit",tnv2);
}

        pxt->SetArrayElement(result, i, tnv1);
    }

    

    delete vio;
    if (pxt->IsFailed()) {
        result = pxt->GetError();
    }
    
    delete pxt; // release
    return result;
}
struct $fun5_value_struct {
    std::string in0;
    
    std::string out;
};

napi_value $fun5_middle(napi_env env, napi_callback_info info)
{
    XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
    if (pxt->IsFailed()) {
        napi_value err = pxt->GetError();
        delete pxt;
        return err;
    }
    

    struct $fun5_value_struct *vio = new $fun5_value_struct();
    
    pxt->SwapJs2CUtf8(pxt->GetArgv(0), vio->in0);
    

    $fun5(vio->in0, vio->out);

    napi_value result = nullptr;
    result = pxt->SwapC2JsUtf8(vio->out.c_str());

    

    delete vio;
    if (pxt->IsFailed()) {
        result = pxt->GetError();
    }
    
    delete pxt; // release
    return result;
}
struct fun6_value_struct {
    Test1 in0;
    
    NUMBER_TYPE_8 out;
};

napi_value fun6_middle(napi_env env, napi_callback_info info)
{
    XNapiTool *pxt = std::make_unique<XNapiTool>(env, info).release();
    if (pxt->IsFailed()) {
        napi_value err = pxt->GetError();
        delete pxt;
        return err;
    }
    

    struct fun6_value_struct *vio = new fun6_value_struct();
    
    
    

    fun6(vio->in0, vio->out);

    napi_value result = nullptr;
    result = NUMBER_C_2_JS(pxt, vio->out);

    

    delete vio;
    if (pxt->IsFailed()) {
        result = pxt->GetError();
    }
    
    delete pxt; // release
    return result;
}}

static napi_value init(napi_env env, napi_value exports)
{
    std::shared_ptr<XNapiTool> pxt = std::make_shared<XNapiTool>(env, exports);

    {
    std::map<const char *,std::map<const char *,napi_callback>> valueList;
    valueList["name"]["getvalue"]=napitest::Test_middle::getvalue_name;
    valueList["name"]["setvalue"]=napitest::Test_middle::setvalue_name;
    valueList["age"]["getvalue"]=napitest::Test_middle::getvalue_age;
    valueList["age"]["setvalue"]=napitest::Test_middle::setvalue_age;
    std::map<const char *, napi_callback> funcList;
    pxt->DefineClass("Test", napitest::Test_middle::constructor, valueList ,funcList);
}
{
    std::map<const char *,std::map<const char *,napi_callback>> valueList;
    valueList["key"]["getvalue"]=napitest::Entry_middle::getvalue_key;
    valueList["key"]["setvalue"]=napitest::Entry_middle::setvalue_key;
    valueList["value"]["getvalue"]=napitest::Entry_middle::getvalue_value;
    valueList["value"]["setvalue"]=napitest::Entry_middle::setvalue_value;
    valueList["isExit"]["getvalue"]=napitest::Entry_middle::getvalue_isExit;
    valueList["isExit"]["setvalue"]=napitest::Entry_middle::setvalue_isExit;
    std::map<const char *, napi_callback> funcList;
    pxt->DefineClass("Entry", napitest::Entry_middle::constructor, valueList ,funcList);
}
{
    std::map<const char *,std::map<const char *,napi_callback>> valueList;
    std::map<const char *, napi_callback> funcList;
    funcList["$listInputMethod"] = napitest::Test1_middle::$listInputMethod_middle;
    pxt->DefineClass("Test1", napitest::Test1_middle::constructor, valueList ,funcList);
}
{
    std::map<const char *,std::map<const char *,napi_callback>> valueList;
    std::map<const char *, napi_callback> funcList;
    funcList["$getMaxAge"] = napitest::Person_middle::$getMaxAge_middle;
    pxt->DefineClass("Person", napitest::Person_middle::constructor, valueList ,funcList);
}
    pxt->DefineFunction("testArray", napitest::testArray_middle);
    pxt->DefineFunction("testArray1", napitest::testArray1_middle);
    pxt->DefineFunction("testArray2", napitest::testArray2_middle);
    pxt->DefineFunction("fun1", napitest::fun1_middle);
    pxt->DefineFunction("fun2", napitest::fun2_middle);
    pxt->DefineFunction("fun3", napitest::fun3_middle);
    pxt->DefineFunction("fun4", napitest::fun4_middle);
    pxt->DefineFunction("$fun5", napitest::$fun5_middle);
    pxt->DefineFunction("fun6", napitest::fun6_middle);


    return exports;
}

static napi_module g_napitest_Module = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = init,
    .nm_modname = "test",
    .nm_priv = ((void *)0),
    .reserved = {(void *)0},
};

extern "C" __attribute__((constructor)) void Register_napitest_Module(void)
{
    napi_module_register(&g_napitest_Module);
}
