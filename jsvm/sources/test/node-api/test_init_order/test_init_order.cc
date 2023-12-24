#include <node_api.h>
#include <memory>
#include <string>
#include "../../js-native-api/common.h"

// This test verifies that use of the JSVM_MODULE in C++ code does not
// interfere with the C++ dynamic static initializers.

namespace {

// This class uses dynamic static initializers for the test.
// In production code developers must avoid dynamic static initializers because
// they affect the start up time. They must prefer static initialization such as
// use of constexpr functions or classes with constexpr constructors. E.g.
// instead of using std::string, it is preferrable to use const char[], or
// constexpr std::string_view starting with C++17, or even constexpr
// std::string starting with C++20.
struct MyClass {
  static const std::unique_ptr<int> valueHolder;
  static const std::string testString;
};

const std::unique_ptr<int> MyClass::valueHolder =
    std::unique_ptr<int>(new int(42));
// NOLINTNEXTLINE(runtime/string)
const std::string MyClass::testString = std::string("123");

}  // namespace

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_Value cppIntValue, cppStringValue;
  NODE_API_CALL(env,
                OH_JSVM_CreateInt32(env, *MyClass::valueHolder, &cppIntValue));
  NODE_API_CALL(
      env,
      OH_JSVM_CreateStringUtf8(
          env, MyClass::testString.c_str(), JSVM_AUTO_LENGTH, &cppStringValue));

  JSVM_PropertyDescriptor descriptors[] = {
      DECLARE_NODE_API_PROPERTY_VALUE("cppIntValue", cppIntValue),
      DECLARE_NODE_API_PROPERTY_VALUE("cppStringValue", cppStringValue)};

  NODE_API_CALL(env,
                OH_JSVM_DefineProperties(
                    env, exports, std::size(descriptors), descriptors));

  return exports;
}
EXTERN_C_END

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
