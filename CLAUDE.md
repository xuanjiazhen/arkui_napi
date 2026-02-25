# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

This is an **OpenHarmony NAPI component** that uses **GN (Generate Ninja)** as its build system.

### Building the Project

This component is part of the larger OpenHarmony codebase. It cannot be built in isolation.

```bash
# Build must be done within the full OpenHarmony tree
# The main target is:
#   //foundation/arkui/napi:ace_napi        - Shared library (ohos)
#   //foundation/arkui/napi:ace_napi        - Static library (arkui_x)

# Build from OpenHarmony root with full build system
./build.sh --product-name <product> --build-target arkui_napi
```

Key build configuration:
- Main build file: [BUILD.gn](BUILD.gn)
- Build configuration: [napi.gni](napi.gni) (defines source lists and compile options)
- The library `libace_napi.so` is the main output

### Sample Modules

Sample modules demonstrate how to use NAPI:
- [sample/native_module_demo/](sample/native_module_demo/) - Basic module and JS class wrapping
- [sample/native_module_calc/](sample/native_module_calc/) - Calculator example
- [sample/native_module_callback/](sample/native_module_callback/) - Callback patterns
- [sample/native_module_netserver/](sample/native_module_netserver/) - Networking
- [sample/native_module_storage/](sample/native_module_storage/) - Storage operations

Build samples:
```bash
# From full OpenHarmony tree
./build.sh --product-name <product> --build-target //foundation/arkui/napi/sample/native_module_demo:demo
```

### Testing

```bash
# Unit tests
./build.sh --product-name <product> --build-target //foundation/arkui/napi/test/unittest:unittest

# Fuzz tests
./build.sh --product-name <product> --build-target //foundation/arkui/napi/test/fuzztest:fuzztest
```

Test structure:
- [test/unittest/](test/unittest/) - GoogleTest-based unit tests
- [test/fuzztest/](test/fuzztest/) - Fuzzing tests

## Architecture

### Core Abstraction Layers

**NAPI** provides a Node.js N-API compatible layer that abstracts different JavaScript engines:

```
┌─────────────────────────────────────────┐
│     User Native Modules (your code)     │
├─────────────────────────────────────────┤
│         NAPI Public API Layer           │  ← interfaces/kits/napi/native_api.h
│         (Node.js N-API compatible)      │
├─────────────────────────────────────────┤
│      NAPI Internal Implementation       │  ← interfaces/inner_api/napi/
├─────────────────────────────────────────┤
│         NativeEngine (Abstraction)      │  ← native_engine/native_engine.h
├─────────────────────────────────────────┤
│    ArkNativeEngine (Ark/ECMA VM impl)   │  ← native_engine/impl/ark/
├─────────────────────────────────────────┤
│      Panda/Ark ECMAScript Runtime       │
└─────────────────────────────────────────┘
```

### Key Components

1. **NativeEngine** ([native_engine/native_engine.h](native_engine/native_engine.h))
   - Abstract base class that wraps the JavaScript engine
   - Provides unified interface regardless of underlying JS engine
   - Manages the `napi_env` (environment context)
   - Handles scope management, reference counting, async work

2. **ArkNativeEngine** ([native_engine/impl/ark/ark_native_engine.h](native_engine/impl/ark/ark_native_engine.h))
   - Concrete implementation for Ark (Panda) ECMAScript VM
   - Bridges NAPI calls to Ark's C++ API
   - Most engine-specific code lives here

3. **ModuleManager** ([module_manager/](module_manager/))
   - Handles dynamic loading of native modules (.so files)
   - Manages module registration and lifecycle
   - Implements module loading paths and namespaces

4. **ReferenceManager** ([reference_manager/](reference_manager/))
   - Manages `NativeReference` objects that prevent JS values from being garbage collected
   - Handles reference lifecycle and cleanup

5. **ScopeManager** (part of NativeEngine)
   - Manages handle scopes for automatic cleanup
   - Used via `napi_handle_scope` and `napi_escapable_handle_scope`

6. **CallbackScopeManager** ([callback_scope_manager/](callback_scope_manager/))
   - Manages scopes for callback execution
   - Ensures proper cleanup during callback invocations

### CJ FFI Layer

[interfaces/inner_api/cjffi/](interfaces/inner_api/cjffi/) provides a Foreign Function Interface (FFI) for cross-language calls (CJ = C/Java interop):
- `cj_ffi/` - FFI definitions for marshaling data types
- `native/` - Native-side implementation
- `ark_interop/` - Ark runtime interop layer

This enables integration between native code and ArkTS/Java code.

## Module Development

### Creating a NAPI Module

NAPI modules follow this pattern:

```cpp
// 1. Implement your NAPI callback
static napi_value MyFunction(napi_env env, napi_callback_info info) {
    // Your implementation
}

// 2. Export the module
static napi_value MyModuleExport(napi_env env, napi_value exports) {
    static napi_property_descriptor desc[] = {
        DECLARE_NAPI_FUNCTION("myFunction", MyFunction),
    };
    NAPI_CALL(env, napi_define_properties(env, exports,
                                          sizeof(desc)/sizeof(desc[0]), desc));
    return exports;
}

// 3. Register the module
static napi_module myModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = MyModuleExport,
    .nm_modname = "myModule",
    .nm_priv = nullptr,
    .reserved = {0}
};

extern "C" __attribute__((constructor)) void MyModuleRegister() {
    napi_module_register(&myModule);
}
```

### BUILD.gn for Your Module

```gni
import("//build/ohos.gni")

ohos_shared_library("my_module") {
  sources = [
    "my_module.cpp",
  ]

  deps = [ "//foundation/arkui/napi:ace_napi" ]

  external_deps = [ "hilog:libhilog" ]

  relative_install_dir = "module"
  subsystem_name = "arkui"
  part_name = "napi"
}
```

### Important Conventions

- **Thread Safety**: NAPI calls must be made from the correct thread. Use `napi_threadsafe_function` for cross-thread callbacks
- **Error Handling**: Always check return values with `NAPI_CALL` macro
- **Memory Management**: Use scopes (`napi_open_handle_scope`) to allow garbage collection
- **Async Work**: Use `napi_create_async_work` for long-running operations to avoid blocking the JS thread
- **Reference Counting**: Create references for values that need to persist beyond the current scope

## Header Files

- **Public API**: [interfaces/kits/napi/native_api.h](interfaces/kits/napi/native_api.h) - Public NAPI interface (stable)
- **Internal API**: [interfaces/inner_api/napi/](interfaces/inner_api/napi/) - Internal interfaces (may change)
  - `native_node_api.h` - Extended NAPI functions
  - `native_node_hybrid_api.h` - Hybrid app support

## Platform-Specific Code

The codebase supports multiple platforms:
- **OHOS** (OpenHarmony) - Standard platform
- **Windows** (mingw) - Preview/simulation
- **Linux** - Development
- **macOS** - Development
- **ArkUI-X** - Cross-platform framework variant

Platform detection is done via GN variables (`is_ohos`, `is_mingw`, etc.) and preprocessor defines (`OHOS_PLATFORM`, `WINDOWS_PLATFORM`, etc.).

## Code Style

- Follow the [.clang-format](.clang-format) configuration:
  - 4 spaces for indentation
  - 120 character line limit
  - Specific brace wrapping rules (functions have braces on new line)
- Use the `NAPI_CALL` macro for error handling
- All public APIs must match Node.js N-API specification
