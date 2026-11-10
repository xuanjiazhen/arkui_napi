/*
 * napi_dumper.cpp — NAPI Runtime Module Info Extractor
 *
 * Single binary: executable provides all napi stubs via --export-dynamic.
 * Uses real napi headers from codebase (deps on ace_napi).
 * Zero environment variables. One .so per invocation.
 *
 * Usage:
 *   napi_dumper [options] <so_path>
 *   napi_dumper /system/lib64/module/libdeviceinfo.z.so
 *   napi_dumper -f json /system/lib64/module/libdeviceinfo.z.so
 *   napi_dumper --help
 */

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <cstdarg>
#include <unistd.h>
#include <dlfcn.h>
#include <csignal>
#include <csetjmp>

#include "securec.h"
#include "napi/native_api.h"
#include "napi/native_node_api.h"

/* ===== Constants ===== */
constexpr int kMaxIfaces = 512;
constexpr int kMaxClasses = 64;
constexpr int kMaxMethods = 128;
constexpr int kMaxName = 256;
constexpr int kMaxModules = 8;
constexpr int kMaxSoPath = 512;
constexpr int kDummyEnvSize = 8192;
constexpr int kDummyExportsSize = 256;

/* ===== Interface type enum (replaces magic numbers 0-4) ===== */
enum IfaceType {
    kIfaceFunction = 0,
    kIfaceGetter = 1,
    kIfaceSetter = 2,
    kIfaceProperty = 3,
    kIfaceGetterSetter = 4,
};

/* ===== Registration type enum (replaces magic numbers 0/1) ===== */
enum RegType {
    kRegNapiModuleRegister = 0,
    kRegNapiModuleWithJsRegister = 1,
};

/* ===== Captured data structures ===== */
struct CapturedIface {
    char name[kMaxName];
    IfaceType type;
    void* callbackAddr;
    char callbackName[kMaxName];
    char soPath[kMaxSoPath];
};

struct CapturedClass {
    char className[kMaxName];
    void* ctorAddr;
    char ctorName[kMaxName];
    CapturedIface* methods;
    int methodCount;
};

struct CapturedModule {
    char moduleName[kMaxName];
    int nmVersion;
    unsigned int nmFlags;
    const char* nmFilename;
    void* registerFuncAddr;
    char registerFuncName[kMaxName];
    char registerSoPath[kMaxSoPath];
    RegType registrationType;
    bool hasJsCode;
    bool hasAbcCode;
    CapturedIface* interfaces;
    int ifaceCount;
    CapturedClass* classes;
    int classCount;
};

/* ===== Global state ===== */
static CapturedModule* g_modules[kMaxModules];
static int g_moduleCount = 0;
static CapturedModule* g_current = nullptr;
static bool g_capture = false;
static int g_crashCount = 0;
static bool g_verbose = false;
static napi_value g_dummy = reinterpret_cast<napi_value>(0xDEAD);

static void LogMsg(const char* fmt, ...)
{
    if (!g_verbose) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    (void)vfprintf(stderr, fmt, args);
    va_end(args);
}

/* ===== Safe string copy helper ===== */
static void CopyStr(char* dst, int dstSize, const char* src)
{
    if (dst == nullptr || dstSize <= 0 || src == nullptr) {
        return;
    }
    (void)strncpy_s(dst, dstSize, src, dstSize - 1);
    dst[dstSize - 1] = '\0';
}

/* ===== Safe memory copy helper ===== */
static void CopyMem(void* dst, int dstSize, const void* src, int srcSize)
{
    if (dst == nullptr || src == nullptr || dstSize <= 0 || srcSize <= 0) {
        return;
    }
    int copyLen = (srcSize < dstSize) ? srcSize : dstSize;
    (void)memcpy_s(dst, dstSize, src, copyLen);
}

