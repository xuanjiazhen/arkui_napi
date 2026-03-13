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

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <ifaddrs.h>
#include <net/if.h>
#include "native_api.h"

namespace {

// HTTP请求（简化版）
std::string http_get(const std::string& url) {
    // 简化实现，实际项目中应使用libcurl或类似库
    return "HTTP GET response from " + url;
}

std::string http_post(const std::string& url, const std::string& data) {
    return "HTTP POST response from " + url + " with data: " + data;
}

std::string http_put(const std::string& url, const std::string& data) {
    return "HTTP PUT response from " + url + " with data: " + data;
}

std::string http_delete(const std::string& url) {
    return "HTTP DELETE response from " + url;
}

// 网络状态
enum NetworkType {
    NETWORK_TYPE_NONE = 0,
    NETWORK_TYPE_WIFI = 1,
    NETWORK_TYPE_MOBILE = 2,
    NETWORK_TYPE_ETHERNET = 3
};

struct NetworkStatus {
    bool connected;
    NetworkType type;
    std::string ssid;
    int signalStrength;
};

NetworkStatus get_network_status() {
    // 简化实现，实际项目中应使用系统API
    NetworkStatus status;
    status.connected = true;
    status.type = NETWORK_TYPE_WIFI;
    status.ssid = "TestSSID";
    status.signalStrength = 85;
    return status;
}

bool is_connected() {
    return get_network_status().connected;
}

NetworkType get_network_type() {
    return get_network_status().type;
}

// 网络工具
bool ping(const std::string& host) {
    // 简化实现，实际项目中应使用系统ping命令
    return true;
}

std::string get_ip_address() {
    struct ifaddrs *ifaddr, *ifa;
    std::string ip;
    
    if (getifaddrs(&ifaddr) == -1) {
        return "";
    }
    
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        if (ifa->ifa_addr->sa_family == AF_INET) {
            char addr[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr, addr, INET_ADDRSTRLEN);
            if (strcmp(ifa->ifa_name, "lo") != 0) {
                ip = addr;
                break;
            }
        }
    }
    
    freeifaddrs(ifaddr);
    return ip.empty() ? "127.0.0.1" : ip;
}

std::string get_mac_address() {
    // 简化实现，实际项目中应使用系统API
    return "00:11:22:33:44:55";
}

std::vector<std::string> resolve_dns(const std::string& domain) {
    struct hostent *he;
    struct in_addr **addr_list;
    std::vector<std::string> ips;
    
    if ((he = gethostbyname(domain.c_str())) == NULL) {
        return ips;
    }
    
    addr_list = (struct in_addr **)he->h_addr_list;
    for (int i = 0; addr_list[i] != NULL; i++) {
        ips.push_back(inet_ntoa(*addr_list[i]));
    }
    
    return ips;
}

// 网络配置
struct ProxySettings {
    bool enabled;
    std::string host;
    int port;
};

ProxySettings get_proxy_settings() {
    // 简化实现，实际项目中应使用系统API
    ProxySettings settings;
    settings.enabled = false;
    settings.host = "";
    settings.port = 0;
    return settings;
}

void set_proxy_settings(const ProxySettings& settings) {
    // 简化实现，实际项目中应使用系统API
}

// 网络统计
struct NetworkStats {
    long long bytesSent;
    long long bytesReceived;
    int packetsSent;
    int packetsReceived;
};

NetworkStats get_network_stats() {
    // 简化实现，实际项目中应使用系统API
    NetworkStats stats;
    stats.bytesSent = 1024 * 1024;
    stats.bytesReceived = 2048 * 1024;
    stats.packetsSent = 1000;
    stats.packetsReceived = 1200;
    return stats;
}

double get_bandwidth() {
    // 简化实现，实际项目中应测量网络带宽
    return 100.0; // Mbps
}

// WebSocket（简化版）
class WebSocket {
public:
    WebSocket(const std::string& url) : url_(url), connected_(false) {}
    
    bool connect() {
        connected_ = true;
        return true;
    }
    
    bool send(const std::string& message) {
        if (!connected_) return false;
        // 简化实现
        return true;
    }
    
    void close() {
        connected_ = false;
    }
    
    bool isConnected() const {
        return connected_;
    }
    
private:
    std::string url_;
    bool connected_;
};

std::map<int, WebSocket*> webSockets;
int nextWebSocketId = 1;

// 网络检测
bool check_internet_connection() {
    return ping("google.com");
}

bool check_server_availability(const std::string& server) {
    return ping(server);
}

// 网络安全
bool validate_certificate(const std::string& certificate) {
    // 简化实现，实际项目中应使用证书验证库
    return true;
}

// 其他网络功能
std::string download_file(const std::string& url, const std::string& path) {
    // 简化实现，实际项目中应使用下载库
    return "Downloaded file from " + url + " to " + path;
}

