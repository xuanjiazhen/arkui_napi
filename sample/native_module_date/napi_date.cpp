// Copyright (c) 2026 Huawei Device Co., Ltd.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>
#include <map>
#include "native_api.h"

namespace {

// 时间戳类型
typedef std::chrono::time_point<std::chrono::system_clock> TimePoint;

// 时区偏移映射
std::map<std::string, int> timezoneOffsets = {
    {"UTC", 0},
    {"GMT", 0},
    {"CST", 8},
    {"EST", -5},
    {"PST", -8},
    {"MST", -7},
    {"CST", -6},
    {"AST", -4},
    {"HST", -10}
};

// 星期几名称
const std::string weekdays[] = {
    "Sunday", "Monday", "Tuesday", "Wednesday",
    "Thursday", "Friday", "Saturday"
};

// 月份名称
const std::string months[] = {
    "January", "February", "March", "April", "May", "June",
    "July", "August", "September", "October", "November", "December"
};

// 将时间点转换为time_t
std::time_t toTimeT(const TimePoint& tp) {
    return std::chrono::system_clock::to_time_t(tp);
}

// 将time_t转换为时间点
TimePoint fromTimeT(std::time_t tt) {
    return std::chrono::system_clock::from_time_t(tt);
}

// 获取当前时间点
TimePoint now() {
    return std::chrono::system_clock::now();
}

// 检查是否为闰年
bool isLeapYear(int year) {
    if (year % 4 != 0) return false;
    if (year % 100 != 0) return true;
    return year % 400 == 0;
}

// 获取月份天数
int getDaysInMonth(int year, int month) {
    static const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 1 && isLeapYear(year)) {
        return 29;
    }
    return daysInMonth[month];
}

// 格式化日期时间
std::string formatDateTime(const TimePoint& tp, const std::string& format) {
    std::time_t tt = toTimeT(tp);
    std::tm* tm = std::localtime(&tt);
    std::stringstream ss;
    
    for (size_t i = 0; i < format.size(); ++i) {
        if (format[i] == '%') {
            if (i + 1 < format.size()) {
                switch (format[i + 1]) {
                    case 'Y': ss << std::setw(4) << std::setfill('0') << (tm->tm_year + 1900); break;
                    case 'm': ss << std::setw(2) << std::setfill('0') << (tm->tm_mon + 1); break;
                    case 'd': ss << std::setw(2) << std::setfill('0') << tm->tm_mday; break;
                    case 'H': ss << std::setw(2) << std::setfill('0') << tm->tm_hour; break;
                    case 'M': ss << std::setw(2) << std::setfill('0') << tm->tm_min; break;
                    case 'S': ss << std::setw(2) << std::setfill('0') << tm->tm_sec; break;
                    case 'w': ss << tm->tm_wday; break;
                    case 'a': ss << weekdays[tm->tm_wday].substr(0, 3); break;
                    case 'A': ss << weekdays[tm->tm_wday]; break;
                    case 'b': ss << months[tm->tm_mon].substr(0, 3); break;
                    case 'B': ss << months[tm->tm_mon]; break;
                    default: ss << format[i] << format[i + 1]; break;
                }
                ++i;
            } else {
                ss << format[i];
            }
        } else {
            ss << format[i];
        }
    }
    
    return ss.str();
}

// 解析日期字符串
TimePoint parseDateTime(const std::string& dateStr, const std::string& format) {
    std::tm tm = {};
    std::istringstream ss(dateStr);
    
    for (size_t i = 0; i < format.size(); ++i) {
        if (format[i] == '%') {
            if (i + 1 < format.size()) {
                switch (format[i + 1]) {
                    case 'Y': ss >> std::get_time(&tm, "%Y"); break;
                    case 'm': ss >> std::get_time(&tm, "%m"); break;
                    case 'd': ss >> std::get_time(&tm, "%d"); break;
                    case 'H': ss >> std::get_time(&tm, "%H"); break;
                    case 'M': ss >> std::get_time(&tm, "%M"); break;
                    case 'S': ss >> std::get_time(&tm, "%S"); break;
                    default: break;
                }
                ++i;
            }
        }
    }
    
    return fromTimeT(std::mktime(&tm));
}

