// Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#include <cstdio>
#include <cstring>
#include "native_api.h"

namespace fs = std::filesystem;

namespace {

// 读取文件
std::string read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file");
    }
    
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return content;
}

// 写入文件
void write_file(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file for writing");
    }
    
    file.write(content.c_str(), content.size());
    if (!file) {
        throw std::runtime_error("Failed to write to file");
    }
}

// 追加文件
void append_file(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary | std::ios::app);
    if (!file) {
        throw std::runtime_error("Failed to open file for appending");
    }
    
    file.write(content.c_str(), content.size());
    if (!file) {
        throw std::runtime_error("Failed to append to file");
    }
}

// 删除文件
void delete_file(const std::string& path) {
    if (std::remove(path.c_str()) != 0) {
        throw std::runtime_error("Failed to delete file");
    }
}

// 复制文件
void copy_file(const std::string& src, const std::string& dest) {
    std::ifstream src_file(src, std::ios::binary);
    if (!src_file) {
        throw std::runtime_error("Failed to open source file");
    }
    
    std::ofstream dest_file(dest, std::ios::binary);
    if (!dest_file) {
        throw std::runtime_error("Failed to open destination file");
    }
    
    dest_file << src_file.rdbuf();
    if (!dest_file) {
        throw std::runtime_error("Failed to copy file");
    }
}

// 移动文件
void move_file(const std::string& src, const std::string& dest) {
    if (std::rename(src.c_str(), dest.c_str()) != 0) {
        // 如果重命名失败，尝试复制后删除
        copy_file(src, dest);
        delete_file(src);
    }
}

// 重命名文件
void rename_file(const std::string& old_path, const std::string& new_path) {
    if (std::rename(old_path.c_str(), new_path.c_str()) != 0) {
        throw std::runtime_error("Failed to rename file");
    }
}

// 创建目录
void create_directory(const std::string& path) {
    if (mkdir(path.c_str(), 0755) != 0) {
        throw std::runtime_error("Failed to create directory");
    }
}

// 删除目录
void delete_directory(const std::string& path) {
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        throw std::runtime_error("Failed to open directory");
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        std::string entry_path = path + "/" + entry->d_name;
        if (entry->d_type == DT_DIR) {
            delete_directory(entry_path);
        } else {
            delete_file(entry_path);
        }
    }
    
    closedir(dir);
    if (rmdir(path.c_str()) != 0) {
        throw std::runtime_error("Failed to remove directory");
    }
}

// 列出目录内容
std::vector<std::string> list_directory(const std::string& path) {
    std::vector<std::string> entries;
    DIR* dir = opendir(path.c_str());
    if (!dir) {
        throw std::runtime_error("Failed to open directory");
    }
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            entries.push_back(entry->d_name);
        }
    }
    
    closedir(dir);
    return entries;
}

// 检查文件是否存在
bool file_exists(const std::string& path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

// 检查是否是目录
bool is_directory(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        return false;
    }
    return S_ISDIR(buffer.st_mode);
}

// 检查是否是文件
bool is_file(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        return false;
    }
    return S_ISREG(buffer.st_mode);
}

// 获取文件大小
off_t get_file_size(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        throw std::runtime_error("Failed to get file size");
    }
    return buffer.st_size;
}

// 获取文件权限
mode_t get_file_permissions(const std::string& path) {
    struct stat buffer;
    if (stat(path.c_str(), &buffer) != 0) {
        throw std::runtime_error("Failed to get file permissions");
    }
    return buffer.st_mode;
}

// 设置文件权限
void set_file_permissions(const std::string& path, mode_t mode) {
    if (chmod(path.c_str(), mode) != 0) {
        throw std::runtime_error("Failed to set file permissions");
    }
}

// 连接路径
std::string join_path(const std::string& path1, const std::string& path2) {
    return fs::path(path1) / fs::path(path2);
}

// 获取文件名
std::string basename(const std::string& path) {
    return fs::path(path).filename().string();
}

// 获取目录名
std::string dirname(const std::string& path) {
    return fs::path(path).parent_path().string();
}

// 获取扩展名
std::string extname(const std::string& path) {
    return fs::path(path).extension().string();
}

// 规范化路径
std::string normalize_path(const std::string& path) {
    return fs::canonical(path).string();
}

// 获取绝对路径
std::string absolute_path(const std::string& path) {
    return fs::absolute(path).string();
}

