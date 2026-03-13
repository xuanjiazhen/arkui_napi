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

const networkModule = require('libnapi_network.so');

module.exports = {
  // HTTP请求
  httpGet: networkModule.httpGet,
  httpPost: networkModule.httpPost,
  httpPut: networkModule.httpPut,
  httpDelete: networkModule.httpDelete,
  // 网络状态
  getNetworkStatus: networkModule.getNetworkStatus,
  isConnected: networkModule.isConnected,
  getNetworkType: networkModule.getNetworkType,
  // 网络工具
  ping: networkModule.ping,
  getIPAddress: networkModule.getIPAddress,
  getMACAddress: networkModule.getMACAddress,
  resolveDNS: networkModule.resolveDNS,
  // 网络配置
  getProxySettings: networkModule.getProxySettings,
  setProxySettings: networkModule.setProxySettings,
  // 网络统计
  getNetworkStats: networkModule.getNetworkStats,
  getBandwidth: networkModule.getBandwidth,
  // WebSocket
  createWebSocket: networkModule.createWebSocket,
  sendWebSocketMessage: networkModule.sendWebSocketMessage,
  closeWebSocket: networkModule.closeWebSocket,
  // 网络检测
  checkInternetConnection: networkModule.checkInternetConnection,
  checkServerAvailability: networkModule.checkServerAvailability,
  // 网络安全
  validateCertificate: networkModule.validateCertificate,
  // 其他网络功能
  downloadFile: networkModule.downloadFile,
  uploadFile: networkModule.uploadFile,
  getNetworkInfo: networkModule.getNetworkInfo,
  setNetworkPreference: networkModule.setNetworkPreference
};