// 计算日期差值（天数）
double dateDiffDays(const TimePoint& start, const TimePoint& end) {
    auto duration = end - start;
    return std::chrono::duration<double, std::ratio<86400>>(duration).count();
}

// 添加天数
TimePoint addDays(const TimePoint& tp, int days) {
    return tp + std::chrono::hours(24 * days);
}

// 添加月份
TimePoint addMonths(const TimePoint& tp, int months) {
    std::time_t tt = toTimeT(tp);
    std::tm* tm = std::localtime(&tt);
    
    int year = tm->tm_year + 1900;
    int month = tm->tm_mon + 1;
    int day = tm->tm_mday;
    
    month += months;
    while (month > 12) {
        month -= 12;
        year++;
    }
    while (month < 1) {
        month += 12;
        year--;
    }
    
    int daysInMonth = getDaysInMonth(year, month - 1);
    if (day > daysInMonth) {
        day = daysInMonth;
    }
    
    tm->tm_year = year - 1900;
    tm->tm_mon = month - 1;
    tm->tm_mday = day;
    
    return fromTimeT(std::mktime(tm));
}

// 添加年份
TimePoint addYears(const TimePoint& tp, int years) {
    std::time_t tt = toTimeT(tp);
    std::tm* tm = std::localtime(&tt);
    
    int year = tm->tm_year + 1900 + years;
    int month = tm->tm_mon;
    int day = tm->tm_mday;
    
    int daysInMonth = getDaysInMonth(year, month);
    if (day > daysInMonth) {
        day = daysInMonth;
    }
    
    tm->tm_year = year - 1900;
    tm->tm_mday = day;
    
    return fromTimeT(std::mktime(tm));
}

// 获取星期几
int getDayOfWeek(const TimePoint& tp) {
    std::time_t tt = toTimeT(tp);
    std::tm* tm = std::localtime(&tt);
    return tm->tm_wday;
}

// 获取季度
int getQuarter(const TimePoint& tp) {
    std::time_t tt = toTimeT(tp);
    std::tm* tm = std::localtime(&tt);
    return (tm->tm_mon / 3) + 1;
}

// 获取年份的第几周
int getWeekOfYear(const TimePoint& tp) {
    std::time_t tt = toTimeT(tp);
    std::tm* tm = std::localtime(&tt);
    return tm->tm_yday / 7 + 1;
}

// 检查是否是同一天
bool isSameDay(const TimePoint& tp1, const TimePoint& tp2) {
    std::time_t tt1 = toTimeT(tp1);
    std::time_t tt2 = toTimeT(tp2);
    std::tm* tm1 = std::localtime(&tt1);
    std::tm* tm2 = std::localtime(&tt2);
    
    return tm1->tm_year == tm2->tm_year && 
           tm1->tm_mon == tm2->tm_mon && 
           tm1->tm_mday == tm2->tm_mday;
}

// 检查是否在指定范围内
bool isInRange(const TimePoint& tp, const TimePoint& start, const TimePoint& end) {
    return tp >= start && tp <= end;
}

// 获取相对时间描述
std::string getRelativeTime(const TimePoint& tp) {
    auto nowTp = now();
    auto duration = nowTp - tp;
    
    double seconds = std::chrono::duration<double>(duration).count();
    if (seconds < 60) {
        return "刚刚";
    }
    
    double minutes = seconds / 60;
    if (minutes < 60) {
        return std::to_string(static_cast<int>(minutes)) + "分钟前";
    }
    
    double hours = minutes / 60;
    if (hours < 24) {
        return std::to_string(static_cast<int>(hours)) + "小时前";
    }
    
    double days = hours / 24;
    if (days < 30) {
        return std::to_string(static_cast<int>(days)) + "天前";
    }
    
    double months = days / 30;
    if (months < 12) {
        return std::to_string(static_cast<int>(months)) + "个月前";
    }
    
    double years = months / 12;
    return std::to_string(static_cast<int>(years)) + "年前";
}

