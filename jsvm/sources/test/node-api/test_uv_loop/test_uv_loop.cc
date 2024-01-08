#include "../../js-native-api/common.h"

#include <node_api.h>
#include <uv.h>

#include <cassert>
#include <memory>
#include <utility>

template <typename T>
void* SetImmediate(JSVM_Env env, T&& cb) {
  T* ptr = new T(std::move(cb));
  uv_loop_t* loop = nullptr;
  uv_check_t* check = new uv_check_t;
  check->data = ptr;
  NODE_API_ASSERT(env,
              jsvm_get_uv_event_loop(env, &loop) == JSVM_OK,
              "can get event loop");
  uv_check_init(loop, check);
  uv_check_start(check, [](uv_check_t* check) {
    std::unique_ptr<T> ptr {static_cast<T*>(check->data)};
    T cb = std::move(*ptr);
    uv_close(reinterpret_cast<uv_handle_t*>(check), [](uv_handle_t* handle) {
      delete reinterpret_cast<uv_check_t*>(handle);
    });

    assert(cb() != nullptr);
  });
  // Idle handle is needed only to stop the event loop from blocking in poll.
  uv_idle_t* idle = new uv_idle_t;
  uv_idle_init(loop, idle);
  uv_idle_start(idle, [](uv_idle_t* idle) {
    uv_close(reinterpret_cast<uv_handle_t*>(idle), [](uv_handle_t* handle) {
      delete reinterpret_cast<uv_check_t*>(handle);
    });
  });

  return nullptr;
}

static char dummy;

JSVM_Value SetImmediateBinding(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value argv[1];
  JSVM_Value _this;
  void* data;
  NODE_API_CALL(env,
    OH_JSVM_GetCbInfo(env, info, &argc, argv, &_this, &data));
  NODE_API_ASSERT(env, argc >= 1, "Not enough arguments, expected 1.");

  JSVM_ValueType t;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, argv[0], &t));
  NODE_API_ASSERT(env, t == jsvm_function,
      "Wrong first argument, function expected.");

  JSVM_Ref cbref;
  NODE_API_CALL(env,
    OH_JSVM_CreateReference(env, argv[0], 1, &cbref));

  SetImmediate(env, [=]() -> char* {
    JSVM_Value undefined;
    JSVM_Value callback;
    JSVM_HandleScope scope;
    NODE_API_CALL(env, OH_JSVM_OpenHandleScope(env, &scope));
    NODE_API_CALL(env, OH_JSVM_GetUndefined(env, &undefined));
    NODE_API_CALL(env, OH_JSVM_GetReferenceValue(env, cbref, &callback));
    NODE_API_CALL(env, OH_JSVM_DeleteReference(env, cbref));
    NODE_API_CALL(env,
        OH_JSVM_CallFunction(env, undefined, callback, 0, nullptr, nullptr));
    NODE_API_CALL(env, OH_JSVM_CloseHandleScope(env, scope));
    return &dummy;
  });

  return nullptr;
}

JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor properties[] = {
    DECLARE_NODE_API_PROPERTY("SetImmediate", SetImmediateBinding)
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(properties) / sizeof(*properties), properties));

  return exports;
}

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
