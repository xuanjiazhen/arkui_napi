// Include node.h first to define JSVM_VERSION built with the
// binary.
// The node.h should also be included first in embedder's use case.
#include "node.h"
#include "node_api.h"
#include "node_test_fixture.h"

void InitializeBinding(v8::Local<v8::Object> exports,
                       v8::Local<v8::Value> module,
                       v8::Local<v8::Context> context,
                       void* priv) {
  v8::Isolate* isolate = context->GetIsolate();
  exports
      ->Set(context,
            v8::String::NewFromOneByte(isolate,
                                       reinterpret_cast<const uint8_t*>("key"))
                .ToLocalChecked(),
            v8::String::NewFromOneByte(
                isolate, reinterpret_cast<const uint8_t*>("value"))
                .ToLocalChecked())
      .FromJust();
}

NODE_MODULE_LINKED(cctest_linkedbinding, InitializeBinding)

class LinkedBindingTest : public EnvironmentTestFixture {};

TEST_F(LinkedBindingTest, SimpleTest) {
  const v8::HandleScope handle_scope(isolate_);
  const Argv argv;
  Env test_env{handle_scope, argv};

  v8::Local<v8::Context> context = isolate_->GetCurrentContext();

  const char* run_script = "process._linkedBinding('cctest_linkedbinding').key";
  v8::Local<v8::Script> script =
      v8::Script::Compile(
          context,
          v8::String::NewFromOneByte(
              isolate_, reinterpret_cast<const uint8_t*>(run_script))
              .ToLocalChecked())
          .ToLocalChecked();
  v8::Local<v8::Value> completion_value = script->Run(context).ToLocalChecked();
  v8::String::Utf8Value utf8val(isolate_, completion_value);
  CHECK_NOT_NULL(*utf8val);
  CHECK_EQ(strcmp(*utf8val, "value"), 0);
}

void InitializeLocalBinding(v8::Local<v8::Object> exports,
                            v8::Local<v8::Value> module,
                            v8::Local<v8::Context> context,
                            void* priv) {
  ++*static_cast<int*>(priv);
  v8::Isolate* isolate = context->GetIsolate();
  exports
      ->Set(context,
            v8::String::NewFromOneByte(isolate,
                                       reinterpret_cast<const uint8_t*>("key"))
                .ToLocalChecked(),
            v8::String::NewFromOneByte(
                isolate, reinterpret_cast<const uint8_t*>("value"))
                .ToLocalChecked())
      .FromJust();
}

TEST_F(LinkedBindingTest, LocallyDefinedLinkedBindingTest) {
  const v8::HandleScope handle_scope(isolate_);
  const Argv argv;
  Env test_env{handle_scope, argv};

  int calls = 0;
  AddLinkedBinding(*test_env, "local_linked", InitializeLocalBinding, &calls);

  v8::Local<v8::Context> context = isolate_->GetCurrentContext();

  const char* run_script = "process._linkedBinding('local_linked').key";
  v8::Local<v8::Script> script =
      v8::Script::Compile(
          context,
          v8::String::NewFromOneByte(
              isolate_, reinterpret_cast<const uint8_t*>(run_script))
              .ToLocalChecked())
          .ToLocalChecked();
  v8::Local<v8::Value> completion_value = script->Run(context).ToLocalChecked();
  v8::String::Utf8Value utf8val(isolate_, completion_value);
  CHECK_NOT_NULL(*utf8val);
  CHECK_EQ(strcmp(*utf8val, "value"), 0);
  CHECK_EQ(calls, 1);
}

JSVM_Value InitializeLocalNapiBinding(JSVM_Env env, JSVM_Value exports) {
  JSVM_Value key, value;
  CHECK_EQ(OH_JSVM_CreateStringUtf8(env, "hello", JSVM_AUTO_LENGTH, &key),
           JSVM_OK);
  CHECK_EQ(OH_JSVM_CreateStringUtf8(env, "world", JSVM_AUTO_LENGTH, &value),
           JSVM_OK);
  CHECK_EQ(OH_JSVM_SetProperty(env, exports, key, value), JSVM_OK);
  return nullptr;
}

static jsvm_module local_linked_napi = {
    JSVM_MODULE_VERSION,
    node::ModuleFlags::kLinked,
    nullptr,
    InitializeLocalNapiBinding,
    "local_linked_napi",
    nullptr,
    {0},
};

TEST_F(LinkedBindingTest, LocallyDefinedLinkedBindingNapiTest) {
  const v8::HandleScope handle_scope(isolate_);
  const Argv argv;
  Env test_env{handle_scope, argv};

  AddLinkedBinding(*test_env, local_linked_napi);

  v8::Local<v8::Context> context = isolate_->GetCurrentContext();

  const char* run_script = "process._linkedBinding('local_linked_napi').hello";
  v8::Local<v8::Script> script =
      v8::Script::Compile(
          context,
          v8::String::NewFromOneByte(
              isolate_, reinterpret_cast<const uint8_t*>(run_script))
              .ToLocalChecked())
          .ToLocalChecked();
  v8::Local<v8::Value> completion_value = script->Run(context).ToLocalChecked();
  v8::String::Utf8Value utf8val(isolate_, completion_value);
  CHECK_NOT_NULL(*utf8val);
  CHECK_EQ(strcmp(*utf8val, "world"), 0);
}

