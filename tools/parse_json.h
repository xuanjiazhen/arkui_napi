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

#ifndef PARSE_JSON_H
#define PARSE_JSON_H

#include <map>
#include <set>
#include <string>
#include <vector>

#include "api_meta.h"
#include "error_codes.h"

namespace ohos::check {

/**
 * @brief 解析完整合法API列表（apiText + apiType + className + file + kit）
 * 解析过程中完成四类归一化：
 *  1. 类型声明行（class/interface/enum/namespace，含 export/declare/abstract/default
 *     修饰词组合）规范化为 "kind NAME"，并抽取 extends/implements 依赖到 inheritMap；
 *  2. method 声明剥离 declare 前缀，统一为 function NAME( / NAME( 形态；
 *  3. field 声明只保留键名（"x?: number" -> "x"，"type X = ..." -> "X"），
 *     enum_instance 去掉赋值（"RED = 1" -> "RED"）；
 *  4. file 字段去掉 d.ts 后缀（"api/@ohos.x.d.ts" -> "api/@ohos.x"）。
 * file/struct/union/annotation 类型条目为元数据/native 专有声明，不参与匹配，跳过。
 * @param resultFile 合法API列表JSON文件
 * @param legalApiList 输出：合法API元数据列表
 * @param inheritMap 输出：className -> 父类/接口名（逗号分隔，支持多个）
 */
CheckErrorCode ParseLegalApiList(
    const std::string& resultFile,
    std::vector<ApiMeta>& legalApiList,
    std::map<std::string, std::string>& inheritMap
);

/**
 * @brief 解析待检测列表 {"kit1": ["file1", "file2"], ...}
 */
CheckErrorCode ParseKitToCheckFiles(
    const std::string& jsonFile,
    std::map<std::string, std::vector<std::string>>& kitFileMap
);

/**
 * @brief 从白名单 json 中提取声明过的模块名集合
 * 遍历条目的 file 字段（"api/@ohos.x.d.ts"），去 d.ts 后缀得规范模块名
 * （"@ohos.x"）；仅收以 '@' 开头的条目（声明文件名形态如 "api/graphics3d/Scene"
 * 不是模块名，跳过）。供目录扫描模式构建 模块名→kit 反查表。
 * @param jsonFile 白名单 json 文件
 * @param moduleNames 输出：规范模块名集合
 */
CheckErrorCode CollectModuleNames(
    const std::string& jsonFile,
    std::set<std::string>& moduleNames
);

/**
 * @brief 输出非法API到JSON文件
 * 格式： {"IllegalApis": {"kit": {"file": [ApiMeta...]}}, "UnknownModules": [...]}
 * UnknownModules 为空时不写该键（保持旧格式兼容）。
 */
CheckErrorCode SaveIllegalApiToJson(
    const std::string& outputFilePath,
    const std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& illegalMap,
    const std::vector<std::string>& unknownModules = {}
);

}

#endif
