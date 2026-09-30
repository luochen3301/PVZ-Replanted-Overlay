#include "framework.h"
#include "render/menu.h"
#include "render/menu_state.h"
#include "config/hotkeys.h"

namespace sibalhook {

Menu& Menu::Instance() {
    static Menu inst;
    return inst;
}

void Menu::PollHotkeys() {
    auto& s = MenuState::Instance();

    int vk = 0;
    if (Hotkeys::ParseVk(s.menuToggleKey, vk) && vk > 0) {
        bool down = (GetAsyncKeyState(vk) & 0x8000) != 0;
        if (down && !lastMenu_) Toggle();
        lastMenu_ = down;
    }
    int pvk = 0;
    if (Hotkeys::ParseVk(s.panicKey, pvk) && pvk > 0) {
        bool down = (GetAsyncKeyState(pvk) & 0x8000) != 0;
        if (down && !lastPanic_) {
            panicHidden = !panicHidden;
            if (panicHidden) open = false;
        }
        lastPanic_ = down;
    }
}

} // namespace sibalhook