TEST_F(LinkedBindingTest, LocallyDefinedLinkedBindingNapiCallbackTest) {
  const v8::HandleScope handle_scope(isolate_);
  const Argv argv;
  Env test_env{handle_scope, argv};

  AddLinkedBinding(*test_env,
                   "local_linked_jsvm_cb",
                   InitializeLocalNapiBinding,
                   JSVM_VERSION);

  v8::Local<v8::Context> context = isolate_->GetCurrentContext();

  const char* run_script =
      "process._linkedBinding('local_linked_jsvm_cb').hello";
  v8::Local<v8::Script> script =
      v8::Script::Compile(
          context,
          v8::String::NewFromOneByte(
              isolate_, reinterpret_cast<const uint8_t*>(run_script))
              .ToLocalChecked())
          .ToLocalChecked();
  v8::Local<v8::Value> completion_value = script->Run(context).ToLocalChecked();
  v8::String::Utf8Value utf8val(isolate_, completion_value);
  CHECK_NOT_NULL(*utf8val);
  CHECK_EQ(strcmp(*utf8val, "world"), 0);
}

static int32_t node_api_version = NODE_API_DEFAULT_MODULE_API_VERSION;

JSVM_Value InitializeLocalNapiRefBinding(JSVM_Env env, JSVM_Value exports) {
  JSVM_Value key, value;
  CHECK_EQ(
      OH_JSVM_CreateStringUtf8(env, "jsvm_ref_created", JSVM_AUTO_LENGTH, &key),
      JSVM_OK);

  // In experimental Node-API version we can create JSVM_Ref to any value type.
  // Here we are trying to create a reference to the key string.
  JSVM_Ref ref{};
  if (node_api_version == JSVM_VERSION_EXPERIMENTAL) {
    CHECK_EQ(OH_JSVM_CreateReference(env, key, 1, &ref), JSVM_OK);
    CHECK_EQ(OH_JSVM_DeleteReference(env, ref), JSVM_OK);
  } else {
    CHECK_EQ(OH_JSVM_CreateReference(env, key, 1, &ref), JSVM_INVALID_ARG);
  }
  CHECK_EQ(OH_JSVM_GetBoolean(env, ref != nullptr, &value), JSVM_OK);
  CHECK_EQ(OH_JSVM_SetProperty(env, exports, key, value), JSVM_OK);
  return nullptr;
}

// JSVM_Ref in Node-API version 8 cannot accept strings.
TEST_F(LinkedBindingTest, LocallyDefinedLinkedBindingNapiRefVersion8Test) {
  node_api_version = NODE_API_DEFAULT_MODULE_API_VERSION;
  const v8::HandleScope handle_scope(isolate_);
  const Argv argv;
  Env test_env{handle_scope, argv};

  AddLinkedBinding(*test_env,
                   "local_linked_jsvm_ref_v8",
                   InitializeLocalNapiRefBinding,
                   node_api_version);

  v8::Local<v8::Context> context = isolate_->GetCurrentContext();

  const char* run_script =
      "process._linkedBinding('local_linked_jsvm_ref_v8').jsvm_ref_created";
  v8::Local<v8::Script> script =
      v8::Script::Compile(
          context,
          v8::String::NewFromOneByte(
              isolate_, reinterpret_cast<const uint8_t*>(run_script))
              .ToLocalChecked())
          .ToLocalChecked();
  v8::Local<v8::Value> completion_value = script->Run(context).ToLocalChecked();
  CHECK(completion_value->IsBoolean());
  CHECK(!completion_value.As<v8::Boolean>()->Value());
}

// Experimental version of JSVM_Ref in Node-API can accept strings.
TEST_F(LinkedBindingTest, LocallyDefinedLinkedBindingNapiRefExperimentalTest) {
  node_api_version = JSVM_VERSION_EXPERIMENTAL;
  const v8::HandleScope handle_scope(isolate_);
  const Argv argv;
  Env test_env{handle_scope, argv};

  AddLinkedBinding(*test_env,
                   "local_linked_jsvm_ref_experimental",
                   InitializeLocalNapiRefBinding,
                   node_api_version);

  v8::Local<v8::Context> context = isolate_->GetCurrentContext();

  const char* run_script = "process._linkedBinding('local_linked_jsvm_ref_"
                           "experimental').jsvm_ref_created";
  v8::Local<v8::Script> script =
      v8::Script::Compile(
          context,
          v8::String::NewFromOneByte(
              isolate_, reinterpret_cast<const uint8_t*>(run_script))
              .ToLocalChecked())
          .ToLocalChecked();
  v8::Local<v8::Value> completion_value = script->Run(context).ToLocalChecked();
  CHECK(completion_value->IsBoolean());
  CHECK(completion_value.As<v8::Boolean>()->Value());
}

