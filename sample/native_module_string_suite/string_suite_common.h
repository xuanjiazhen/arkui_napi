#ifndef FOUNDATION_ACE_NAPI_SAMPLE_NATIVE_MODULE_STRING_SUITE_COMMON_H
#define FOUNDATION_ACE_NAPI_SAMPLE_NATIVE_MODULE_STRING_SUITE_COMMON_H

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

namespace StringSuite {

constexpr size_t KNullTerminatorSize = 1;
constexpr int KCaseNumberWidth = 2;

inline std::string BuildIndexedName(const char* prefix, size_t caseNumber)
{
    std::string suffix = std::to_string(caseNumber);
    if (suffix.size() < static_cast<size_t>(KCaseNumberWidth)) {
        suffix.insert(0, static_cast<std::string::size_type>(KCaseNumberWidth - suffix.size()), '0');
    }
    return std::string(prefix) + suffix;
}

inline size_t GetCaseIndex(void* data) { return static_cast<size_t>(reinterpret_cast<uintptr_t>(data)); }

inline bool ReadUtf8(napi_env env, napi_value value, const char* message, std::string* result)
{
    napi_valuetype type = napi_undefined;
    if (napi_typeof(env, value, &type) != napi_ok) {
        return false;
    }
    if (type != napi_string) {
        napi_throw_type_error(env, nullptr, message);
        return false;
    }

    size_t length = 0;
    if (napi_get_value_string_utf8(env, value, nullptr, 0, &length) != napi_ok) {
        return false;
    }

    std::string buffer(length + KNullTerminatorSize, '\0');
    if (napi_get_value_string_utf8(env, value, buffer.data(), buffer.size(), &length) != napi_ok) {
        return false;
    }
    buffer.resize(length);
    *result = buffer;
    return true;
}

inline bool SetNamedInt32(napi_env env, napi_value object, const char* name, int32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_int32(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

inline bool SetNamedUInt32(napi_env env, napi_value object, const char* name, uint32_t value)
{
    napi_value napiValue = nullptr;
    if (napi_create_uint32(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

inline bool SetNamedBool(napi_env env, napi_value object, const char* name, bool value)
{
    napi_value napiValue = nullptr;
    if (napi_get_boolean(env, value, &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

inline bool SetNamedString(napi_env env, napi_value object, const char* name, const std::string& value)
{
    napi_value napiValue = nullptr;
    if (napi_create_string_utf8(env, value.c_str(), value.size(), &napiValue) != napi_ok) {
        return false;
    }
    return napi_set_named_property(env, object, name, napiValue) == napi_ok;
}

inline bool SetNamedValue(napi_env env, napi_value object, const char* name, napi_value value)
{
    return napi_set_named_property(env, object, name, value) == napi_ok;
}

inline void AppendStringDescriptor(std::vector<std::string>* exportNames,
    std::vector<napi_property_descriptor>* descriptors, std::string exportName, napi_callback callback,
    void* data = nullptr)
{
    exportNames->push_back(std::move(exportName));
    descriptors->push_back(napi_property_descriptor{
        exportNames->back().c_str(), nullptr, callback, nullptr, nullptr, nullptr, napi_default, data});
}

}  // namespace StringSuite

#endif  // FOUNDATION_ACE_NAPI_SAMPLE_NATIVE_MODULE_STRING_SUITE_COMMON_H
