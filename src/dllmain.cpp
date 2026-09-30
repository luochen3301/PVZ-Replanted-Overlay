#include "framework.h"
#include "il2cpp_api.h"
#include "game_data.h"
#include "dx11_hook.h"
#include "app/app.h"
#include "config/config.h"
#include "render/menu_state.h"
#include "render/menu.h"

namespace sibalhook {
void MenuUiDraw();   // unused shim
}

static HMODULE g_self = nullptr;

static DWORD WINAPI init_thread(void*) {
    roh::log("=== ROH v0.5 loaded, pid=%u ===", GetCurrentProcessId());

    roh::log("[build] ESP geometry, colors, FreeType TrueType interpreter v35");
    if (!il2cpp::init()) {
        roh::log("=== il2cpp init FAILED, abort ===");
        return 0;
    }
    if (!game::init()) {
        roh::log("=== game init partial/failed, continue with what we have ===");
    }
    sibalhook::Config::Instance().Load();
    sibalhook::MenuState::Instance().LoadFromConfig();

    // 调试/截图辅助：标记文件存在时注入后自动打开菜单（绕过键盘合成输入不可达的问题）
    if (GetFileAttributesA("C:\\roh_autoopen") != INVALID_FILE_ATTRIBUTES) {
        sibalhook::Menu::Instance().open = true;
        roh::log("[menu] auto-open via marker file");
    }

    if (!dx11::install()) {
        roh::log("=== dx11 hook failed ===");
        return 0;
    }
    game::start();

    // 等待卸载请求
    while (!sibalhook::App::Instance().IsUnloading())
        Sleep(100);

    roh::log("[roh] unloading...");
    game::stop();
    Sleep(300);                       // 让渲染线程退出 Present
    dx11::shutdown();
    roh::log("=== ROH unloaded ===");
    Sleep(100);
    FreeLibraryAndExitThread(g_self, 0);
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_self = hMod;
        DisableThreadLibraryCalls(hMod);
        CreateThread(nullptr, 0, init_thread, nullptr, 0, nullptr);
    }
    return TRUE;
}