// 时区转换
TimePoint convertTimezone(const TimePoint& tp, const std::string& fromTz, const std::string& toTz) {
    int fromOffset = timezoneOffsets.count(fromTz) ? timezoneOffsets[fromTz] : 0;
    int toOffset = timezoneOffsets.count(toTz) ? timezoneOffsets[toTz] : 0;
    int offsetDiff = (toOffset - fromOffset) * 3600; // 转换为秒
    
    return tp + std::chrono::seconds(offsetDiff);
}

// 获取ISO字符串
std::string toISOString(const TimePoint& tp) {
    std::time_t tt = toTimeT(tp);
    std::tm* tm = std::gmtime(&tt);
    std::stringstream ss;
    ss << std::setw(4) << std::setfill('0') << (tm->tm_year + 1900) << '-' 
       << std::setw(2) << std::setfill('0') << (tm->tm_mon + 1) << '-' 
       << std::setw(2) << std::setfill('0') << tm->tm_mday << 'T' 
       << std::setw(2) << std::setfill('0') << tm->tm_hour << ':' 
       << std::setw(2) << std::setfill('0') << tm->tm_min << ':' 
       << std::setw(2) << std::setfill('0') << tm->tm_sec << 'Z';
    return ss.str();
}

// 从ISO字符串解析
TimePoint fromISOString(const std::string& isoStr) {
    std::tm tm = {};
    std::istringstream ss(isoStr);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return fromTimeT(std::mktime(&tm));
}

// 计算年龄
int calculateAge(const TimePoint& birthDate) {
    auto nowTp = now();
    std::time_t tt1 = toTimeT(birthDate);
    std::time_t tt2 = toTimeT(nowTp);
    std::tm* tm1 = std::localtime(&tt1);
    std::tm* tm2 = std::localtime(&tt2);
    
    int age = tm2->tm_year - tm1->tm_year;
    if (tm2->tm_mon < tm1->tm_mon || 
        (tm2->tm_mon == tm1->tm_mon && tm2->tm_mday < tm1->tm_mday)) {
        age--;
    }
    return age;
}

// 检查是否是工作日
bool isWeekday(const TimePoint& tp) {
    int day = getDayOfWeek(tp);
    return day != 0 && day != 6;
}

// 检查是否是周末
bool isWeekend(const TimePoint& tp) {
    int day = getDayOfWeek(tp);
    return day == 0 || day == 6;
}

} // namespace

// 获取当前时间戳
static napi_value GetCurrentTimestamp(napi_env env, napi_callback_info info) {
    auto tp = now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
    
    napi_value result;
    napi_create_double(env, timestamp, &result);
    return result;
}

// 格式化日期
static napi_value FormatDate(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp and format string");
        return nullptr;
    }
    
    double timestamp;
    napi_get_value_double(env, args[0], &timestamp);
    
    size_t formatLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &formatLength);
    char* formatBuffer = new char[formatLength + 1];
    napi_get_value_string_utf8(env, args[1], formatBuffer, formatLength + 1, nullptr);
    std::string format(formatBuffer);
    delete[] formatBuffer;
    
    TimePoint tp = fromTimeT(static_cast<std::time_t>(timestamp / 1000));
    std::string result = formatDateTime(tp, format);
    
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 解析日期字符串
static napi_value ParseDate(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected date string and format string");
        return nullptr;
    }
    
    size_t dateLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &dateLength);
    char* dateBuffer = new char[dateLength + 1];
    napi_get_value_string_utf8(env, args[0], dateBuffer, dateLength + 1, nullptr);
    std::string dateStr(dateBuffer);
    delete[] dateBuffer;
    
    size_t formatLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &formatLength);
    char* formatBuffer = new char[formatLength + 1];
    napi_get_value_string_utf8(env, args[1], formatBuffer, formatLength + 1, nullptr);
    std::string format(formatBuffer);
    delete[] formatBuffer;
    
    TimePoint tp = parseDateTime(dateStr, format);
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
    
    napi_value result;
    napi_create_double(env, timestamp, &result);
    return result;
}

// 计算日期差值
static napi_value DateDiff(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected two timestamps");
        return nullptr;
    }
    
    double timestamp1, timestamp2;
    napi_get_value_double(env, args[0], &timestamp1);
    napi_get_value_double(env, args[1], &timestamp2);
    
    TimePoint tp1 = fromTimeT(static_cast<std::time_t>(timestamp1 / 1000));
    TimePoint tp2 = fromTimeT(static_cast<std::time_t>(timestamp2 / 1000));
    double diff = dateDiffDays(tp1, tp2);
    
    napi_value result;
    napi_create_double(env, diff, &result);
    return result;
}

