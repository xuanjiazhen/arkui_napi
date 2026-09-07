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

#include "parse_json.h"

#include <fstream>
#include <map>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "check_helper.h"
#include "cJSON.h"

namespace ohos::check {
namespace {

// @ohos.xx.xx.d.ts ==> @ohos.xx.xx
std::string GetBeforeSecondLastDot(const std::string& str)
{
    size_t lastDot = str.find_last_of('.');
    if (lastDot == std::string::npos || lastDot == 0) {
        return str;
    }

    size_t secondLastDot = str.find_last_of('.', lastDot - 1);
    if (secondLastDot == std::string::npos) {
        return str;
    }

    return str.substr(0, secondLastDot);
}

// 获取字符串中最后一个.后面的内容
std::string GetLastPartOfDot(const std::string& s)
{
    size_t lastDot = s.find_last_of('.');
    if (lastDot == std::string::npos) {
        return s;
    }
    return s.substr(lastDot + 1);
}

// 只删除所有嵌套的<>内容，（）完全不处理
std::string RemoveNestedAngleBrackets(const std::string& input)
{
    std::string result;
    int level = 0;

    for (char c : input) {
        if (c == '<') {
            level++;
        } else if (c == '>') {
            if (level > 0) {
                level--;
            }
        } else if (level == 0) {
            result += c;
        }
    }
    return result;
}

bool IsModifierToken(const std::string& token)
{
    return token == "export" || token == "declare" || token == "abstract" ||
           token == "default" || token == "const";
}

bool IsValidIdentifierStart(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$';
}

// 记录一条 extends/implements 依赖：同一名字的多个依赖以逗号拼接（去重）
void AppendDependency(std::map<std::string, std::string>& outDep,
                      const std::string& name, const std::string& depend)
{
    auto it = outDep.find(name);
    if (it == outDep.end()) {
        outDep[name] = depend;
    } else if (it->second.find(depend) == std::string::npos) {
        it->second += "," + depend;
    }
}

/**
 * 处理一行 d.ts 声明（泛型已去除），兼容任意修饰词组合：
 *   "[export] [declare] [abstract|default] class A extends B implements C"
 *   "[export] [declare] interface I extends J"
 *   "[export] [declare] [const] enum E"
 *   "[export] [declare] namespace N"
 *   "export { Name }"（re-export 声明）
 * 类型声明规范化为 "kind NAME"（如 "class Task"），并把 extends/implements 的
 * 依赖（多个，逗号分隔）记入 outDep；非声明行原样返回。
 * 实测白名单中存在 declare(695)/export declare/export default/abstract 等
 * 10 种 class 形态、7 种 enum 形态、4 种 namespace 形态，穷举前缀不可行，
 * 故采用"跳过修饰词 + 识别 kind 关键字"的解析方式。
 */
std::string NormalizeDeclAndGetDeps(const std::string& line, std::map<std::string, std::string>& outDep)
{
    std::string r = RemoveNestedAngleBrackets(line);

    std::istringstream iss(r);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }

    // 跳过修饰词，定位声明关键字
    size_t idx = 0;
    while (idx < tokens.size() && IsModifierToken(tokens[idx])) {
        idx++;
    }
    if (idx >= tokens.size()) {
        return r;
    }
    const std::string& kind = tokens[idx];

    // re-export: "export { Name }" -> "export Name"
    if (kind == "{" && idx + 2 < tokens.size() && tokens[idx + 2] == "}") {
        return "export " + tokens[idx + 1];
    }

    if (kind != "class" && kind != "interface" && kind != "enum" && kind != "namespace") {
        return r;
    }
    // 至少要有 kind + 名字
    if (idx + 1 >= tokens.size()) {
        return r;
    }
    const std::string& name = tokens[idx + 1];
    if (name.empty() || !IsValidIdentifierStart(name[0])) {
        return r;
    }

