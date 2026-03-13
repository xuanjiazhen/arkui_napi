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

constexpr int BYTES_PER_MEGABYTE = 1024 * 1024;
constexpr int KILOBYTE = 1024;
constexpr int PACKET_COUNT_DEFAULT = 1000;
constexpr int PACKET_COUNT_RECEIVED = 1200;
constexpr double BANDWIDTH_DEFAULT = 100.0;
constexpr int STRING_BUFFER_SIZE = 256;

std::string HttpGet(const std::string& url)
{
    return "HTTP GET response from " + url;
}

std::string HttpPost(const std::string& url, const std::string& data)
{
    return "HTTP POST response from " + url + " with data: " + data;
}

std::string HttpPut(const std::string& url, const std::string& data)
{
    return "HTTP PUT response from " + url + " with data: " + data;
}

std::string HttpDelete(const std::string& url)
{
    return "HTTP DELETE response from " + url;
}

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

NetworkStatus GetNetworkStatus()
{
    NetworkStatus status;
    status.connected = true;
    status.type = NETWORK_TYPE_WIFI;
    status.ssid = "TestSSID";
    status.signalStrength = 85;
    return status;
}

bool IsConnected()
{
    return GetNetworkStatus().connected;
}

NetworkType GetNetworkType()
{
    return GetNetworkStatus().type;
}

bool Ping(const std::string& host)
{
    return true;
}

std::string GetIpAddress()
{
    struct ifaddrs* ifaddr;
    struct ifaddrs* ifa;
    std::string ip;

    if (getifaddrs(&ifaddr) == -1) {
        return "";
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == nullptr) {
            continue;
        }
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

std::string GetMacAddress()
{
    return "00:11:22:33:44:55";
}

std::vector<std::string> ResolveDns(const std::string& domain)
{
    struct hostent* he;
    struct in_addr** addr_list;
    std::vector<std::string> ips;

    he = gethostbyname(domain.c_str());
    if (he == nullptr) {
        return ips;
    }

    addr_list = (struct in_addr**)he->h_addr_list;
    for (int i = 0; addr_list[i] != nullptr; i++) {
        ips.push_back(inet_ntoa(*addr_list[i]));
    }

    return ips;
}

struct ProxySettings {
    bool enabled;
    std::string host;
    int port;
};

ProxySettings GetProxySettings()
{
    ProxySettings settings;
    settings.enabled = false;
    settings.host = "";
    settings.port = 0;
    return settings;
}

void SetProxySettings(const ProxySettings& settings)
{
}

struct NetworkStats {
    long long bytesSent;
    long long bytesReceived;
    int packetsSent;
    int packetsReceived;
};

NetworkStats GetNetworkStats()
{
    NetworkStats stats;
    stats.bytesSent = BYTES_PER_MEGABYTE;
    stats.bytesReceived = BYTES_PER_MEGABYTE * 2;
    stats.packetsSent = PACKET_COUNT_DEFAULT;
    stats.packetsReceived = PACKET_COUNT_RECEIVED;
    return stats;
}

double GetBandwidth()
{
    return BANDWIDTH_DEFAULT;
}

class WebSocket {
public:
    explicit WebSocket(const std::string& url) : url_(url), connected_(false) {}

    bool Connect()
    {
        connected_ = true;
        return true;
    }

    bool Send(const std::string& message)
    {
        if (!connected_) {
            return false;
        }
        return true;
    }

    void Close()
    {
        connected_ = false;
    }

    bool IsConnected() const
    {
        return connected_;
    }

private:
    std::string url_;
    bool connected_;
};

std::map<int, WebSocket*> g_webSockets;
int g_nextWebSocketId = 1;

bool CheckInternetConnection()
{
    return Ping("google.com");
}

bool CheckServerAvailability(const std::string& server)
{
    return Ping(server);
}

bool ValidateCertificate(const std::string& certificate)
{
    return true;
}

std::string DownloadFile(const std::string& url, const std::string& path)
{
    return "Downloaded file from " + url + " to " + path;
}

std::string UploadFile(const std::string& path, const std::string& url)
{
    return "Uploaded file from " + path + " to " + url;
}

std::map<std::string, std::string> GetNetworkInfo()
{
    std::map<std::string, std::string> info;
    info["ip"] = GetIpAddress();
    info["mac"] = GetMacAddress();
    info["type"] = "WiFi";
    info["ssid"] = "TestSSID";
    return info;
}

void SetNetworkPreference(int preference)
{
}

}

