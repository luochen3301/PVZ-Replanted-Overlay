#include "il2cpp_api.h"
#include <mutex>

namespace il2cpp {

void*       (*domain_get)();
void**      (*domain_get_assemblies)(void* domain, size_t* size);
void*       (*assembly_get_image)(void* assembly);
const char* (*image_get_name)(void* image);
int         (*image_get_class_count)(void* image);
void*       (*image_get_class)(void* image, int index);

const char* (*class_get_name)(void* klass);
const char* (*class_get_namespace)(void* klass);
void*       (*class_get_parent)(void* klass);
bool        (*class_is_enum)(void* klass);
bool        (*class_is_valuetype)(void* klass);
void*       (*class_from_type)(void* type);
void*       (*class_get_element_class)(void* klass);
int         (*class_value_size)(void* klass, uint32_t* align);
bool        (*class_is_assignable_from)(void* klass, void* oklass);

void*       (*class_get_fields)(void* klass, void** iter);
const char* (*field_get_name)(void* field);
int         (*field_get_offset)(void* field);
void*       (*field_get_type)(void* field);
uint32_t    (*field_get_flags)(void* field);

void*       (*class_get_methods)(void* klass, void** iter);
const char* (*method_get_name)(void* method);
int         (*method_get_param_count)(void* method);

void*       (*class_get_field_from_name)(void* klass, const char* name);
void*       (*class_get_method_from_name)(void* klass, const char* name, int argc);
void*       (*class_get_static_field_data)(void* klass);

void*       (*runtime_invoke)(void* method, void* obj, void** params, void** exc);
void*       (*runtime_object_unbox)(void* obj);
char*       (*type_get_name)(void* type);
void        (*type_free)(void* ptr);
void*       (*thread_attach)(void* domain);
void        (*thread_detach)(void* thread);

uint64_t g_gameassembly_base = 0;
void* g_domain = nullptr;

template <typename T>
static bool resolve(HMODULE mod, const char* name, T& out, int& missing, bool required = true) {
    out = (T)GetProcAddress(mod, name);
    if (!out && required) missing++;
    return out != nullptr;
}

bool init() {
    HMODULE ga = GetModuleHandleA("GameAssembly.dll");
    if (!ga) { roh::log("[il2cpp] GameAssembly.dll not loaded"); return false; }
    g_gameassembly_base = (uint64_t)ga;

    int missing = 0;
    resolve(ga, "il2cpp_domain_get",                  domain_get, missing);
    resolve(ga, "il2cpp_domain_get_assemblies",       domain_get_assemblies, missing);
    resolve(ga, "il2cpp_assembly_get_image",          assembly_get_image, missing);
    resolve(ga, "il2cpp_image_get_name",              image_get_name, missing);
    resolve(ga, "il2cpp_image_get_class_count",       image_get_class_count, missing);
    resolve(ga, "il2cpp_image_get_class",             image_get_class, missing);
    resolve(ga, "il2cpp_class_get_name",              class_get_name, missing);
    resolve(ga, "il2cpp_class_get_namespace",         class_get_namespace, missing);
    resolve(ga, "il2cpp_class_get_parent",            class_get_parent, missing);
    resolve(ga, "il2cpp_class_is_enum",               class_is_enum, missing);
    resolve(ga, "il2cpp_class_is_valuetype",          class_is_valuetype, missing);
    resolve(ga, "il2cpp_class_from_type",             class_from_type, missing);
    resolve(ga, "il2cpp_class_get_element_class",     class_get_element_class, missing);
    resolve(ga, "il2cpp_class_value_size",            class_value_size, missing);
    resolve(ga, "il2cpp_class_is_assignable_from",    class_is_assignable_from, missing);
    resolve(ga, "il2cpp_class_get_fields",            class_get_fields, missing);
    resolve(ga, "il2cpp_field_get_name",              field_get_name, missing);
    resolve(ga, "il2cpp_field_get_offset",            field_get_offset, missing);
    resolve(ga, "il2cpp_field_get_type",              field_get_type, missing);
    resolve(ga, "il2cpp_field_get_flags",             field_get_flags, missing);
    resolve(ga, "il2cpp_class_get_methods",           class_get_methods, missing);
    resolve(ga, "il2cpp_method_get_name",             method_get_name, missing);
    resolve(ga, "il2cpp_method_get_param_count",      method_get_param_count, missing);
    resolve(ga, "il2cpp_class_get_field_from_name",   class_get_field_from_name, missing);
    resolve(ga, "il2cpp_class_get_method_from_name",  class_get_method_from_name, missing);
    resolve(ga, "il2cpp_class_get_static_field_data", class_get_static_field_data, missing);
    resolve(ga, "il2cpp_runtime_invoke",              runtime_invoke, missing);
    resolve(ga, "il2cpp_object_unbox",                runtime_object_unbox, missing);
    resolve(ga, "il2cpp_type_get_name",               type_get_name, missing);
    resolve(ga, "il2cpp_free",                        type_free, missing);
    resolve(ga, "il2cpp_thread_attach",               thread_attach, missing);
    resolve(ga, "il2cpp_thread_detach",               thread_detach, missing);

    roh::log("[il2cpp] resolved, missing=%d", missing);
    if (missing) return false;

    g_domain = domain_get();
    if (g_domain && thread_attach) {
        __try { thread_attach(g_domain); roh::log("[il2cpp] thread attached"); }
        __except (EXCEPTION_EXECUTE_HANDLER) { roh::log("[il2cpp] thread_attach threw"); }
    }
    return domain_get && domain_get_assemblies && class_get_name && class_get_fields && field_get_offset;
}

// ---- 缓存的类查找 ----
static std::mutex g_cacheMutex;
static std::map<std::string, void*> g_classCache;
static std::map<std::string, int> g_offCache;
static bool g_assemblyListBuilt = false;
static std::vector<void*> g_images;

static void build_image_list() {
    if (g_assemblyListBuilt) return;
    size_t count = 0;
    void** assemblies = domain_get_assemblies(g_domain, &count);
    for (size_t a = 0; a < count && g_images.size() < 256; a++) {
        void* img = nullptr;
        __try { img = assembly_get_image(assemblies[a]); } __except (1) { continue; }
        if (img) g_images.push_back(img);
    }
    g_assemblyListBuilt = true;
    roh::log("[il2cpp] image list: %zu", g_images.size());
}

static __declspec(noinline) void* scan_class_seh(void* img, const char* ns, const char* name) {
    __try {
        int cc = image_get_class_count(img);
        if (cc <= 0 || cc > 200000) return nullptr;
        for (int i = 0; i < cc; i++) {
            void* k = image_get_class(img, i);
            if (!k) continue;
            const char* kn = class_get_name(k);
            const char* kns = class_get_namespace(k);
            if (kn && strcmp(kn, name) == 0) {
                bool nsOk = (ns == nullptr || !*ns) ? (!kns || !*kns)
                                                    : (kns && strcmp(kns, ns) == 0);
                if (nsOk) return k;
            }
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    return nullptr;
}

void* find_class(const char* ns, const char* name) {
    std::string key = std::string(ns ? ns : "") + "::" + (name ? name : "");
    std::lock_guard<std::mutex> lk(g_cacheMutex);
    auto it = g_classCache.find(key);
    if (it != g_classCache.end()) return it->second;

    build_image_list();
    void* found = nullptr;
    for (void* img : g_images) {
        found = scan_class_seh(img, ns, name);
        if (found) break;
    }
    g_classCache[key] = found;
    roh::log("[il2cpp] find_class %s -> %p", key.c_str(), found);
    return found;
}

// SEH-only 小助手：不能在有 C++ 对象的函数里直接 __try
static __declspec(noinline) void* seh_get_field(void* klass, const char* name) {
    __try { return class_get_field_from_name(klass, name); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return nullptr; }
}
static __declspec(noinline) int seh_field_offset(void* f) {
    __try { return field_get_offset(f); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return -1; }
}

int field_off(void* klass, const char* name) {
    if (!klass || !class_get_field_from_name) return -1;
    std::string key = std::to_string((uint64_t)klass) + "." + name;
    std::lock_guard<std::mutex> lk(g_cacheMutex);
    auto it = g_offCache.find(key);
    if (it != g_offCache.end()) return it->second;

    int off = -1;
    void* f = seh_get_field(klass, name);
    if (f) off = seh_field_offset(f);
    g_offCache[key] = off;
    return off;
}

void* field_type_class(void* klass, const char* name) {
    if (!klass || !class_get_field_from_name || !field_get_type || !class_from_type) return nullptr;
    void* f = nullptr;
    __try { f = class_get_field_from_name(klass, name); } __except (1) { return nullptr; }
    if (!f) return nullptr;
    void* t = nullptr, * kc = nullptr;
    __try {
        t = field_get_type(f);
        if (t) kc = class_from_type(t);
    } __except (1) { return nullptr; }
    return kc;
}

} // namespace il2cpp