    // 遍历找依赖，extends/implements 全部记录（class/interface 可继承）
    if (kind == "class" || kind == "interface") {
        for (size_t i = idx + 2; i + 1 < tokens.size(); ++i) {
            if (tokens[i] == "extends" || tokens[i] == "implements") {
                AppendDependency(outDep, name, tokens[i + 1]);
            }
        }
    }
    return kind + " " + name;
}

// 提取目标 key: xxxx
// "x?: number" -> "x"；"type CallbackFunction = () => void" -> "CallbackFunction"
std::string DeleteTypeDefineOfKey(const std::string& input)
{
    // type 别名声明：保留别名名
    static const std::string typePrefix = "type ";
    if (input.compare(0, typePrefix.size(), typePrefix) == 0) {
        std::string rest = input.substr(typePrefix.size());
        size_t end = rest.find_first_of(" =");
        if (end != std::string::npos && end > 0) {
            return rest.substr(0, end);
        }
        return rest;
    }

    size_t colonPos = input.find(':');
    if (colonPos == std::string::npos) {
        return input;
    }
    std::string left = input.substr(0, colonPos);
    size_t start = left.find_last_of(" ");
    std::string key;
    if (start == std::string::npos) {
        key = left;
    } else {
        key = left.substr(start + 1);
    }
    if (!key.empty() && key.back() == '?') {
        key.pop_back();
    }
    return key;
}

// 输入：xxx = Y 或者 xxx
// 输出：xxx（自动去除赋值及前后空白）
std::string DeleteValueOfEnum(const std::string& input)
{
    size_t eqPos = input.find('=');

    std::string result;
    if (eqPos != std::string::npos) {
        result = input.substr(0, eqPos);
    } else {
        result = input;
    }

    size_t endPos = result.find_last_not_of(" \t");
    if (endPos != std::string::npos) {
        result = result.substr(0, endPos + 1);
    }
    return result;
}

// 元数据/native 专有 apiType：不参与运行时导出面匹配
//  - file: 声明文件标记（apiText 为 d.ts 文件名，className 固定为 'uname'）
//  - struct/union: native 头文件类型，无 JS 运行时形态
//  - annotation: 编译期注解（@interface），运行时不存在
bool IsInertApiType(const std::string& apiType)
{
    return apiType == "file" || apiType == "struct" || apiType == "union" || apiType == "annotation";
}

} // namespace

// 解析单个白名单条目：读取五字段并完成归一化（详见 ParseLegalApiList 的说明）。
// inert 类型条目（file/struct/union/annotation）返回 false，调用方跳过不入列表。
bool ParseApiMetaItem(cJSON* item, ApiMeta& meta,
                      std::map<std::string, std::string>& inheritMap)
{
    // 读取关键字段
    cJSON* apiText = cJSON_GetObjectItem(item, "apiText");
    cJSON* file = cJSON_GetObjectItem(item, "file");
    cJSON* apiType = cJSON_GetObjectItem(item, "apiType");
    cJSON* className = cJSON_GetObjectItem(item, "className");
    cJSON* kit = cJSON_GetObjectItem(item, "kit");

    if (apiText && cJSON_IsString(apiText) && apiText->valuestring != nullptr) {
        meta.apiText = NormalizeDeclAndGetDeps(apiText->valuestring, inheritMap);
    }
    if (file && cJSON_IsString(file) && file->valuestring != nullptr) {
        meta.file = GetBeforeSecondLastDot(file->valuestring);
    }
    if (apiType && cJSON_IsString(apiType) && apiType->valuestring != nullptr) {
        meta.apiType = apiType->valuestring;
    }
    if (className && cJSON_IsString(className) && className->valuestring != nullptr) {
        meta.className = GetLastPartOfDot(className->valuestring);
    }
    if (kit && cJSON_IsString(kit) && kit->valuestring != nullptr) {
        meta.kit = kit->valuestring;
    }
    if (IsInertApiType(meta.apiType)) {
        return false;
    }
    // method 声明去除 declare 前缀，统一为 function NAME( / NAME( 形态
    if (meta.apiType == "method" &&
        meta.apiText.compare(0, 8, "declare ") == 0) { // 8: strlen("declare ")
        meta.apiText = meta.apiText.substr(8);
    }
    if (meta.apiType == "field") {
        meta.apiText = DeleteTypeDefineOfKey(meta.apiText);
    }
    if (meta.apiType == "enum_instance") {
        meta.apiText = DeleteValueOfEnum(meta.apiText);
    }
    return !meta.apiText.empty();
}