// 创建临时文件
std::string create_temp_file() {
    char temp_name[] = "/tmp/temp_XXXXXX";
    int fd = mkstemp(temp_name);
    if (fd == -1) {
        throw std::runtime_error("Failed to create temporary file");
    }
    close(fd);
    return temp_name;
}

// 创建临时目录
std::string create_temp_directory() {
    char temp_name[] = "/tmp/temp_XXXXXX";
    char* dir = mkdtemp(temp_name);
    if (dir == nullptr) {
        throw std::runtime_error("Failed to create temporary directory");
    }
    return dir;
}

// 获取可用空间
off_t get_free_space(const std::string& path) {
    struct statvfs buf;
    if (statvfs(path.c_str(), &buf) != 0) {
        throw std::runtime_error("Failed to get free space");
    }
    return buf.f_bavail * buf.f_frsize;
}

// 获取总空间
off_t get_total_space(const std::string& path) {
    struct statvfs buf;
    if (statvfs(path.c_str(), &buf) != 0) {
        throw std::runtime_error("Failed to get total space");
    }
    return buf.f_blocks * buf.f_frsize;
}

// 截断文件
void truncate_file(const std::string& path, off_t length) {
    if (truncate(path.c_str(), length) != 0) {
        throw std::runtime_error("Failed to truncate file");
    }
}

// 修改文件权限
void chmod_file(const std::string& path, mode_t mode) {
    if (chmod(path.c_str(), mode) != 0) {
        throw std::runtime_error("Failed to change file permissions");
    }
}

// 修改文件所有者
void chown_file(const std::string& path, uid_t owner, gid_t group) {
    if (chown(path.c_str(), owner, group) != 0) {
        throw std::runtime_error("Failed to change file owner");
    }
}

} // namespace

