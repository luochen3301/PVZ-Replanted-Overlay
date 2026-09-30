#pragma once
#include "framework.h"

namespace sibalhook {

class App {
public:
    static App& Instance();

    void RequestUnload();      // 菜单 Unload 按钮 → 卸载 DLL
    bool IsUnloading() const { return unloading_.load(); }

private:
    std::atomic<bool> unloading_{ false };
};

} // namespace sibalhook
