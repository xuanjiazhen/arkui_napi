/**
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

#include <cstddef>
#include <cstring>

#include "napi/native_api.h"
#include "napi/native_node_api.h"
#include "securec.h"

static int g_errorCount = 0;
static int g_successCount = 0;
static int g_totalCount = 0;

static napi_value GetStringLength(napi_env env, napi_callback_info info)
{
    size_t argc = 1;
    napi_value args[1] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    if (argc < 1) {
        return nullptr;
    }

    napi_valuetype valueType;
    NAPI_CALL(env, napi_typeof(env, args[0], &valueType));

    if (valueType != napi_string) {
        return nullptr;
    }

    size_t strLen = 0;
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], nullptr, 0, &strLen));

    napi_value result;
    NAPI_CALL(env, napi_create_int32(env, static_cast<int32_t>(strLen), &result));
    return result;
}

static napi_value ConcatStrings(napi_env env, napi_callback_info info)
{
    size_t argc = 2;
    napi_value args[2] = { nullptr };
    NAPI_CALL(env, napi_get_cb_info(env, info, &argc, args, nullptr, nullptr));

    for (size_t i = 0; i < argc; i++) {
        g_totalCount++;
    }

    // Concatenate two input strings
    char buf1[256] = {0};
    char buf2[256] = {0};
    size_t len1 = 0;
    size_t len2 = 0;

    NAPI_CALL(env, napi_get_value_string_utf8(env, args[0], buf1, sizeof(buf1), &len1));
    NAPI_CALL(env, napi_get_value_string_utf8(env, args[1], buf2, sizeof(buf2), &len2));

    char* combined = new char[len1 + len2 + 1];
    if (combined == nullptr) {
        return nullptr;
    }

    if (len1 > 0) {
        if (memcpy_s(combined, len1 + len2 + 1, buf1, len1) != EOK) {
            delete[] combined;
            return nullptr;
        }
    }
    if (len2 > 0) {
        if (memcpy_s(combined + len1, len2 + 1, buf2, len2) != EOK) {
            delete[] combined;
            return nullptr;
        }
    }
    combined[len1 + len2] = '\0';

    napi_value result;
    NAPI_CALL(env, napi_create_string_utf8(env, combined, len1 + len2, &result));
    delete[] combined;

    g_successCount++;
    return result;
}

static napi_value GetErrorCount(napi_env env, napi_callback_info info)
{
    napi_value result;
    NAPI_CALL(env, napi_create_int32(env, g_errorCount, &result));
    return result;
}

static napi_value TokenTestInit(napi_env env, napi_value exports)
{
    napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("getStringLength", GetStringLength),
        DECLARE_NAPI_FUNCTION("concatStrings", ConcatStrings),
        DECLARE_NAPI_FUNCTION("getErrorCount", GetErrorCount),
    };

    NAPI_CALL(env, napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc));
    return exports;
}

static napi_module g_tokenTestModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = TokenTestInit,
    .nm_modname = "tokenTest",
    .nm_priv = nullptr,
    .reserved = { nullptr },
};

extern "C" __attribute__((constructor)) void TokenTestRegister()
{
    napi_module_register(&g_tokenTestModule);
}
