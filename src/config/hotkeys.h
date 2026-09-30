#pragma once
#include <string>

namespace sibalhook {

class Hotkeys {
public:
    // VK -> 显示名（"Insert"、"mouse1"、"escape" ...）
    static std::string VkName(int vk);
    // 名字 -> VK；失败返回 false
    static bool ParseVk(const std::string& name, int& vk);
};

} // namespace sibalhook
