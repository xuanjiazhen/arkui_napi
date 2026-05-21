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

#include "check_helper.h"

#include <iostream>

#include "ark_native_engine.h"
#include "check_helper.h"
#include "jsnapi.h"
#include "native_utils.h"

namespace ohos::check {

napi_env CreateArkEnv() {
    panda::RuntimeOption option;
    option.SetLogLevel(panda::RuntimeOption::LOG_LEVEL::INFO);
    EcmaVM* vm = panda::JSNApi::CreateJSVM(option);
    if (vm == nullptr) {
        return nullptr;
    }
    ArkNativeEngine* arkEngine = new ArkNativeEngine(vm, nullptr);
    auto cleanEnv = [vm]() {
        if (vm != nullptr) {
            panda::JSNApi::DestroyJSVM(vm);
        }
    };
    arkEngine->SetCleanEnv(cleanEnv);
    napi_env env = reinterpret_cast<napi_env>(arkEngine);
    return env;
}

void DestroyArkEnv(napi_env env) {
    if (env != nullptr) {
        napi_destroy_runtime(env);
    }
}

// 忽略内置属性名单
bool IsIgnoredProperty(const char* name) {
    if (!name || strlen(name) == 0) return true;
    return strcmp(name, "__proto__") == 0
        || strcmp(name, "constructor") == 0
        || strcmp(name, "length") == 0
        || strcmp(name, "name") == 0
        || strcmp(name, "prototype") == 0
        || strcmp(name, "toString") == 0
        || strcmp(name, "toLocaleString") == 0
        || strcmp(name, "valueOf") == 0
        || strcmp(name, "hasOwnProperty") == 0
        || strcmp(name, "isPrototypeOf") == 0
        || strcmp(name, "propertyIsEnumerable") == 0;
}

// 从NAPI值获取类型
std::string GetApiType(napi_env env, napi_value value) {
    napi_valuetype type;
    napi_typeof(env, value, &type);

    if (type == napi_function) {
        NativeEngine *engine = reinterpret_cast<NativeEngine *>(env);
        auto vm = engine->GetEcmaVm();
        auto jsVal = LocalValueFromJsValue(value);
        if (jsVal->IsClassConstructor(vm)) {
            return "class";
        }

        return "method";
    }

    return "field";
}

// 递归遍历导出对象
void TraverseExports(
    napi_env env,
    napi_value exports,
    const std::string& parent_class,
    std::vector<ApiMeta>& out_meta
) {
    napi_status status;

    NativeEngine *engine = reinterpret_cast<NativeEngine *>(env);
    auto vm = engine->GetEcmaVm();
    auto jsVal = LocalValueFromJsValue(exports);
    auto retP = Local<panda::ObjectRef>(jsVal)->GetOwnPropertyNames(vm);
    napi_value properties = JsValueFromLocalValue(retP);

    uint32_t count = 0;
    status = napi_get_array_length(env, properties, &count);
    if (status != napi_ok) {
        std::cout << "napi_get_array_length fail" << std::endl;
        return;
    }
    
    for (uint32_t i = 0; i < count; ++i) {
        napi_value name_val;
        napi_get_element(env, properties, i, &name_val);

        char name_buf[256] = {0};
        size_t len = 0;
        napi_get_value_string_utf8(env, name_val, name_buf, sizeof(name_buf)-1, &len);

        if (IsIgnoredProperty(name_buf)) {
            std::cout << "TraverseExports: " << name_buf << " is ignored!" << std::endl;
            continue;
        }

        std::string key = name_buf;
        napi_value value;
        napi_get_property(env, exports, name_val, &value);

        std::string type = GetApiType(env, value);

        // 加入API元数据
        ApiMeta meta;
        meta.apiText = key;
        meta.apiType = type;
        meta.className = parent_class;
        out_meta.push_back(meta);

        std::cout << "TraverseExports: " << key << ", type: " << type << " parent: " << parent_class << std::endl;
        // 递归遍历class内部成员
        if (type == "class") {
            TraverseExports(env, value, key, out_meta);
            napi_value prototype;
            napi_get_named_property(env, value, "prototype", &prototype);
            TraverseExports(env, prototype, key, out_meta);
        }
    }
}

// 判断字符串是否以指定前缀开头
bool StartsWith(const std::string& str, const std::string& prefix)
{
    if (prefix.empty()) return true;
    if (str.size() < prefix.size()) return false;
    return str.compare(0, prefix.size(), prefix) == 0;
}

// 判断字符串是否以指定后缀结尾
bool EndsWith(const std::string& str, const std::string& suffix) {
    // 后缀比原串长 → 直接返回 false
    if (suffix.size() > str.size()) {
        return false;
    }
    // 对比最后 N 个字符
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

}