static napi_value HttpGet(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected URL");
        return nullptr;
    }

    size_t urlLength = 0;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &urlLength);
    char* urlBuffer = new char[urlLength + 1];
    napi_get_value_string_utf8(env, args[0], urlBuffer, urlLength + 1, nullptr);
    std::string url(urlBuffer);
    delete[] urlBuffer;

    std::string result = HttpGet(url);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

static napi_value HttpPost(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected URL and data");
        return nullptr;
    }

    size_t urlLength = 0;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &urlLength);
    char* urlBuffer = new char[urlLength + 1];
    napi_get_value_string_utf8(env, args[0], urlBuffer, urlLength + 1, nullptr);
    std::string url(urlBuffer);
    delete[] urlBuffer;

    size_t dataLength = 0;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &dataLength);
    char* dataBuffer = new char[dataLength + 1];
    napi_get_value_string_utf8(env, args[1], dataBuffer, dataLength + 1, nullptr);
    std::string data(dataBuffer);
    delete[] dataBuffer;

    std::string result = HttpPost(url, data);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

static napi_value HttpPut(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected URL and data");
        return nullptr;
    }

    size_t urlLength = 0;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &urlLength);
    char* urlBuffer = new char[urlLength + 1];
    napi_get_value_string_utf8(env, args[0], urlBuffer, urlLength + 1, nullptr);
    std::string url(urlBuffer);
    delete[] urlBuffer;

    size_t dataLength = 0;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &dataLength);
    char* dataBuffer = new char[dataLength + 1];
    napi_get_value_string_utf8(env, args[1], dataBuffer, dataLength + 1, nullptr);
    std::string data(dataBuffer);
    delete[] dataBuffer;

    std::string result = HttpPut(url, data);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

static napi_value HttpDelete(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected URL");
        return nullptr;
    }

    size_t urlLength = 0;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &urlLength);
    char* urlBuffer = new char[urlLength + 1];
    napi_get_value_string_utf8(env, args[0], urlBuffer, urlLength + 1, nullptr);
    std::string url(urlBuffer);
    delete[] urlBuffer;

    std::string result = HttpDelete(url);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

static napi_value GetNetworkStatus(napi_env env, napi_callback_info info)
{
    NetworkStatus status = GetNetworkStatus();

    napi_value result;
    napi_create_object(env, &result);

    napi_value connected;
    napi_value type;
    napi_value ssid;
    napi_value signalStrength;
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

static napi_value IsConnected(napi_env env, napi_callback_info info)
{
    bool connected = IsConnected();
    napi_value result;
    napi_create_boolean(env, connected, &result);
    return result;
}

static napi_value GetNetworkType(napi_env env, napi_callback_info info)
{
    NetworkType type = GetNetworkType();
    napi_value result;
    napi_create_int32(env, type, &result);
    return result;
}

static napi_value Ping(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected host");
        return nullptr;
    }

    size_t hostLength = 0;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &hostLength);
    char* hostBuffer = new char[hostLength + 1];
    napi_get_value_string_utf8(env, args[0], hostBuffer, hostLength + 1, nullptr);
    std::string host(hostBuffer);
    delete[] hostBuffer;

    bool result = Ping(host);
    napi_value resultValue;
    napi_create_boolean(env, result, &resultValue);
    return resultValue;
}

static napi_value GetIpAddress(napi_env env, napi_callback_info info)
{
    std::string ip = GetIpAddress();
    napi_value result;
    napi_create_string_utf8(env, ip.c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

static napi_value GetMacAddress(napi_env env, napi_callback_info info)
{
    std::string mac = GetMacAddress();
    napi_value result;
    napi_create_string_utf8(env, mac.c_str(), NAPI_AUTO_LENGTH, &result);
    return result;
}

static napi_value ResolveDns(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected domain");
        return nullptr;
    }

    size_t domainLength = 0;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &domainLength);
    char* domainBuffer = new char[domainLength + 1];
    napi_get_value_string_utf8(env, args[0], domainBuffer, domainLength + 1, nullptr);
    std::string domain(domainBuffer);
    delete[] domainBuffer;

    std::vector<std::string> ips = ResolveDns(domain);
    napi_value result;
    napi_create_array(env, &result);

    for (size_t i = 0; i < ips.size(); i++) {
        napi_value ip;
        napi_create_string_utf8(env, ips[i].c_str(), NAPI_AUTO_LENGTH, &ip);
        napi_set_element(env, result, i, ip);
    }

    return result;
}