// 添加天数
static napi_value AddDays(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp and days");
        return nullptr;
    }
    
    double timestamp;
    int days;
    napi_get_value_double(env, args[0], &timestamp);
    napi_get_value_int32(env, args[1], &days);
    
    TimePoint tp = fromTimeT(static_cast<std::time_t>(timestamp / 1000));
    TimePoint newTp = addDays(tp, days);
    auto newTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(newTp.time_since_epoch()).count();
    
    napi_value result;
    napi_create_double(env, newTimestamp, &result);
    return result;
}

// 添加月份
static napi_value AddMonths(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp and months");
        return nullptr;
    }
    
    double timestamp;
    int months;
    napi_get_value_double(env, args[0], &timestamp);
    napi_get_value_int32(env, args[1], &months);
    
    TimePoint tp = fromTimeT(static_cast<std::time_t>(timestamp / 1000));
    TimePoint newTp = addMonths(tp, months);
    auto newTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(newTp.time_since_epoch()).count();
    
    napi_value result;
    napi_create_double(env, newTimestamp, &result);
    return result;
}

// 添加年份
static napi_value AddYears(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp and years");
        return nullptr;
    }
    
    double timestamp;
    int years;
    napi_get_value_double(env, args[0], &timestamp);
    napi_get_value_int32(env, args[1], &years);
    
    TimePoint tp = fromTimeT(static_cast<std::time_t>(timestamp / 1000));
    TimePoint newTp = addYears(tp, years);
    auto newTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(newTp.time_since_epoch()).count();
    
    napi_value result;
    napi_create_double(env, newTimestamp, &result);
    return result;
}

// 检查是否为闰年
static napi_value IsLeapYear(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected year");
        return nullptr;
    }
    
    int year;
    napi_get_value_int32(env, args[0], &year);
    
    bool result = isLeapYear(year);
    napi_value resultValue;
    napi_create_boolean(env, result, &resultValue);
    return resultValue;
}

// 获取月份天数
static napi_value GetDaysInMonth(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected year and month");
        return nullptr;
    }
    
    int year, month;
    napi_get_value_int32(env, args[0], &year);
    napi_get_value_int32(env, args[1], &month);
    
    int days = getDaysInMonth(year, month - 1); // 月份从0开始
    napi_value result;
    napi_create_int32(env, days, &result);
    return result;
}

// 获取星期几
static napi_value GetDayOfWeek(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp");
        return nullptr;
    }
    
    double timestamp;
    napi_get_value_double(env, args[0], &timestamp);
    
    TimePoint tp = fromTimeT(static_cast<std::time_t>(timestamp / 1000));
    int day = getDayOfWeek(tp);
    
    napi_value result;
    napi_create_int32(env, day, &result);
    return result;
}

// 获取季度
static napi_value GetQuarter(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp");
        return nullptr;
    }
    
    double timestamp;
    napi_get_value_double(env, args[0], &timestamp);
    
    TimePoint tp = fromTimeT(static_cast<std::time_t>(timestamp / 1000));
    int quarter = getQuarter(tp);
    
    napi_value result;
    napi_create_int32(env, quarter, &result);
    return result;
}

// 获取年份的第几周
static napi_value GetWeekOfYear(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp");
        return nullptr;
    }
    
    double timestamp;
    napi_get_value_double(env, args[0], &timestamp);
    
    TimePoint tp = fromTimeT(static_cast<std::time_t>(timestamp / 1000));
    int week = getWeekOfYear(tp);
    
    napi_value result;
    napi_create_int32(env, week, &result);
    return result;
}

