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

#include "load_abc.h"

#include <climits>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

#include "ark_native_engine.h"
#include "check_helper.h"
#include "jsnapi.h"
#include "native_utils.h"

namespace ohos::check {

CheckErrorCode LoadJSAndGetExposedMeta(
    napi_env env,
    const std::string& kit,
    const std::string& jsFilePath,
    std::vector<ApiMeta>& exposedMeta,
    const std::set<std::string>* expandableNames
) {
    // 统一转绝对路径，ExecuteForAbsolutePath/GetExportsFromFile 均按文件路径加载
    char absBuf[PATH_MAX] = {0};
    if (realpath(jsFilePath.c_str(), absBuf) == nullptr) {
        LOG_WARN("LoadJSAndGetExposedMeta: file not found: " << jsFilePath);
        return ERROR_FILE_OPEN_FAILED;
    }
    std::string absPath = absBuf;

    NativeEngine *engine = reinterpret_cast<NativeEngine *>(env);
    const EcmaVM *vm = engine->GetEcmaVm();

    // 入口候选：
    // 1) 未merge的abc（es2abc --module 单文件产物）：CheckAndGetRecordInfo 对 bundle-pack
    //    返回首条记录，任意入口可执行，但模块按文件名注册，入口必须与文件路径一致；
    // 2) --merge-abc 产物：记录名为输出文件名去掉扩展名，需按记录名精确匹配。
    std::string baseName = absPath.substr(absPath.find_last_of('/') + 1);
    size_t dotPos = baseName.find_last_of('.');
    std::string mergedEntry = (dotPos == std::string::npos) ? baseName : baseName.substr(0, dotPos);
    std::string candidates[] = {absPath, mergedEntry};

    for (const auto& entry : candidates) {
        if (entry.empty()) {
            continue;
        }
        bool ret = panda::JSNApi::ExecuteForAbsolutePath(vm, absPath, entry);
        if (!ret) {
            ClearPendingException(env);
            continue;
        }
        LOG_DEBUG("ExecuteForAbsolutePath: " << absPath << " entry: " << entry);

        Local<panda::ObjectRef> obj =
            panda::JSNApi::GetExportsFromFile(const_cast<panda::EcmaVM *>(vm), absPath, entry);
        ClearPendingException(env);
        if (obj->IsUndefined()) {
            continue;
        }

        napi_value result = JsValueFromLocalValue(obj);

        // 遍历导出结果 自动生成apimeta
        TraverseContext ctx;
        ctx.env = env;
        ctx.kit = kit;
        ctx.file = absPath;
        ctx.expandableNames = expandableNames;
        ctx.outMeta = &exposedMeta;
        TraverseExports(ctx, result, "");

        LOG_DEBUG("TraverseExports: " << absPath << " end!");
        return CHECK_OK;
    }

    LOG_ERROR("LoadJSAndGetExposedMeta failed: " << absPath);
    return ERROR_JS_EXECUTE_FAILED;
}

}
