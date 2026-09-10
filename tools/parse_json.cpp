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

std::string GetLastPartOfDot(const std::string& s)
{
    size_t lastDot = s.find_last_of('.');
    if (lastDot == std::string::npos) {
        return s;
    }
    return s.substr(lastDot + 1);
}

// Removes only nested <...> content; parentheses are left untouched.
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

// Records one dependency under name; duplicates are detected per comma segment (exact compare,
// so a new dependency "Task" is not swallowed by an existing "TaskPool").
void AppendDependency(std::map<std::string, std::string>& outDep,
                      const std::string& name, const std::string& depend)
{
    std::string& value = outDep[name];
    size_t start = 0;
    while (start <= value.size()) {
        size_t pos = value.find(',', start);
        size_t end = (pos == std::string::npos) ? value.size() : pos;
        if (value.compare(start, end - start, depend) == 0) {
            return; // exact segment match means it already exists
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

// Extracts extends/implements parents into outDep, including comma-separated
// multi-inheritance lists ("extends A, B, C") where every parent is recorded individually.
void ExtractExtendsList(const std::vector<std::string>& tokens, size_t startIdx,
                        const std::string& name, std::map<std::string, std::string>& outDep)
{
    size_t i = startIdx;
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
                break; // not a parent token ('{' etc.): clause ends
            }
            // Record the last dot segment so qualified parents (lang.ISendable) line up with
            // the bare-name inheritMap keys and classNames.
            AppendDependency(outDep, name, GetLastPartOfDot(parent));
            ++j;
            if (!hasComma) {
                break; // no comma: last parent of this clause
            }
        }
        i = j;
    }
}

// Handles one d.ts declaration line with any modifier combination and normalizes type
// declarations to "kind NAME"; modifiers are skipped (not enumerated) before the kind keyword.
std::string NormalizeDeclAndGetDeps(const std::string& line, std::map<std::string, std::string>& outDep)
{
    std::string r = RemoveNestedAngleBrackets(line);

    std::istringstream iss(r);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }

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
    if (idx + 1 >= tokens.size()) {
        return r;
    }
    const std::string& name = tokens[idx + 1];
    if (name.empty() || !IsValidIdentifierStart(name[0])) {
        return r;
    }

    if (kind == "class" || kind == "interface") {
        ExtractExtendsList(tokens, idx + 2, name, outDep);
    }
    return kind + " " + name;
}

// Untyped value-constant fallback: "export const NAME = v" / "readonly NAME = v" /
// "static NAME = v" -> NAME. Requires a stripped modifier, an '=' and a pure identifier.
std::string ExtractValueConstName(const std::string& body)
{
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
    if (!stripped) {
        return "";
    }
    size_t eqPos = t.find('=');
    if (eqPos == std::string::npos) {
        return "";
    }
    std::string name = t.substr(0, eqPos);
    while (!name.empty() && name.back() == ' ') {
        name.pop_back();
    }
    return IsPureIdentifier(name) ? name : "";
}

// "x?: number" -> "x"; "type CallbackFunction = () => void" -> "CallbackFunction".
std::string DeleteTypeDefineOfKey(const std::string& input)
{
    // Type aliases keep their name: d.ts often re-exports internal enums this way
    // ("export type X = _X") and the runtime object is mounted under the alias name.
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
        std::string name = ExtractValueConstName(body);
        return name.empty() ? input : name;
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

// "xxx = Y" or "xxx" -> "xxx" (assignment and surrounding whitespace removed).
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

// Metadata/native-only apiTypes that never appear on the JS runtime surface.
bool IsInertApiType(const std::string& apiType)
{
    return apiType == "file" || apiType == "struct" || apiType == "union" || apiType == "annotation";
}

} // namespace

// If t starts with "get "/"set " followed by "NAME(", converts meta to field semantics.
void TryConvertAccessor(const std::string& t, ApiMeta& meta)
{
    for (const std::string& acc : {"get ", "set "}) {
        if (t.compare(0, acc.size(), acc) != 0) {
            continue;
        }
        std::string rest = t.substr(acc.size());
        size_t pos = rest.find('(');
        if (pos != std::string::npos && IsPureIdentifier(rest.substr(0, pos))) {
            meta.apiType = "field";
            meta.apiText = rest.substr(0, pos);
        }
        return;
    }
}

