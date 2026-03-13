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

const cryptoModule = require('libnapi_crypto.so');

module.exports = {
  // 哈希函数
  md5: cryptoModule.md5,
  sha1: cryptoModule.sha1,
  sha256: cryptoModule.sha256,
  sha512: cryptoModule.sha512,
  // 加密解密
  encryptAES: cryptoModule.encryptAES,
  decryptAES: cryptoModule.decryptAES,
  encryptDES: cryptoModule.encryptDES,
  decryptDES: cryptoModule.decryptDES,
  // 编码解码
  base64Encode: cryptoModule.base64Encode,
  base64Decode: cryptoModule.base64Decode,
  urlEncode: cryptoModule.urlEncode,
  urlDecode: cryptoModule.urlDecode,
  // 随机数
  randomBytes: cryptoModule.randomBytes,
  randomInt: cryptoModule.randomInt,
  // HMAC
  hmacMD5: cryptoModule.hmacMD5,
  hmacSHA1: cryptoModule.hmacSHA1,
  hmacSHA256: cryptoModule.hmacSHA256,
  // 密码生成
  generateSalt: cryptoModule.generateSalt,
  hashPassword: cryptoModule.hashPassword,
  verifyPassword: cryptoModule.verifyPassword,
  // UUID
  generateUUID: cryptoModule.generateUUID,
  validateUUID: cryptoModule.validateUUID,
  // 安全比较
  secureCompare: cryptoModule.secureCompare,
  // 时间戳签名
  signTimestamp: cryptoModule.signTimestamp,
  verifyTimestamp: cryptoModule.verifyTimestamp,
  // 数据完整性
  generateChecksum: cryptoModule.generateChecksum,
  verifyChecksum: cryptoModule.verifyChecksum,
  // 加密工具
  encrypt: cryptoModule.encrypt,
  decrypt: cryptoModule.decrypt,
  // 密钥管理
  generateKey: cryptoModule.generateKey,
  deriveKey: cryptoModule.deriveKey,
  // 证书相关
  generateCertificate: cryptoModule.generateCertificate,
  validateCertificate: cryptoModule.validateCertificate
};