JSVM_Value NapiLinkedWithInstanceData(JSVM_Env env, JSVM_Value exports) {
  int* instance_data = new int(0);
  CHECK_EQ(OH_JSVM_SetInstanceData(
               env,
               instance_data,
               [](JSVM_Env env, void* data, void* hint) {
                 ++*static_cast<int*>(data);
               },
               nullptr),
           JSVM_OK);

  JSVM_Value key, value;
  CHECK_EQ(OH_JSVM_CreateStringUtf8(env, "hello", JSVM_AUTO_LENGTH, &key),
           JSVM_OK);
  CHECK_EQ(OH_JSVM_CreateExternal(env, instance_data, nullptr, nullptr, &value),
           JSVM_OK);
  CHECK_EQ(OH_JSVM_SetProperty(env, exports, key, value), JSVM_OK);
  return nullptr;
}

static jsvm_module local_linked_jsvm_id = {
    JSVM_MODULE_VERSION,
    node::ModuleFlags::kLinked,
    nullptr,
    NapiLinkedWithInstanceData,
    "local_linked_jsvm_id",
    nullptr,
    {0},
};

TEST_F(LinkedBindingTest, LocallyDefinedLinkedBindingNapiInstanceDataTest) {
  const v8::HandleScope handle_scope(isolate_);
  int* instance_data = nullptr;

  {
    const Argv argv;
    Env test_env{handle_scope, argv};

    AddLinkedBinding(*test_env, local_linked_jsvm_id);

    v8::Local<v8::Context> context = isolate_->GetCurrentContext();

    const char* run_script =
        "process._linkedBinding('local_linked_jsvm_id').hello";
    v8::Local<v8::Script> script =
        v8::Script::Compile(
            context,
            v8::String::NewFromOneByte(
                isolate_, reinterpret_cast<const uint8_t*>(run_script))
                .ToLocalChecked())
            .ToLocalChecked();
    v8::Local<v8::Value> completion_value =
        script->Run(context).ToLocalChecked();
    CHECK(completion_value->IsExternal());
    instance_data =
        static_cast<int*>(completion_value.As<v8::External>()->Value());
    CHECK_NE(instance_data, nullptr);
    CHECK_EQ(*instance_data, 0);
  }

  CHECK_EQ(*instance_data, 1);
  delete instance_data;
}

TEST_F(LinkedBindingTest,
       LocallyDefinedLinkedBindingNapiCallbackInstanceDataTest) {
  const v8::HandleScope handle_scope(isolate_);
  int* instance_data = nullptr;

  {
    const Argv argv;
    Env test_env{handle_scope, argv};

    AddLinkedBinding(*test_env,
                     "local_linked_jsvm_id_cb",
                     NapiLinkedWithInstanceData,
                     JSVM_VERSION);

    v8::Local<v8::Context> context = isolate_->GetCurrentContext();

    const char* run_script =
        "process._linkedBinding('local_linked_jsvm_id_cb').hello";
    v8::Local<v8::Script> script =
        v8::Script::Compile(
            context,
            v8::String::NewFromOneByte(
                isolate_, reinterpret_cast<const uint8_t*>(run_script))
                .ToLocalChecked())
            .ToLocalChecked();
    v8::Local<v8::Value> completion_value =
        script->Run(context).ToLocalChecked();
    CHECK(completion_value->IsExternal());
    instance_data =
        static_cast<int*>(completion_value.As<v8::External>()->Value());
    CHECK_NE(instance_data, nullptr);
    CHECK_EQ(*instance_data, 0);
  }

  CHECK_EQ(*instance_data, 1);
  delete instance_data;
}

TEST_F(LinkedBindingTest, ManyBindingsTest) {
  const v8::HandleScope handle_scope(isolate_);
  const Argv argv;
  Env test_env{handle_scope, argv};

  int calls = 0;
  AddLinkedBinding(*test_env, "local_linked1", InitializeLocalBinding, &calls);
  AddLinkedBinding(*test_env, "local_linked2", InitializeLocalBinding, &calls);
  AddLinkedBinding(*test_env, "local_linked3", InitializeLocalBinding, &calls);
  AddLinkedBinding(*test_env, local_linked_napi);  // Add a N-API addon as well.
  AddLinkedBinding(*test_env, "local_linked4", InitializeLocalBinding, &calls);
  AddLinkedBinding(*test_env, "local_linked5", InitializeLocalBinding, &calls);

  v8::Local<v8::Context> context = isolate_->GetCurrentContext();

  const char* run_script =
      "for (let i = 1; i <= 5; i++)process._linkedBinding(`local_linked${i}`);"
      "process._linkedBinding('local_linked_napi').hello";
  v8::Local<v8::Script> script =
      v8::Script::Compile(
          context,
          v8::String::NewFromOneByte(
              isolate_, reinterpret_cast<const uint8_t*>(run_script))
              .ToLocalChecked())
          .ToLocalChecked();
  v8::Local<v8::Value> completion_value = script->Run(context).ToLocalChecked();
  v8::String::Utf8Value utf8val(isolate_, completion_value);
  CHECK_NOT_NULL(*utf8val);
  CHECK_EQ(strcmp(*utf8val, "world"), 0);
  CHECK_EQ(calls, 5);
}
