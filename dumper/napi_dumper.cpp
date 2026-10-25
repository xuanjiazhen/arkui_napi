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
#define MAX_IFACES 512
#define MAX_CLASSES 64
#define MAX_METHODS 128
#define MAX_NAME 256
#define MAX_MODULES 8

struct CapturedIface {
    char name[MAX_NAME];
    int type;
    void* callback_addr;
    char callback_name[MAX_NAME];
    char so_path[512];
};
struct CapturedClass {
    char class_name[MAX_NAME];
    void* ctor_addr;
    char ctor_name[MAX_NAME];
    CapturedIface* methods;
    int method_count;
};
struct CapturedModule {
    char module_name[MAX_NAME];
    int nm_version;
    unsigned int nm_flags;
    const char* nm_filename;
    void* register_func_addr;
    char register_func_name[MAX_NAME];
    char register_so_path[512];
    int registration_type;
    int has_js_code;
    int has_abc_code;
    CapturedIface* interfaces;
    int iface_count;
    CapturedClass* classes;
    int class_count;
};

/* ===== Global state ===== */
static CapturedModule* g_modules[MAX_MODULES];
static int g_module_count = 0;
static CapturedModule* g_current = nullptr;
static int g_capture = 0;
static int g_crash_count = 0;
static int g_verbose = 0;
static napi_value s_dummy = (napi_value)0xDEAD;

#define LOG(...) do { if (g_verbose) fprintf(stderr, __VA_ARGS__); } while(0)

/* ===== Symbol resolution ===== */
static void resolve_symbol(void* addr, char* name_buf, int ns, char* path_buf, int ps) {
    Dl_info info{};
    name_buf[0] = path_buf[0] = '\0';
    if (addr && dladdr(addr, &info)) {
        if (info.dli_sname) { strncpy(name_buf, info.dli_sname, ns-1); name_buf[ns-1]='\0'; }
        if (info.dli_fname) { strncpy(path_buf, info.dli_fname, ps-1); path_buf[ps-1]='\0'; }
    }
}

static void capture_from_desc(const napi_property_descriptor* d, CapturedIface* f) {
    memset(f, 0, sizeof(*f));
    if (d->utf8name) strncpy(f->name, d->utf8name, MAX_NAME-1);
    if (d->method) { f->type=0; f->callback_addr=(void*)d->method; }
    else if (d->getter && d->setter) { f->type=4; f->callback_addr=(void*)d->getter; }
    else if (d->getter) { f->type=1; f->callback_addr=(void*)d->getter; }
    else if (d->setter) { f->type=2; f->callback_addr=(void*)d->setter; }
    else { f->type=3; f->callback_addr=nullptr; }
    if (f->callback_addr) resolve_symbol(f->callback_addr, f->callback_name, MAX_NAME, f->so_path, 512);
}

/* ===== Data-capturing napi functions ===== */
void napi_module_register(napi_module* mod) {
    if (!mod || g_module_count >= MAX_MODULES) return;
    auto m = (CapturedModule*)calloc(1, sizeof(CapturedModule));
    if (!m) return;
    g_modules[g_module_count++] = m;
    if (mod->nm_modname) strncpy(m->module_name, mod->nm_modname, MAX_NAME-1);
    m->nm_version = mod->nm_version;
    m->nm_flags = mod->nm_flags;
    m->nm_filename = mod->nm_filename;
    m->register_func_addr = (void*)mod->nm_register_func;
    m->registration_type = 0;
    m->interfaces = (CapturedIface*)calloc(MAX_IFACES, sizeof(CapturedIface));
    m->classes = (CapturedClass*)calloc(MAX_CLASSES, sizeof(CapturedClass));
    if (mod->nm_register_func)
        resolve_symbol((void*)mod->nm_register_func, m->register_func_name, MAX_NAME, m->register_so_path, 512);
    LOG("[dumper] Module registered: %s\n", m->module_name);
}