/* ===== Symbol resolution ===== */
static void ResolveSymbol(void* addr, char* nameBuf, int nameSize, char* pathBuf, int pathSize)
{
    if (nameBuf != nullptr && nameSize > 0) {
        nameBuf[0] = '\0';
    }
    if (pathBuf != nullptr && pathSize > 0) {
        pathBuf[0] = '\0';
    }
    if (addr == nullptr) {
        return;
    }
    Dl_info info {};
    if (dladdr(addr, &info) == 0) {
        return;
    }
    if (info.dli_sname != nullptr) {
        CopyStr(nameBuf, nameSize, info.dli_sname);
    }
    if (info.dli_fname != nullptr) {
        CopyStr(pathBuf, pathSize, info.dli_fname);
    }
}

/* ===== Capture helpers ===== */
static IfaceType DescToType(const napi_property_descriptor* desc)
{
    if (desc->method != nullptr) {
        return kIfaceFunction;
    }
    if (desc->getter != nullptr && desc->setter != nullptr) {
        return kIfaceGetterSetter;
    }
    if (desc->getter != nullptr) {
        return kIfaceGetter;
    }
    if (desc->setter != nullptr) {
        return kIfaceSetter;
    }
    return kIfaceProperty;
}

static void CaptureFromDesc(const napi_property_descriptor* desc, CapturedIface* iface)
{
    if (memset_s(iface, sizeof(*iface), 0, sizeof(*iface)) != EOK) {
        return;
    }
    if (desc->utf8name != nullptr) {
        CopyStr(iface->name, kMaxName, desc->utf8name);
    }
    iface->type = DescToType(desc);
    if (desc->method != nullptr) {
        iface->callbackAddr = reinterpret_cast<void*>(desc->method);
    } else if (desc->getter != nullptr) {
        iface->callbackAddr = reinterpret_cast<void*>(desc->getter);
    } else if (desc->setter != nullptr) {
        iface->callbackAddr = reinterpret_cast<void*>(desc->setter);
    } else {
        iface->callbackAddr = nullptr;
    }
    if (iface->callbackAddr != nullptr) {
        ResolveSymbol(iface->callbackAddr, iface->callbackName, kMaxName, iface->soPath, kMaxSoPath);
    }
}

static void CopyNameLen(char* dst, int dstSize, const char* src, size_t len)
{
    if (dst == nullptr || dstSize <= 0 || src == nullptr) {
        return;
    }
    size_t actualLen = (len == NAPI_AUTO_LENGTH) ? strlen(src) : len;
    if (actualLen >= static_cast<size_t>(dstSize)) {
        actualLen = dstSize - 1;
    }
    CopyMem(dst, dstSize, src, static_cast<int>(actualLen));
    dst[actualLen] = '\0';
}

/* ===== Module allocation helper ===== */
static CapturedModule* AllocModule()
{
    auto m = static_cast<CapturedModule*>(calloc(1, sizeof(CapturedModule)));
    if (m == nullptr) {
        return nullptr;
    }
    m->interfaces = static_cast<CapturedIface*>(calloc(kMaxIfaces, sizeof(CapturedIface)));
    m->classes = static_cast<CapturedClass*>(calloc(kMaxClasses, sizeof(CapturedClass)));
    if (m->interfaces == nullptr || m->classes == nullptr) {
        free(m->interfaces);
        free(m->classes);
        free(m);
        return nullptr;
    }
    return m;
}

/* ===== Data-capturing napi functions ===== */
void napi_module_register(napi_module* mod)
{
    if (mod == nullptr || g_moduleCount >= kMaxModules) {
        return;
    }
    auto m = AllocModule();
    if (m == nullptr) {
        return;
    }
    g_modules[g_moduleCount++] = m;
    if (mod->nm_modname != nullptr) {
        CopyStr(m->moduleName, kMaxName, mod->nm_modname);
    }
    m->nmVersion = mod->nm_version;
    m->nmFlags = mod->nm_flags;
    m->nmFilename = mod->nm_filename;
    m->registerFuncAddr = reinterpret_cast<void*>(mod->nm_register_func);
    m->registrationType = kRegNapiModuleRegister;
    if (mod->nm_register_func != nullptr) {
        ResolveSymbol(reinterpret_cast<void*>(mod->nm_register_func),
                      m->registerFuncName, kMaxName, m->registerSoPath, kMaxSoPath);
    }
    LogMsg("[dumper] Module registered: %s\n", m->moduleName);
}

