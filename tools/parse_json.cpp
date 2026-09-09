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

#include <cstdio>
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

// 名字是否为纯标识符（首字符合法，其余为字母数字/_/$）
bool IsPureIdentifier(const std::string& name)
{
    if (name.empty() || !IsValidIdentifierStart(name[0])) {
        return false;
    }
    for (size_t i = 1; i < name.size(); ++i) {
        char c = name[i];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') || c == '_' || c == '$')) {
            return false;
        }
    }
    return true;
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
    // type 别名声明：保留别名名。"export type X = _Y" / "declare type X = Y"
    // 先剥别名外的修饰词再按 type 别名处理——公共 d.ts 常以再导出别名形态
    // 声明内部枚举（autoFillManager 的 "export type AutoFillType = _AutoFillType"），
    // 运行期该枚举对象以别名名挂载在导出面上。别名条目保持原样（死键）会让
    // 对应导出对象失去合法依据而伪报（实测 autoFillManager 整组误报）。
    static const std::string typePrefix = "type ";
    std::string body = input;
    for (const std::string& mod : {"export ", "declare "}) {
        if (body.size() > mod.size() && body.compare(0, mod.size(), mod) == 0 &&
            IsValidIdentifierStart(body[mod.size()])) {
            body = body.substr(mod.size());
            break;
        }
    }
    if (body.compare(0, typePrefix.size(), typePrefix) == 0) {
        std::string rest = body.substr(typePrefix.size());
        size_t end = rest.find_first_of(" =");
        if (end != std::string::npos && end > 0) {
            return rest.substr(0, end);
        }
        return rest;
    }

    size_t colonPos = body.find(':');
    if (colonPos == std::string::npos) {
        // B5（附录 T）：无类型标注的真值常量回退——"export const NAME = v" /
        // "const NAME = v" / "readonly NAME = v" / "static NAME = v" 提取 NAME。
        // 守卫三重：至少剥到一个修饰词前缀（防裸 "NAME = v" 误认）、'=' 存在、
        // '=' 左元为纯标识符——type 别名已在上面的别名分支提前处理。
        // 16 kit 差分：62 条真值常量复活，其余无冒号条目字节级不变。
        static const std::string valueModifiers[] = {
            "export ", "declare ", "const ", "readonly ", "static "};
        std::string t = body;
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
                if (IsPureIdentifier(name)) {
                    return name;
                }
            }
        }
        return input;
    }
    std::string left = body.substr(0, colonPos);
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
        // 生成器占位符归一化：d.ts 顶层函数/字段没有作用域名时，生成器在
        // className 里记字面量 "unnamed"（实测全库 28 个白名单文件共 1252 条：
        // field 960 + method 292，含现行 kit 的 CoreFileKit/ArkUI 等，并非
        // 废弃文件独有）。运行时暴露侧顶层成员 className 为空串、永不为
        // "unnamed"（已对真机两轮结果的全部非法条目验证）——不归一化则这批
        // 条目永远无法命中、只产误报（真机 scan-dir 的 fileio/document 60 条
        // 即此因；加载污染修复后现行 kit 的千余条 unnamed 条目也将激活，
        // 本归一化为其必要配套）。
        if (meta.className == "unnamed") {
            meta.className = "";
        }
    }
    if (kit && cJSON_IsString(kit) && kit->valuestring != nullptr) {
        meta.kit = kit->valuestring;
    }
    if (IsInertApiType(meta.apiType)) {
        return false;
    }
    // 访问器声明归一化（"get x(): T" / "set x(v: T)"）：运行期表现为实例属性
    // （遍历侧 GetApiType 对非函数一律 field），而类型矩阵没有 (field, method)
    // 组合——不转换则 d.ts 访问器声明的属性（distributedKVStore Schema 的
    // root/indexes/mode/skip）全部伪报。识别形态：剥掉 export/declare/static
    // 等修饰词后首 token 为 get/set 且后随 "NAME("；命中后按 field 语义参与
    // 匹配（apiText = 属性名）。static 一并剥除：static 访问器（如 ArkUI 的
    // "static get IDENTITY(): ContentTransitionEffect"，全库 21 条）运行期是
    // 构造器上的访问器（遍历侧记 field），不转换则索引键为含空格的
    // "get IDENTITY" 永不命中；非访问器的 "static NAME(" 不受影响（t 仅本地
    // 剥离，未命中 get/set 前缀即丢弃，meta.apiText 原样交后续 method 归一化）
    if (meta.apiType == "method") {
        static const std::string plainModifiers[] = {
            "export ", "declare ", "abstract ", "default ", "async ",
            "public ", "private ", "protected ", "static "};
        std::string t = meta.apiText;
        bool changed = true;
        while (changed) {
            changed = false;
            for (const std::string& mod : plainModifiers) {
                if (t.size() > mod.size() && t.compare(0, mod.size(), mod) == 0 &&
                    IsValidIdentifierStart(t[mod.size()])) {
                    t = t.substr(mod.size());
                    changed = true;
                }
            }
        }
        for (const std::string& acc : {"get ", "set "}) {
            if (t.compare(0, acc.size(), acc) == 0) {
                std::string rest = t.substr(acc.size());
                size_t pos = rest.find('(');
                if (pos != std::string::npos) {
                    std::string name = rest.substr(0, pos);
                    if (IsPureIdentifier(name)) {
                        meta.apiType = "field";
                        meta.apiText = name;
                    }
                }
                break;
            }
        }
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
    const std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& deprecatedMap,
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

    // 废弃桶结果段：快照失配通道，与 IllegalApis 分开归档（非空时输出键，
    // 与 UnknownModules 的兼容策略一致）
    if (!deprecatedMap.empty()) {
        cJSON* deprecatedObj = cJSON_CreateObject();
        if (deprecatedObj != nullptr) {
            cJSON_AddItemToObject(root, "DeprecatedModules", deprecatedObj);
            for (const auto& kitEntry : deprecatedMap) {
                AddKitToJson(deprecatedObj, kitEntry.first, kitEntry.second);
            }
        }
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

    // 原子写：先写 <path>.tmp 再 rename——增量落盘（每模块检查后调用本函数）
    // 依赖此性质：进程任意时点死亡都不会留下半写损坏的结果文件。
    // rename 失败（跨文件系统等）时降级为直接写目标路径
    const std::string tmpPath = outputFilePath + ".tmp";
    std::ofstream ofs(tmpPath);
    if (!ofs.is_open()) {
        LOG_WARN("[SaveIllegalApiToJson] open file failed: " << tmpPath);
        cJSON_free(jsonStr);
        cJSON_Delete(root);
        return ERROR_FILE_OPEN_FAILED;
    }
    ofs << jsonStr << std::endl;
    ofs.close();
    if (std::rename(tmpPath.c_str(), outputFilePath.c_str()) != 0) {
        LOG_WARN("[SaveIllegalApiToJson] rename failed, fallback direct write: " << outputFilePath);
        std::ofstream dst(outputFilePath);
        if (!dst.is_open()) {
            LOG_WARN("[SaveIllegalApiToJson] open file failed: " << outputFilePath);
            cJSON_free(jsonStr);
            cJSON_Delete(root);
            return ERROR_FILE_OPEN_FAILED;
        }
        dst << jsonStr << std::endl;
    }

    cJSON_free(jsonStr);
    cJSON_Delete(root);

    return CHECK_OK;
}

}