void napi_module_with_js_register(napi_module_with_js* mod) {
    if (!mod || g_module_count >= MAX_MODULES) return;
    auto m = (CapturedModule*)calloc(1, sizeof(CapturedModule));
    if (!m) return;
    g_modules[g_module_count++] = m;
    if (mod->nm_modname) strncpy(m->module_name, mod->nm_modname, MAX_NAME-1);
    m->nm_version = mod->nm_version;
    m->nm_flags = mod->nm_flags;
    m->nm_filename = mod->nm_filename;
    m->register_func_addr = (void*)mod->nm_register_func;
    m->registration_type = 1;
    m->has_js_code = (mod->nm_get_js_code != nullptr);
    m->has_abc_code = (mod->nm_get_abc_code != nullptr);
    m->interfaces = (CapturedIface*)calloc(MAX_IFACES, sizeof(CapturedIface));
    m->classes = (CapturedClass*)calloc(MAX_CLASSES, sizeof(CapturedClass));
    if (mod->nm_register_func)
        resolve_symbol((void*)mod->nm_register_func, m->register_func_name, MAX_NAME, m->register_so_path, 512);
    LOG("[dumper] Module registered (with_js): %s\n", m->module_name);
}

napi_status napi_define_properties(napi_env, napi_value, size_t count, const napi_property_descriptor* props) {
    if (!g_capture || !g_current || !props) return napi_ok;
    for (size_t i = 0; i < count && g_current->iface_count < MAX_IFACES; i++)
        capture_from_desc(&props[i], &g_current->interfaces[g_current->iface_count++]);
    return napi_ok;
}

napi_status napi_define_class(napi_env, const char* name, size_t len,
    napi_callback ctor, void*, size_t count, const napi_property_descriptor* props, napi_value* result) {
    if (result) *result = s_dummy;
    if (!g_capture || !g_current || g_current->class_count >= MAX_CLASSES) return napi_ok;
    auto c = &g_current->classes[g_current->class_count++];
    memset(c, 0, sizeof(*c));
    c->methods = (CapturedIface*)calloc(MAX_METHODS, sizeof(CapturedIface));
    if (name) {
        size_t n = (len == NAPI_AUTO_LENGTH) ? strlen(name) : len;
        size_t cp = n < MAX_NAME-1 ? n : MAX_NAME-1;
        memcpy(c->class_name, name, cp); c->class_name[cp] = '\0';
    }
    c->ctor_addr = (void*)ctor;
    if (ctor) resolve_symbol((void*)ctor, c->ctor_name, MAX_NAME, nullptr, 0);
    if (props)
        for (size_t i = 0; i < count && c->method_count < MAX_METHODS; i++)
            capture_from_desc(&props[i], &c->methods[c->method_count++]);
    return napi_ok;
}

napi_status napi_create_function(napi_env, const char* name, size_t len,
    napi_callback cb, void*, napi_value* result) {
    if (result) *result = s_dummy;
    if (!g_capture || !g_current || g_current->iface_count >= MAX_IFACES) return napi_ok;
    auto f = &g_current->interfaces[g_current->iface_count++];
    memset(f, 0, sizeof(*f));
    f->type = 0; f->callback_addr = (void*)cb;
    if (name) {
        size_t n = (len == NAPI_AUTO_LENGTH) ? strlen(name) : len;
        size_t cp = n < MAX_NAME-1 ? n : MAX_NAME-1;
        memcpy(f->name, name, cp); f->name[cp] = '\0';
    }
    if (cb) resolve_symbol((void*)cb, f->callback_name, MAX_NAME, f->so_path, 512);
    return napi_ok;
}

/* ===== SIGSEGV recovery ===== */
static sigjmp_buf g_jmp;
static volatile sig_atomic_t g_in_init = 0;
static void sig_handler(int) {
    if (g_in_init) { g_crash_count++; siglongjmp(g_jmp, 1); }
    _exit(1);
}

/* ===== Output helpers ===== */
static const char* itype(int t) {
    switch(t) { case 0: return "function"; case 1: return "getter"; case 2: return "setter";
        case 3: return "property"; case 4: return "getter_setter"; default: return "unknown"; }
}

static void jesc(const char* s) {
    if (!s) { printf("null"); return; }
    putchar('"');
    for (; *s; s++) {
        switch (*s) { case '"': printf("\\\""); break; case '\\': printf("\\\\"); break;
            case '\n': printf("\\n"); break; case '\t': printf("\\t"); break; default: putchar(*s); }
    }
    putchar('"');
}