void napi_module_with_js_register(napi_module_with_js* mod)
{
    if (mod == nullptr || g_moduleCount >= kMaxModules) {
        return;
    }
    auto m = AllocModule();
    if (m == nullptr) {
        return;
    }
    g_modules[g_moduleCount++] = m;
    if (mod->nm_modname != nullptr) {
        CopyStr(m->moduleName, kMaxName, mod->nm_modname);
    }
    m->nmVersion = mod->nm_version;
    m->nmFlags = mod->nm_flags;
    m->nmFilename = mod->nm_filename;
    m->registerFuncAddr = reinterpret_cast<void*>(mod->nm_register_func);
    m->registrationType = kRegNapiModuleWithJsRegister;
    m->hasJsCode = (mod->nm_get_js_code != nullptr);
    m->hasAbcCode = (mod->nm_get_abc_code != nullptr);
    if (mod->nm_register_func != nullptr) {
        ResolveSymbol(reinterpret_cast<void*>(mod->nm_register_func),
                      m->registerFuncName, kMaxName, m->registerSoPath, kMaxSoPath);
    }
    LogMsg("[dumper] Module registered (with_js): %s\n", m->moduleName);
}

napi_status napi_define_properties(napi_env, napi_value, size_t count,
    const napi_property_descriptor* props)
{
    if (!g_capture || g_current == nullptr || props == nullptr) {
        return napi_ok;
    }
    for (size_t i = 0; i < count && g_current->ifaceCount < kMaxIfaces; i++) {
        CaptureFromDesc(&props[i], &g_current->interfaces[g_current->ifaceCount++]);
    }
    return napi_ok;
}

napi_status napi_define_class(napi_env, const char* name, size_t len,
    napi_callback ctor, void*, size_t count, const napi_property_descriptor* props,
    napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    if (!g_capture || g_current == nullptr || g_current->classCount >= kMaxClasses) {
        return napi_ok;
    }
    auto c = &g_current->classes[g_current->classCount++];
    if (memset_s(c, sizeof(*c), 0, sizeof(*c)) != EOK) {
        return napi_ok;
    }
    c->methods = static_cast<CapturedIface*>(calloc(kMaxMethods, sizeof(CapturedIface)));
    if (name != nullptr) {
        CopyNameLen(c->className, kMaxName, name, len);
    }
    c->ctorAddr = reinterpret_cast<void*>(ctor);
    if (ctor != nullptr) {
        ResolveSymbol(reinterpret_cast<void*>(ctor), c->ctorName, kMaxName, nullptr, 0);
    }
    if (props != nullptr) {
        for (size_t i = 0; i < count && c->methodCount < kMaxMethods; i++) {
            CaptureFromDesc(&props[i], &c->methods[c->methodCount++]);
        }
    }
    return napi_ok;
}

napi_status napi_create_function(napi_env, const char* name, size_t len,
    napi_callback cb, void*, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    if (!g_capture || g_current == nullptr || g_current->ifaceCount >= kMaxIfaces) {
        return napi_ok;
    }
    auto f = &g_current->interfaces[g_current->ifaceCount++];
    if (memset_s(f, sizeof(*f), 0, sizeof(*f)) != EOK) {
        return napi_ok;
    }
    f->type = kIfaceFunction;
    f->callbackAddr = reinterpret_cast<void*>(cb);
    if (name != nullptr) {
        CopyNameLen(f->name, kMaxName, name, len);
    }
    if (cb != nullptr) {
        ResolveSymbol(reinterpret_cast<void*>(cb), f->callbackName, kMaxName, f->soPath, kMaxSoPath);
    }
    return napi_ok;
}

