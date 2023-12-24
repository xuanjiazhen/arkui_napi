#include <stdlib.h>
#define JSVM_EXPERIMENTAL
#include <node_api.h>

#define JSVM_CALL(env, call)                          \
  do {                                                \
    JSVM_Status status = (call);                      \
    if (status != JSVM_OK) {                          \
      OH_JSVM_ThrowError((env), NULL, #call " failed"); \
      return NULL;                                    \
    }                                                 \
  } while (0)

static JSVM_Value
GetCount(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  size_t* count;

  JSVM_CALL(env, OH_JSVM_GetInstanceData(env, (void**)&count));
  JSVM_CALL(env, OH_JSVM_CreateUint32(env, *count, &result));

  return result;
}

static JSVM_Value
SetCount(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t* count;

  JSVM_CALL(env, OH_JSVM_GetInstanceData(env, (void**)&count));

  // Set the count to zero irrespective of what is passed into the setter.
  *count = 0;

  return NULL;
}

static void
IncrementCounter(JSVM_Env env, void* data, void* hint) {
  size_t* count = data;
  (*count) = (*count) + 1;
}

static JSVM_Value
NewWeak(JSVM_Env env, JSVM_CallbackInfo info) {
  JSVM_Value result;
  void* instance_data;

  JSVM_CALL(env, OH_JSVM_CreateObject(env, &result));
  JSVM_CALL(env, OH_JSVM_GetInstanceData(env, &instance_data));
  JSVM_CALL(env, OH_JSVM_AddFinalizer(env,
                                    result,
                                    instance_data,
                                    IncrementCounter,
                                    NULL,
                                    NULL));

  return result;
}

static void
FreeCount(JSVM_Env env, void* data, void* hint) {
  free(data);
}

/* JSVM_Value */
JSVM_MODULE_INIT(/* JSVM_Env env, JSVM_Value exports */) {
  JSVM_PropertyDescriptor props[] = {
    { "count", NULL, NULL, GetCount, SetCount, NULL, JSVM_ENUMERABLE, NULL },
    { "newWeak", NULL, NewWeak, NULL, NULL, NULL, JSVM_ENUMERABLE, NULL }
  };

  size_t* count = malloc(sizeof(*count));
  *count = 0;

  JSVM_CALL(env, OH_JSVM_DefineProperties(env,
                                        exports,
                                        sizeof(props) / sizeof(*props),
                                        props));
  JSVM_CALL(env, OH_JSVM_SetInstanceData(env, count, FreeCount, NULL));

  return exports;
}
