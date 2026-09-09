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

#ifndef LOAD_SO_H
#define LOAD_SO_H

#include <set>
#include <string>
#include <vector>

#include "api_meta.h"
#include "error_codes.h"
#include "napi/native_api.h"

namespace ohos::check {

/**
 * @brief 计算传给 requireNapi 的模块名。
 * 镜像真机加载链的映射规则（加载器 GetNativeModulePath 把参数中所有点转
 * 成目录，最后一段拼 so 名）：
 *  - '@ohos.taskpool'      -> 'taskpool'        （/system/lib[64]/module/libtaskpool.z.so）
 *  - '@ohos.arkui.node'    -> 'arkui.node'      （module/arkui/libnode.z.so，点号转目录）
 *  - '@system.prompt'      -> 'prompt'
 *  - '@hms.x.y'            -> 'hms.x.y'         （module/hms/x/liby.z.so，保留全名）
 *  - '@arkts.collections'  -> 'arkts.collections'（module/arkts/libcollections.z.so，保留全名）
 *  - 特例集合（GetOhmurl 的 NATIVE_MODULE）：'@system.app' -> 'system.app'、
 *    '@ohos.curves' -> 'ohos.curves' 等，保留全名参与路径拼接。
 * @hms/@arkts 若剥首段（旧实现的错误）会拼出不存在的目录：实测
 * 'core.ar.arengine' -> module/core/ar/... 加载必败，保留全名才是真机路径。
 * @param file 模块名（如 @ohos.taskpool），需以 '@' 开头
 * @return requireNapi 参数；非 '@' 开头返回原串
 */
std::string ModuleNameForRequireNapi(const std::string& file);

// requireNapi 加载参数：kit 用于归属统计，displayName 参与导出面 file 匹配，
// requireName 为传给 requireNapi 的实际模块名（ModuleNameForRequireNapi 的结果）
struct SoLoadParam {
    std::string kit;
    std::string displayName;
    std::string requireName;
};

/**
 * @brief 通过 requireNapi 加载native模块并提取导出API元数据
 * @param param 加载参数（kit / displayName / requireName）
 * @param exposedMeta 输出：apiText + apiType + className
 * @param expandableNames 需要展开成员的已声明命名空间/枚举名集合，可为 nullptr
 * @return 错误码
 */
CheckErrorCode LoadSoAndGetExposedMeta(
    napi_env env,
    const SoLoadParam& param,
    std::vector<ApiMeta>& exposedMeta,
    const std::set<std::string>* expandableNames = nullptr
);

}

#endif