/* ===== SIGSEGV recovery ===== */
static sigjmp_buf g_jmp;
static volatile sig_atomic_t g_inInit = 0;

static void SigHandler(int)
{
    if (g_inInit) {
        g_crashCount++;
        siglongjmp(g_jmp, 1);
    }
    _exit(1);
}

/* ===== Output helpers ===== */
static const char* IfaceTypeName(IfaceType type)
{
    switch (type) {
        case kIfaceFunction:
            return "function";
        case kIfaceGetter:
            return "getter";
        case kIfaceSetter:
            return "setter";
        case kIfaceProperty:
            return "property";
        case kIfaceGetterSetter:
            return "getter_setter";
        default:
            return "unknown";
    }
}

static const char* RegTypeName(RegType type)
{
    return (type == kRegNapiModuleWithJsRegister) ? "napi_module_with_js_register"
                                                   : "napi_module_register";
}

static void JsonEscape(const char* str)
{
    if (str == nullptr) {
        printf("null");
        return;
    }
    putchar('"');
    for (; *str != '\0'; str++) {
        switch (*str) {
            case '"':
                printf("\\\"");
                break;
            case '\\':
                printf("\\\\");
                break;
            case '\n':
                printf("\\n");
                break;
            case '\t':
                printf("\\t");
                break;
            default:
                putchar(*str);
                break;
        }
    }
    putchar('"');
}

/* ===== Text output (split into per-module/per-iface/per-class) ===== */
static void OutputTextIface(const CapturedIface* f, int indent)
{
    for (int s = 0; s < indent; s++) {
        putchar(' ');
    }
    printf("[%-12s] %-30s", IfaceTypeName(f->type), f->name[0] ? f->name : "(unknown)");
    if (f->callbackName[0]) {
        printf(" -> %s", f->callbackName);
    }
    if (f->callbackAddr != nullptr) {
        printf(" @ %p", f->callbackAddr);
    }
    printf("\n");
}

static void OutputTextClass(const CapturedClass* c)
{
    printf("    %s", c->className[0] ? c->className : "(unknown)");
    if (c->ctorName[0]) {
        printf(" (ctor: %s)", c->ctorName);
    }
    printf("\n");
    for (int k = 0; k < c->methodCount; k++) {
        OutputTextIface(&c->methods[k], 6);
    }
}

static void OutputTextModule(const CapturedModule* m)
{
    printf("=== Module: %s ===\n", m->moduleName[0] ? m->moduleName : "(unknown)");
    printf("  Type:         %s\n", RegTypeName(m->registrationType));
    printf("  Version:      %d\n", m->nmVersion);
    printf("  Init:         %s", m->registerFuncName[0] ? m->registerFuncName : "(unknown)");
    if (m->registerFuncAddr != nullptr) {
        printf(" @ %p", m->registerFuncAddr);
    }
    printf("\n");
    if (m->registerSoPath[0]) {
        printf("  SO:           %s\n", m->registerSoPath);
    }
    if (m->hasJsCode) {
        printf("  Has JS Code:  yes\n");
    }
    if (m->hasAbcCode) {
        printf("  Has ABC Code: yes\n");
    }
    printf("  Interfaces (%d):\n", m->ifaceCount);
    for (int j = 0; j < m->ifaceCount; j++) {
        OutputTextIface(&m->interfaces[j], 4);
    }
    if (m->classCount > 0) {
        printf("  Classes (%d):\n", m->classCount);
        for (int j = 0; j < m->classCount; j++) {
            OutputTextClass(&m->classes[j]);
        }
    }
    printf("\n");
}

