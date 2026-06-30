# ArkUI NAPI Project Overview

The **foundation/arkui/napi** repository is a critical component of the OpenHarmony ArkUI framework. It provides a development framework for extending JavaScript Native Modules, offering APIs based on the Node.js N-API standard for external use. This allows for seamless interoperability between JavaScript and C/C++ code.

## Key Technologies and Architecture

- **C++ (Backend):** The core implementation is in C++, providing a high-performance bridge between the JS engine and native system capabilities.
- **Ark/ETS Runtime:** The primary JS engine supported is the ArkCompiler (ETS runtime).
- **Node.js N-API:** Follows the N-API standard, ensuring familiarity for developers coming from the Node.js ecosystem.
- **GN Build System:** Uses Generate Ninja (GN) for build configuration, integrated into the OpenHarmony build process.

### Architecture Components

- **NativeEngine:** An abstraction layer that unifies API behavior across different JS engines.
- **ModuleManager:** Responsible for loading, caching, and managing NAPI modules.
- **ScopeManager & ReferenceManager:** Manage the lifecycle of NativeValues and NativeReferences, ensuring proper memory management and object persistence.

## Directory Structure

```text
foundation/arkui/napi
├── interfaces
│   ├── kits/napi          # Public NAPI header files (e.g., native_api.h)
│   └── inner_api/napi     # Internal NAPI header files
├── native_engine          # JS engine abstraction layer
│   └── impl/ark           # Ark-based NativeEngine implementation
├── module_manager         # Module loading and management logic
├── scope_manager          # NativeValue lifecycle management
├── reference_manager      # NativeReference lifecycle management
├── test                   # Comprehensive test suite (Unit, Fuzz, System)
├── sample                 # Sample NAPI modules for reference
└── utils                  # Shared utility functions (logging, platform-specific code)
```

## Building and Testing

### Building
The project is typically built as part of the OpenHarmony build system. The main targets are:
- `//foundation/arkui/napi:ace_napi`: The core NAPI shared library.
- `//foundation/arkui/napi:napi_packages`: A group target including the library and related components.

To build the project standalone (if configured):
```bash
# This is an example, actual commands depend on the OpenHarmony build environment (hb or build.py)
./build.py --product-name <product_name> --build-target foundation/arkui/napi:napi_packages
```

### Testing
The project includes extensive unit tests and fuzz tests.
- **Unit Tests:** Located in `test/unittest` and `module_manager/test/unittest`.
- **Fuzz Tests:** Located in `test/fuzztest`.

To run unit tests:
```bash
./build.py --product-name <product_name> --build-target foundation/arkui/napi:napi_packages_test
# After building, run the generated test binaries (typically in out/<product>/tests/unittest/...)
```

## Development Conventions

- **Code Style:** Strictly adheres to the project's `.clang-format` configuration.
- **NAPI Standard:** New APIs should follow the N-API design patterns to maintain compatibility and consistency.
- **Memory Management:** Use `napi_handle_scope` and `napi_ref` carefully to prevent memory leaks or use-after-free issues in the JS/Native boundary.
- **Error Handling:** Use `NAPI_CALL` and related macros to ensure all NAPI calls are checked for success.
- **Performance:** For CPU-intensive tasks, utilize `napi_create_async_work` to avoid blocking the main JS thread.

## Usage in Apps

Developers can use NAPI to encapsulate native capabilities (e.g., network, multimedia, sensors) and expose them as JS APIs. A typical module registration involves defining a `napi_module` structure and a registration function marked with `__attribute__((constructor))`.