// get/set accessors behave as instance fields at runtime, so convert them to field semantics
// (apiText = property name); "static get X" forms are stripped too. Without this, accessor
// declarations never match the (field, ...) type matrix.
void NormalizeAccessorDecl(ApiMeta& meta)
{
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
    TryConvertAccessor(t, meta);
}

// Strips modifier prefixes; the identifier-start lookahead keeps method names like "get" intact.
bool StripMethodModifiers(std::string& text, const std::string* modifiers, size_t count)
{
    bool stripped = true;
    bool any = false;
    while (stripped) {
        stripped = false;
        for (size_t i = 0; i < count; ++i) {
            const std::string& mod = modifiers[i];
            if (text.size() > mod.size() && text.compare(0, mod.size(), mod) == 0 &&
                IsValidIdentifierStart(text[mod.size()])) {
                text = text.substr(mod.size());
                stripped = true;
                any = true;
            }
        }
    }
    return any;
}

// Normalizes method declarations to function NAME( / static NAME( / NAME( forms and drops
// optional markers ("foo?(" / "foo?: (" -> "foo(") so index keys and MatchMethod line up.
void NormalizeMethodDecl(ApiMeta& meta)
{
    static const std::string methodModifiers[] = {
        "export ", "declare ", "abstract ", "default ", "async ",
        "public ", "private ", "protected ", "get ", "set "};
    StripMethodModifiers(meta.apiText, methodModifiers, sizeof(methodModifiers) / sizeof(methodModifiers[0]));

    size_t pos = meta.apiText.find('(');
    if (pos == std::string::npos || pos == 0) {
        return;
    }
    std::string head = meta.apiText.substr(0, pos);
    std::string trimmed = head;
    auto rtrim = [](std::string& s) {
        while (!s.empty() && s.back() == ' ') {
            s.pop_back();
        }
    };
    rtrim(trimmed);
    if (!trimmed.empty() && trimmed.back() == ':') { // "foo?: (" / "foo: (" shapes
        trimmed.pop_back();
        rtrim(trimmed);
    }
    if (!trimmed.empty() && trimmed.back() == '?') { // optional-method marker
        trimmed.pop_back();
        rtrim(trimmed);
    }
    if (trimmed.size() != head.size()) {
        meta.apiText = trimmed + meta.apiText.substr(pos);
    }
}

