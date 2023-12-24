#include <jsvm.h>
#include <stdlib.h>
#include "../common.h"
#include "../entry_point.h"

static size_t g_call_count = 0;

static void Destructor(JSVM_Env env, void* data, void* nothing) {
  JSVM_Ref* ref = data;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, *ref));
  free(ref);
}

static void NoDeleteDestructor(JSVM_Env env, void* data, void* hint) {
  JSVM_Ref* ref = data;
  size_t* call_count = hint;

  // This destructor must be called exactly once.
  if ((*call_count) > 0) abort();
  *call_count = ((*call_count) + 1);
  free(ref);
}

static JSVM_Value New(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value js_this, js_delete;
  bool delete;
  JSVM_Ref* ref = malloc(sizeof(*ref));

  NODE_API_CALL(env,
      OH_JSVM_GetCbInfo(env, info, &argc, &js_delete, &js_this, NULL));
  NODE_API_CALL(env, OH_JSVM_GetValueBool(env, js_delete, &delete));

  if (delete) {
    NODE_API_CALL(env,
        OH_JSVM_Wrap(env, js_this, ref, Destructor, NULL, ref));
  } else {
    NODE_API_CALL(env,
        OH_JSVM_Wrap(env, js_this, ref, NoDeleteDestructor, &g_call_count, ref));
  }
  NODE_API_CALL(env, OH_JSVM_ReferenceRef(env, *ref, NULL));

  return js_this;
}

static void NoopDeleter(JSVM_Env env, void* data, void* hint) {}

// Tests that calling OH_JSVM_RemoveWrap and OH_JSVM_DeleteReference consecutively
// doesn't crash the process.
// This is analogous to the test https://github.com/nodejs/node-addon-api/blob/main/test/objectwrap_constructor_exception.cc.
// In which the Napi::ObjectWrap<> is being destructed immediately after OH_JSVM_Wrap.
// As Napi::ObjectWrap<> is a subclass of Napi::Reference<>, OH_JSVM_RemoveWrap
// in the destructor of Napi::ObjectWrap<> is called before OH_JSVM_DeleteReference
// in the destructor of Napi::Reference<>.
static JSVM_Value DeleteImmediately(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value js_obj;
  JSVM_Ref ref;
  JSVM_ValueType type;

  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, &js_obj, NULL, NULL));

  NODE_API_CALL(env, OH_JSVM_Typeof(env, js_obj, &type));
  NODE_API_ASSERT(env, type == jsvm_object, "Expected object parameter");

  NODE_API_CALL(env, OH_JSVM_Wrap(env, js_obj, NULL, NoopDeleter, NULL, &ref));
  NODE_API_CALL(env, OH_JSVM_RemoveWrap(env, js_obj, NULL));
  NODE_API_CALL(env, OH_JSVM_DeleteReference(env, ref));

  return NULL;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_Value myobj_ctor;
  NODE_API_CALL(env,
      OH_JSVM_DefineClass(
          env, "MyObject", JSVM_AUTO_LENGTH, New, NULL, 0, NULL, &myobj_ctor));
  NODE_API_CALL(env,
      OH_JSVM_SetNamedProperty(env, exports, "MyObject", myobj_ctor));

  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("deleteImmediately", DeleteImmediately),
  };
  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  return exports;
}
EXTERN_C_END