// 读取文件
static napi_value ReadFile(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected file path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    try {
        std::string content = read_file(path);
        napi_value result;
        napi_create_string_utf8(env, content.c_str(), NAPI_AUTO_LENGTH, &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 写入文件
static napi_value WriteFile(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected file path and content");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    size_t contentLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &contentLength);
    char* contentBuffer = new char[contentLength + 1];
    napi_get_value_string_utf8(env, args[1], contentBuffer, contentLength + 1, nullptr);
    std::string content(contentBuffer);
    delete[] contentBuffer;
    
    try {
        write_file(path, content);
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 追加文件
static napi_value AppendFile(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected file path and content");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    size_t contentLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &contentLength);
    char* contentBuffer = new char[contentLength + 1];
    napi_get_value_string_utf8(env, args[1], contentBuffer, contentLength + 1, nullptr);
    std::string content(contentBuffer);
    delete[] contentBuffer;
    
    try {
        append_file(path, content);
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 删除文件
static napi_value DeleteFile(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected file path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    try {
        delete_file(path);
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 复制文件
static napi_value CopyFile(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected source and destination paths");
        return nullptr;
    }
    
    size_t srcLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &srcLength);
    char* srcBuffer = new char[srcLength + 1];
    napi_get_value_string_utf8(env, args[0], srcBuffer, srcLength + 1, nullptr);
    std::string src(srcBuffer);
    delete[] srcBuffer;
    
    size_t destLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &destLength);
    char* destBuffer = new char[destLength + 1];
    napi_get_value_string_utf8(env, args[1], destBuffer, destLength + 1, nullptr);
    std::string dest(destBuffer);
    delete[] destBuffer;
    
    try {
        copy_file(src, dest);
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 移动文件
static napi_value MoveFile(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected source and destination paths");
        return nullptr;
    }
    
    size_t srcLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &srcLength);
    char* srcBuffer = new char[srcLength + 1];
    napi_get_value_string_utf8(env, args[0], srcBuffer, srcLength + 1, nullptr);
    std::string src(srcBuffer);
    delete[] srcBuffer;
    
    size_t destLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &destLength);
    char* destBuffer = new char[destLength + 1];
    napi_get_value_string_utf8(env, args[1], destBuffer, destLength + 1, nullptr);
    std::string dest(destBuffer);
    delete[] destBuffer;
    
    try {
        move_file(src, dest);
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 重命名文件
static napi_value RenameFile(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected old and new paths");
        return nullptr;
    }
    
    size_t oldPathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &oldPathLength);
    char* oldPathBuffer = new char[oldPathLength + 1];
    napi_get_value_string_utf8(env, args[0], oldPathBuffer, oldPathLength + 1, nullptr);
    std::string oldPath(oldPathBuffer);
    delete[] oldPathBuffer;
    
    size_t newPathLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &newPathLength);
    char* newPathBuffer = new char[newPathLength + 1];
    napi_get_value_string_utf8(env, args[1], newPathBuffer, newPathLength + 1, nullptr);
    std::string newPath(newPathBuffer);
    delete[] newPathBuffer;
    
    try {
        rename_file(oldPath, newPath);
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 创建目录
static napi_value CreateDirectory(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected directory path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    try {
        create_directory(path);
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 删除目录
static napi_value DeleteDirectory(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected directory path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    try {
        delete_directory(path);
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 列出目录内容
static napi_value ListDirectory(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected directory path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    try {
        std::vector<std::string> entries = list_directory(path);
        napi_value result;
        napi_create_array(env, &result);
        
        for (size_t i = 0; i < entries.size(); i++) {
            napi_value entry;
            napi_create_string_utf8(env, entries[i].c_str(), NAPI_AUTO_LENGTH, &entry);
            napi_set_element(env, result, i, entry);
        }
        
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 检查文件是否存在
static napi_value FileExists(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected file path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    bool exists = file_exists(path);
    napi_value result;
    napi_create_boolean(env, exists, &result);
    return result;
}

// 检查是否是目录
static napi_value IsDirectory(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    bool isDir = is_directory(path);
    napi_value result;
    napi_create_boolean(env, isDir, &result);
    return result;
}

// 检查是否是文件
static napi_value IsFile(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    bool isFile = is_file(path);
    napi_value result;
    napi_create_boolean(env, isFile, &result);
    return result;
}

// 获取文件大小
static napi_value GetFileSize(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected file path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    try {
        off_t size = get_file_size(path);
        napi_value result;
        napi_create_double(env, static_cast<double>(size), &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 获取文件状态
static napi_value GetFileStats(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected file path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    try {
        struct stat buffer;
        if (stat(path.c_str(), &buffer) != 0) {
            throw std::runtime_error("Failed to get file stats");
        }
        
        napi_value result;
        napi_create_object(env, &result);
        
        napi_value size, mode, mtime;
        napi_create_double(env, static_cast<double>(buffer.st_size), &size);
        napi_create_double(env, static_cast<double>(buffer.st_mode), &mode);
        napi_create_double(env, static_cast<double>(buffer.st_mtime), &mtime);
        
        napi_set_named_property(env, result, "size", size);
        napi_set_named_property(env, result, "mode", mode);
        napi_set_named_property(env, result, "mtime", mtime);
        
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 获取文件权限
static napi_value GetFilePermissions(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected file path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    try {
        mode_t permissions = get_file_permissions(path);
        napi_value result;
        napi_create_double(env, static_cast<double>(permissions), &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 设置文件权限
static napi_value SetFilePermissions(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected file path and permissions");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    double permissions;
    napi_get_value_double(env, args[1], &permissions);
    
    try {
        set_file_permissions(path, static_cast<mode_t>(permissions));
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 连接路径
static napi_value JoinPath(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected two paths");
        return nullptr;
    }
    
    size_t path1Length;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &path1Length);
    char* path1Buffer = new char[path1Length + 1];
    napi_get_value_string_utf8(env, args[0], path1Buffer, path1Length + 1, nullptr);
    std::string path1(path1Buffer);
    delete[] path1Buffer;
    
    size_t path2Length;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &path2Length);
    char* path2Buffer = new char[path2Length + 1];
    napi_get_value_string_utf8(env, args[1], path2Buffer, path2Length + 1, nullptr);
    std::string path2(path2Buffer);
    delete[] path2Buffer;
    
    std::string result = join_path(path1, path2);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 获取文件名
static napi_value Basename(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    std::string result = basename(path);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 获取目录名
static napi_value Dirname(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    std::string result = dirname(path);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 获取扩展名
static napi_value Extname(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    std::string result = extname(path);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 规范化路径
static napi_value NormalizePath(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    try {
        std::string result = normalize_path(path);
        napi_value resultValue;
        napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
        return resultValue;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 获取绝对路径
static napi_value AbsolutePath(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    try {
        std::string result = absolute_path(path);
        napi_value resultValue;
        napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
        return resultValue;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 创建临时文件
static napi_value CreateTempFile(napi_env env, napi_callback_info info) {
    try {
        std::string result = create_temp_file();
        napi_value resultValue;
        napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
        return resultValue;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 创建临时目录
static napi_value CreateTempDirectory(napi_env env, napi_callback_info info) {
    try {
        std::string result = create_temp_directory();
        napi_value resultValue;
        napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
        return resultValue;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 获取可用空间
static napi_value GetFreeSpace(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    try {
        off_t space = get_free_space(path);
        napi_value result;
        napi_create_double(env, static_cast<double>(space), &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 获取总空间
static napi_value GetTotalSpace(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    try {
        off_t space = get_total_space(path);
        napi_value result;
        napi_create_double(env, static_cast<double>(space), &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 监控文件（简化版）
static napi_value WatchFile(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected file path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    // 简化实现，实际项目中应使用inotify或类似机制
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

// 取消监控文件
static napi_value UnwatchFile(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected file path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    // 简化实现
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

// 同步读取文件
static napi_value ReadFileSync(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected file path");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    try {
        std::string content = read_file(path);
        napi_value result;
        napi_create_string_utf8(env, content.c_str(), NAPI_AUTO_LENGTH, &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 同步写入文件
static napi_value WriteFileSync(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected file path and content");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    size_t contentLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &contentLength);
    char* contentBuffer = new char[contentLength + 1];
    napi_get_value_string_utf8(env, args[1], contentBuffer, contentLength + 1, nullptr);
    std::string content(contentBuffer);
    delete[] contentBuffer;
    
    try {
        write_file(path, content);
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 截断文件
static napi_value TruncateFile(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected file path and length");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    double length;
    napi_get_value_double(env, args[1], &length);
    
    try {
        truncate_file(path, static_cast<off_t>(length));
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 修改文件权限
static napi_value Chmod(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected file path and mode");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    double mode;
    napi_get_value_double(env, args[1], &mode);
    
    try {
        chmod_file(path, static_cast<mode_t>(mode));
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 修改文件所有者
static napi_value Chown(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 3) {
        napi_throw_error(env, "Invalid arguments", "Expected file path, owner and group");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    double owner, group;
    napi_get_value_double(env, args[1], &owner);
    napi_get_value_double(env, args[2], &group);
    
    try {
        chown_file(path, static_cast<uid_t>(owner), static_cast<gid_t>(group));
        napi_value result;
        napi_get_undefined(env, &result);
        return result;
    } catch (const std::exception& e) {
        napi_throw_error(env, "File error", e.what());
        return nullptr;
    }
}

// 模块初始化
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor descriptors[] = {
        {"readFile", nullptr, ReadFile, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"writeFile", nullptr, WriteFile, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"appendFile", nullptr, AppendFile, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"deleteFile", nullptr, DeleteFile, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"copyFile", nullptr, CopyFile, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"moveFile", nullptr, MoveFile, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"renameFile", nullptr, RenameFile, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"createDirectory", nullptr, CreateDirectory, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"deleteDirectory", nullptr, DeleteDirectory, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"listDirectory", nullptr, ListDirectory, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"fileExists", nullptr, FileExists, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"isDirectory", nullptr, IsDirectory, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"isFile", nullptr, IsFile, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getFileSize", nullptr, GetFileSize, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getFileStats", nullptr, GetFileStats, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getFilePermissions", nullptr, GetFilePermissions, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setFilePermissions", nullptr, SetFilePermissions, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"joinPath", nullptr, JoinPath, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"basename", nullptr, Basename, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"dirname", nullptr, Dirname, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"extname", nullptr, Extname, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"normalizePath", nullptr, NormalizePath, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"absolutePath", nullptr, AbsolutePath, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"createTempFile", nullptr, CreateTempFile, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"createTempDirectory", nullptr, CreateTempDirectory, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getFreeSpace", nullptr, GetFreeSpace, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getTotalSpace", nullptr, GetTotalSpace, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"watchFile", nullptr, WatchFile, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"unwatchFile", nullptr, UnwatchFile, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"readFileSync", nullptr, ReadFileSync, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"writeFileSync", nullptr, WriteFileSync, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"truncateFile", nullptr, TruncateFile, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"chmod", nullptr, Chmod, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"chown", nullptr, Chown, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    
    napi_define_properties(env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
    return exports;
}

NAPI_MODULE(napi_file, Init)