/* ===== Text output ===== */
static void output_text() {
    if (g_module_count == 0) {
        printf("No NAPI modules found.\n");
        return;
    }
    for (int i = 0; i < g_module_count; i++) {
        auto m = g_modules[i];
        if (!m) continue;
        printf("=== Module: %s ===\n", m->module_name[0] ? m->module_name : "(unknown)");
        printf("  Type:         %s\n", m->registration_type ? "napi_module_with_js_register" : "napi_module_register");
        printf("  Version:      %d\n", m->nm_version);
        printf("  Init:         %s", m->register_func_name[0] ? m->register_func_name : "(unknown)");
        if (m->register_func_addr) printf(" @ %p", m->register_func_addr);
        printf("\n");
        if (m->register_so_path[0]) printf("  SO:           %s\n", m->register_so_path);
        if (m->has_js_code)  printf("  Has JS Code:  yes\n");
        if (m->has_abc_code) printf("  Has ABC Code: yes\n");

        printf("  Interfaces (%d):\n", m->iface_count);
        for (int j = 0; j < m->iface_count; j++) {
            auto f = &m->interfaces[j];
            printf("    [%-12s] %-30s", itype(f->type), f->name[0] ? f->name : "(unknown)");
            if (f->callback_name[0]) printf(" -> %s", f->callback_name);
            if (f->callback_addr) printf(" @ %p", f->callback_addr);
            printf("\n");
        }

        if (m->class_count > 0) {
            printf("  Classes (%d):\n", m->class_count);
            for (int j = 0; j < m->class_count; j++) {
                auto c = &m->classes[j];
                printf("    %s", c->class_name[0] ? c->class_name : "(unknown)");
                if (c->ctor_name[0]) printf(" (ctor: %s)", c->ctor_name);
                printf("\n");
                for (int k = 0; k < c->method_count; k++) {
                    auto m2 = &c->methods[k];
                    printf("      [%-12s] %-28s", itype(m2->type), m2->name[0] ? m2->name : "(unknown)");
                    if (m2->callback_name[0]) printf(" -> %s", m2->callback_name);
                    printf("\n");
                }
            }
        }
        printf("\n");
    }
    if (g_crash_count)
        printf("*** WARNING: %d module(s) crashed during Init (data may be incomplete) ***\n", g_crash_count);
}

/* ===== JSON output ===== */
static const char* g_so_path = nullptr;

static void output_json() {
    printf("{\"so_path\":");
    jesc(g_so_path ? g_so_path : "");
    printf(",\"modules\":[");
    for (int i = 0; i < g_module_count; i++) {
        auto m = g_modules[i];
        if (!m) continue;
        if (i) printf(",");
        printf("{\"module_name\":"); jesc(m->module_name[0]?m->module_name:nullptr);
        printf(",\"registration_type\":\"%s\"", m->registration_type ? "napi_module_with_js_register":"napi_module_register");
        printf(",\"nm_version\":%d,\"nm_flags\":%u", m->nm_version, m->nm_flags);
        printf(",\"init_func\":"); jesc(m->register_func_name[0]?m->register_func_name:nullptr);
        if (m->register_func_addr) printf(",\"init_addr\":\"%p\"", m->register_func_addr);
        if (m->register_so_path[0]) { printf(",\"init_so\":"); jesc(m->register_so_path); }
        if (m->has_js_code) printf(",\"has_js_code\":true");
        if (m->has_abc_code) printf(",\"has_abc_code\":true");
        printf(",\"interfaces\":[");
        for (int j=0; j<m->iface_count; j++) {
            auto f=&m->interfaces[j];
            if (j) printf(",");
            printf("{\"name\":"); jesc(f->name[0]?f->name:nullptr);
            printf(",\"type\":\"%s\"", itype(f->type));
            printf(",\"callback\":"); jesc(f->callback_name[0]?f->callback_name:nullptr);
            if (f->callback_addr) printf(",\"callback_addr\":\"%p\"", f->callback_addr);
            printf("}");
        }
        printf("],\"classes\":[");
        for (int j=0; j<m->class_count; j++) {
            auto c=&m->classes[j];
            if (j) printf(",");
            printf("{\"class_name\":"); jesc(c->class_name[0]?c->class_name:nullptr);
            printf(",\"constructor\":"); jesc(c->ctor_name[0]?c->ctor_name:nullptr);
            printf(",\"methods\":[");
            for (int k=0; k<c->method_count; k++) {
                auto m2=&c->methods[k];
                if (k) printf(",");
                printf("{\"name\":"); jesc(m2->name[0]?m2->name:nullptr);
                printf(",\"type\":\"%s\"", itype(m2->type));
                printf(",\"callback\":"); jesc(m2->callback_name[0]?m2->callback_name:nullptr);
                printf("}");
            }
            printf("]}");
        }
        printf("]}");
    }
    printf("]");
    if (g_crash_count) printf(",\"crashed\":true,\"crash_count\":%d", g_crash_count);
    printf("}\n");
}