std::string upload_file(const std::string& path, const std::string& url) {
    // 简化实现，实际项目中应使用上传库
    return "Uploaded file from " + path + " to " + url;
}

std::map<std::string, std::string> get_network_info() {
    std::map<std::string, std::string> info;
    info["ip"] = get_ip_address();
    info["mac"] = get_mac_address();
    info["type"] = "WiFi";
    info["ssid"] = "TestSSID";
    return info;
}

void set_network_preference(int preference) {
    // 简化实现，实际项目中应设置网络偏好
}

} // namespace

// HTTP GET请求
static napi_value HttpGet(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected URL");
        return nullptr;
    }
    
    size_t urlLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &urlLength);
    char* urlBuffer = new char[urlLength + 1];
    napi_get_value_string_utf8(env, args[0], urlBuffer, urlLength + 1, nullptr);
    std::string url(urlBuffer);
    delete[] urlBuffer;
    
    std::string result = http_get(url);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// HTTP POST请求
static napi_value HttpPost(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected URL and data");
        return nullptr;
    }
    
    size_t urlLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &urlLength);
    char* urlBuffer = new char[urlLength + 1];
    napi_get_value_string_utf8(env, args[0], urlBuffer, urlLength + 1, nullptr);
    std::string url(urlBuffer);
    delete[] urlBuffer;
    
    size_t dataLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &dataLength);
    char* dataBuffer = new char[dataLength + 1];
    napi_get_value_string_utf8(env, args[1], dataBuffer, dataLength + 1, nullptr);
    std::string data(dataBuffer);
    delete[] dataBuffer;
    
    std::string result = http_post(url, data);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// HTTP PUT请求
static napi_value HttpPut(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected URL and data");
        return nullptr;
    }
    
    size_t urlLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &urlLength);
    char* urlBuffer = new char[urlLength + 1];
    napi_get_value_string_utf8(env, args[0], urlBuffer, urlLength + 1, nullptr);
    std::string url(urlBuffer);
    delete[] urlBuffer;
    
    size_t dataLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &dataLength);
    char* dataBuffer = new char[dataLength + 1];
    napi_get_value_string_utf8(env, args[1], dataBuffer, dataLength + 1, nullptr);
    std::string data(dataBuffer);
    delete[] dataBuffer;
    
    std::string result = http_put(url, data);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// HTTP DELETE请求
static napi_value HttpDelete(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected URL");
        return nullptr;
    }
    
    size_t urlLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &urlLength);
    char* urlBuffer = new char[urlLength + 1];
    napi_get_value_string_utf8(env, args[0], urlBuffer, urlLength + 1, nullptr);
    std::string url(urlBuffer);
    delete[] urlBuffer;
    
    std::string result = http_delete(url);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 获取网络状态
static napi_value GetNetworkStatus(napi_env env, napi_callback_info info) {
    NetworkStatus status = get_network_status();
    
    napi_value result;
    napi_create_object(env, &result);
    
    napi_value connected, type, ssid, signalStrength;
    napi_create_boolean(env, status.connected, &connected);
    napi_create_int32(env, status.type, &type);
    napi_create_string_utf8(env, status.ssid.c_str(), NAPI_AUTO_LENGTH, &ssid);
    napi_create_int32(env, status.signalStrength, &signalStrength);
    
    napi_set_named_property(env, result, "connected", connected);
    napi_set_named_property(env, result, "type", type);
    napi_set_named_property(env, result, "ssid", ssid);
    napi_set_named_property(env, result, "signalStrength", signalStrength);
    
    return result;
}

// 检查是否连接
static napi_value IsConnected(napi_env env, napi_callback_info info) {
    bool connected = is_connected();
    napi_value result;
    napi_create_boolean(env, connected, &result);
    return result;
}

// 获取网络类型
static napi_value GetNetworkType(napi_env env, napi_callback_info info) {
    NetworkType type = get_network_type();
    napi_value result;
    napi_create_int32(env, type, &result);
    return result;
}

// Ping操作
static napi_value Ping(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected host");
        return nullptr;
    }
    
    size_t hostLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &hostLength);
    char* hostBuffer = new char[hostLength + 1];
    napi_get_value_string_utf8(env, args[0], hostBuffer, hostLength + 1, nullptr);
    std::string host(hostBuffer);
    delete[] hostBuffer;
    
    bool result = ping(host);
    napi_value resultValue;
    napi_create_boolean(env, result, &resultValue);
    return resultValue;
}