static void OutputText()
{
    if (g_moduleCount == 0) {
        printf("No NAPI modules found.\n");
        return;
    }
    for (int i = 0; i < g_moduleCount; i++) {
        if (g_modules[i] != nullptr) {
            OutputTextModule(g_modules[i]);
        }
    }
    if (g_crashCount) {
        printf("*** WARNING: %d module(s) crashed during Init (data may be incomplete) ***\n",
               g_crashCount);
    }
}

/* ===== JSON output (split into per-module/per-iface/per-class) ===== */
static const char* g_soPath = nullptr;

static void OutputJsonIface(const CapturedIface* f, bool first)
{
    if (!first) {
        printf(",");
    }
    printf("{\"name\":");
    JsonEscape(f->name[0] ? f->name : nullptr);
    printf(",\"type\":\"%s\"", IfaceTypeName(f->type));
    printf(",\"callback\":");
    JsonEscape(f->callbackName[0] ? f->callbackName : nullptr);
    if (f->callbackAddr != nullptr) {
        printf(",\"callback_addr\":\"%p\"", f->callbackAddr);
    }
    printf("}");
}

static void OutputJsonClass(const CapturedClass* c, bool first)
{
    if (!first) {
        printf(",");
    }
    printf("{\"class_name\":");
    JsonEscape(c->className[0] ? c->className : nullptr);
    printf(",\"constructor\":");
    JsonEscape(c->ctorName[0] ? c->ctorName : nullptr);
    printf(",\"methods\":[");
    for (int k = 0; k < c->methodCount; k++) {
        OutputJsonIface(&c->methods[k], k == 0);
    }
    printf("]}");
}

static void OutputJsonModule(const CapturedModule* m, bool first)
{
    if (!first) {
        printf(",");
    }
    printf("{\"module_name\":");
    JsonEscape(m->moduleName[0] ? m->moduleName : nullptr);
    printf(",\"registration_type\":\"%s\"", RegTypeName(m->registrationType));
    printf(",\"nm_version\":%d,\"nm_flags\":%u", m->nmVersion, m->nmFlags);
    printf(",\"init_func\":");
    JsonEscape(m->registerFuncName[0] ? m->registerFuncName : nullptr);
    if (m->registerFuncAddr != nullptr) {
        printf(",\"init_addr\":\"%p\"", m->registerFuncAddr);
    }
    if (m->registerSoPath[0]) {
        printf(",\"init_so\":");
        JsonEscape(m->registerSoPath);
    }
    if (m->hasJsCode) {
        printf(",\"has_js_code\":true");
    }
    if (m->hasAbcCode) {
        printf(",\"has_abc_code\":true");
    }
    printf(",\"interfaces\":[");
    for (int j = 0; j < m->ifaceCount; j++) {
        OutputJsonIface(&m->interfaces[j], j == 0);
    }
    printf("],\"classes\":[");
    for (int j = 0; j < m->classCount; j++) {
        OutputJsonClass(&m->classes[j], j == 0);
    }
    printf("]}");
}

static void OutputJson()
{
    printf("{\"so_path\":");
    JsonEscape(g_soPath != nullptr ? g_soPath : "");
    printf(",\"modules\":[");
    bool first = true;
    for (int i = 0; i < g_moduleCount; i++) {
        if (g_modules[i] == nullptr) {
            continue;
        }
        OutputJsonModule(g_modules[i], first);
        first = false;
    }
    printf("]");
    if (g_crashCount) {
        printf(",\"crashed\":true,\"crash_count\":%d", g_crashCount);
    }
    printf("}\n");
}

/* ===== Usage ===== */
static void PrintUsage(const char* prog)
{
    (void)fprintf(stderr,
        "NAPI Runtime Dumper — extract NAPI module info from .so files\n"
        "\n"
        "Usage: %s [options] <so_path>\n"
        "\n"
        "Load the specified .so via dlopen, intercept all napi calls, and call\n"
        "each module's Init function to capture interface/class registration info.\n"
        "\n"
        "Options:\n"
        "  -f, --format <fmt>   Output format: text (default) or json\n"
        "  -v, --verbose        Print progress info to stderr\n"
        "  -h, --help           Show this help\n"
        "\n"
        "Examples:\n"
        "  %s /system/lib64/module/libdeviceinfo.z.so\n"
        "  %s -f json /system/lib64/module/libdeviceinfo.z.so\n"
        "  %s -v /system/lib64/module/libutil.z.so\n"
        "\n"
        "Output goes to stdout (results only). Use -v for progress logs on stderr.\n",
        prog, prog, prog, prog);
}

