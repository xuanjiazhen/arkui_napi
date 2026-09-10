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

#include "check_helper.h"

#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <ostream>
#include <queue>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "ark_native_engine.h"
#include "jsnapi.h"
#include "native_utils.h"
#include "napi/native_node_api.h"

namespace ohos::check {
namespace {

// 防御性递归上限：正常 kit 导出层级远小于该值，超深说明出现自引用等异常结构
constexpr int MAX_TRAVERSE_DEPTH = 8;

// ---------------- 日志实现 ----------------

LogLevel g_logLevel = LogLevel::INFO;
std::ofstream g_logFile;
std::mutex g_logMutex;

const char* LogLevelName(LogLevel level)
{
    switch (level) {
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::WARN:  return "WARN";
        case LogLevel::INFO:  return "INFO";
        case LogLevel::DEBUG: return "DEBUG";
        default:              return "LOG";
    }
}

std::vector<std::string> SplitByComma(const std::string& value)
{
    std::vector<std::string> result;
    size_t start = 0;
    while (start < value.size()) {
        size_t pos = value.find(',', start);
        if (pos == std::string::npos) {
            result.push_back(value.substr(start));
            break;
        }
        result.push_back(value.substr(start, pos - start));
        start = pos + 1;
    }
    return result;
}

// 模块名 -> d.ts 命名空间名：取最后一个 '.' 之后的一段。
// '@ohos.taskpool' -> 'taskpool'；'@hms.xrGlassesService.xrGlassesAppService' -> 'xrGlassesAppService'。
// 与 OpenHarmony d.ts 的命名约定一致（namespace 名 = 模块名末段）。
std::string GetNamespaceOfModule(const std::string& moduleName)
{
    size_t pos = moduleName.find_last_of('.');
    if (pos == std::string::npos || pos + 1 >= moduleName.size()) {
        return moduleName;
    }
    return moduleName.substr(pos + 1);
}

// 读取数组 properties 第 index 个字符串元素：outName 为原 napi 值（供后续
// napi_get_property 按名取属性），outKey 为其 UTF-8 内容；失败返回 false
bool GetStringElement(napi_env env, napi_value properties, uint32_t index,
                      napi_value& outName, std::string& outKey)
{
    napi_value nameVal = nullptr;
    if (napi_get_element(env, properties, index, &nameVal) != napi_ok) {
        return false;
    }
    size_t len = 0;
    if (napi_get_value_string_utf8(env, nameVal, nullptr, 0, &len) != napi_ok) {
        return false;
    }
    std::vector<char> nameBuf(len + 1, '\0');
    size_t copied = 0;
    if (napi_get_value_string_utf8(env, nameVal, nameBuf.data(), nameBuf.size(), &copied) != napi_ok) {
        return false;
    }
    outName = nameVal;
    outKey.assign(nameBuf.data(), copied);
    return true;
}

// 记录一条导出成员元数据到 ctx.outMeta
void AppendApiMeta(TraverseContext& ctx, const std::string& key, const std::string& type,
                   const std::string& parentClass)
{
    ApiMeta meta;
    meta.apiText = key;
    meta.file = ctx.file;
    meta.apiType = type;
    meta.className = parentClass;
    meta.kit = ctx.kit;
    ctx.outMeta->push_back(meta);

    LOG_DEBUG("TraverseExports: " << key << ", apiType: " << type <<
              ", className: " << parentClass << ", file: " << ctx.file);
}

// 属性读取结果：VALUE=拿到值；SKIP_ACCESSOR=访问器属性（不读值，按 field
// 记录）；SKIP_ERROR=描述符获取/读值抛异常（代理陷阱等，跳过不记录）
enum class PropReadResult {
    VALUE,
    SKIP_ACCESSOR,
    SKIP_ERROR,
};

// 取全局 Object.getOwnPropertyDescriptor（纯内建，取描述符不调用任何访问器）。
// 失败（内建缺失/异常环境）返回 false，调用方退化为直接读值
bool GetOwnPropertyDescriptorFn(napi_env env, napi_value& outFunc)
{
    outFunc = nullptr;
    napi_value global = nullptr;
    if (napi_get_global(env, &global) != napi_ok) {
        return false;
    }
    napi_value objectCtor = nullptr;
    if (napi_get_named_property(env, global, "Object", &objectCtor) != napi_ok) {
        ClearPendingException(env);
        return false;
    }
    if (napi_get_named_property(env, objectCtor, "getOwnPropertyDescriptor", &outFunc) != napi_ok) {
        ClearPendingException(env);
        return false;
    }
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, outFunc, &type) != napi_ok || type != napi_function) {
        // 必须清空：否则调用方持有非函数句柄，napi_call_function 恒失败，
        // 全部属性被误判为 SKIP_ERROR 静默跳过（0 条导出的假"通过"）
        outFunc = nullptr;
        return false;
    }
    return true;
}

