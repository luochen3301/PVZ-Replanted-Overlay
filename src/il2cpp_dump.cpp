#include "il2cpp_api.h"
#include "paths.h"

namespace il2cpp_dump {

// 我们关心的镜像，按优先级排序；跳过 mscorlib/System（不需要且泛型类易死锁）
// 注意 image_get_name 返回带 .dll 后缀
static int image_priority(const char* name) {
    if (!name) return -1;
    if (strstr(name, "Assembly-CSharp")) return 0;
    if (strstr(name, "Spine") || strstr(name, "spine")) return 1;
    if (strstr(name, "UnityEngine.CoreModule")) return 2;
    if (strncmp(name, "UnityEngine", 11) == 0) return 3;
    return -1; // 不 dump
}

static bool hot_match(const char* ns, const char* name) {
    static const char* kw[] = {
        "Zombie", "Plant", "Lawn", "Board", "Sun", "Coin", "Money", "Gem",
        "Seed", "Packet", "Card", "Wave", "Stage", "Level", "Phase", "HugeWave",
        "Camera", "Spine", "Skeleton", "Bone", "Game", "Projectile", "Pea",
        "Fog", "Cursor", "Almanac", "Store", "Shop", "Upgrade", "Reward",
        "ZombieType", "PlantType", "TypeValue", "Definition", "Binder",
        "Reanim", "Anim", "Battle", "Spawn", "Progress", "Conveyor", "Loot", "Pickup", "Drop"
    };
    char buf[512];
    if (ns && *ns) _snprintf(buf, sizeof(buf), "%s.%s", ns, name);
    else           _snprintf(buf, sizeof(buf), "%s", name);
    for (auto k : kw) {
        if (strstr(buf, k)) return true;
        if (ns && *ns && strstr(ns, k)) return true;
    }
    return false;
}

static void dump_class_inner(FILE* f, void* klass) {
    const char* ns   = il2cpp::class_get_namespace(klass);
    const char* name = il2cpp::class_get_name(klass);
    if (!name) return;

    const char* kind = "class";
    if (il2cpp::class_is_enum && il2cpp::class_is_enum(klass)) kind = "enum";
    else if (il2cpp::class_is_valuetype && il2cpp::class_is_valuetype(klass)) kind = "struct";

    fprintf(f, "\n// %s%s%s\n", ns && *ns ? ns : "", ns && *ns ? "." : "", name);
    fprintf(f, "%s %s%s%s\n{\n", kind, ns && *ns ? ns : "", ns && *ns ? "." : "", name);

    if (il2cpp::class_get_fields && il2cpp::field_get_name && il2cpp::field_get_offset) {
        void* iter = nullptr;
        void* fi = nullptr;
        fprintf(f, "\t// Fields\n");
        while ((fi = il2cpp::class_get_fields(klass, &iter)) != nullptr) {
            const char* fname = il2cpp::field_get_name(fi);
            if (!fname) break;
            int off = il2cpp::field_get_offset(fi);
            uint32_t flg = il2cpp::field_get_flags ? il2cpp::field_get_flags(fi) : 0;
            bool isStatic = (flg & 0x10) != 0;
            bool isLiteral = (flg & 0x40) != 0;

            const char* tname = "object";
            char* alloc = nullptr;
            if (il2cpp::field_get_type && il2cpp::type_get_name) {
                alloc = il2cpp::type_get_name(il2cpp::field_get_type(fi));
                if (alloc) tname = alloc;
            }
            fprintf(f, "\t%s[0x%X] %s %s; // %s%s\n",
                    isStatic ? "static " : "",
                    off, tname, fname,
                    isLiteral ? "literal " : "",
                    isStatic ? "static" : "instance");
            if (alloc && il2cpp::type_free) il2cpp::type_free(alloc);
        }
    }

    if (il2cpp::class_get_methods && il2cpp::method_get_name) {
        void* iter = nullptr;
        void* mi = nullptr;
        fprintf(f, "\n\t// Methods\n");
        while ((mi = il2cpp::class_get_methods(klass, &iter)) != nullptr) {
            const char* mname = il2cpp::method_get_name(mi);
            if (!mname) break;
            int pc = il2cpp::method_get_param_count ? il2cpp::method_get_param_count(mi) : -1;
            char pcs[16];
            if (pc >= 0) _snprintf(pcs, sizeof(pcs), "%d", pc);
            else strcpy_s(pcs, "?");
            fprintf(f, "\t%s(%s);\n", mname, pcs);
        }
    }
    fprintf(f, "}\n");
}

static int __declspec(noinline) safe_dump_class(FILE* f, void* klass) {
    __try {
        dump_class_inner(f, klass);
        return 1;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
}

static bool __declspec(noinline) class_is_hot(void* klass) {
    __try {
        const char* ns = il2cpp::class_get_namespace(klass);
        const char* nm = il2cpp::class_get_name(klass);
        return nm && hot_match(ns, nm);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

static void __declspec(noinline) dump_image(FILE* f, FILE* fh, void* image, const char* imgName) {
    int cc = 0;
    __try {
        cc = il2cpp::image_get_class_count ? il2cpp::image_get_class_count(image) : 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        roh::log("[dump] image %s: class_count threw", imgName);
        return;
    }
    if (cc <= 0 || cc > 100000) { roh::log("[dump] image %s: bad count %d", imgName, cc); return; }
    roh::log("[dump] image %-36s begin classes=%d", imgName, cc);

    fprintf(f, "\n//==================== IMAGE: %s (classes=%d) ====================\n", imgName, cc);
    int ok = 0, bad = 0;
    for (int i = 0; i < cc; i++) {
        void* klass = nullptr;
        __try { klass = il2cpp::image_get_class(image, i); }
        __except (EXCEPTION_EXECUTE_HANDLER) { continue; }
        if (!klass) continue;
        if (safe_dump_class(f, klass)) ok++; else bad++;
        if (fh) {
            __try {
                if (class_is_hot(klass)) safe_dump_class(fh, klass);
            } __except (EXCEPTION_EXECUTE_HANDLER) {}
        }
        if ((i % 200) == 199) {
            roh::log("[dump]   %s: %d/%d (ok=%d bad=%d)", imgName, i + 1, cc, ok, bad);
            fflush(f);
            if (fh) fflush(fh);
        }
    }
    fprintf(f, "// image done: ok=%d bad=%d\n", ok, bad);
    fflush(f);
    if (fh) fflush(fh);
    roh::log("[dump] image %-36s DONE ok=%d bad=%d", imgName, ok, bad);
}

void run() {
    roh::log("[dump] begin (filtered images)");
    FILE* f  = _wfopen(ROH_DUMP_PATH, L"w");
    FILE* fh = _wfopen(ROH_DUMP_HOT,  L"w");
    if (!f || !fh) { roh::log("[dump] cannot open output files"); if (f) fclose(f); if (fh) fclose(fh); return; }

    void* domain = il2cpp::domain_get();
    if (!domain) { roh::log("[dump] domain==null"); fclose(f); fclose(fh); return; }

    if (il2cpp::thread_attach) {
        __try { il2cpp::thread_attach(domain); roh::log("[dump] thread attached"); }
        __except (EXCEPTION_EXECUTE_HANDLER) { roh::log("[dump] thread_attach threw, continue"); }
    }

    size_t count = 0;
    void** assemblies = il2cpp::domain_get_assemblies(domain, &count);
    roh::log("[dump] assemblies=%zu", count);
    if (!assemblies || !count) { fclose(f); fclose(fh); return; }

    // 收集 (priority, image, name)，按优先级处理
    struct Target { int pri; void* image; const char* name; };
    Target targets[128];
    int nT = 0;
    for (size_t a = 0; a < count && nT < 128; a++) {
        void* image = nullptr;
        const char* iname = nullptr;
        __try {
            image = il2cpp::assembly_get_image(assemblies[a]);
            if (image && il2cpp::image_get_name) iname = il2cpp::image_get_name(image);
        } __except (EXCEPTION_EXECUTE_HANDLER) { continue; }
        int pri = image_priority(iname);
        if (pri >= 0) { targets[nT].pri = pri; targets[nT].image = image; targets[nT].name = iname; nT++; }
    }
    roh::log("[dump] target images=%d", nT);
    for (int i = 0; i < nT; i++)
        for (int j = i + 1; j < nT; j++)
            if (targets[j].pri < targets[i].pri) { Target t = targets[i]; targets[i] = targets[j]; targets[j] = t; }

    for (int i = 0; i < nT; i++)
        dump_image(f, fh, targets[i].image, targets[i].name);

    fprintf(f, "\n// done, images=%d\n", nT);
    fclose(f); fclose(fh);
    roh::log("[dump] all done");
}

} // namespace il2cpp_dump