// 检查是否是同一天
static napi_value IsSameDay(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected two timestamps");
        return nullptr;
    }
    
    double timestamp1, timestamp2;
    napi_get_value_double(env, args[0], &timestamp1);
    napi_get_value_double(env, args[1], &timestamp2);
    
    TimePoint tp1 = fromTimeT(static_cast<std::time_t>(timestamp1 / 1000));
    TimePoint tp2 = fromTimeT(static_cast<std::time_t>(timestamp2 / 1000));
    bool result = isSameDay(tp1, tp2);
    
    napi_value resultValue;
    napi_create_boolean(env, result, &resultValue);
    return resultValue;
}

// 检查是否在指定范围内
static napi_value IsInRange(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 3) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp, start and end");
        return nullptr;
    }
    
    double timestamp, start, end;
    napi_get_value_double(env, args[0], &timestamp);
    napi_get_value_double(env, args[1], &start);
    napi_get_value_double(env, args[2], &end);
    
    TimePoint tp = fromTimeT(static_cast<std::time_t>(timestamp / 1000));
    TimePoint startTp = fromTimeT(static_cast<std::time_t>(start / 1000));
    TimePoint endTp = fromTimeT(static_cast<std::time_t>(end / 1000));
    bool result = isInRange(tp, startTp, endTp);
    
    napi_value resultValue;
    napi_create_boolean(env, result, &resultValue);
    return resultValue;
}

// 获取相对时间描述
static napi_value GetRelativeTime(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp");
        return nullptr;
    }
    
    double timestamp;
    napi_get_value_double(env, args[0], &timestamp);
    
    TimePoint tp = fromTimeT(static_cast<std::time_t>(timestamp / 1000));
    std::string result = getRelativeTime(tp);
    
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 时区转换
static napi_value ConvertTimezone(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 3) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp, from timezone and to timezone");
        return nullptr;
    }
    
    double timestamp;
    napi_get_value_double(env, args[0], &timestamp);
    
    size_t fromTzLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &fromTzLength);
    char* fromTzBuffer = new char[fromTzLength + 1];
    napi_get_value_string_utf8(env, args[1], fromTzBuffer, fromTzLength + 1, nullptr);
    std::string fromTz(fromTzBuffer);
    delete[] fromTzBuffer;
    
    size_t toTzLength;
    napi_get_value_string_utf8(env, args[2], nullptr, 0, &toTzLength);
    char* toTzBuffer = new char[toTzLength + 1];
    napi_get_value_string_utf8(env, args[2], toTzBuffer, toTzLength + 1, nullptr);
    std::string toTz(toTzBuffer);
    delete[] toTzBuffer;
    
    TimePoint tp = fromTimeT(static_cast<std::time_t>(timestamp / 1000));
    TimePoint newTp = convertTimezone(tp, fromTz, toTz);
    auto newTimestamp = std::chrono::duration_cast<std::chrono::milliseconds>(newTp.time_since_epoch()).count();
    
    napi_value result;
    napi_create_double(env, newTimestamp, &result);
    return result;
}

// 获取时间戳的各个部分
static napi_value GetDateParts(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp");
        return nullptr;
    }
    
    double timestamp;
    napi_get_value_double(env, args[0], &timestamp);
    
    TimePoint tp = fromTimeT(static_cast<std::time_t>(timestamp / 1000));
    std::time_t tt = toTimeT(tp);
    std::tm* tm = std::localtime(&tt);
    
    napi_value result;
    napi_create_object(env, &result);
    
    napi_value year, month, day, hour, minute, second, weekday;
    napi_create_int32(env, tm->tm_year + 1900, &year);
    napi_create_int32(env, tm->tm_mon + 1, &month);
    napi_create_int32(env, tm->tm_mday, &day);
    napi_create_int32(env, tm->tm_hour, &hour);
    napi_create_int32(env, tm->tm_min, &minute);
    napi_create_int32(env, tm->tm_sec, &second);
    napi_create_int32(env, tm->tm_wday, &weekday);
    
    napi_set_named_property(env, result, "year", year);
    napi_set_named_property(env, result, "month", month);
    napi_set_named_property(env, result, "day", day);
    napi_set_named_property(env, result, "hour", hour);
    napi_set_named_property(env, result, "minute", minute);
    napi_set_named_property(env, result, "second", second);
    napi_set_named_property(env, result, "weekday", weekday);
    
    return result;
}