// 获取IP地址
static napi_value GetIPAddress(napi_env env, napi_callback_info info) {
    std::string ip = get_ip_address();
    napi_value result;
    napi_create_string_utf8(env, ip.c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

// 获取MAC地址
static napi_value GetMACAddress(napi_env env, napi_callback_info info) {
    std::string mac = get_mac_address();
    napi_value result;
    napi_create_string_utf8(env, mac.c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

// 解析DNS
static napi_value ResolveDNS(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected domain");
        return nullptr;
    }
    
    size_t domainLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &domainLength);
    char* domainBuffer = new char[domainLength + 1];
    napi_get_value_string_utf8(env, args[0], domainBuffer, domainLength + 1, nullptr);
    std::string domain(domainBuffer);
    delete[] domainBuffer;
    
    std::vector<std::string> ips = resolve_dns(domain);
    napi_value result;
    napi_create_array(env, &result);
    
    for (size_t i = 0; i < ips.size(); i++) {
        napi_value ip;
        napi_create_string_utf8(env, ips[i].c_str(), NAPI_AUTO_LENGTH, &ip);
        napi_set_element(env, result, i, ip);
    }
    
    return result;
}

// 获取代理设置
static napi_value GetProxySettings(napi_env env, napi_callback_info info) {
    ProxySettings settings = get_proxy_settings();
    
    napi_value result;
    napi_create_object(env, &result);
    
    napi_value enabled, host, port;
    napi_create_boolean(env, settings.enabled, &enabled);
    napi_create_string_utf8(env, settings.host.c_str(), NAPI_AUTO_LENGTH, &host);
    napi_create_int32(env, settings.port, &port);
    
    napi_set_named_property(env, result, "enabled", enabled);
    napi_set_named_property(env, result, "host", host);
    napi_set_named_property(env, result, "port", port);
    
    return result;
}

// 设置代理设置
static napi_value SetProxySettings(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[3];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 3) {
        napi_throw_error(env, "Invalid arguments", "Expected enabled, host and port");
        return nullptr;
    }
    
    bool enabled;
    napi_get_value_boolean(env, args[0], &enabled);
    
    size_t hostLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &hostLength);
    char* hostBuffer = new char[hostLength + 1];
    napi_get_value_string_utf8(env, args[1], hostBuffer, hostLength + 1, nullptr);
    std::string host(hostBuffer);
    delete[] hostBuffer;
    
    int port;
    napi_get_value_int32(env, args[2], &port);
    
    ProxySettings settings;
    settings.enabled = enabled;
    settings.host = host;
    settings.port = port;
    set_proxy_settings(settings);
    
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

// 获取网络统计
static napi_value GetNetworkStats(napi_env env, napi_callback_info info) {
    NetworkStats stats = get_network_stats();
    
    napi_value result;
    napi_create_object(env, &result);
    
    napi_value bytesSent, bytesReceived, packetsSent, packetsReceived;
    napi_create_double(env, static_cast<double>(stats.bytesSent), &bytesSent);
    napi_create_double(env, static_cast<double>(stats.bytesReceived), &bytesReceived);
    napi_create_int32(env, stats.packetsSent, &packetsSent);
    napi_create_int32(env, stats.packetsReceived, &packetsReceived);
    
    napi_set_named_property(env, result, "bytesSent", bytesSent);
    napi_set_named_property(env, result, "bytesReceived", bytesReceived);
    napi_set_named_property(env, result, "packetsSent", packetsSent);
    napi_set_named_property(env, result, "packetsReceived", packetsReceived);
    
    return result;
}

// 获取带宽
static napi_value GetBandwidth(napi_env env, napi_callback_info info) {
    double bandwidth = get_bandwidth();
    napi_value result;
    napi_create_double(env, bandwidth, &result);
    return result;
}

// 创建WebSocket
static napi_value CreateWebSocket(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected URL");
        return nullptr;
    }
    
    size_t urlLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &urlLength);
    char* urlBuffer = new char[urlLength + 1];
    napi_get_value_string_utf8(env, args[0], urlBuffer, urlLength + 1, nullptr);
    std::string url(urlBuffer);
    delete[] urlBuffer;
    
    WebSocket* ws = new WebSocket(url);
    ws->connect();
    int id = nextWebSocketId++;
    webSockets[id] = ws;
    
    napi_value result;
    napi_create_int32(env, id, &result);
    return result;
}

// 发送WebSocket消息
static napi_value SendWebSocketMessage(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected WebSocket ID and message");
        return nullptr;
    }
    
    int id;
    napi_get_value_int32(env, args[0], &id);
    
    size_t messageLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &messageLength);
    char* messageBuffer = new char[messageLength + 1];
    napi_get_value_string_utf8(env, args[1], messageBuffer, messageLength + 1, nullptr);
    std::string message(messageBuffer);
    delete[] messageBuffer;
    
    auto it = webSockets.find(id);
    if (it == webSockets.end()) {
        napi_throw_error(env, "WebSocket error", "WebSocket not found");
        return nullptr;
    }
    
    bool success = it->second->send(message);
    napi_value result;
    napi_create_boolean(env, success, &result);
    return result;
}

