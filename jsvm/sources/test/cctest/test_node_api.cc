#include <stdio.h>
#include <cstdio>
#include <string>
#include "env-inl.h"
#include "gtest/gtest.h"
#include "node_api_internals.h"
#include "node_binding.h"
#include "node_test_fixture.h"

using v8::Local;
using v8::Object;

static JSVM_Env addon_env;

class NodeApiTest : public EnvironmentTestFixture {
 private:
  void SetUp() override { EnvironmentTestFixture::SetUp(); }

  void TearDown() override { EnvironmentTestFixture::TearDown(); }
};

TEST_F(NodeApiTest, CreateNodeApiEnv) {
  const v8::HandleScope handle_scope(isolate_);
  Argv argv;

  Env test_env{handle_scope, argv};

  node::Environment* env = *test_env;
  node::LoadEnvironment(env, "");

  jsvm_addon_register_func init = [](JSVM_Env env, JSVM_Value exports) {
    addon_env = env;
    return exports;
  };
  Local<Object> module_obj = Object::New(isolate_);
  Local<Object> exports_obj = Object::New(isolate_);
  jsvm_module_register_by_symbol(
      exports_obj, module_obj, env->context(), init, JSVM_VERSION);
  ASSERT_NE(addon_env, nullptr);
  node_jsvm_env internal_env = reinterpret_cast<node_jsvm_env>(addon_env);
  EXPECT_EQ(internal_env->node_env(), env);
}
