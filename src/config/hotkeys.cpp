#include "framework.h"
#include "config/hotkeys.h"
#include <cctype>

namespace sibalhook {

struct KeyName { int vk; const char* name; };
static const KeyName kNames[] = {
    {VK_LBUTTON, "mouse1"}, {VK_RBUTTON, "mouse2"}, {VK_MBUTTON, "mouse3"},
    {VK_XBUTTON1, "mouse4"}, {VK_XBUTTON2, "mouse5"},
    {VK_BACK, "backspace"}, {VK_TAB, "tab"}, {VK_RETURN, "enter"},
    {VK_SHIFT, "shift"}, {VK_CONTROL, "ctrl"}, {VK_MENU, "alt"}, {VK_CAPITAL, "capslock"},
    {VK_ESCAPE, "escape"}, {VK_SPACE, "space"}, {VK_PRIOR, "pageup"}, {VK_NEXT, "pagedown"},
    {VK_END, "end"}, {VK_HOME, "home"}, {VK_LEFT, "left"}, {VK_UP, "up"},
    {VK_RIGHT, "right"}, {VK_DOWN, "down"}, {VK_SNAPSHOT, "printscreen"},
    {VK_INSERT, "insert"}, {VK_DELETE, "delete"},
    {VK_NUMLOCK, "numlock"}, {VK_SCROLL, "scrolllock"}, {VK_PAUSE, "pause"},
    {0x30, "0"}, {0x31, "1"}, {0x32, "2"}, {0x33, "3"}, {0x34, "4"},
    {0x35, "5"}, {0x36, "6"}, {0x37, "7"}, {0x38, "8"}, {0x39, "9"},
};

static std::string tolower_s(std::string s) {
    for (auto& c : s) c = (char)tolower((unsigned char)c);
    return s;
}

std::string Hotkeys::VkName(int vk) {
    char one[2] = { (char)vk, 0 };
    if (vk >= 'A' && vk <= 'Z') return tolower_s(one);
    if (vk >= '0' && vk <= '9') return std::string(one);
    if (vk >= VK_F1 && vk <= VK_F24) return "f" + std::to_string(vk - VK_F1 + 1);
    for (auto& k : kNames)
        if (k.vk == vk) return k.name;
    return "vk" + std::to_string(vk);
}

bool Hotkeys::ParseVk(const std::string& nameIn, int& vk) {
    std::string name = tolower_s(nameIn);
    if (name.empty() || name == "none") { vk = 0; return false; }
    if (name.size() == 1) {
        char c = name[0];
        if (c >= 'a' && c <= 'z') { vk = toupper(c); return true; }
        if (c >= '0' && c <= '9') { vk = c; return true; }
    }
    if (name.size() > 1 && name[0] == 'f') {
        int n = atoi(name.c_str() + 1);
        if (n >= 1 && n <= 24) { vk = VK_F1 + n - 1; return true; }
    }
    for (auto& k : kNames)
        if (name == k.name) { vk = k.vk; return true; }
    if (name.rfind("vk", 0) == 0) { vk = atoi(name.c_str() + 2); return vk > 0; }
    return false;
}

} // namespace sibalhook
