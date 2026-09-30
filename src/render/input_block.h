#pragma once
#include "framework.h"

namespace sibalhook {

// 菜单控件的按键查询（热键捕获控件用）
class InputBlock {
public:
    static bool QueryKeyDown(int vk) {
        return (GetAsyncKeyState(vk) & 0x8000) != 0;
    }
};

} // namespace sibalhook