// 描述符的 get/set 任一为函数即为访问器（描述符是内建创建的普通对象，读字段安全）
bool DescriptorHasAccessor(napi_env env, napi_value desc)
{
    for (const char* field : { "get", "set" }) {
        napi_value accessor = nullptr;
        if (napi_get_named_property(env, desc, field, &accessor) != napi_ok) {
            ClearPendingException(env);
            continue;
        }
        napi_valuetype type = napi_undefined;
        if (napi_typeof(env, accessor, &type) == napi_ok && type == napi_function) {
            return true;
        }
    }
    return false;
}

// 安全读属性值。访问器属性不读值：类原型上的原生 getter 期望实例 receiver
// （内部 napi_unwrap(this) 后解引用），以原型/导出对象为 receiver 调用会
// 空指针崩溃——真机 SIGSEGV @0x0 即 @ohos.util 的 TextEncoder 原型
// encoding getter（libutil.z.so GetEncoding）此死法，JS 异常清理无法拦截
PropReadResult ReadExportProperty(napi_env env, napi_value descFunc, napi_value object,
                                  napi_value key, napi_value& outValue)
{
    outValue = nullptr;
    if (descFunc != nullptr) {
        napi_value global = nullptr;
        if (napi_get_global(env, &global) == napi_ok) {
            static const uint32_t argc = 2; // 2: getOwnPropertyDescriptor(object, key)
            napi_value args[argc] = { object, key };
            napi_value desc = nullptr;
            if (napi_call_function(env, global, descFunc, argc, args, &desc) != napi_ok) {
                // 代理陷阱抛出：直接读值同样会抛，清异常按不可读跳过
                ClearPendingException(env);
                return PropReadResult::SKIP_ERROR;
            }
            napi_valuetype descType = napi_undefined;
            if (napi_typeof(env, desc, &descType) == napi_ok && descType == napi_object &&
                DescriptorHasAccessor(env, desc)) {
                return PropReadResult::SKIP_ACCESSOR;
            }
        }
    }
    if (napi_get_property(env, object, key, &outValue) != napi_ok) {
        ClearPendingException(env);
        return PropReadResult::SKIP_ERROR;
    }
    return PropReadResult::VALUE;
}

