#include <stdio.h>
#include <stdlib.h>
#include <node_api.h>

static void addon_free(JSVM_Env env, void* data, void* hint) {
  JSVM_Ref* ref = data;
  OH_JSVM_DeleteReference(env, *ref);
  free(ref);
  fprintf(stderr, "addon_free");
}

JSVM_Value addon_new(JSVM_Env env, JSVM_Value exports, bool ref_first) {
  JSVM_Ref* ref = malloc(sizeof(*ref));
  if (ref_first) {
    OH_JSVM_CreateReference(env, exports, 1, ref);
    OH_JSVM_SetInstanceData(env, ref, addon_free, NULL);
  } else {
    OH_JSVM_SetInstanceData(env, ref, addon_free, NULL);
    OH_JSVM_CreateReference(env, exports, 1, ref);
  }
  return exports;
}
