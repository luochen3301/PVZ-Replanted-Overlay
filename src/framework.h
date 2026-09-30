#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "paths.h"
#include <windows.h>

#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>

namespace roh {
    inline void log_raw(const wchar_t* path, const char* fmt, va_list ap) {
        FILE* f = _wfopen(path, L"a");
        if (!f) return;
        vfprintf(f, fmt, ap);
        fputc('\n', f);
        fclose(f);
    }
    inline void log(const char* fmt, ...) {
        va_list ap; va_start(ap, fmt);
        log_raw(ROH_LOG_PATH, fmt, ap);
        va_end(ap);
    }
    // SEH 保护的内存读取（游戏对象指针不可靠时用）。
    // 必须 noinline：__try 不能出现在带析构对象的函数里（C2712）。
    template <typename T>
    __declspec(noinline) inline bool safe_read(uint64_t addr, T& out) {
        __try {
            out = *(T*)addr;
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }
    __declspec(noinline) inline uint64_t safe_read_ptr(uint64_t addr) {
        uint64_t v = 0;
        return safe_read(addr, v) ? v : 0;
    }
    __declspec(noinline) inline bool safe_write(uint64_t addr, const void* data, size_t len) {
        __try {
            memcpy((void*)addr, data, len);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }
    __declspec(noinline) inline bool safe_readbuf(uint64_t addr, void* dst, size_t len) {
        __try {
            memcpy(dst, (const void*)addr, len);
            return true;
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            return false;
        }
    }
}