void TraverseExportsImpl(TraverseContext& ctx, napi_value exports,
                         const std::string& parentClass, int depth)
{
    if (exports == nullptr || depth >= MAX_TRAVERSE_DEPTH) {
        return;
    }

    NativeEngine *engine = reinterpret_cast<NativeEngine *>(ctx.env);
    const EcmaVM *vm = engine->GetEcmaVm();
    auto jsVal = LocalValueFromJsValue(exports);
    if (jsVal->IsUndefined() || !jsVal->IsObject(vm)) {
        return;
    }

    auto retP = Local<panda::ObjectRef>(jsVal)->GetOwnPropertyNames(vm);
    napi_value properties = JsValueFromLocalValue(retP);

    uint32_t count = 0;
    if (napi_get_array_length(ctx.env, properties, &count) != napi_ok) {
        LOG_WARN("napi_get_array_length fail: " << ctx.file);
        ClearPendingException(ctx.env);
        return;
    }

    napi_value descFunc = nullptr;
    if (!GetOwnPropertyDescriptorFn(ctx.env, descFunc)) {
        // 内建不可用（异常环境）：退化为直接读值（访问器会被触发）
        LOG_WARN("Object.getOwnPropertyDescriptor unavailable, accessor-skip disabled: " << ctx.file);
    }

    for (uint32_t i = 0; i < count; ++i) {
        napi_value nameVal = nullptr;
        std::string key;
        if (!GetStringElement(ctx.env, properties, i, nameVal, key)) {
            ClearPendingException(ctx.env);
            continue;
        }

        if (IsIgnoredProperty(key.c_str())) {
            continue;
        }

        napi_value value = nullptr;
        PropReadResult readResult = ReadExportProperty(ctx.env, descFunc, exports, nameVal, value);
        if (readResult == PropReadResult::SKIP_ACCESSOR) {
            // 访问器属性按 field 记录（d.ts 的 readonly/getter 声明即 field 语义，
            // 解析侧访问器归一化同口径），不读值不递归
            AppendApiMeta(ctx, key, "field", parentClass);
            LOG_DEBUG("accessor property recorded as field (value not read): " << key <<
                      ", className: " << parentClass << ", file: " << ctx.file);
            continue;
        }
        if (readResult == PropReadResult::SKIP_ERROR) {
            // 代理陷阱/读值抛异常：清异常（防污染后续，见 ModuleCheckGuard）后跳过
            LOG_WARN("property unreadable (proxy trap or getter threw?): file: " << ctx.file <<
                     ", key: " << key << ", className: " << parentClass);
            continue;
        }

        std::string type = GetApiType(ctx.env, value);

        // 加入API元数据
        AppendApiMeta(ctx, key, type, parentClass);

        // 递归遍历class内部成员
        if (type == "class") {
            // 构造函数上的静态成员
            TraverseExportsImpl(ctx, value, key, depth + 1);
            // 原型上的实例方法
            napi_value prototype = nullptr;
            if (napi_get_named_property(ctx.env, value, "prototype", &prototype) == napi_ok) {
                TraverseExportsImpl(ctx, prototype, key, depth + 1);
            }
            continue;
        }
        // 命名空间/枚举对象展开：key 是合法列表中声明的 namespace/enum_class/class 名时，
        // 其成员同样是 API 导出面的一部分（枚举成员、嵌套命名空间成员）。
        if (type == "field" && ctx.expandableNames != nullptr && ctx.expandableNames->count(key) > 0) {
            napi_valuetype valueType;
            if (napi_typeof(ctx.env, value, &valueType) == napi_ok && valueType == napi_object) {
                TraverseExportsImpl(ctx, value, key, depth + 1);
            }
        }
    }
}

bool MatchMethod(const ApiMeta &exposed, const ApiMeta &legal)
{
    // 解析侧已把声明规范化为 "function NAME(" / "static NAME(" / "NAME(" 三种形态
    if (StartsWith(legal.apiText, "function " + exposed.apiText + "(")) {
        return true;
    }
    if (StartsWith(legal.apiText, "static " + exposed.apiText + "(")) {
        return true;
    }
    if (StartsWith(legal.apiText, exposed.apiText + "(")) {
        return true;
    }
    return false;
}

bool MatchNamespace(const ApiMeta &exposed, const ApiMeta &legal)
{
    // 解析侧已把声明规范化为 "namespace NAME"
    return legal.apiText == "namespace " + exposed.apiText;
}

bool MatchClass(const ApiMeta &exposed, const ApiMeta &legal)
{
    // 解析侧已把声明规范化为 "class NAME"
    return legal.apiText == "class " + exposed.apiText;
}

bool MatchInterface(const ApiMeta &exposed, const ApiMeta &legal)
{
    // 解析侧已把声明规范化为 "interface NAME"
    return legal.apiText == "interface " + exposed.apiText;
}

bool MatchEnum(const ApiMeta &exposed, const ApiMeta &legal)
{
    // 解析侧已把声明规范化为 "enum NAME"
    return legal.apiText == "enum " + exposed.apiText;
}

bool MatchExport(const ApiMeta &exposed, const ApiMeta &legal)
{
    // 解析侧已把 re-export 声明规范化为 "export NAME"
    return legal.apiText == "export " + exposed.apiText;
}

// d.ts 中的"类型声明"条目：class/interface/enum/namespace/export。
// 它们描述的都是模块顶层导出的类型/对象本身。
bool IsTypeDeclApiType(const std::string& apiType)
{
    return apiType == "class" || apiType == "interface" || apiType == "enum_class" ||
           apiType == "namespace" || apiType == "export";
}

