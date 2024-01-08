#include <jsvm.h>
#include <string.h>
#include "../common.h"
#include "../entry_point.h"

static JSVM_Value CreateDataView(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 3;
  JSVM_Value args [3];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc == 3, "Wrong number of arguments");

  JSVM_ValueType valuetype0;
  JSVM_Value arraybuffer = args[0];

  NODE_API_CALL(env, OH_JSVM_Typeof(env, arraybuffer, &valuetype0));
  NODE_API_ASSERT(env, valuetype0 == jsvm_object,
              "Wrong type of arguments. Expects a ArrayBuffer as the first "
              "argument.");

  bool is_arraybuffer;
  NODE_API_CALL(env, OH_JSVM_IsArraybuffer(env, arraybuffer, &is_arraybuffer));
  NODE_API_ASSERT(env, is_arraybuffer,
              "Wrong type of arguments. Expects a ArrayBuffer as the first "
              "argument.");

  JSVM_ValueType valuetype1;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[1], &valuetype1));

  NODE_API_ASSERT(env, valuetype1 == jsvm_number,
      "Wrong type of arguments. Expects a number as second argument.");

  size_t byte_offset = 0;
  NODE_API_CALL(env, OH_JSVM_GetValueUint32(env, args[1], (uint32_t*)(&byte_offset)));

  JSVM_ValueType valuetype2;
  NODE_API_CALL(env, OH_JSVM_Typeof(env, args[2], &valuetype2));

  NODE_API_ASSERT(env, valuetype2 == jsvm_number,
      "Wrong type of arguments. Expects a number as third argument.");

  size_t length = 0;
  NODE_API_CALL(env, OH_JSVM_GetValueUint32(env, args[2], (uint32_t*)(&length)));

  JSVM_Value output_dataview;
  NODE_API_CALL(env,
            OH_JSVM_CreateDataview(env, length, arraybuffer,
                                 byte_offset, &output_dataview));

  return output_dataview;
}

static JSVM_Value CreateDataViewFromJSDataView(JSVM_Env env, JSVM_CallbackInfo info) {
  size_t argc = 1;
  JSVM_Value args [1];
  NODE_API_CALL(env, OH_JSVM_GetCbInfo(env, info, &argc, args, NULL, NULL));

  NODE_API_ASSERT(env, argc == 1, "Wrong number of arguments");

  JSVM_ValueType valuetype;
  JSVM_Value input_dataview = args[0];

  NODE_API_CALL(env, OH_JSVM_Typeof(env, input_dataview, &valuetype));
  NODE_API_ASSERT(env, valuetype == jsvm_object,
              "Wrong type of arguments. Expects a DataView as the first "
              "argument.");

  bool is_dataview;
  NODE_API_CALL(env, OH_JSVM_IsDataview(env, input_dataview, &is_dataview));
  NODE_API_ASSERT(env, is_dataview,
              "Wrong type of arguments. Expects a DataView as the first "
              "argument.");
  size_t byte_offset = 0;
  size_t length = 0;
  JSVM_Value buffer;
  NODE_API_CALL(env,
            OH_JSVM_GetDataviewInfo(env, input_dataview, &length, NULL,
                                   &buffer, &byte_offset));

  JSVM_Value output_dataview;
  NODE_API_CALL(env,
            OH_JSVM_CreateDataview(env, length, buffer,
                                 byte_offset, &output_dataview));


  return output_dataview;
}

EXTERN_C_START
JSVM_Value Init(JSVM_Env env, JSVM_Value exports) {
  JSVM_PropertyDescriptor descriptors[] = {
    DECLARE_NODE_API_PROPERTY("CreateDataView", CreateDataView),
    DECLARE_NODE_API_PROPERTY("CreateDataViewFromJSDataView",
        CreateDataViewFromJSDataView)
  };

  NODE_API_CALL(env, OH_JSVM_DefineProperties(
      env, exports, sizeof(descriptors) / sizeof(*descriptors), descriptors));

  return exports;
}
EXTERN_C_END