// 格式化时间为ISO字符串
static napi_value ToISOString(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp");
        return nullptr;
    }
    
    double timestamp;
    napi_get_value_double(env, args[0], &timestamp);
    
    TimePoint tp = fromTimeT(static_cast<std::time_t>(timestamp / 1000));
    std::string result = toISOString(tp);
    
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 从ISO字符串解析
static napi_value FromISOString(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected ISO string");
        return nullptr;
    }
    
    size_t isoLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &isoLength);
    char* isoBuffer = new char[isoLength + 1];
    napi_get_value_string_utf8(env, args[0], isoBuffer, isoLength + 1, nullptr);
    std::string isoStr(isoBuffer);
    delete[] isoBuffer;
    
    TimePoint tp = fromISOString(isoStr);
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch()).count();
    
    napi_value result;
    napi_create_double(env, timestamp, &result);
    return result;
}

// 获取当前日期
static napi_value GetCurrentDate(napi_env env, napi_callback_info info) {
    auto tp = now();
    std::string result = formatDateTime(tp, "%Y-%m-%d");
    
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 获取当前时间
static napi_value GetCurrentTime(napi_env env, napi_callback_info info) {
    auto tp = now();
    std::string result = formatDateTime(tp, "%H:%M:%S");
    
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 获取当前日期时间
static napi_value GetCurrentDateTime(napi_env env, napi_callback_info info) {
    auto tp = now();
    std::string result = formatDateTime(tp, "%Y-%m-%d %H:%M:%S");
    
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 计算年龄
static napi_value CalculateAge(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected birth timestamp");
        return nullptr;
    }
    
    double timestamp;
    napi_get_value_double(env, args[0], &timestamp);
    
    TimePoint tp = fromTimeT(static_cast<std::time_t>(timestamp / 1000));
    int age = calculateAge(tp);
    
    napi_value result;
    napi_create_int32(env, age, &result);
    return result;
}

// 检查是否是工作日
static napi_value IsWeekday(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp");
        return nullptr;
    }
    
    double timestamp;
    napi_get_value_double(env, args[0], &timestamp);
    
    TimePoint tp = fromTimeT(static_cast<std::time_t>(timestamp / 1000));
    bool result = isWeekday(tp);
    
    napi_value resultValue;
    napi_create_boolean(env, result, &resultValue);
    return resultValue;
}

// 检查是否是周末
static napi_value IsWeekend(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected timestamp");
        return nullptr;
    }
    
    double timestamp;
    napi_get_value_double(env, args[0], &timestamp);
    
    TimePoint tp = fromTimeT(static_cast<std::time_t>(timestamp / 1000));
    bool result = isWeekend(tp);
    
    napi_value resultValue;
    napi_create_boolean(env, result, &resultValue);
    return resultValue;
}

// 模块初始化
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor descriptors[] = {
        {"getCurrentTimestamp", nullptr, GetCurrentTimestamp, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"formatDate", nullptr, FormatDate, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"parseDate", nullptr, ParseDate, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"dateDiff", nullptr, DateDiff, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"addDays", nullptr, AddDays, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"addMonths", nullptr, AddMonths, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"addYears", nullptr, AddYears, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"isLeapYear", nullptr, IsLeapYear, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getDaysInMonth", nullptr, GetDaysInMonth, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getDayOfWeek", nullptr, GetDayOfWeek, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getQuarter", nullptr, GetQuarter, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getWeekOfYear", nullptr, GetWeekOfYear, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"isSameDay", nullptr, IsSameDay, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"isInRange", nullptr, IsInRange, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getRelativeTime", nullptr, GetRelativeTime, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"convertTimezone", nullptr, ConvertTimezone, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getDateParts", nullptr, GetDateParts, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"toISOString", nullptr, ToISOString, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"fromISOString", nullptr, FromISOString, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getCurrentDate", nullptr, GetCurrentDate, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getCurrentTime", nullptr, GetCurrentTime, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getCurrentDateTime", nullptr, GetCurrentDateTime, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"calculateAge", nullptr, CalculateAge, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"isWeekday", nullptr, IsWeekday, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"isWeekend", nullptr, IsWeekend, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    
    napi_define_properties(env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
    return exports;
}

NAPI_MODULE(napi_date, Init)