// method 形态：合法条目为 method/interface/class/export 时的文本匹配
bool MatchMethodApi(const ApiMeta &exposed, const ApiMeta &legal)
{
    if (legal.apiType == "method") {
        return MatchMethod(exposed, legal);
    }
    if (legal.apiType == "interface") {
        return MatchInterface(exposed, legal);
    }
    if (legal.apiType == "class") {
        return MatchClass(exposed, legal);
    }
    if (legal.apiType == "export") {
        return MatchExport(exposed, legal);
    }
    return false;
}

// class 形态：合法条目为 class/enum_class/interface/namespace/export 时的文本匹配
bool MatchClassApi(const ApiMeta &exposed, const ApiMeta &legal)
{
    if (legal.apiType == "class") {
        return MatchClass(exposed, legal);
    }
    // 注意：此处刻意不匹配 legal method（方案A 决策，见 napicheck.md 5.2）：
    // d.ts 声明 function 而实现为 napi_define_class 时报非法——IsClassConstructor
    // 可区分两者，且类构造函数不可无 new 调用，属真实行为断裂而非类型噪音。
    // 如需对齐"全部算合法"的宽松口径，在此补 MatchMethod 分支并在 Match 的
    // apiType 矩阵中补 (class, method) 组合即转方案B。
    if (legal.apiType == "enum_class") {
        return MatchEnum(exposed, legal);
    }
    if (legal.apiType == "interface") {
        return MatchInterface(exposed, legal);
    }
    if (legal.apiType == "namespace") {
        return MatchNamespace(exposed, legal);
    }
    if (legal.apiType == "export") {
        return MatchExport(exposed, legal);
    }
    return false;
}

// field 形态：合法条目为 enum_class/interface/namespace/export 时的文本匹配
bool MatchFieldApi(const ApiMeta &exposed, const ApiMeta &legal)
{
    if (legal.apiType == "enum_class") {
        return MatchEnum(exposed, legal);
    }
    if (legal.apiType == "interface") {
        return MatchInterface(exposed, legal);
    }
    if (legal.apiType == "namespace") {
        return MatchNamespace(exposed, legal);
    }
    if (legal.apiType == "export") {
        return MatchExport(exposed, legal);
    }
    return false;
}

bool MatchApiText(const ApiMeta &exposed, const ApiMeta &legal)
{
    if (legal.apiText == exposed.apiText) {
        return true;
    }
    if (exposed.apiType == "method") {
        return MatchMethodApi(exposed, legal);
    }
    if (exposed.apiType == "class") {
        return MatchClassApi(exposed, legal);
    }
    if (exposed.apiType == "field") {
        return MatchFieldApi(exposed, legal);
    }
    return false;
}

bool IsInherit(const ApiMeta &exposed, const ApiMeta &legal,
               const std::map<std::string, std::string>& inheritMap)
{
    const std::string& from = exposed.className;
    const std::string& to = legal.className;
    if (from == to) {
        return true;
    }

    // BFS 沿继承链上溯，inheritMap 的 value 可能是逗号分隔的多个父类/接口
    std::set<std::string> visited;
    std::queue<std::string> pending;
    visited.insert(from);
    pending.push(from);

    while (!pending.empty()) {
        std::string current = pending.front();
        pending.pop();
        auto it = inheritMap.find(current);
        if (it == inheritMap.end()) {
            continue;
        }
        for (const auto& parent : SplitByComma(it->second)) {
            if (parent.empty()) {
                continue;
            }
            if (parent == to) {
                return true;
            }
            if (visited.find(parent) == visited.end()) {
                visited.insert(parent);
                pending.push(parent);
            }
        }
    }
    return false;
}