/* ===== Argument parsing ===== */
static bool ParseArgs(int argc, char** argv, bool& useJson, const char*& soPath)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            PrintUsage(argv[0]);
            return false;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            g_verbose = true;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--format") == 0) {
            if (i + 1 < argc && strcmp(argv[i + 1], "json") == 0) {
                useJson = true;
                i++;
            } else if (i + 1 < argc && strcmp(argv[i + 1], "text") == 0) {
                useJson = false;
                i++;
            } else {
                (void)fprintf(stderr, "Error: -f requires 'json' or 'text'\n");
                return false;
            }
        } else if (argv[i][0] != '-') {
            soPath = argv[i];
        } else {
            (void)fprintf(stderr, "Unknown option: %s\n", argv[i]);
            PrintUsage(argv[0]);
            return false;
        }
    }
    if (soPath == nullptr) {
        PrintUsage(argv[0]);
        return false;
    }
    return true;
}

/* ===== Init call with crash recovery ===== */
static void CallInitFunctions()
{
    static char* dummyEnvMem = static_cast<char*>(calloc(1, kDummyEnvSize));
    static char* dummyExportsMem = static_cast<char*>(calloc(1, kDummyExportsSize));

    for (int mi = 0; mi < g_moduleCount; mi++) {
        auto mod = g_modules[mi];
        if (mod == nullptr || mod->registerFuncAddr == nullptr) {
            continue;
        }
        LogMsg("[dumper] Calling Init: %s\n", mod->moduleName);
        g_current = mod;
        g_capture = true;
        g_inInit = 1;
        if (sigsetjmp(g_jmp, 1) == 0) {
            auto init = reinterpret_cast<napi_value(*)(napi_env, napi_value)>(mod->registerFuncAddr);
            init(reinterpret_cast<napi_env>(dummyEnvMem), reinterpret_cast<napi_value>(dummyExportsMem));
            LogMsg("[dumper] OK: %d ifaces, %d classes\n", mod->ifaceCount, mod->classCount);
        } else {
            LogMsg("[dumper] CRASH recovered: %d ifaces before crash\n", mod->ifaceCount);
        }
        g_inInit = 0;
        g_capture = false;
        g_current = nullptr;
    }
}

/* ===== Main ===== */
int main(int argc, char** argv)
{
    bool useJson = false;
    const char* soPath = nullptr;

    if (!ParseArgs(argc, argv, useJson, soPath)) {
        return 1;
    }

    struct sigaction sa {};
    sa.sa_handler = SigHandler;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);

    g_soPath = soPath;
    LogMsg("[dumper] Loading: %s\n", soPath);

    void* handle = dlopen(soPath, RTLD_NOW | RTLD_GLOBAL);
    if (handle == nullptr) {
        const char* err = dlerror();
        LogMsg("[dumper] dlopen failed: %s\n", err);
        if (useJson) {
            printf("{\"so_path\":");
            JsonEscape(soPath);
            printf(",\"modules\":[],\"error\":\"dlopen_failed\"}\n");
        } else {
            printf("Error: dlopen failed: %s\n", err != nullptr ? err : "unknown");
        }
        return 1;
    }

    LogMsg("[dumper] Modules registered: %d\n", g_moduleCount);

    CallInitFunctions();

    if (useJson) {
        OutputJson();
    } else {
        OutputText();
    }

    fflush(stdout);
    fflush(stderr);
    _exit(0);
}
