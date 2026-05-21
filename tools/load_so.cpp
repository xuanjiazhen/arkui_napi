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

#include "load_so.h"

#include <iostream>
#include <fstream>
#include <dlfcn.h>
#include <cstring>

#include "ark_native_engine.h"
#include "check_helper.h"
#include "jsnapi.h"
#include "native_utils.h"

namespace ohos::check {

CheckErrorCode LoadSoAndGetExposedMeta(const std::string& soPath, std::vector<ApiMeta>& exposedMeta) {
    // 创建轻量NAPI环境
    napi_env env = CreateArkEnv();
    if (env == nullptr) {
        return ERROR_JS_ENGINE_INIT;
    }

    napi_status status;
    napi_value global;
    status = napi_get_global(env, &global);
    if (status != napi_ok) {
        return ERROR_JS_EXECUTE_FAILED;
    }

    napi_value requireNapi;
    status = napi_get_named_property(env, global, "requireNapi", &requireNapi);
    if (status != napi_ok) {
        return ERROR_JS_EXECUTE_FAILED;
    }

    napi_value soName;
    status = napi_create_string_utf8(env, soPath.c_str(), soPath.length(), &soName);
    if (status != napi_ok) {
        return ERROR_JS_EXECUTE_FAILED;
    }

    napi_value exports;
    static const uint32_t argc = 1;
    napi_value args[argc] = { soName };
    status = napi_call_function(env, global, requireNapi, argc, args, &exports);
    if (status != napi_ok) {
        return ERROR_JS_EXECUTE_FAILED;
    }

    // 遍历导出
    TraverseExports(env, exports, "", exposedMeta);

    std::cout << "TraverseExports: " << soPath << " end!" << std::endl;

    DestroyArkEnv(env);
    return CHECK_OK;
}

}