// 提取合法条目的"可匹配名"，用于构建名称索引：
// - method: 'function NAME(' / 'static NAME(' / 'NAME(' -> NAME（'typedef void on(' 这类
//   native 头文件形态提取出的名字与 MatchMethod 的前缀判定一致，天然不匹配）
// - 类型声明(解析侧已规范化): 'class NAME' 等 -> NAME
// - 其余(field/enum_instance/裸名): apiText 本身即名字
std::string ExtractMatchName(const ApiMeta& entry)
{
    const std::string& text = entry.apiText;
    if (text.empty()) {
        return "";
    }
    if (entry.apiType == "method") {
        std::string s = text;
        for (const std::string& prefix : {"function ", "static "}) {
            if (StartsWith(s, prefix)) {
                s = s.substr(prefix.size());
            }
        }
        size_t pos = s.find('(');
        if (pos == std::string::npos) {
            // 裸名形态（无参数列表，如 {"apiText":"foo","apiType":"method"}）
            return s.find(' ') == std::string::npos ? s : "";
        }
        if (pos == 0) {
            return "";
        }
        return s.substr(0, pos);
    }
    for (const std::string& kind : {"class ", "interface ", "enum ", "namespace ", "export "}) {
        if (StartsWith(text, kind)) {
            std::string name = text.substr(kind.size());
            // 名字部分必须是单一标识符（无空格），否则不是规范化形态
            if (!name.empty() && name.find(' ') == std::string::npos) {
                return name;
            }
            return "";
        }
    }
    // 裸名（field/enum_instance/未规范化的条目）：不含空格才可作为索引键
    if (text.find(' ') == std::string::npos) {
        return text;
    }
    return "";
}

} // namespace

// ---------------- 日志接口实现 ----------------

void SetLogLevel(LogLevel level)
{
    g_logLevel = level;
}

LogLevel GetLogLevel()
{
    return g_logLevel;
}

bool ParseLogLevel(const std::string& name, LogLevel& out)
{
    std::string lower;
    lower.reserve(name.size());
    for (char c : name) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    if (lower == "error") {
        out = LogLevel::ERROR;
    } else if (lower == "warn" || lower == "warning") {
        out = LogLevel::WARN;
    } else if (lower == "info") {
        out = LogLevel::INFO;
    } else if (lower == "debug") {
        out = LogLevel::DEBUG;
    } else {
        return false;
    }
    return true;
}

bool SetLogFile(const std::string& path)
{
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile.is_open()) {
        g_logFile.close();
    }
    g_logFile.open(path, std::ios::out | std::ios::trunc);
    return g_logFile.is_open();
}

void CloseLogFile()
{
    std::lock_guard<std::mutex> lock(g_logMutex);
    if (g_logFile.is_open()) {
        g_logFile.close();
    }
}

void WriteLog(LogLevel level, const std::string& msg)
{
    std::lock_guard<std::mutex> lock(g_logMutex);
    const std::string line = std::string("[") + LogLevelName(level) + "] " + msg + "\n";
    if (g_logFile.is_open()) {
        g_logFile << line;
        g_logFile.flush();
    }
    // WARN/ERROR 始终打到 stderr，保证告警可见；其余级别无日志文件时打到 stderr
    if (level <= LogLevel::WARN || !g_logFile.is_open()) {
        std::cerr << line;
    }
}

// ---------------- 引擎环境 ----------------

napi_env CreateArkEnv()
{
    panda::RuntimeOption option;
    option.SetGcType(panda::RuntimeOption::GC_TYPE::GEN_GC);
    option.SetLogLevel(panda::RuntimeOption::LOG_LEVEL::ERROR);
    option.SetDebuggerLibraryPath("");
    EcmaVM* vm = panda::JSNApi::CreateJSVM(option);
    if (vm == nullptr) {
        return nullptr;
    }
    ArkNativeEngine* arkEngine = new (std::nothrow) ArkNativeEngine(vm, nullptr);
    if (arkEngine == nullptr) {
        panda::JSNApi::DestroyJSVM(vm);
        return nullptr;
    }
    auto cleanEnv = [vm]() {
        if (vm != nullptr) {
            panda::JSNApi::DestroyJSVM(vm);
        }
    };
    arkEngine->SetCleanEnv(cleanEnv);
    napi_env env = reinterpret_cast<napi_env>(arkEngine);
    return env;
}

void DestroyArkEnv(napi_env env)
{
    if (env != nullptr) {
        napi_destroy_runtime(env);
    }
}

void ClearPendingException(napi_env env)
{
    bool pending = false;
    if (napi_is_exception_pending(env, &pending) == napi_ok && pending) {
        napi_value exception = nullptr;
        napi_get_and_clear_last_exception(env, &exception);
    }
}