static napi_value GetProxySettings(napi_env env, napi_callback_info info)
{
    ProxySettings settings = GetProxySettings();

    napi_value result;
    napi_create_object(env, &result);

    napi_value enabled;
    napi_value host;
    napi_value port;
    napi_create_boolean(env, settings.enabled, &enabled);
    napi_create_string_utf8(env, settings.host.c_str(), NAPI_AUTO_LENGTH, &host);
    napi_create_int32(env, settings.port, &port);

    napi_set_named_property(env, result, "enabled", enabled);
    napi_set_named_property(env, result, "host", host);
    napi_set_named_property(env, result, "port", port);

    return result;
}

static napi_value SetProxySettings(napi_env env, napi_callback_info info)
{
    size_t argc = 3;
    napi_value args[3];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 3) {
        napi_throw_error(env, "Invalid arguments", "Expected enabled, host and port");
        return nullptr;
    }

    bool enabled = false;
    napi_get_value_bool(env, args[0], &enabled);

    size_t hostLength = 0;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &hostLength);
    char* hostBuffer = new char[hostLength + 1];
    napi_get_value_string_utf8(env, args[1], hostBuffer, hostLength + 1, nullptr);
    std::string host(hostBuffer);
    delete[] hostBuffer;

    int port = 0;
    napi_get_value_int32(env, args[2], &port);

    ProxySettings settings;
    settings.enabled = enabled;
    settings.host = host;
    settings.port = port;
    SetProxySettings(settings);

    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

static napi_value GetNetworkStats(napi_env env, napi_callback_info info)
{
    NetworkStats stats = GetNetworkStats();

    napi_value result;
    napi_create_object(env, &result);

    napi_value bytesSent;
    napi_value bytesReceived;
    napi_value packetsSent;
    napi_value packetsReceived;
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

static napi_value GetBandwidth(napi_env env, napi_callback_info info)
{
    double bandwidth = GetBandwidth();
    napi_value result;
    napi_create_double(env, bandwidth, &result);
    return result;
}

static napi_value CreateWebSocket(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected URL");
        return nullptr;
    }

    size_t urlLength = 0;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &urlLength);
    char* urlBuffer = new char[urlLength + 1];
    napi_get_value_string_utf8(env, args[0], urlBuffer, urlLength + 1, nullptr);
    std::string url(urlBuffer);
    delete[] urlBuffer;

    WebSocket* ws = new WebSocket(url);
    ws->Connect();
    int id = g_nextWebSocketId++;
    g_webSockets[id] = ws;

    napi_value result;
    napi_create_int32(env, id, &result);
    return result;
}

static napi_value SendWebSocketMessage(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected WebSocket ID and message");
        return nullptr;
    }

    int id = 0;
    napi_get_value_int32(env, args[0], &id);

    size_t messageLength = 0;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &messageLength);
    char* messageBuffer = new char[messageLength + 1];
    napi_get_value_string_utf8(env, args[1], messageBuffer, messageLength + 1, nullptr);
    std::string message(messageBuffer);
    delete[] messageBuffer;

    auto it = g_webSockets.find(id);
    if (it == g_webSockets.end()) {
        napi_throw_error(env, "WebSocket error", "WebSocket not found");
        return nullptr;
    }

    bool success = it->second->Send(message);
    napi_value result;
    napi_create_boolean(env, success, &result);
    return result;
}

static napi_value CloseWebSocket(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected WebSocket ID");
        return nullptr;
    }

    int id = 0;
    napi_get_value_int32(env, args[0], &id);

    auto it = g_webSockets.find(id);
    if (it != g_webSockets.end()) {
        it->second->Close();
        delete it->second;
        g_webSockets.erase(it);
    }

    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

