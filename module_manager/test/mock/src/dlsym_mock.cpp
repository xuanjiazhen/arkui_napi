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

#include "dlsym_mock.h"

#include <dlfcn.h>
#include <string>
#include <map>
#include <set>
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cstdarg>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <gtest/gtest.h>

namespace {

std::map<std::string, std::string> g_mockFileContent;
std::set<std::string> g_mockFileExists;
bool g_dlsymMockEnabled = false;
bool g_debugMode = true;

struct MockFileDescriptor {
    std::string path;
    std::string content;
    size_t readPos;
};
std::map<int, MockFileDescriptor> g_mockFdTable;
int g_nextMockFd = 20000;
void DebugLog(const char* func, const char* fmt, ...)
{
    if (!g_debugMode) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    char buf[1024];
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    GTEST_LOG_(INFO) << "[DLSYM_MOCK][" << func << "] " << buf;
}
int AllocateMockFd(const std::string& path, const std::string& content)
{
    int fd = g_nextMockFd++;
    MockFileDescriptor mockFd;
    mockFd.path = path;
    mockFd.content = content;
    mockFd.readPos = 0;
    g_mockFdTable[fd] = mockFd;
    return fd;
}
MockFileDescriptor* GetMockFd(int fd)
{
    auto it = g_mockFdTable.find(fd);
    if (it != g_mockFdTable.end()) {
        return &(it->second);
    }
    return nullptr;
}
void FreeMockFd(int fd)
{
    g_mockFdTable.erase(fd);
}
}
extern "C" {
void DlsymMock_Enable(void)
{
    g_dlsymMockEnabled = true;
    DebugLog("DlsymMock_Enable", "Mock enabled");
}
void DlsymMock_Disable(void)
{
    g_dlsymMockEnabled = false;
    DebugLog("DlsymMock_Disable", "Mock disabled");
}
void DlsymMock_SetFileContent(const char* path, const char* content)
{
    if (path == nullptr) {
        return;
    }
    std::string key(path);
    g_mockFileContent[key] = content ? std::string(content) : std::string();
    g_mockFileExists.insert(key);
    DebugLog("DlsymMock_SetFileContent", "Set content for %s, size=%zu", path, g_mockFileContent[key].size());
}
void DlsymMock_SetFileExists(const char* path, int exists)
{
    if (path == nullptr) {
        return;
    }
    std::string key(path);
    if (exists) {
        g_mockFileExists.insert(key);
        DebugLog("DlsymMock_SetFileExists", "Mark file exists: %s", path);
    } else {
        g_mockFileExists.erase(key);
        g_mockFileContent.erase(key);
        DebugLog("DlsymMock_SetFileExists", "Mark file not exists: %s", path);
    }
}
void DlsymMock_ClearAll(void)
{
    g_mockFileContent.clear();
    g_mockFileExists.clear();
    g_mockFdTable.clear();
    DebugLog("DlsymMock_ClearAll", "All mock data cleared");
}
int DlsymMock_IsEnabled(void)
{
    return g_dlsymMockEnabled ? 1 : 0;
}
void DlsymMock_SetDebugMode(int enable)
{
    g_debugMode = enable ? true : false;
}
int access(const char* pathname, int mode)
{
    DebugLog("access", ">>> ENTER");
    dlsym_access_func_t real_access = reinterpret_cast<dlsym_access_func_t>(dlsym(RTLD_NEXT, "access"));
    if (!g_dlsymMockEnabled) {
        if (real_access == nullptr) {
            errno = ENOSYS;
            return -1;
        }
        DebugLog("access", ">>> PASSING THROUGH (mock disabled) path=%s", pathname ? pathname : "NULL");
        return real_access(pathname, mode);
    }
    if (pathname == nullptr) {
        errno = EFAULT;
        return -1;
    }
    std::string key(pathname);
    bool hasContent = g_mockFileContent.find(key) != g_mockFileContent.end();
    bool markedExists = g_mockFileExists.find(key) != g_mockFileExists.end();
    if (hasContent || markedExists) {
        DebugLog("access", ">>> MOCKED: file exists: %s", pathname);
        return 0;
    }
    if (real_access == nullptr) {
        DebugLog("access", ">>> MOCKED: file not in mock list: %s (returning ENOENT)", pathname);
        errno = ENOENT;
        return -1;
    }
    DebugLog("access", ">>> PASSING THROUGH (not in mock list) path=%s", pathname);
    return real_access(pathname, mode);
}
int open(const char* pathname, int flags, ...)
{
    DebugLog("open", ">>> ENTER");
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    dlsym_open_func_t real_open = reinterpret_cast<dlsym_open_func_t>(dlsym(RTLD_NEXT, "open"));
    if (!g_dlsymMockEnabled) {
        if (real_open == nullptr) {
            errno = ENOSYS;
            return -1;
        }
        DebugLog("open", ">>> PASSING THROUGH (mock disabled) path=%s", pathname ? pathname : "NULL");
        return real_open(pathname, flags, mode);
    }
    if (pathname == nullptr) {
        errno = EFAULT;
        return -1;
    }
    std::string key(pathname);
    auto contentIt = g_mockFileContent.find(key);
    if (contentIt != g_mockFileContent.end()) {
        int fd = AllocateMockFd(key, contentIt->second);
        DebugLog("open", ">>> MOCKED: opened mock file: %s, fd=%d", pathname, fd);
        return fd;
    }
    if (g_mockFileExists.find(key) != g_mockFileExists.end()) {
        int fd = AllocateMockFd(key, "");
        DebugLog("open", ">>> MOCKED: opened empty mock file: %s, fd=%d", pathname, fd);
        return fd;
    }
    if (real_open == nullptr) {
        DebugLog("open", ">>> MOCKED: file not in mock list: %s (returning ENOENT)", pathname);
        errno = ENOENT;
        return -1;
    }
    DebugLog("open", ">>> PASSING THROUGH (not in mock list) path=%s", pathname);
    return real_open(pathname, flags, mode);
}
ssize_t read(int fd, void* buf, size_t count)
{
    DebugLog("read", ">>> ENTER");
    dlsym_read_func_t real_read = reinterpret_cast<dlsym_read_func_t>(dlsym(RTLD_NEXT, "read"));
    if (!g_dlsymMockEnabled) {
        if (real_read == nullptr) {
            errno = ENOSYS;
            return -1;
        }
        DebugLog("read", ">>> PASSING THROUGH (mock disabled) fd=%d", fd);
        return real_read(fd, buf, count);
    }
    MockFileDescriptor* mockFd = GetMockFd(fd);
    if (mockFd == nullptr) {
        if (real_read == nullptr) {
            errno = ENOSYS;
            return -1;
        }
        DebugLog("read", ">>> PASSING THROUGH (not a mock fd) fd=%d", fd);
        return real_read(fd, buf, count);
    }
    if (buf == nullptr) {
        DebugLog("read", ">>> ERROR: buf is NULL");
        errno = EFAULT;
        return -1;
    }
    size_t remaining = mockFd->content.size() - mockFd->readPos;
    size_t toRead = (count < remaining) ? count : remaining;
    if (toRead > 0) {
        memcpy(buf, mockFd->content.data() + mockFd->readPos, toRead);
        mockFd->readPos += toRead;
    }
    DebugLog("read", ">>> MOCKED: read %zu bytes from mock fd=%d", toRead, fd);
    return static_cast<ssize_t>(toRead);
}
ssize_t write(int fd, const void* buf, size_t count)
{
    DebugLog("write", ">>> ENTER");
    dlsym_write_func_t real_write = reinterpret_cast<dlsym_write_func_t>(dlsym(RTLD_NEXT, "write"));
    if (!g_dlsymMockEnabled) {
        if (real_write == nullptr) {
            errno = ENOSYS;
            return -1;
        }
        DebugLog("write", ">>> PASSING THROUGH (mock disabled) fd=%d", fd);
        return real_write(fd, buf, count);
    }
    if (GetMockFd(fd) != nullptr) {
        DebugLog("write", ">>> MOCKED: write to mock fd not allowed, fd=%d", fd);
        errno = EROFS;
        return -1;
    }
    if (real_write == nullptr) {
        errno = ENOSYS;
        return -1;
    }
    DebugLog("write", ">>> PASSING THROUGH to real write, fd=%d", fd);
    return real_write(fd, buf, count);
}
int close(int fd)
{
    DebugLog("close", ">>> ENTER");
    dlsym_close_func_t real_close = reinterpret_cast<dlsym_close_func_t>(dlsym(RTLD_NEXT, "close"));
    if (!g_dlsymMockEnabled) {
        if (real_close == nullptr) {
            errno = ENOSYS;
            return -1;
        }
        DebugLog("close", ">>> PASSING THROUGH (mock disabled) fd=%d", fd);
        return real_close(fd);
    }
    if (GetMockFd(fd) != nullptr) {
        FreeMockFd(fd);
        DebugLog("close", ">>> MOCKED: closed mock fd=%d", fd);
        return 0;
    }
    if (real_close == nullptr) {
        errno = ENOSYS;
        return -1;
    }
    DebugLog("close", ">>> PASSING THROUGH to real close, fd=%d", fd);
    return real_close(fd);
}
off_t lseek(int fd, off_t offset, int whence)
{
    DebugLog("lseek", ">>> ENTER");
    dlsym_lseek_func_t real_lseek = reinterpret_cast<dlsym_lseek_func_t>(dlsym(RTLD_NEXT, "lseek"));
    if (!g_dlsymMockEnabled) {
        if (real_lseek == nullptr) {
            errno = ENOSYS;
            return -1;
        }
        DebugLog("lseek", ">>> PASSING THROUGH (mock disabled) fd=%d", fd);
        return real_lseek(fd, offset, whence);
    }
    MockFileDescriptor* mockFd = GetMockFd(fd);
    if (mockFd == nullptr) {
        if (real_lseek == nullptr) {
            errno = ENOSYS;
            return -1;
        }
        DebugLog("lseek", ">>> PASSING THROUGH (not a mock fd) fd=%d", fd);
        return real_lseek(fd, offset, whence);
    }
    off_t newPos = 0;
    switch (whence) {
        case SEEK_SET:
            newPos = offset;
            break;
        case SEEK_CUR:
            newPos = static_cast<off_t>(mockFd->readPos) + offset;
            break;
        case SEEK_END:
            newPos = static_cast<off_t>(mockFd->content.size()) + offset;
            break;
        default:
            errno = EINVAL;
            return -1;
    }
    if (newPos < 0) {
        errno = EINVAL;
        return -1;
    }
    if (static_cast<size_t>(newPos) > mockFd->content.size()) {
        mockFd->readPos = mockFd->content.size();
    } else {
        mockFd->readPos = static_cast<size_t>(newPos);
    }
    DebugLog("lseek", ">>> MOCKED: new pos=%ld, fd=%d", static_cast<long>(newPos), fd);
    return newPos;
}
int stat(const char* pathname, struct stat* statbuf)
{
    DebugLog("stat", ">>> ENTER");
    dlsym_stat_func_t real_stat = reinterpret_cast<dlsym_stat_func_t>(dlsym(RTLD_NEXT, "stat"));
    if (!g_dlsymMockEnabled) {
        if (real_stat == nullptr) {
            errno = ENOSYS;
            return -1;
        }
        DebugLog("stat", ">>> PASSING THROUGH (mock disabled) path=%s", pathname ? pathname : "NULL");
        return real_stat(pathname, statbuf);
    }
    if (pathname == nullptr || statbuf == nullptr) {
        DebugLog("stat", ">>> ERROR: pathname or statbuf is NULL");
        errno = EFAULT;
        return -1;
    }
    std::string key(pathname);
    auto contentIt = g_mockFileContent.find(key);
    if (contentIt != g_mockFileContent.end()) {
        memset(statbuf, 0, sizeof(struct stat));
        statbuf->st_mode = S_IFREG | 0644;
        statbuf->st_size = static_cast<off_t>(contentIt->second.size());
        statbuf->st_blksize = 4096;
        statbuf->st_blocks = (contentIt->second.size() + 511) / 512;
        DebugLog("stat", ">>> MOCKED: returning stat for %s, size=%zu", pathname,
                 static_cast<size_t>(statbuf->st_size));
        return 0;
    }
    if (g_mockFileExists.find(key) != g_mockFileExists.end()) {
        memset(statbuf, 0, sizeof(struct stat));
        statbuf->st_mode = S_IFREG | 0644;
        statbuf->st_size = 0;
        statbuf->st_blksize = 4096;
        DebugLog("stat", ">>> MOCKED: returning empty stat (file exists only) %s", pathname);
        return 0;
    }
    if (real_stat == nullptr) {
        DebugLog("stat", ">>> MOCKED: file not in mock list: %s (returning ENOENT)", pathname);
        errno = ENOENT;
        return -1;
    }
    DebugLog("stat", ">>> PASSING THROUGH (not in mock list) path=%s", pathname);
    return real_stat(pathname, statbuf);
}
int fstat(int fd, struct stat* statbuf)
{
    DebugLog("fstat", ">>> ENTER");
    dlsym_fstat_func_t real_fstat = reinterpret_cast<dlsym_fstat_func_t>(dlsym(RTLD_NEXT, "fstat"));
    if (!g_dlsymMockEnabled) {
        if (real_fstat == nullptr) {
            errno = ENOSYS;
            return -1;
        }
        DebugLog("fstat", ">>> PASSING THROUGH (mock disabled) fd=%d", fd);
        return real_fstat(fd, statbuf);
    }
    if (statbuf == nullptr) {
        DebugLog("fstat", ">>> ERROR: statbuf is NULL");
        errno = EFAULT;
        return -1;
    }
    MockFileDescriptor* mockFd = GetMockFd(fd);
    if (mockFd != nullptr) {
        memset(statbuf, 0, sizeof(struct stat));
        statbuf->st_mode = S_IFREG | 0644;
        statbuf->st_size = static_cast<off_t>(mockFd->content.size());
        statbuf->st_blksize = 4096;
        statbuf->st_blocks = (mockFd->content.size() + 511) / 512;
        DebugLog("fstat", ">>> MOCKED: fd=%d is mock fd, size=%zu", fd, mockFd->content.size());
        return 0;
    }
    if (real_fstat == nullptr) {
        errno = ENOSYS;
        return -1;
    }
    DebugLog("fstat", ">>> PASSING THROUGH (not a mock fd) fd=%d", fd);
    return real_fstat(fd, statbuf);
}
FILE* fopen(const char* pathname, const char* mode)
{
    DebugLog("fopen", ">>> ENTER");
    dlsym_fopen_func_t real_fopen = reinterpret_cast<dlsym_fopen_func_t>(dlsym(RTLD_NEXT, "fopen"));
    if (!g_dlsymMockEnabled) {
        if (real_fopen == nullptr) {
            errno = ENOSYS;
            return nullptr;
        }
        DebugLog("fopen", ">>> PASSING THROUGH (mock disabled) path=%s", pathname ? pathname : "NULL");
        return real_fopen(pathname, mode);
    }
    if (pathname == nullptr) {
        DebugLog("fopen", ">>> ERROR: pathname is NULL");
        errno = EFAULT;
        return nullptr;
    }
    std::string key(pathname);
    auto contentIt = g_mockFileContent.find(key);
    if (contentIt != g_mockFileContent.end()) {
        FILE* memfile = fmemopen(const_cast<char*>(contentIt->second.c_str()),
                                  contentIt->second.size(), mode);
        DebugLog("fopen", ">>> MOCKED: opened mock file via fmemopen: %s, size=%zu",
                 pathname, contentIt->second.size());
        return memfile;
    }
    if (g_mockFileExists.find(key) != g_mockFileExists.end()) {
        static char emptyBuf[1] = {0};
        FILE* memfile = fmemopen(emptyBuf, 0, mode);
        DebugLog("fopen", ">>> MOCKED: opened empty mock file via fmemopen: %s", pathname);
        return memfile;
    }
    if (real_fopen == nullptr) {
        DebugLog("fopen", ">>> MOCKED: file not in mock list: %s (returning ENOENT)", pathname);
        errno = ENOENT;
        return nullptr;
    }
    DebugLog("fopen", ">>> PASSING THROUGH (not in mock list) path=%s", pathname);
    return real_fopen(pathname, mode);
}
}