ModuleCheckGuard::ModuleCheckGuard(napi_env env) : env_(env)
{
    // 先清异常再开 scope：napi_open_handle_scope 同样会被挂起异常短路
    ClearPendingException(env_);
    napi_open_handle_scope(env_, &scope_);
}

ModuleCheckGuard::~ModuleCheckGuard()
{
    if (env_ != nullptr) {
        ClearPendingException(env_);
        if (scope_ != nullptr) {
            napi_close_handle_scope(env_, scope_);
        }
    }
}

// 忽略内置属性名单
bool IsIgnoredProperty(const char* name)
{
    if (!name || strlen(name) == 0) {
        return true;
    }
    // 引擎挂载的内部标记（ets_runtime NapiWrapperString），非声明 API：
    // 实测 18 个 AbilityKit 模块因该属性被误报非法
    if (strcmp(name, "_napiwrapper") == 0) {
        return true;
    }
    // TS 枚举反向映射键（"0"、"1"、"-1"）：native 模块模拟 TS 枚举时同时挂
    // 数字->成员名 的反查键，白名单 enum_instance 只有命名键，逐条必伪报
    // （实测 137/443 条"非法"为数字键）
    const char* p = name;
    if (*p == '-') {
        p++;
    }
    if (*p != '\0') {
        bool allDigits = true;
        for (; *p != '\0'; ++p) {
            if (*p < '0' || *p > '9') {
                allDigits = false;
                break;
            }
        }
        if (allDigits) {
            return true;
        }
    }
    return strcmp(name, "__proto__") == 0 ||
        strcmp(name, "constructor") == 0 ||
        strcmp(name, "length") == 0 ||
        strcmp(name, "name") == 0 ||
        strcmp(name, "prototype") == 0 ||
        strcmp(name, "toString") == 0 ||
        strcmp(name, "toLocaleString") == 0 ||
        strcmp(name, "valueOf") == 0 ||
        strcmp(name, "hasOwnProperty") == 0 ||
        strcmp(name, "isPrototypeOf") == 0 ||
        strcmp(name, "propertyIsEnumerable") == 0;
}

// 从NAPI值获取类型
std::string GetApiType(napi_env env, napi_value value)
{
    napi_valuetype type;
    if (napi_typeof(env, value, &type) != napi_ok) {
        return "field";
    }

    if (type == napi_function) {
        NativeEngine *engine = reinterpret_cast<NativeEngine *>(env);
        const EcmaVM *vm = engine->GetEcmaVm();
        auto jsVal = LocalValueFromJsValue(value);
        if (jsVal->IsClassConstructor(vm)) {
            return "class";
        }
        return "method";
    }
    return "field";
}

// 递归遍历导出对象
void TraverseExports(TraverseContext& ctx, napi_value exports, const std::string& parentClass)
{
    TraverseExportsImpl(ctx, exports, parentClass, 0);
}

void BuildLegalIndex(const std::vector<ApiMeta>& legalList, LegalIndex& index)
{
    index.clear();
    for (const ApiMeta& entry : legalList) {
        std::string name = ExtractMatchName(entry);
        if (!name.empty()) {
            index[name].push_back(&entry);
        }
    }
}

void CollectExpandableNames(const std::vector<ApiMeta>& legalList, std::set<std::string>& names)
{
    names.clear();
    for (const ApiMeta& entry : legalList) {
        // namespace/enum_class/class 的名字（className 已取末段；canonical
        // 声明的 apiText 形如 "class NAME"，两者一致，双保险都收集）
        if (entry.apiType == "namespace" || entry.apiType == "enum_class" || entry.apiType == "class") {
            if (!entry.className.empty()) {
                names.insert(entry.className);
            }
            std::string name = ExtractMatchName(entry);
            if (!name.empty()) {
                names.insert(name);
            }
        }
    }
}

// 判断字符串是否以指定前缀开头
bool StartsWith(const std::string& str, const std::string& prefix)
{
    if (prefix.empty()) {
        return true;
    }
    if (str.size() < prefix.size()) {
        return false;
    }
    return str.compare(0, prefix.size(), prefix) == 0;
}

