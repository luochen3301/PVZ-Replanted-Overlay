#pragma once

#include <array>
#include <cmath>
#include <cstdio>
#include <string>

namespace sibalhook {

using EspColor = std::array<float, 4>; // RGBA, each channel in [0, 1]

inline int ColorByte(float value) {
    if (!std::isfinite(value)) return 0;
    if (value < 0.0f) value = 0.0f;
    if (value > 1.0f) value = 1.0f;
    return static_cast<int>(value * 255.0f + 0.5f);
}

inline std::string ColorToHex(const EspColor& color) {
    char text[10]{};
    std::snprintf(text, sizeof(text), "#%02X%02X%02X%02X",
                  ColorByte(color[0]), ColorByte(color[1]),
                  ColorByte(color[2]), ColorByte(color[3]));
    return text;
}

inline bool ColorFromHex(const std::string& text, EspColor& color) {
    const size_t start = text.size() == 9 && text[0] == '#' ? 1 : 0;
    if (text.size() - start != 8) return false;
    EspColor parsed{};
    for (int i = 0; i < 4; ++i) {
        int value = 0;
        for (int j = 0; j < 2; ++j) {
            char ch = text[start + i * 2 + j];
            int digit = ch >= '0' && ch <= '9' ? ch - '0' :
                        ch >= 'a' && ch <= 'f' ? ch - 'a' + 10 :
                        ch >= 'A' && ch <= 'F' ? ch - 'A' + 10 : -1;
            if (digit < 0) return false;
            value = value * 16 + digit;
        }
        parsed[i] = value / 255.0f;
    }
    color = parsed;
    return true;
}

} // namespace sibalhook
