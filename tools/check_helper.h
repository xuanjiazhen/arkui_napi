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

#ifndef CHECK_HELPER_H
#define CHECK_HELPER_H

#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

#include "api_meta.h"
#include "napi/native_api.h"

namespace ohos::check {

// ---------------- 日志框架（--log-level / --log-file） ----------------
// 级别：ERROR(0) < WARN(1) < INFO(2) < DEBUG(3)，默认 INFO。
// WARN/ERROR 始终输出到 stderr（保证告警可见）；其余级别仅在未指定
// --log-file 时输出到 stderr，指定后只写文件。
enum class LogLevel : int {
    ERROR = 0,
    WARN = 1,
    INFO = 2,
    DEBUG = 3,
};

void SetLogLevel(LogLevel level);
LogLevel GetLogLevel();
bool ParseLogLevel(const std::string& name, LogLevel& out);   // DEBUG/INFO/WARN/ERROR
bool SetLogFile(const std::string& path);                     // 打开失败返回 false
void CloseLogFile();
void WriteLog(LogLevel level, const std::string& msg);

#define CHECKAPI_LOG(lv, expr)                                                       \
    do {                                                                             \
        if ((lv) <= ohos::check::GetLogLevel()) {                                    \
            std::ostringstream logOs_;                                               \
            logOs_ << expr;                                                          \
            ohos::check::WriteLog((lv), logOs_.str());                               \
        }                                                                            \
    } while (0)

#define LOG_ERROR(expr) CHECKAPI_LOG(ohos::check::LogLevel::ERROR, expr)
#define LOG_WARN(expr)  CHECKAPI_LOG(ohos::check::LogLevel::WARN, expr)
#define LOG_INFO(expr)  CHECKAPI_LOG(ohos::check::LogLevel::INFO, expr)
#define LOG_DEBUG(expr) CHECKAPI_LOG(ohos::check::LogLevel::DEBUG, expr)

// ---------------- 引擎环境 ----------------

// 创建轻量 Ark 引擎环境（CreateJSVM + ArkNativeEngine）
napi_env CreateArkEnv();

// 销毁环境（napi_destroy_runtime -> ~NativeEngine -> cleanEnv -> DestroyJSVM）
void DestroyArkEnv(napi_env env);

// 清理 VM 上挂起的 JS 异常，避免影响后续检查
void ClearPendingException(napi_env env);

// 单模块检查守卫：进入时清残留挂起异常并开独立 handle scope，退出时再清一次
// 异常并关 scope。此前全程共用一个 scope 且从不清异常：任一模块遍历导出时
// getter 抛出的异常会永久污染 env（napi_get_named_property 带 NAPI_PREAMBLE，
// 挂起异常下直接短路），导致之后所有模块的 requireNapi 查找全部失败
// （真机实测 515/597 个模块连续误报 "requireNapi not found"）。
// 不可拷贝；构造顺序为先清异常再开 scope（open 带同样会短路）。
class ModuleCheckGuard {
public:
    explicit ModuleCheckGuard(napi_env env);
    ~ModuleCheckGuard();
    ModuleCheckGuard(const ModuleCheckGuard&) = delete;
    ModuleCheckGuard& operator=(const ModuleCheckGuard&) = delete;

private:
    napi_env env_ = nullptr;
    napi_handle_scope scope_ = nullptr;
};

// ---------------- 导出遍历 ----------------

// 忽略内置属性名单
bool IsIgnoredProperty(const char* name);

// 从napi值获取类型: class / method / field
std::string GetApiType(napi_env env, napi_value value);

// 贯穿一次导出面遍历的不变参数（环境、归属信息与输出口）
struct TraverseContext {
    napi_env env = nullptr;
    std::string kit;
    std::string file;
    const std::set<std::string>* expandableNames = nullptr;
    std::vector<ApiMeta>* outMeta = nullptr;
};

// 递归遍历导出对象。
// ctx.expandableNames: 需要展开成员的"已声明命名空间/枚举/类名"集合（来自合法列表
// namespace/enum_class/class 条目的 className 与规范名，双保险收集）；
// 传 nullptr 则只展开 class。
void TraverseExports(TraverseContext& ctx, napi_value exports, const std::string& parentClass);

// 从合法列表构建名称索引（匹配前置加速，见 check_match.cpp）
using LegalIndex = std::unordered_map<std::string, std::vector<const ApiMeta*>>;
void BuildLegalIndex(const std::vector<ApiMeta>& legalList, LegalIndex& index);

// 收集"可展开"的声明名集合（namespace/enum_class/class 条目的名字）。
// TraverseExports 遍历到同名 field 对象时递归展开其成员——这些对象的
// 成员（枚举成员、嵌套命名空间成员）同样属于 API 导出面。
void CollectExpandableNames(const std::vector<ApiMeta>& legalList, std::set<std::string>& names);

// ---------------- 匹配 ----------------

bool StartsWith(const std::string& str, const std::string& prefix);
bool EndsWith(const std::string& str, const std::string& suffix);
// 大小写不敏感相等比较：d.ts 的 namespace/类名（avSession、AtManager）与
// 运行时属性名/模块名末段（avsession、atManager）大小写常不一致，
// className 级比较需容忍（同名不同大小写视为同类，防大面积伪报）
bool EqualsIgnoreCase(const std::string& a, const std::string& b);

bool Match(const ApiMeta &exposed, const ApiMeta &legal, const std::map<std::string, std::string> &inheritMap);

}

#endif
