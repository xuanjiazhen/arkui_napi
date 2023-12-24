#include <stdio.h>
#include <stdlib.h>
#include <node_api.h>

JSVM_Value addon_new(JSVM_Env env, JSVM_Value exports, bool ref_first);

// static JSVM_Value
JSVM_MODULE_INIT(/*JSVM_Env env, JSVM_Value exports */) {
  return addon_new(env, exports, false);
}
