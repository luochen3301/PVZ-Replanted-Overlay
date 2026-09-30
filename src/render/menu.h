#pragma once
#include "framework.h"

namespace sibalhook {

// 菜单开关状态 + 热键切换 + 一键隐藏
class Menu {
public:
    static Menu& Instance();

    bool open = false;
    bool panicHidden = false;   // panic 键：隐藏全部可视化

    void Toggle() { open = !open; }
    bool IsOpen() const { return open; }

    // 每帧调用：处理菜单键/panic 键（带边沿检测）
    void PollHotkeys();

private:
    Menu() = default;
    bool lastMenu_ = false;
    bool lastPanic_ = false;
};

} // namespace sibalhook