static napi_value CheckInternetConnection(napi_env env, napi_callback_info info)
{
    bool connected = CheckInternetConnection();
    napi_value result;
    napi_create_boolean(env, connected, &result);
    return result;
}

static napi_value CheckServerAvailability(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected server");
        return nullptr;
    }

    size_t serverLength = 0;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &serverLength);
    char* serverBuffer = new char[serverLength + 1];
    napi_get_value_string_utf8(env, args[0], serverBuffer, serverLength + 1, nullptr);
    std::string server(serverBuffer);
    delete[] serverBuffer;

    bool available = CheckServerAvailability(server);
    napi_value result;
    napi_create_boolean(env, available, &result);
    return result;
}

static napi_value ValidateCertificate(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected certificate");
        return nullptr;
    }

    size_t certLength = 0;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &certLength);
    char* certBuffer = new char[certLength + 1];
    napi_get_value_string_utf8(env, args[0], certBuffer, certLength + 1, nullptr);
    std::string cert(certBuffer);
    delete[] certBuffer;

    bool valid = ValidateCertificate(cert);
    napi_value result;
    napi_create_boolean(env, valid, &result);
    return result;
}

static napi_value DownloadFile(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected URL and path");
        return nullptr;
    }

    size_t urlLength = 0;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &urlLength);
    char* urlBuffer = new char[urlLength + 1];
    napi_get_value_string_utf8(env, args[0], urlBuffer, urlLength + 1, nullptr);
    std::string url(urlBuffer);
    delete[] urlBuffer;

    size_t pathLength = 0;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[1], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;

    std::string result = DownloadFile(url, path);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

static napi_value UploadFile(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 2) {
        napi_throw_error(env, "Invalid arguments", "Expected path and URL");
        return nullptr;
    }

    size_t pathLength = 0;
    napi_get_value_string_utf8(env, args[0], nullptr, 0, &pathLength);
    char* pathBuffer = new char[pathLength + 1];
    napi_get_value_string_utf8(env, args[0], pathBuffer, pathLength + 1, nullptr);
    std::string path(pathBuffer);
    delete[] pathBuffer;

    size_t urlLength = 0;
    napi_get_value_string_utf8(env, args[1], nullptr, 0, &urlLength);
    char* urlBuffer = new char[urlLength + 1];
    napi_get_value_string_utf8(env, args[1], urlBuffer, urlLength + 1, nullptr);
    std::string url(urlBuffer);
    delete[] urlBuffer;

    std::string result = UploadFile(path, url);
    napi_value resultValue;
    napi_create_string_utf8(env, result.c_str(), NAPI_AUTO_LENGTH, &resultValue);
    return resultValue;
}

static napi_value GetNetworkInfo(napi_env env, napi_callback_info info)
{
    std::map<std::string, std::string> infoMap = GetNetworkInfo();

    napi_value result;
    napi_create_object(env, &result);

    for (const auto& entry : infoMap) {
        napi_value value;
        napi_create_string_utf8(env, entry.second.c_str(), NAPI_AUTO_LENGTH, &value);
        napi_set_named_property(env, result, entry.first.c_str(), value);
    }

    return result;
}

static napi_value SetNetworkPreference(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);

    if (argc < 1) {
        napi_throw_error(env, "Invalid arguments", "Expected preference");
        return nullptr;
    }

    int preference = 0;
    napi_get_value_int32(env, args[0], &preference);

    SetNetworkPreference(preference);

    napi_value result;
    napi_get_undefined(env, &result);
    return result;
}

static napi_value Init(napi_env env, napi_value exports)
{
    napi_property_descriptor descriptors[] = {
        {"httpGet", nullptr, HttpGet, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"httpPost", nullptr, HttpPost, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"httpPut", nullptr, HttpPut, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"httpDelete", nullptr, HttpDelete, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getNetworkStatus", nullptr, GetNetworkStatus, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"isConnected", nullptr, IsConnected, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getNetworkType", nullptr, GetNetworkType, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"ping", nullptr, Ping, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getIPAddress", nullptr, GetIpAddress, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"getMACAddress", nullptr, GetMacAddress, nullptr, nullptr, nullptr, napi_default, nullptr},
        {"resolveDNS", nullptr, ResolveDns, nullptr, nullptr, nullptr, napi_default, nullptr},
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
