#pragma once
#include "framework.h"

// 通过 GetProcAddress("GameAssembly.dll") 按名解析的 il2cpp 运行时 API。
// 不硬编码任何偏移，游戏小版本更新可自愈。
namespace il2cpp {

// ---- 基础元数据 ----
extern void*       (*domain_get)();
extern void**      (*domain_get_assemblies)(void* domain, size_t* size);
extern void*       (*assembly_get_image)(void* assembly);
extern const char* (*image_get_name)(void* image);
extern int         (*image_get_class_count)(void* image);
extern void*       (*image_get_class)(void* image, int index);

extern const char* (*class_get_name)(void* klass);
extern const char* (*class_get_namespace)(void* klass);
extern void*       (*class_get_parent)(void* klass);
extern bool        (*class_is_enum)(void* klass);
extern bool        (*class_is_valuetype)(void* klass);
extern void*       (*class_from_type)(void* type);          // Il2CppType* -> Il2CppClass*（泛型实例化类）
extern void*       (*class_get_element_class)(void* klass); // 数组类 -> 元素类
extern int         (*class_value_size)(void* klass, uint32_t* align);
extern bool        (*class_is_assignable_from)(void* klass, void* oklass);

extern void*       (*class_get_fields)(void* klass, void** iter);
extern const char* (*field_get_name)(void* field);
extern int         (*field_get_offset)(void* field);
extern void*       (*field_get_type)(void* field);
extern uint32_t    (*field_get_flags)(void* field);

extern void*       (*class_get_methods)(void* klass, void** iter);
extern const char* (*method_get_name)(void* method);
extern int         (*method_get_param_count)(void* method);

extern void*       (*class_get_field_from_name)(void* klass, const char* name);
extern void*       (*class_get_method_from_name)(void* klass, const char* name, int argc);
extern void*       (*class_get_static_field_data)(void* klass);

// ---- 调用 ----
extern void*       (*runtime_invoke)(void* method, void* obj, void** params, void** exc);
extern void*       (*runtime_object_unbox)(void* obj);   // il2cpp_object_unbox
extern char*       (*type_get_name)(void* type);
extern void        (*type_free)(void* ptr);              // il2cpp_free
extern void*       (*thread_attach)(void* domain);
extern void        (*thread_detach)(void* thread);

// ---- 辅助（自己实现） ----
// 在所有程序集里按 命名空间+类名 找类（带缓存）
void* find_class(const char* ns, const char* name);
// 字段偏移（带缓存）；失败返回 -1
int   field_off(void* klass, const char* name);
// 字段的实例化类型对应的类（泛型字段用；失败返回 nullptr）
void* field_type_class(void* klass, const char* name);
// il2cpp 数组辅助：读长度；SZArray 数据基址 = 对象+0x20
inline uint32_t array_len(uint64_t arrObj) {
    uint64_t v = 0;
    if (!roh::safe_read(arrObj + 0x20 - 8, v)) return 0; // length 紧贴数据前
    return (uint32_t)v;
}
inline uint64_t array_data(uint64_t arrObj) { return arrObj + 0x20; }

bool init();
extern uint64_t g_gameassembly_base;
extern void* g_domain;

} // namespace il2cpp
