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
#include <unistd.h>
#include <dlfcn.h>
#include <signal.h>
#include <setjmp.h>

#include "napi/native_api.h"
#include "napi/native_node_api.h"

/* ===== Captured data structures ===== */
constexpr int kMaxIfaces = 512;
constexpr int kMaxClasses = 64;
constexpr int kMaxMethods = 128;
constexpr int kMaxName = 256;
constexpr int kMaxModules = 8;
constexpr int kMaxSoPath = 512;
constexpr int kDummyEnvSize = 8192;
constexpr int kDummyExportsSize = 256;

struct CapturedIface {
    char name[kMaxName];
    int type;
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
    int registrationType;
    int hasJsCode;
    int hasAbcCode;
    CapturedIface* interfaces;
    int ifaceCount;
    CapturedClass* classes;
    int classCount;
};

/* ===== Global state ===== */
static CapturedModule* g_modules[kMaxModules];
static int g_moduleCount = 0;
static CapturedModule* g_current = nullptr;
static int g_capture = 0;
static int g_crashCount = 0;
static int g_verbose = 0;
static napi_value g_dummy = reinterpret_cast<napi_value>(0xDEAD);

static void LogMsg(const char* fmt, ...)
{
    if (!g_verbose) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
}

/* ===== Symbol resolution ===== */
static void ResolveSymbol(void* addr, char* nameBuf, int nameSize, char* pathBuf, int pathSize)
{
    Dl_info info {};
    nameBuf[0] = '\0';
    pathBuf[0] = '\0';
    if (addr == nullptr || dladdr(addr, &info) == 0) {
        return;
    }
    if (info.dli_sname != nullptr) {
        strncpy(nameBuf, info.dli_sname, nameSize - 1);
        nameBuf[nameSize - 1] = '\0';
    }
    if (info.dli_fname != nullptr) {
        strncpy(pathBuf, info.dli_fname, pathSize - 1);
        pathBuf[pathSize - 1] = '\0';
    }
}

static void CaptureFromDesc(const napi_property_descriptor* desc, CapturedIface* iface)
{
    memset(iface, 0, sizeof(*iface));
    if (desc->utf8name != nullptr) {
        strncpy(iface->name, desc->utf8name, kMaxName - 1);
    }
    if (desc->method != nullptr) {
        iface->type = 0;
        iface->callbackAddr = reinterpret_cast<void*>(desc->method);
    } else if (desc->getter != nullptr && desc->setter != nullptr) {
        iface->type = 4;
        iface->callbackAddr = reinterpret_cast<void*>(desc->getter);
    } else if (desc->getter != nullptr) {
        iface->type = 1;
        iface->callbackAddr = reinterpret_cast<void*>(desc->getter);
    } else if (desc->setter != nullptr) {
        iface->type = 2;
        iface->callbackAddr = reinterpret_cast<void*>(desc->setter);
    } else {
        iface->type = 3;
        iface->callbackAddr = nullptr;
    }
    if (iface->callbackAddr != nullptr) {
        ResolveSymbol(iface->callbackAddr, iface->callbackName, kMaxName, iface->soPath, kMaxSoPath);
    }
}

/* ===== Data-capturing napi functions ===== */
void napi_module_register(napi_module* mod)
{
    if (mod == nullptr || g_moduleCount >= kMaxModules) {
        return;
    }
    auto m = static_cast<CapturedModule*>(calloc(1, sizeof(CapturedModule)));
    if (m == nullptr) {
        return;
    }
    g_modules[g_moduleCount++] = m;
    if (mod->nm_modname != nullptr) {
        strncpy(m->moduleName, mod->nm_modname, kMaxName - 1);
    }
    m->nmVersion = mod->nm_version;
    m->nmFlags = mod->nm_flags;
    m->nmFilename = mod->nm_filename;
    m->registerFuncAddr = reinterpret_cast<void*>(mod->nm_register_func);
    m->registrationType = 0;
    m->interfaces = static_cast<CapturedIface*>(calloc(kMaxIfaces, sizeof(CapturedIface)));
    m->classes = static_cast<CapturedClass*>(calloc(kMaxClasses, sizeof(CapturedClass)));
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
    auto m = static_cast<CapturedModule*>(calloc(1, sizeof(CapturedModule)));
    if (m == nullptr) {
        return;
    }
    g_modules[g_moduleCount++] = m;
    if (mod->nm_modname != nullptr) {
        strncpy(m->moduleName, mod->nm_modname, kMaxName - 1);
    }
    m->nmVersion = mod->nm_version;
    m->nmFlags = mod->nm_flags;
    m->nmFilename = mod->nm_filename;
    m->registerFuncAddr = reinterpret_cast<void*>(mod->nm_register_func);
    m->registrationType = 1;
    m->hasJsCode = (mod->nm_get_js_code != nullptr) ? 1 : 0;
    m->hasAbcCode = (mod->nm_get_abc_code != nullptr) ? 1 : 0;
    m->interfaces = static_cast<CapturedIface*>(calloc(kMaxIfaces, sizeof(CapturedIface)));
    m->classes = static_cast<CapturedClass*>(calloc(kMaxClasses, sizeof(CapturedClass)));
    if (mod->nm_register_func != nullptr) {
        ResolveSymbol(reinterpret_cast<void*>(mod->nm_register_func),
                      m->registerFuncName, kMaxName, m->registerSoPath, kMaxSoPath);
    }
    LogMsg("[dumper] Module registered (with_js): %s\n", m->moduleName);
}

