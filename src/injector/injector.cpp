// Lawnbox 注入器
// - 无参数：自动选注入器所在目录下最新的 Lawnbox*.dll（按修改时间）
// - 游戏没开时自动等待进程出现（最多 10 分钟，Ctrl+C 取消）
// - 目标进程已加载过 Lawnbox*.dll 时跳过，防止双注入导致双重 hook
// - 按子串匹配进程名，默认 replanted
// 用法: injector.exe [dll路径] [-p 进程名子串] [-nowait]
#include <windows.h>
#include <tlhelp32.h>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <conio.h>

static void str_lower(char* s) {
    for (; *s; ++s) *s = (char)tolower((unsigned char)*s);
}

// 进程里是否已加载 Lawnbox*.dll（防双注入）
static bool find_roh_module(DWORD pid, char* out, size_t n) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snap == INVALID_HANDLE_VALUE) return false;
    MODULEENTRY32W me = { sizeof(me) };
    bool found = false;
    if (Module32FirstW(snap, &me)) {
        do {
            char name[MAX_PATH] = {0};
            WideCharToMultiByte(CP_UTF8, 0, me.szModule, -1, name, sizeof(name), 0, 0);
            str_lower(name);
            if (strncmp(name, "lawnbox", 7) == 0 && strstr(name, ".dll")) {
                strncpy(out, name, n - 1);
                out[n - 1] = 0;
                found = true;
                break;
            }
        } while (Module32NextW(snap, &me));
    }
    CloseHandle(snap);
    return found;
}

// 按进程名子串收集进程（不区分大小写），返回数量
static int find_all_processes_sub(const char* nameSub, DWORD* out, int max) {
    char want[64] = {0};
    strncpy(want, nameSub, sizeof(want) - 1);
    str_lower(want);
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snap == INVALID_HANDLE_VALUE) return 0;
    PROCESSENTRY32W pe = { sizeof(pe) };
    int n = 0;
    if (Process32FirstW(snap, &pe)) {
        do {
            char name[MAX_PATH] = {0};
            WideCharToMultiByte(CP_UTF8, 0, pe.szExeFile, -1, name, sizeof(name), 0, 0);
            str_lower(name);
            if (strstr(name, want) && n < max) out[n++] = pe.th32ProcessID;
        } while (Process32NextW(snap, &pe));
    }
    CloseHandle(snap);
    return n;
}

// 挑注入目标：多个同名进程时优先选没加载过 Lawnbox 的（旧实例已注入就跳过）。
// 静默（轮询时反复调用不打印）；anyFound = 是否存在匹配进程（全已注入时为 true）
static DWORD pick_target(const char* procName, bool* anyFound) {
    DWORD pids[8];
    const int n = find_all_processes_sub(procName, pids, 8);
    *anyFound = n > 0;
    for (int i = 0; i < n; ++i) {
        char loaded[64] = "";
        if (!find_roh_module(pids[i], loaded, sizeof(loaded))) return pids[i];
    }
    return 0;
}

// 注入器所在目录里修改时间最新的 Lawnbox*.dll
static bool newest_roh_dll(char* out, size_t n) {
    char exePath[MAX_PATH];
    if (!GetModuleFileNameA(nullptr, exePath, MAX_PATH)) return false;
    char* slash = strrchr(exePath, '\\');
    if (!slash) return false;
    *slash = 0;
    char pattern[MAX_PATH];
    _snprintf(pattern, sizeof(pattern), "%s\\Lawnbox*.dll", exePath);
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA(pattern, &fd);
    if (h == INVALID_HANDLE_VALUE) return false;
    FILETIME best = {};
    char bestName[MAX_PATH] = "";
    SYSTEMTIME bestSt = {};
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (bestName[0] == 0 || CompareFileTime(&fd.ftLastWriteTime, &best) > 0) {
            best = fd.ftLastWriteTime;
            strncpy(bestName, fd.cFileName, sizeof(bestName) - 1);
            bestName[sizeof(bestName) - 1] = 0;
            FILETIME localFt = fd.ftLastWriteTime;
            FileTimeToLocalFileTime(&localFt, &localFt);
            FileTimeToSystemTime(&localFt, &bestSt);
        }
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    if (!bestName[0]) return false;
    _snprintf(out, n, "%s\\%s", exePath, bestName);
    printf("[*] dll: %s (newest %02u:%02u)\n", out, bestSt.wHour, bestSt.wMinute);
    return true;
}