// Reads the five fields of one whitelist entry and normalizes them; returns false for inert
// types (file/struct/union/annotation) so the caller can skip the entry.
bool ParseApiMetaItem(cJSON* item, ApiMeta& meta,
                      std::map<std::string, std::string>& inheritMap)
{
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
            // Keep enum value expressions verbatim: stripping <> would eat "<<" shift
            // operators in values like "X = 1 << 2". The name is still cut at the first '='.
            meta.apiText = apiText->valuestring;
        } else {
            meta.apiText = NormalizeDeclAndGetDeps(apiText->valuestring, inheritMap);
        }
    }
    if (file && cJSON_IsString(file) && file->valuestring != nullptr) {
        // Skip .h-sourced entries: legal.file can never equal a module name, so they can
        // never match and only pollute the expandable-name set.
        const std::string rawFile = file->valuestring;
        if (rawFile.size() >= 2 && rawFile.compare(rawFile.size() - 2, 2, ".h") == 0) {
            return false;
        }
        meta.file = GetBeforeSecondLastDot(rawFile);
    }
    if (className && cJSON_IsString(className) && className->valuestring != nullptr) {
        meta.className = GetLastPartOfDot(className->valuestring);
        // The generator writes "unnamed" for top-level scopes; runtime top-level members
        // carry an empty className, so normalize or these entries can never match.
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
    if (meta.apiType == "method") {
        NormalizeAccessorDecl(meta);
    }
    if (meta.apiType == "method") {
        NormalizeMethodDecl(meta);
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
        // Keep only '@'-prefixed module names; plain declaration file names
        // ("api/graphics3d/Scene") are not modules.
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

// Attaches one kit's illegal APIs to root: {kitName: {file: [ApiMeta...]}}.
void AddKitToJson(cJSON* root, const std::string& kitName,
                  const std::map<std::string, std::vector<ApiMeta>>& fileMap)
{
    cJSON* kitObj = cJSON_CreateObject();
    if (kitObj == nullptr) {
        return;
    }
    cJSON_AddItemToObject(root, kitName.c_str(), kitObj);

    for (const auto& fileEntry : fileMap) {
        const std::string& fileName = fileEntry.first;
        const auto& apiList = fileEntry.second;

        cJSON* apiArray = cJSON_CreateArray();
        if (apiArray == nullptr) {
            continue;
        }
        cJSON_AddItemToObject(kitObj, fileName.c_str(), apiArray);

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

// Builds the result json tree (IllegalApis + DeprecatedModules + UnknownModules).
cJSON* BuildResultJsonTree(
    const std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& illegalMap,
    const std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& deprecatedMap,
    const std::vector<std::string>& unknownModules)
{
    cJSON* root = cJSON_CreateObject();
    if (root == nullptr) {
        return nullptr;
    }

    cJSON* illegalObj = cJSON_CreateObject();
    if (illegalObj == nullptr) {
        cJSON_Delete(root);
        return nullptr;
    }
    cJSON_AddItemToObject(root, "IllegalApis", illegalObj);
    for (const auto& kitEntry : illegalMap) {
        AddKitToJson(illegalObj, kitEntry.first, kitEntry.second);
    }

    // deprecated-bucket section: kept separate from IllegalApis
    if (!deprecatedMap.empty()) {
        cJSON* deprecatedObj = cJSON_CreateObject();
        if (deprecatedObj != nullptr) {
            cJSON_AddItemToObject(root, "DeprecatedModules", deprecatedObj);
            for (const auto& kitEntry : deprecatedMap) {
                AddKitToJson(deprecatedObj, kitEntry.first, kitEntry.second);
            }
        }
    }

    // omitted when empty to keep the old result format compatible
    if (!unknownModules.empty()) {
        cJSON* unknownArray = cJSON_CreateArray();
        if (unknownArray != nullptr) {
            cJSON_AddItemToObject(root, "UnknownModules", unknownArray);
            for (const std::string& name : unknownModules) {
                cJSON_AddItemToArray(unknownArray, cJSON_CreateString(name.c_str()));
            }
        }
    }
    return root;
}

// Atomic write via <path>.tmp + rename so the per-module checkpoint never leaves a
// half-written file behind; falls back to a direct write if rename fails.
bool AtomicWriteJson(const std::string& outputFilePath, const char* jsonStr)
{
    const std::string tmpPath = outputFilePath + ".tmp";
    std::ofstream ofs(tmpPath);
    if (!ofs.is_open()) {
        LOG_WARN("[SaveIllegalApiToJson] open file failed: " << tmpPath);
        return false;
    }
    ofs << jsonStr << std::endl;
    ofs.close();
    if (std::rename(tmpPath.c_str(), outputFilePath.c_str()) != 0) {
        LOG_WARN("[SaveIllegalApiToJson] rename failed, fallback direct write: " << outputFilePath);
        std::ofstream dst(outputFilePath);
        if (!dst.is_open()) {
            LOG_WARN("[SaveIllegalApiToJson] open file failed: " << outputFilePath);
            return false;
        }
        dst << jsonStr << std::endl;
    }
    return true;
}

CheckErrorCode SaveIllegalApiToJson(
    const std::string& outputFilePath,
    const std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& illegalMap,
    const std::map<std::string, std::map<std::string, std::vector<ApiMeta>>>& deprecatedMap,
    const std::vector<std::string>& unknownModules)
{
    cJSON* root = BuildResultJsonTree(illegalMap, deprecatedMap, unknownModules);
    if (root == nullptr) {
        return ERROR_JSON_PARSE_FAILED;
    }
    char* jsonStr = cJSON_Print(root);
    cJSON_Delete(root);
    if (jsonStr == nullptr) {
        return ERROR_JSON_PARSE_FAILED;
    }
    bool written = AtomicWriteJson(outputFilePath, jsonStr);
    cJSON_free(jsonStr);
    return written ? CHECK_OK : ERROR_FILE_OPEN_FAILED;
}

}
