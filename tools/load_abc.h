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

#ifndef LOAD_ABC_H
#define LOAD_ABC_H

#include <set>
#include <string>
#include <vector>

#include "api_meta.h"
#include "error_codes.h"
#include "napi/native_api.h"

namespace ohos::check {

// ABC 加载参数（与 SoLoadParam 同模式，收敛函数参数个数）
// kit: ApiMeta.kit 字段；displayName: ApiMeta.file 字段（模块名或 abc 路径）；
// requireName: requireNapi 参数（符号名与记录名候选的基础，混合 so 用）；
// abcFilePath: 独立 abc 文件绝对路径（仅 LoadAbcFileAndGetExposedMeta 用）；
// soPath: so 文件路径（仅 LoadMixedSoAbcAndGetExposedMeta 用）；
// expandableNames: 需要展开成员的已声明命名空间/枚举名集合，可为 nullptr
struct AbcLoadParam {
    std::string kit;
    std::string displayName;
    std::string requireName;
    std::string abcFilePath;
    std::string soPath;
    const std::set<std::string>* expandableNames = nullptr;
};

/**
 * @brief 执行独立 .abc 文件并提取导出API元数据
 * @param env 引擎环境
 * @param param 加载参数（kit / displayName / abcFilePath / expandableNames）
 * @param exposedMeta 输出：导出API元数据
 * @return 错误码
 */
CheckErrorCode LoadAbcFileAndGetExposedMeta(
    napi_env env,
    const AbcLoadParam& param,
    std::vector<ApiMeta>& exposedMeta
);

/**
 * @brief 检测 so 文件是否内嵌 ArkTS ABC（含 GetABCCode 导出符号）。
 * 只读文件字节做子串搜索（与 strings 同理），不 dlopen 不执行任何代码
 * @param soPath so 文件路径
 * @return true=内嵌 ABC（混合模块）
 */
bool SoHasEmbeddedAbc(const std::string& soPath);

/**
 * @brief 从混合 so 中提取内嵌 ABC 并执行、提取导出API元数据。
 * requireNapi 失败（引擎走 LoadArkModule 在裸进程缺运行时上下文）时的
 * 备选通道：直接 dlopen so、dlsym NAPI_<模块>_GetABCCode 取字节码、
 * 多候选记录名执行。不经过 NativeModuleManager（避免其状态污染）
 * @param env 引擎环境
 * @param param 加载参数（kit / displayName / requireName / soPath / expandableNames）
 * @param exposedMeta 输出
 * @return 错误码
 */
CheckErrorCode LoadMixedSoAbcAndGetExposedMeta(
    napi_env env,
    const AbcLoadParam& param,
    std::vector<ApiMeta>& exposedMeta
);

/**
 * @brief 按引擎 GetNativeModulePath 同款规则计算 so 候选路径
 * @param soDir 模块目录（scan-dir 模式的位置参数 / 平台默认前缀）
 * @param requireName requireNapi 参数
 * @return 候选路径列表（主形态 + _napi 变体，存在的排前）
 */
std::vector<std::string> CandidateSoPaths(const std::string& soDir, const std::string& requireName);

} // namespace ohos::check

#endif // LOAD_ABC_H