// 判断字符串是否以指定后缀结尾
bool EndsWith(const std::string& str, const std::string& suffix)
{
    if (suffix.empty()) {
        return true;
    }
    if (str.size() < suffix.size()) {
        return false;
    }
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

// 判断两个字符串是否相等（忽略大小写）
bool EqualsIgnoreCase(const std::string& a, const std::string& b)
{
    if (a.size() != b.size()) {
        return false;
    }
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

std::ostream& operator<<(std::ostream& os, const ApiMeta& v)
{
    os << "apiText: " << v.apiText << std::endl;
    os << "file: " << v.file << std::endl;
    os << "apiType: " << v.apiType << std::endl;
    os << "className: " << v.className << std::endl;
    os << "kit: " << v.kit << std::endl;
    return os;
}

bool Match(const ApiMeta &exposed, const ApiMeta &legal, const std::map<std::string, std::string> &inheritMap)
{
    // 预筛：合法 apiText 必须包含暴露名
    if (legal.apiText.find(exposed.apiText) == std::string::npos) {
        return false;
    }
    // kit 校验：合法条目未填 kit 时视为通配（ignore list 场景）
    if (!legal.kit.empty() && exposed.kit != legal.kit) {
        return false;
    }
    if (exposed.apiType != legal.apiType && !(
        // 方案A：矩阵刻意不含 (class, method) 组合——d.ts 声明 function 而实现为
        // 类构造函数时报非法（真实行为断裂），见 napicheck.md 5.2
        (exposed.apiType == "method" && legal.apiType == "class") ||
        (exposed.apiType == "method" && legal.apiType == "interface") ||
        (exposed.apiType == "method" && legal.apiType == "export") ||
        (exposed.apiType == "class" && legal.apiType == "namespace") ||
        (exposed.apiType == "class" && legal.apiType == "enum_class") ||
        (exposed.apiType == "class" && legal.apiType == "interface") ||
        (exposed.apiType == "class" && legal.apiType == "export") ||
        (exposed.apiType == "field" && legal.apiType == "namespace") ||
        (exposed.apiType == "field" && legal.apiType == "enum_class") ||
        (exposed.apiType == "field" && legal.apiType == "enum_instance") ||
        (exposed.apiType == "field" && legal.apiType == "interface") ||
        (exposed.apiType == "field" && legal.apiType == "export"))) {
        return false;
    }
    // className 校验：d.ts 的 className 是"声明作用域"（命名空间或类）。
    //  - 大小写不敏感：d.ts 命名空间/类名（avSession、AtManager）与运行时
    //    属性名/模块名末段（avsession、atManager）大小写常不一致，精确比较
    //    会把整个命名空间级函数/单例成员全部伪报（实测 34+24 条）；
    //  - 相等：直接通过（类成员 / 命名空间对象成员）
    //  - 暴露项为模块顶层（className 为空）时：
    //      a) 合法条目属于本模块命名空间（className == 模块名末段，如 taskpool）——
    //         d.ts 的 declare namespace 在运行时即模块顶层导出面；
    //      b) 合法条目是类型声明（class/interface/enum/namespace/export）——
    //         类型必然作为模块顶层成员导出，名字由 MatchApiText 精确核对。
    //  - 否则不匹配，防止顶层暴露项误匹配任意类内同名方法（漏报漏洞）。
    //  - 暴露项为类成员时：可沿继承链匹配父类/接口中声明的合法条目。
    if (!EqualsIgnoreCase(legal.className, exposed.className)) {
        if (exposed.className.empty()) {
            bool inModuleNamespace = !legal.className.empty() &&
                EqualsIgnoreCase(legal.className, GetNamespaceOfModule(exposed.file));
            if (!inModuleNamespace && !IsTypeDeclApiType(legal.apiType)) {
                return false;
            }
        } else {
            // 类成员沿继承链匹配父类/接口中的声明：父类可能在另一个声明文件中
            // （跨文件继承），此路径不参与 file 校验，由类名与文本形态共同约束。
            return IsInherit(exposed, legal, inheritMap) && MatchApiText(exposed, legal);
        }
    }
    // file 校验：合法条目未填 file 时视为通配；目录前缀不敏感（EndsWith 兜底
    // "api/@ohos.x" vs "@ohos.x" 两种坐标系写法）
    if (!legal.file.empty() && legal.file != exposed.file && !EndsWith(legal.file, "/" + exposed.file)) {
        return false;
    }
    if (!MatchApiText(exposed, legal)) {
        return false;
    }
    return true;
}

}