/* ===== Usage ===== */
static void print_usage(const char* prog) {
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
int main(int argc, char** argv) {
    int use_json = 0;
    const char* so_path = nullptr;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")) {
            print_usage(argv[0]);
            return 0;
        } else if (!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")) {
            g_verbose = 1;
        } else if (!strcmp(argv[i], "-f") || !strcmp(argv[i], "--format")) {
            if (i+1 < argc && !strcmp(argv[i+1], "json")) { use_json = 1; i++; }
            else if (i+1 < argc && !strcmp(argv[i+1], "text")) { use_json = 0; i++; }
            else { fprintf(stderr, "Error: -f requires 'json' or 'text'\n"); return 1; }
        } else if (argv[i][0] != '-') {
            so_path = argv[i];
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }

    if (!so_path) {
        print_usage(argv[0]);
        return 1;
    }

    struct sigaction sa{}; sa.sa_handler = sig_handler; sigemptyset(&sa.sa_mask);
    sigaction(SIGSEGV, &sa, nullptr); sigaction(SIGABRT, &sa, nullptr);

    g_so_path = so_path;
    LOG("[dumper] Loading: %s\n", so_path);

    void* handle = dlopen(so_path, RTLD_NOW | RTLD_GLOBAL);
    if (!handle) {
        const char* err = dlerror();
        LOG("[dumper] dlopen failed: %s\n", err);
        if (use_json) {
            printf("{\"so_path\":");
            jesc(so_path);
            printf(",\"modules\":[],\"error\":\"dlopen_failed\"}\n");
        } else
            printf("Error: dlopen failed: %s\n", err ? err : "unknown");
        return 1;
    }

    LOG("[dumper] Modules registered: %d\n", g_module_count);

    // Allocate zeroed memory for dummy env/exports.
    // Using calloc ensures NativeEngine member accesses read zeros (false)
    // instead of SIGSEGV on invalid pointers like 0x1.
    // Zeroed jsThreadType_ = MAIN_THREAD, so IsMainThread()==true,
    // IsWorkerThread()==false, IsRestrictedWorkerThread()==false.
    // IsAlive() returns false (not in alive engine set) via short-circuit.
    static char* dummy_env_mem = (char*)calloc(1, 8192);
    static char* dummy_exports_mem = (char*)calloc(1, 256);

    for (int mi = 0; mi < g_module_count; mi++) {
        auto mod = g_modules[mi];
        if (!mod || !mod->register_func_addr) continue;
        LOG("[dumper] Calling Init: %s\n", mod->module_name);
        g_current = mod; g_capture = 1; g_in_init = 1;
        if (sigsetjmp(g_jmp, 1) == 0) {
            auto init = (napi_value(*)(napi_env, napi_value))mod->register_func_addr;
            init((napi_env)dummy_env_mem, (napi_value)dummy_exports_mem);
            LOG("[dumper] OK: %d ifaces, %d classes\n", mod->iface_count, mod->class_count);
        } else {
            LOG("[dumper] CRASH recovered: %d ifaces before crash\n", mod->iface_count);
        }
        g_in_init = 0; g_capture = 0; g_current = nullptr;
    }

    if (use_json) output_json();
    else output_text();

    fflush(stdout);
    fflush(stderr);
    _exit(0);
}
