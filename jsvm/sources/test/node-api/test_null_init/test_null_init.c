#include <node_api.h>

// This test uses old module initialization style deprecated in current code.
// The goal is to see that all previously compiled code continues to work the
// same way as before.
// The test has a copy of previous macro definitions which are removed from
// the node_api.h file.

#if defined(_MSC_VER)
#if defined(__cplusplus)
#define JSVM_C_CTOR(fn)                                                        \
  static void JSVM_CDECL fn(void);                                             \
  namespace {                                                                  \
  struct fn##_ {                                                               \
    fn##_() { fn(); }                                                          \
  } fn##_v_;                                                                   \
  }                                                                            \
  static void JSVM_CDECL fn(void)
#else  // !defined(__cplusplus)
#pragma section(".CRT$XCU", read)
// The JSVM_C_CTOR macro defines a function fn that is called during CRT
// initialization.
// C does not support dynamic initialization of static variables and this code
// simulates C++ behavior. Exporting the function pointer prevents it from being
// optimized. See for details:
// https://docs.microsoft.com/en-us/cpp/c-runtime-library/crt-initialization?view=msvc-170
#define JSVM_C_CTOR(fn)                                                        \
  static void JSVM_CDECL fn(void);                                             \
  __declspec(dllexport, allocate(".CRT$XCU")) void(JSVM_CDECL * fn##_)(void) = \
      fn;                                                                      \
  static void JSVM_CDECL fn(void)
#endif  // defined(__cplusplus)
#else
#define JSVM_C_CTOR(fn)                                                        \
  static void fn(void) __attribute__((constructor));                           \
  static void fn(void)
#endif

#define JSVM_MODULE_TEST(modname, regfunc)                                     \
  EXTERN_C_START                                                               \
  static jsvm_module _module = {                                               \
      JSVM_MODULE_VERSION,                                                     \
      0,                                                                       \
      __FILE__,                                                                \
      regfunc,                                                                 \
      #modname,                                                                \
      NULL,                                                                    \
      {0},                                                                     \
  };                                                                           \
  JSVM_C_CTOR(_register_##modname) { jsvm_module_register(&_module); }         \
  EXTERN_C_END

JSVM_MODULE_TEST(NODE_GYP_MODULE_NAME, NULL)