CheckErrorCode ParseLegalApiList(const std::string& resultFile,
                                 std::vector<ApiMeta>& legalApiList,
                                 std::map<std::string, std::string> &inheritMap)
{
    std::ifstream ifs(resultFile);
    if (!ifs.is_open()) {
        LOG_WARN("[ParseLegalApiList] open file failed: " << resultFile);
        return ERROR_FILE_OPEN_FAILED;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    cJSON* root = cJSON_Parse(content.c_str());

    if (!root || !cJSON_IsArray(root)) {
        LOG_WARN("[ParseLegalApiList] json parse failed or not array: " << resultFile);
        cJSON_Delete(root);
        return ERROR_JSON_PARSE_FAILED;
    }

    int size = cJSON_GetArraySize(root);
    LOG_DEBUG("ParseLegalApiList " << resultFile << " size: " << size);
    for (int i = 0; i < size; i++) {
        cJSON* item = cJSON_GetArrayItem(root, i);
        if (!item || !cJSON_IsObject(item)) {
            continue;
        }
        ApiMeta meta{};
        if (ParseApiMetaItem(item, meta, inheritMap)) {
            legalApiList.push_back(meta);
        }
    }

    cJSON_Delete(root);
    return CHECK_OK;
}

CheckErrorCode ParseKitToCheckFiles(const std::string& jsonFile,
                                    std::map<std::string, std::vector<std::string>>& kitMap)
{
    std::ifstream ifs(jsonFile);
    if (!ifs.is_open()) {
        LOG_WARN("[ParseKitToCheckFiles] open failed: " << jsonFile);
        return ERROR_FILE_OPEN_FAILED;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    cJSON* root = cJSON_Parse(content.c_str());

    if (!root || !cJSON_IsObject(root)) {
        LOG_WARN("[ParseKitToCheckFiles] invalid JSON, not object: " << jsonFile);
        cJSON_Delete(root);
        return ERROR_JSON_PARSE_FAILED;
    }

    // 遍历顶层所有kit
    cJSON* key = nullptr;
    cJSON_ArrayForEach(key, root) {
        if (key->string == nullptr) {
            continue;
        }
        std::string kitName = key->string;
        cJSON* fileArray = cJSON_GetObjectItem(root, kitName.c_str());

        if (!fileArray || !cJSON_IsArray(fileArray)) {
            continue;
        }

        std::vector<std::string> files;
        int arrSize = cJSON_GetArraySize(fileArray);
        for (int i = 0; i < arrSize; i++) {
            cJSON* fileItem = cJSON_GetArrayItem(fileArray, i);
            if (fileItem && cJSON_IsString(fileItem) && fileItem->valuestring != nullptr) {
                files.emplace_back(fileItem->valuestring);
            }
        }
        kitMap[kitName] = files;
    }
    cJSON_Delete(root);
    return CHECK_OK;
}

CheckErrorCode CollectModuleNames(const std::string& jsonFile, std::set<std::string>& moduleNames)
{
    std::ifstream ifs(jsonFile);
    if (!ifs.is_open()) {
        LOG_WARN("[CollectModuleNames] open file failed: " << jsonFile);
        return ERROR_FILE_OPEN_FAILED;
    }

    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    cJSON* root = cJSON_Parse(content.c_str());
    if (!root || !cJSON_IsArray(root)) {
        LOG_WARN("[CollectModuleNames] json parse failed or not array: " << jsonFile);
        cJSON_Delete(root);
        return ERROR_JSON_PARSE_FAILED;
    }

    int size = cJSON_GetArraySize(root);
    for (int i = 0; i < size; i++) {
        cJSON* item = cJSON_GetArrayItem(root, i);
        if (!item || !cJSON_IsObject(item)) {
            continue;
        }
        cJSON* file = cJSON_GetObjectItem(item, "file");
        if (!file || !cJSON_IsString(file) || file->valuestring == nullptr) {
            continue;
        }
        std::string name = GetBeforeSecondLastDot(file->valuestring);
        // 剥目录前缀（"api/@ohos.x" → "@ohos.x"；无前缀原样）。
        // 仅收模块名形态（'@' 开头）；声明文件名（如 "api/graphics3d/Scene"
        // 剥前缀后为 "Scene"，非 '@' 开头）跳过。
        size_t slash = name.find_last_of('/');
        if (slash != std::string::npos) {
            name = name.substr(slash + 1);
        }
        if (!name.empty() && name[0] == '@') {
            moduleNames.insert(name);
        }
    }

    cJSON_Delete(root);
    return CHECK_OK;
}

// 将一个 kit 的全部非法 API 挂到 root 下：{kitName: {file: [ApiMeta...]}}
void AddKitToJson(cJSON* root, const std::string& kitName,
                  const std::map<std::string, std::vector<ApiMeta>>& fileMap)
{
    cJSON* kitObj = cJSON_CreateObject();
    if (kitObj == nullptr) {
        return;
    }
    cJSON_AddItemToObject(root, kitName.c_str(), kitObj);

    // 遍历file
    for (const auto& fileEntry : fileMap) {
        const std::string& fileName = fileEntry.first;
        const auto& apiList = fileEntry.second;

        cJSON* apiArray = cJSON_CreateArray();
        if (apiArray == nullptr) {
            continue;
        }
        cJSON_AddItemToObject(kitObj, fileName.c_str(), apiArray);

        // 遍历每个非法ApiMeta
        for (const ApiMeta& meta : apiList) {
            cJSON* apiObj = cJSON_CreateObject();
            if (apiObj == nullptr) {
                continue;
            }
            cJSON_AddStringToObject(apiObj, "apiText", meta.apiText.c_str());
            cJSON_AddStringToObject(apiObj, "file", meta.file.c_str());
            cJSON_AddStringToObject(apiObj, "apiType", meta.apiType.c_str());
            cJSON_AddStringToObject(apiObj, "className", meta.className.c_str());
            cJSON_AddStringToObject(apiObj, "kit", meta.kit.c_str());
            cJSON_AddItemToArray(apiArray, apiObj);
        }
    }
}

CheckErrorCode SaveIllegalApiToJson(
    const std::string& outputFilePath,
    const std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& illegalMap,
    const std::vector<std::string>& unknownModules)
{
    cJSON* root = cJSON_CreateObject();
    if (!root) {
        return ERROR_JSON_PARSE_FAILED;
    }

    // 遍历kit
    cJSON* illegalObj = cJSON_CreateObject();
    if (illegalObj == nullptr) {
        cJSON_Delete(root);
        return ERROR_JSON_PARSE_FAILED;
    }
    cJSON_AddItemToObject(root, "IllegalApis", illegalObj);
    for (const auto& kitEntry : illegalMap) {
        AddKitToJson(illegalObj, kitEntry.first, kitEntry.second);
    }

    // 未登记模块清单（目录扫描模式）：空时不写键，保持旧格式兼容
    if (!unknownModules.empty()) {
        cJSON* unknownArray = cJSON_CreateArray();
        if (unknownArray != nullptr) {
            cJSON_AddItemToObject(root, "UnknownModules", unknownArray);
            for (const std::string& name : unknownModules) {
                cJSON_AddItemToArray(unknownArray, cJSON_CreateString(name.c_str()));
            }
        }
    }

    // 打印并写入文件
    char* jsonStr = cJSON_Print(root);
    if (jsonStr == nullptr) {
        cJSON_Delete(root);
        return ERROR_JSON_PARSE_FAILED;
    }

    std::ofstream ofs(outputFilePath);
    if (!ofs.is_open()) {
        LOG_WARN("[SaveIllegalApiToJson] open file failed: " << outputFilePath);
        cJSON_free(jsonStr);
        cJSON_Delete(root);
        return ERROR_FILE_OPEN_FAILED;
    }
    ofs << jsonStr << std::endl;
    ofs.close();

    cJSON_free(jsonStr);
    cJSON_Delete(root);

    LOG_INFO("非法API报告已保存: " << outputFilePath);
    return CHECK_OK;
}

}
