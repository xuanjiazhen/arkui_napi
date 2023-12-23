#include <assert.h>
#include <stdlib.h>
#include "../../js-native-api/common.h"
#include "node_api.h"
#include "uv.h"

static int cleanup_hook_count = 0;
static void cleanup(void* arg) {
  cleanup_hook_count++;
  printf("cleanup(%d)\n", *(int*)(arg));
}

static int secret = 42;
static int wrong_secret = 17;

static void ObjectFinalizer(JSVM_Env env, void* data, void* hint) {
  // cleanup is called once.
  assert(cleanup_hook_count == 1);

  JSVM_Ref* ref = data;
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_DeleteReference(env, *ref));
  free(ref);
}

static void CreateObjectWrap(JSVM_Env env) {
  JSVM_Value js_obj;
  JSVM_Ref* ref = malloc(sizeof(*ref));
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_CreateObject(env, &js_obj));
  NODE_API_CALL_RETURN_VOID(
      env, OH_JSVM_Wrap(env, js_obj, ref, ObjectFinalizer, NULL, ref));
  // create a strong reference so that the finalizer is called at shutdown.
  NODE_API_CALL_RETURN_VOID(env, OH_JSVM_ReferenceRef(env, *ref, NULL));
}

static JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  // Create object wrap before cleanup hooks.
  CreateObjectWrap(env);

  jsvm_add_env_cleanup_hook(env, cleanup, &wrong_secret);
  jsvm_add_env_cleanup_hook(env, cleanup, &secret);
  jsvm_remove_env_cleanup_hook(env, cleanup, &wrong_secret);

  // Create object wrap after cleanup hooks.
  CreateObjectWrap(env);

  return NULL;
}

JSVM_MODULE(NODE_GYP_MODULE_NAME, Init)
