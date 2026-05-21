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

#include "load_abc.h"

#include <fstream>
#include <iostream>

#include "ark_native_engine.h"
#include "check_helper.h"
#include "jsnapi.h"
#include "native_utils.h"

namespace ohos::check {

CheckErrorCode LoadJSAndGetExposedMeta(
    const std::string& jsFilePath,
    std::vector<ApiMeta>& exposedMeta
) {
    napi_env env = CreateArkEnv();
    if (env == nullptr) {
        return ERROR_JS_ENGINE_INIT;
    }

    NativeEngine *engine = reinterpret_cast<NativeEngine *>(env);
    auto vm = engine->GetEcmaVm();
    bool ret = panda::JSNApi::ExecuteForAbsolutePath(vm, jsFilePath, jsFilePath);
    if (!ret) {
        DestroyArkEnv(env);
        return ERROR_JS_EXECUTE_FAILED;
    }

    std::cout << "ExecuteForAbsolutePath: " << jsFilePath << std::endl;

    Local<panda::ObjectRef> obj = panda::JSNApi::GetExportsFromFile(const_cast<panda::EcmaVM *>(vm), jsFilePath, jsFilePath);

    std::cout << "GetExportObjectFromBuffer: " << jsFilePath << std::endl;

    napi_value result = JsValueFromLocalValue(obj);

    // 遍历导出结果 → 自动生成ApiMeta
    TraverseExports(env, result, "", exposedMeta);

    std::cout << "TraverseExports: " << jsFilePath << " end!" << std::endl;

    DestroyArkEnv(env);
    return CHECK_OK;
}

}