static int run(int argc, char** argv);

int main(int argc, char** argv) {
    // 双击运行也能看清结果：结束时暂停等按键（-nopause 跳过，供脚本/自动化用）
    bool pause = true;
    for (int i = 1; i < argc; ++i)
        if (strcmp(argv[i], "-nopause") == 0) pause = false;
    const int rc = run(argc, argv);
    if (pause) {
        printf("[*] press any key to close...\n");
        _getch();
    }
    return rc;
}

static int run(int argc, char** argv) {
    char dllPath[MAX_PATH] = "";
    const char* procName = "replanted";
    bool wait = true;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            procName = argv[++i];
        } else if (strcmp(argv[i], "-nowait") == 0) {
            wait = false;
        } else if (argv[i][0] != '-') {
            GetFullPathNameA(argv[i], MAX_PATH, dllPath, nullptr);
        }
    }
    if (!dllPath[0]) {
        char newest[MAX_PATH];
        if (newest_roh_dll(newest, sizeof(newest)))
            strncpy(dllPath, newest, sizeof(dllPath) - 1);
        else
            GetFullPathNameA("Lawnbox.dll", MAX_PATH, dllPath, nullptr);
        dllPath[sizeof(dllPath) - 1] = 0;
    } else {
        printf("[*] dll: %s\n", dllPath);
    }
    if (GetFileAttributesA(dllPath) == INVALID_FILE_ATTRIBUTES) {
        printf("[!] DLL not found: %s\n", dllPath);
        return 1;
    }

    // 找进程；没开就等（最多 10 分钟）。多个同名进程时挑没注入过的那个
    DWORD pid = 0;
    const ULONGLONG start = GetTickCount64();
    bool announced = false;
    for (;;) {
        bool anyFound = false;
        pid = pick_target(procName, &anyFound);
        if (pid) break;
        if (!wait) {
            if (anyFound)
                printf("[!] all matching '%s' processes already injected\n", procName);
            else
                printf("[!] no process matching '%s'\n", procName);
            return 1;
        }
        if (!announced) {
            printf("[*] waiting for an uninjected '%s' process (up to 10 min, Ctrl+C to cancel)...\n",
                   procName);
            announced = true;
        }
        if (GetTickCount64() - start > 600000) {
            printf("[!] timed out waiting for '%s'\n", procName);
            return 1;
        }
        Sleep(500);
    }
    printf("[*] target pid: %lu\n", pid);

    HANDLE h = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    if (!h) { printf("[!] OpenProcess failed: %lu\n", GetLastError()); return 1; }

    SIZE_T len = strlen(dllPath) + 1;
    void* remote = VirtualAllocEx(h, nullptr, len, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!remote) { printf("[!] VirtualAllocEx failed\n"); CloseHandle(h); return 1; }
    WriteProcessMemory(h, remote, dllPath, len, nullptr);

    HANDLE t = CreateRemoteThread(h, nullptr, 0,
        (LPTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandleA("kernel32.dll"), "LoadLibraryA"),
        remote, 0, nullptr);
    if (!t) { printf("[!] CreateRemoteThread failed: %lu\n", GetLastError()); CloseHandle(h); return 1; }
    WaitForSingleObject(t, 8000);
    DWORD exitCode = 0;
    GetExitCodeThread(t, &exitCode);
    CloseHandle(t);
    VirtualFreeEx(h, remote, 0, MEM_RELEASE);
    CloseHandle(h);
    if (exitCode == 0) {
        printf("[!] LoadLibraryA returned NULL in target (AV blocked or missing deps)\n");
        return 1;
    }
    printf("[+] injected: %s\n", dllPath);
    return 0;
}
