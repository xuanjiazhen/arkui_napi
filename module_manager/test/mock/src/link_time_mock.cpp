/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "link_time_mock.h"

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

namespace {

std::map<std::string, std::string> g_mockFileContent;
std::set<std::string> g_mockFileExists;
bool g_linkTimeMockEnabled = false;

struct MockFileDescriptor {
    std::string path;
    std::string content;
    size_t readPos;
};

std::map<int, MockFileDescriptor> g_mockFdTable;
int g_nextMockFd = 10000;

int AllocateMockFd(const std::string& path, const std::string& content) {
    int fd = g_nextMockFd++;
    g_mockFdTable[fd] = {path, content, 0};
    return fd;
}

MockFileDescriptor* GetMockFd(int fd) {
    auto it = g_mockFdTable.find(fd);
    if (it != g_mockFdTable.end()) {
        return &it->second;
    }
    return nullptr;
}

void FreeMockFd(int fd) {
    g_mockFdTable.erase(fd);
}

bool IsMockedPath(const std::string& path) {
    return g_mockFileContent.find(path) != g_mockFileContent.end() ||
           g_mockFileExists.find(path) != g_mockFileExists.end();
}

}

extern "C" {

void LinkTimeMock_Enable(void)
{
    g_linkTimeMockEnabled = true;
    g_mockFileContent.clear();
    g_mockFileExists.clear();
    g_mockFdTable.clear();
    g_nextMockFd = 10000;
}

void LinkTimeMock_Disable(void)
{
    g_linkTimeMockEnabled = false;
    g_mockFileContent.clear();
    g_mockFileExists.clear();
    g_mockFdTable.clear();
    g_nextMockFd = 10000;
}

void LinkTimeMock_SetFileContent(const char* path, const char* content)
{
    if (path == nullptr || content == nullptr) {
        return;
    }
    g_mockFileContent[std::string(path)] = std::string(content);
    g_mockFileExists.insert(std::string(path));
}

void LinkTimeMock_SetFileExists(const char* path, bool exists)
{
    if (path == nullptr) {
        return;
    }
    std::string key(path);
    if (exists) {
        g_mockFileExists.insert(key);
    } else {
        g_mockFileExists.erase(key);
        g_mockFileContent.erase(key);
    }
}

void LinkTimeMock_ClearAll(void)
{
    g_mockFileContent.clear();
    g_mockFileExists.clear();
    g_mockFdTable.clear();
}

int __wrap_access(const char* pathname, int mode)
{
    if (!g_linkTimeMockEnabled) {
        return __real_access(pathname, mode);
    }
    
    if (pathname == nullptr) {
        errno = EFAULT;
        return -1;
    }
    
    std::string key(pathname);
    if (g_mockFileContent.find(key) != g_mockFileContent.end() ||
        g_mockFileExists.find(key) != g_mockFileExists.end()) {
        return 0;
    }
    
    return __real_access(pathname, mode);
}

FILE* __wrap_fopen(const char* pathname, const char* mode)
{
    if (!g_linkTimeMockEnabled) {
        return __real_fopen(pathname, mode);
    }
    
    if (pathname == nullptr || mode == nullptr) {
        errno = EINVAL;
        return nullptr;
    }
    
    std::string key(pathname);
    auto it = g_mockFileContent.find(key);
    if (it != g_mockFileContent.end()) {
        FILE* memfile = fmemopen(const_cast<char*>(it->second.c_str()), 
                                 it->second.size(), mode);
        return memfile;
    }
    
    if (g_mockFileExists.find(key) != g_mockFileExists.end()) {
        errno = ENOENT;
        return nullptr;
    }
    
    return __real_fopen(pathname, mode);
}

int __wrap_stat(const char* pathname, struct stat* statbuf)
{
    if (!g_linkTimeMockEnabled) {
        return __real_stat(pathname, statbuf);
    }
    
    if (pathname == nullptr || statbuf == nullptr) {
        errno = EFAULT;
        return -1;
    }
    
    std::string key(pathname);
    auto contentIt = g_mockFileContent.find(key);
    if (contentIt != g_mockFileContent.end()) {
        memset(statbuf, 0, sizeof(struct stat));
        statbuf->st_mode = S_IFREG | 0644;
        statbuf->st_size = contentIt->second.size();
        statbuf->st_blksize = 4096;
        statbuf->st_blocks = (contentIt->second.size() + 511) / 512;
        return 0;
    }
    
    if (g_mockFileExists.find(key) != g_mockFileExists.end()) {
        memset(statbuf, 0, sizeof(struct stat));
        statbuf->st_mode = S_IFREG | 0644;
        return 0;
    }
    
    return __real_stat(pathname, statbuf);
}

int __wrap_open(const char* pathname, int flags, ...)
{
    mode_t mode = 0;
    if (flags & O_CREAT) {
        va_list args;
        va_start(args, flags);
        mode = va_arg(args, mode_t);
        va_end(args);
    }
    
    if (!g_linkTimeMockEnabled) {
        if (flags & O_CREAT) {
            return __real_open(pathname, flags, mode);
        }
        return __real_open(pathname, flags);
    }
    
    if (pathname == nullptr) {
        errno = EFAULT;
        return -1;
    }
    
    std::string key(pathname);
    auto contentIt = g_mockFileContent.find(key);
    if (contentIt != g_mockFileContent.end()) {
        if ((flags & O_ACCMODE) == O_WRONLY || (flags & O_ACCMODE) == O_RDWR) {
            errno = EROFS;
            return -1;
        }
        int fd = AllocateMockFd(key, contentIt->second);
        return fd;
    }
    
    if (g_mockFileExists.find(key) != g_mockFileExists.end()) {
        errno = ENOENT;
        return -1;
    }
    
    if (flags & O_CREAT) {
        return __real_open(pathname, flags, mode);
    }
    return __real_open(pathname, flags);
}

ssize_t __wrap_read(int fd, void* buf, size_t count)
{
    if (!g_linkTimeMockEnabled) {
        return __real_read(fd, buf, count);
    }
    
    MockFileDescriptor* mockFd = GetMockFd(fd);
    if (mockFd == nullptr) {
        return __real_read(fd, buf, count);
    }
    
    if (buf == nullptr) {
        errno = EFAULT;
        return -1;
    }
    
    size_t remaining = mockFd->content.size() - mockFd->readPos;
    size_t toRead = (count < remaining) ? count : remaining;
    
    if (toRead > 0) {
        memcpy(buf, mockFd->content.data() + mockFd->readPos, toRead);
        mockFd->readPos += toRead;
    }
    
    return static_cast<ssize_t>(toRead);
}

ssize_t __wrap_write(int fd, const void* buf, size_t count)
{
    if (!g_linkTimeMockEnabled) {
        return __real_write(fd, buf, count);
    }
    
    if (GetMockFd(fd) != nullptr) {
        errno = EROFS;
        return -1;
    }
    
    return __real_write(fd, buf, count);
}

int __wrap_close(int fd)
{
    if (!g_linkTimeMockEnabled) {
        return __real_close(fd);
    }
    
    if (GetMockFd(fd) != nullptr) {
        FreeMockFd(fd);
        return 0;
    }
    
    return __real_close(fd);
}

off_t __wrap_lseek(int fd, off_t offset, int whence)
{
    if (!g_linkTimeMockEnabled) {
        return __real_lseek(fd, offset, whence);
    }
    
    MockFileDescriptor* mockFd = GetMockFd(fd);
    if (mockFd == nullptr) {
        return __real_lseek(fd, offset, whence);
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
    
    mockFd->readPos = static_cast<size_t>(newPos);
    return newPos;
}

int __wrap_fstat(int fd, struct stat* statbuf)
{
    if (!g_linkTimeMockEnabled) {
        return __real_fstat(fd, statbuf);
    }
    
    MockFileDescriptor* mockFd = GetMockFd(fd);
    if (mockFd == nullptr) {
        return __real_fstat(fd, statbuf);
    }
    
    if (statbuf == nullptr) {
        errno = EFAULT;
        return -1;
    }
    
    memset(statbuf, 0, sizeof(struct stat));
    statbuf->st_mode = S_IFREG | 0644;
    statbuf->st_size = mockFd->content.size();
    statbuf->st_blksize = 4096;
    statbuf->st_blocks = (mockFd->content.size() + 511) / 512;
    return 0;
}

}