napi_status napi_define_properties(napi_env, napi_value, size_t count, const napi_property_descriptor* props)
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
    napi_callback ctor, void*, size_t count, const napi_property_descriptor* props, napi_value* result)
{
    if (result != nullptr) {
        *result = g_dummy;
    }
    if (!g_capture || g_current == nullptr || g_current->classCount >= kMaxClasses) {
        return napi_ok;
    }
    auto c = &g_current->classes[g_current->classCount++];
    memset(c, 0, sizeof(*c));
    c->methods = static_cast<CapturedIface*>(calloc(kMaxMethods, sizeof(CapturedIface)));
    if (name != nullptr) {
        size_t n = (len == NAPI_AUTO_LENGTH) ? strlen(name) : len;
        size_t copyLen = (n < kMaxName - 1) ? n : kMaxName - 1;
        memcpy(c->className, name, copyLen);
        c->className[copyLen] = '\0';
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
    memset(f, 0, sizeof(*f));
    f->type = 0;
    f->callbackAddr = reinterpret_cast<void*>(cb);
    if (name != nullptr) {
        size_t n = (len == NAPI_AUTO_LENGTH) ? strlen(name) : len;
        size_t copyLen = (n < kMaxName - 1) ? n : kMaxName - 1;
        memcpy(f->name, name, copyLen);
        f->name[copyLen] = '\0';
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
static const char* IfaceTypeName(int type)
{
    switch (type) {
        case 0:
            return "function";
        case 1:
            return "getter";
        case 2:
            return "setter";
        case 3:
            return "property";
        case 4:
            return "getter_setter";
        default:
            return "unknown";
    }
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

/* ===== Text output ===== */
static void OutputText()
{
    if (g_moduleCount == 0) {
        printf("No NAPI modules found.\n");
        return;
    }
    for (int i = 0; i < g_moduleCount; i++) {
        auto m = g_modules[i];
        if (m == nullptr) {
            continue;
        }
        printf("=== Module: %s ===\n", m->moduleName[0] ? m->moduleName : "(unknown)");
        printf("  Type:         %s\n",
               m->registrationType ? "napi_module_with_js_register" : "napi_module_register");
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
            auto f = &m->interfaces[j];
            printf("    [%-12s] %-30s", IfaceTypeName(f->type), f->name[0] ? f->name : "(unknown)");
            if (f->callbackName[0]) {
                printf(" -> %s", f->callbackName);
            }
            if (f->callbackAddr != nullptr) {
                printf(" @ %p", f->callbackAddr);
            }
            printf("\n");
        }

        if (m->classCount > 0) {
            printf("  Classes (%d):\n", m->classCount);
            for (int j = 0; j < m->classCount; j++) {
                auto c = &m->classes[j];
                printf("    %s", c->className[0] ? c->className : "(unknown)");
                if (c->ctorName[0]) {
                    printf(" (ctor: %s)", c->ctorName);
                }
                printf("\n");
                for (int k = 0; k < c->methodCount; k++) {
                    auto m2 = &c->methods[k];
                    printf("      [%-12s] %-28s", IfaceTypeName(m2->type),
                           m2->name[0] ? m2->name : "(unknown)");
                    if (m2->callbackName[0]) {
                        printf(" -> %s", m2->callbackName);
                    }
                    printf("\n");
                }
            }
        }
        printf("\n");
    }
    if (g_crashCount) {
        printf("*** WARNING: %d module(s) crashed during Init (data may be incomplete) ***\n",
               g_crashCount);
    }
}

/* ===== JSON output ===== */
static const char* g_soPath = nullptr;

static void OutputJson()
{
    printf("{\"so_path\":");
    JsonEscape(g_soPath != nullptr ? g_soPath : "");
    printf(",\"modules\":[");
    for (int i = 0; i < g_moduleCount; i++) {
        auto m = g_modules[i];
        if (m == nullptr) {
            continue;
        }
        if (i) {
            printf(",");
        }
        printf("{\"module_name\":");
        JsonEscape(m->moduleName[0] ? m->moduleName : nullptr);
        printf(",\"registration_type\":\"%s\"",
               m->registrationType ? "napi_module_with_js_register" : "napi_module_register");
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
            auto f = &m->interfaces[j];
            if (j) {
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
        printf("],\"classes\":[");
        for (int j = 0; j < m->classCount; j++) {
            auto c = &m->classes[j];
            if (j) {
                printf(",");
            }
            printf("{\"class_name\":");
            JsonEscape(c->className[0] ? c->className : nullptr);
            printf(",\"constructor\":");
            JsonEscape(c->ctorName[0] ? c->ctorName : nullptr);
            printf(",\"methods\":[");
            for (int k = 0; k < c->methodCount; k++) {
                auto m2 = &c->methods[k];
                if (k) {
                    printf(",");
                }
                printf("{\"name\":");
                JsonEscape(m2->name[0] ? m2->name : nullptr);
                printf(",\"type\":\"%s\"", IfaceTypeName(m2->type));
                printf(",\"callback\":");
                JsonEscape(m2->callbackName[0] ? m2->callbackName : nullptr);
                printf("}");
            }
            printf("]}");
        }
        printf("]}");
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
    fprintf(stderr,
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

/* ===== Main ===== */
int main(int argc, char** argv)
{
    int useJson = 0;
    const char* soPath = nullptr;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            PrintUsage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            g_verbose = 1;
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--format") == 0) {
            if (i + 1 < argc && strcmp(argv[i + 1], "json") == 0) {
                useJson = 1;
                i++;
            } else if (i + 1 < argc && strcmp(argv[i + 1], "text") == 0) {
                useJson = 0;
                i++;
            } else {
                fprintf(stderr, "Error: -f requires 'json' or 'text'\n");
                return 1;
            }
        } else if (argv[i][0] != '-') {
            soPath = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            PrintUsage(argv[0]);
            return 1;
        }
    }

    if (soPath == nullptr) {
        PrintUsage(argv[0]);
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

    // Allocate zeroed memory for dummy env/exports.
    // Using calloc ensures NativeEngine member accesses read zeros (false)
    // instead of SIGSEGV on invalid pointers like 0x1.
    // Zeroed jsThreadType_ = MAIN_THREAD, so IsMainThread()==true,
    // IsWorkerThread()==false, IsRestrictedWorkerThread()==false.
    // IsAlive() returns false (not in alive engine set) via short-circuit.
    static char* dummyEnvMem = static_cast<char*>(calloc(1, kDummyEnvSize));
    static char* dummyExportsMem = static_cast<char*>(calloc(1, kDummyExportsSize));

    for (int mi = 0; mi < g_moduleCount; mi++) {
        auto mod = g_modules[mi];
        if (mod == nullptr || mod->registerFuncAddr == nullptr) {
            continue;
        }
        LogMsg("[dumper] Calling Init: %s\n", mod->moduleName);
        g_current = mod;
        g_capture = 1;
        g_inInit = 1;
        if (sigsetjmp(g_jmp, 1) == 0) {
            auto init = reinterpret_cast<napi_value(*)(napi_env, napi_value)>(mod->registerFuncAddr);
            init(reinterpret_cast<napi_env>(dummyEnvMem), reinterpret_cast<napi_value>(dummyExportsMem));
            LogMsg("[dumper] OK: %d ifaces, %d classes\n", mod->ifaceCount, mod->classCount);
        } else {
            LogMsg("[dumper] CRASH recovered: %d ifaces before crash\n", mod->ifaceCount);
        }
        g_inInit = 0;
        g_capture = 0;
        g_current = nullptr;
    }

    if (useJson) {
        OutputJson();
    } else {
        OutputText();
    }

    fflush(stdout);
    fflush(stderr);
    _exit(0);
}
