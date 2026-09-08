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

// 记录一条 extends/implements 依赖：同一名字的多个依赖以逗号拼接（精确去重）。
// 去重按逗号段精确比较——子串匹配会把前缀名误判为已存在
// （已存 "TaskPool" 时新依赖 "Task" 被吞，详见 lcl_napicheck.md 附录 O）
void AppendDependency(std::map<std::string, std::string>& outDep,
                      const std::string& name, const std::string& depend)
{
    std::string& value = outDep[name];
    size_t start = 0;
    while (start <= value.size()) {
        size_t pos = value.find(',', start);
        size_t end = (pos == std::string::npos) ? value.size() : pos;
        if (value.compare(start, end - start, depend) == 0) {
            return; // 段内精确相等才视为已存在
        }
        if (pos == std::string::npos) {
            break;
        }
        start = pos + 1;
    }
    if (!value.empty()) {
        value += ",";
    }
    value += depend;
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

    // 遍历找依赖，extends/implements 全部记录（class/interface 可继承）。
    // 逗号多继承（"extends A, B, C"，TS interface 合法形态）逐父类记录：
    // 以逗号结尾的 token 其后继仍是父类；无逗号结尾的标识符是本子句最后一个。
    // 旧实现只记第一个 token，逗号列表其余父类全部丢失（CameraKit 43 条
    // 子句丢 183 个父类、650 条成员匹配受影响，详见 lcl_napicheck.md 附录 O）
    if (kind == "class" || kind == "interface") {
        size_t i = idx + 2;
        while (i + 1 < tokens.size()) {
            if (tokens[i] != "extends" && tokens[i] != "implements") {
                ++i;
                continue;
            }
            size_t j = i + 1;
            while (j < tokens.size()) {
                std::string parent = tokens[j];
                bool hasComma = !parent.empty() && parent.back() == ',';
                if (hasComma) {
                    parent.pop_back();
                }
                if (parent.empty() || parent == "extends" || parent == "implements" ||
                    !IsValidIdentifierStart(parent[0])) {
                    break; // 非父类 token（'{' 等），子句结束
                }
                // B3（附录 T）：父类记末段——与 className 侧 GetLastPartOfDot、
                // inheritMap 键的裸名语义对齐。限定名父类（lang.ISendable 型）
                // 存全名时 IsInherit 的 BFS 与裸名键/className 接不上（死边）；
                // 裸名经 GetLastPartOfDot 幂等不变（16 kit 差分：1,358 裸名边
                // 字节级不变、68 限定名边死转活、同 kit 同末段歧义 0 命中）
                AppendDependency(outDep, name, GetLastPartOfDot(parent));
                ++j;
                if (!hasComma) {
                    break; // 无逗号：本子句最后一个父类
                }
            }
            i = j; // 跳过已消费的父类，继续找后续 implements 子句
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
        // B5（附录 T）：无类型标注的真值常量回退——"export const NAME = v" /
        // "const NAME = v" / "readonly NAME = v" / "static NAME = v" 提取 NAME。
        // 守卫三重：至少剥到一个修饰词前缀（防裸 "NAME = v" 误认）、'=' 存在、
        // '=' 左元为纯标识符——type 别名（"export type X = object" 剥前缀后
        // 左元含空格 "type X"）天然被纯标识符守卫排除，保持死键（编译期擦除）。
        // 16 kit 差分：62 条真值常量复活，其余无冒号条目字节级不变。
        static const std::string valueModifiers[] = {
            "export ", "declare ", "const ", "readonly ", "static "};
        std::string t = input;
        bool stripped = false;
        bool changed = true;
        while (changed) {
            changed = false;
            for (const std::string& mod : valueModifiers) {
                if (t.size() > mod.size() && t.compare(0, mod.size(), mod) == 0 &&
                    IsValidIdentifierStart(t[mod.size()])) {
                    t = t.substr(mod.size());
                    stripped = true;
                    changed = true;
                }
            }
        }
        if (stripped) {
            size_t eqPos = t.find('=');
            if (eqPos != std::string::npos) {
                std::string name = t.substr(0, eqPos);
                while (!name.empty() && name.back() == ' ') {
                    name.pop_back();
                }
                bool valid = !name.empty() && IsValidIdentifierStart(name[0]);
                for (size_t i = 1; valid && i < name.size(); ++i) {
                    char c = name[i];
                    if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
                          (c >= '0' && c <= '9') || c == '_' || c == '$')) {
                        valid = false;
                    }
                }
                if (valid) {
                    return name;
                }
            }
        }
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

    if (apiType && cJSON_IsString(apiType) && apiType->valuestring != nullptr) {
        meta.apiType = apiType->valuestring;
    }
    if (apiText && cJSON_IsString(apiText) && apiText->valuestring != nullptr) {
        if (meta.apiType == "enum_instance") {
            // 枚举值表达式（如 "X = 1 << 2"）不含泛型，剥 <> 会把移位运算符
            // 误当嵌套泛型开头、吞掉右操作数（1 << 2 → 1，详见 lcl_napicheck.md
            // 附录 L/M）。原样保留：名字仍由 DeleteValueOfEnum 在首个 '='
            // 截断，值部分以原文存在，留给未来按值校验的消费者解析。
            // 已全量差分验证：16 kit 111,853 条最终 apiText/索引键/inheritMap 零变化。
            meta.apiText = apiText->valuestring;
        } else {
            meta.apiText = NormalizeDeclAndGetDeps(apiText->valuestring, inheritMap);
        }
    }
    if (file && cJSON_IsString(file) && file->valuestring != nullptr) {
        // .h 来源条目整体跳过（附录 R B4）：file 校验（Match 第 4 级）要求
        // legal.file 与模块名相等，头文件路径永不相等，28,992 条非惰性 .h 条目
        // 必死；剔除不损失匹配能力，并清掉 expandableNames 的头文件守卫名噪音
        const std::string rawFile = file->valuestring;
        if (rawFile.size() >= 2 && rawFile.compare(rawFile.size() - 2, 2, ".h") == 0) {
            return false;
        }
        meta.file = GetBeforeSecondLastDot(rawFile);
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
    // method 声明归一化（附录 R B1/B2）：
    //  - 剥修饰词前缀（export/declare/abstract/default/async/public/private/
    //    protected/get/set），统一为 function NAME( / static NAME( / NAME( 形态；
    //    前瞻修饰词后首字符必须是标识符起始，防误剥名为 get/set/export 的方法
    //  - 剥名字尾部可选标记（"foo?(" / "foo?: (" → "foo("），否则索引键与
    //    MatchMethod 的 NAME( 前缀全部失配（实测 1,543 条由脏转净、干净键零变化）
    if (meta.apiType == "method") {
        static const std::string methodModifiers[] = {
            "export ", "declare ", "abstract ", "default ", "async ",
            "public ", "private ", "protected ", "get ", "set "};
        bool stripped = true;
        while (stripped) {
            stripped = false;
            for (const std::string& mod : methodModifiers) {
                if (meta.apiText.size() > mod.size() &&
                    meta.apiText.compare(0, mod.size(), mod) == 0 &&
                    IsValidIdentifierStart(meta.apiText[mod.size()])) {
                    meta.apiText = meta.apiText.substr(mod.size());
                    stripped = true;
                }
            }
        }
        size_t pos = meta.apiText.find('(');
        if (pos != std::string::npos && pos > 0) {
            std::string head = meta.apiText.substr(0, pos);
            std::string trimmed = head;
            auto rtrim = [](std::string& s) {
                while (!s.empty() && s.back() == ' ') {
                    s.pop_back();
                }
            };
            rtrim(trimmed);
            if (!trimmed.empty() && trimmed.back() == ':') { // "foo?: (" / "foo: (" 形态
                trimmed.pop_back();
                rtrim(trimmed);
            }
            if (!trimmed.empty() && trimmed.back() == '?') { // 可选方法标记
                trimmed.pop_back();
                rtrim(trimmed);
            }
            if (trimmed.size() != head.size()) { // 头部有修剪则重建，索引键/MatchMethod 对齐
                meta.apiText = trimmed + meta.apiText.substr(pos);
            }
        }
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