// 关闭WebSocket
static napi_value CloseWebSocket(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected WebSocket ID");
        return nullptr;
    }
    
    int id;
    napi_get_value_int32(env, args[0], &id);
    
    auto it = webSockets.find(id);
    if (it != webSockets.end()) {
        it->second->close();
        delete it->second;
        webSockets.erase(it);
    }
    
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

// 检查互联网连接
static napi_value CheckInternetConnection(napi_env env, napi_callback_info info) {
    bool connected = check_internet_connection();
    napi_value result;
    napi_create_boolean(env, connected, &result);
    return result;
}

// 检查服务器可用性
static napi_value CheckServerAvailability(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected server");
        return nullptr;
    }
    
    size_t serverLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &serverLength);
    char* serverBuffer = new char[serverLength + 1];
    napi_get_value_string_utf8(env, args[0], serverBuffer, serverLength + 1, nullptr);
    std::string server(serverBuffer);
    delete[] serverBuffer;
    
    bool available = check_server_availability(server);
    napi_value result;
    napi_create_boolean(env, available, &result);
    return result;
}

// 验证证书
static napi_value ValidateCertificate(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected certificate");
        return nullptr;
    }
    
    size_t certLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &certLength);
    char* certBuffer = new char[certLength + 1];
    napi_get_value_string_utf8(env, args[0], certBuffer, certLength + 1, nullptr);
    std::string cert(certBuffer);
    delete[] certBuffer;
    
    bool valid = validate_certificate(cert);
    napi_value result;
    napi_create_boolean(env, valid, &result);
    return result;
}

// 下载文件
static napi_value DownloadFile(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected URL and path");
        return nullptr;
    }
    
    size_t urlLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &urlLength);
    char* urlBuffer = new char[urlLength + 1];
    napi_get_value_string_utf8(env, args[0], urlBuffer, urlLength + 1, nullptr);
    std::string url(urlBuffer);
    delete[] urlBuffer;
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[1], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    std::string result = download_file(url, path);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 上传文件
static napi_value UploadFile(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected path and URL");
        return nullptr;
    }
    
    size_t pathLength;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;
    
    size_t urlLength;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &urlLength);
    char* urlBuffer = new char[urlLength + 1];
    napi_get_value_string_utf8(env, args[1], urlBuffer, urlLength + 1, nullptr);
    std::string url(urlBuffer);
    delete[] urlBuffer;
    
    std::string result = upload_file(path, url);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

// 获取网络信息
static napi_value GetNetworkInfo(napi_env env, napi_callback_info info) {
    std::map<std::string, std::string> infoMap = get_network_info();
    
    napi_value result;
    napi_create_object(env, &result);
    
    for (const auto& entry : infoMap) {
        napi_value value;
        napi_create_string_utf8(env, entry.second.c_str(), NAPI_AUTO_LENGTH, &value);
        napi_set_named_property(env, result, entry.first.c_str(), value);
    }
    
    return result;
}

// 设置网络偏好
static napi_value SetNetworkPreference(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    
    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected preference");
        return nullptr;
    }
    
    int preference;
    napi_get_value_int32(env, args[0], &preference);
    
    set_network_preference(preference);
    
    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

// 模块初始化
static napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor descriptors[] = {
        {"httpGet", nullptr, HttpGet, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"httpPost", nullptr, HttpPost, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"httpPut", nullptr, HttpPut, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"httpDelete", nullptr, HttpDelete, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getNetworkStatus", nullptr, GetNetworkStatus, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"isConnected", nullptr, IsConnected, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getNetworkType", nullptr, GetNetworkType, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"ping", nullptr, Ping, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getIPAddress", nullptr, GetIPAddress, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getMACAddress", nullptr, GetMACAddress, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"resolveDNS", nullptr, ResolveDNS, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getProxySettings", nullptr, GetProxySettings, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setProxySettings", nullptr, SetProxySettings, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getNetworkStats", nullptr, GetNetworkStats, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getBandwidth", nullptr, GetBandwidth, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"createWebSocket", nullptr, CreateWebSocket, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"sendWebSocketMessage", nullptr, SendWebSocketMessage, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"closeWebSocket", nullptr, CloseWebSocket, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"checkInternetConnection", nullptr, CheckInternetConnection, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"checkServerAvailability", nullptr, CheckServerAvailability, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"validateCertificate", nullptr, ValidateCertificate, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"downloadFile", nullptr, DownloadFile, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"uploadFile", nullptr, UploadFile, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getNetworkInfo", nullptr, GetNetworkInfo, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"setNetworkPreference", nullptr, SetNetworkPreference, nullptr, nullptr, nullptr, napi_default, nullptr}
    };
    
    napi_define_properties(env, exports, sizeof(descriptors) / sizeof(descriptors[0]), descriptors);
    return exports;
}

NAPI_MODULE(napi_network, Init)
