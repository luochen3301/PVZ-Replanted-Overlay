#include "framework.h"
#include "render/ui_theme.h"
#include "render/ui_lang.h"
#include "config/config.h"

#include <cmath>

namespace sibalhook::ui {
namespace {

constexpr const char* kSection = "ui.theme";
constexpr double kAutoSaveDelay = 1.5; // seconds after the last edit

double NowSeconds() { return static_cast<double>(GetTickCount64()) * 0.001; }

std::string HexOf(unsigned rgb) {
    char buf[8]{};
    std::snprintf(buf, sizeof(buf), "#%02X%02X%02X", (rgb >> 16) & 0xFF, (rgb >> 8) & 0xFF,
                  rgb & 0xFF);
    return buf;
}

bool ParseHex(const std::string& text, unsigned& out) {
    size_t i = (!text.empty() && text[0] == '#') ? 1 : 0;
    if (text.size() - i != 6 && text.size() - i != 8) {
        return false;
    }
    unsigned v = 0;
    for (int c = 0; c < 6; ++c, ++i) {
        const char ch = text[i];
        const int d = ch >= '0' && ch <= '9'   ? ch - '0'
                      : ch >= 'a' && ch <= 'f' ? ch - 'a' + 10
                      : ch >= 'A' && ch <= 'F' ? ch - 'A' + 10
                                               : -1;
        if (d < 0) {
            return false;
        }
        v = v * 16 + static_cast<unsigned>(d);
    }
    out = v;
    return true;
}

void RgbToHsv(float r, float g, float b, float& h, float& s, float& v) {
    const float mx = std::fmaxf(r, std::fmaxf(g, b));
    const float mn = std::fminf(r, std::fminf(g, b));
    const float d = mx - mn;
    v = mx;
    s = mx > 0.0f ? d / mx : 0.0f;
    if (d <= 0.0f) {
        h = 0.0f;
        return;
    }
    if (mx == r) {
        h = (g - b) / d + (g < b ? 6.0f : 0.0f);
    } else if (mx == g) {
        h = (b - r) / d + 2.0f;
    } else {
        h = (r - g) / d + 4.0f;
    }
    h /= 6.0f;
}

void HsvToRgb(float h, float s, float v, float& r, float& g, float& b) {
    h -= std::floorf(h);
    const int i = static_cast<int>(h * 6.0f);
    const float f = h * 6.0f - static_cast<float>(i);
    const float p = v * (1.0f - s);
    const float q = v * (1.0f - s * f);
    const float t = v * (1.0f - s * (1.0f - f));
    switch (i % 6) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
    }
}

unsigned HsvToHex(float h, float s, float v) {
    float r = 0.0f, g = 0.0f, b = 0.0f;
    HsvToRgb(h, s, v, r, g, b);
    const int ri = static_cast<int>(r * 255.0f + 0.5f);
    const int gi = static_cast<int>(g * 255.0f + 0.5f);
    const int bi = static_cast<int>(b * 255.0f + 0.5f);
    return (static_cast<unsigned>(ri) << 16) | (static_cast<unsigned>(gi) << 8) |
           static_cast<unsigned>(bi);
}

} // namespace

ImU32 HsvToU32(float h, float s, float v) { return Rgb(HsvToHex(h, s, v)); }

const char* ThemeSlotName(int slot) {
    static const char* const names[kThemeSlotCount] = {
        "Borders", "Background", "Panels", "Tabs", "Text", "Controls"};
    return (slot >= 0 && slot < kThemeSlotCount) ? names[slot] : "?";
}

const char* ThemeSlotDisplayName(int slot) { return TR(ThemeSlotName(slot)); }

unsigned ThemeSlotColor(const UITheme& t, int slot) {
    const unsigned fields[kThemeSlotCount] = {
        t.Border, t.Background, t.Panel, t.Tab, t.Text, t.Control};
    return (slot >= 0 && slot < kThemeSlotCount) ? fields[slot] : 0;
}

unsigned& ThemeSlotRef(UITheme& t, int slot) {
    unsigned* fields[kThemeSlotCount] = {&t.Border, &t.Background, &t.Panel,
                                         &t.Tab,    &t.Text,       &t.Control};
    return *fields[slot];
}

const UITheme& ActiveTheme() { return ThemeManager::Instance().theme(); }

ThemeManager& ThemeManager::Instance() {
    static ThemeManager inst;
    return inst;
}

ThemeManager::ThemeManager() { ApplyDefaults(); }

void ThemeManager::ApplyDefaults() {
    for (int i = 0; i < kThemeSlotCount; ++i) {
        const unsigned hex = kThemeDefaults[i];
        ThemeSlotRef(cur_, i) = hex;
        RgbToHsv(static_cast<float>((hex >> 16) & 0xFF) / 255.0f,
                 static_cast<float>((hex >> 8) & 0xFF) / 255.0f,
                 static_cast<float>(hex & 0xFF) / 255.0f, hsv_[i][0], hsv_[i][1], hsv_[i][2]);
    }
    dirty_ = false;
}

void ThemeManager::CommitHSV(int slot) {
    if (slot < 0 || slot >= kThemeSlotCount) {
        return;
    }
    ThemeSlotRef(cur_, slot) = HsvToHex(hsv_[slot][0], hsv_[slot][1], hsv_[slot][2]);
    dirty_ = true;
    lastEdit_ = NowSeconds();
}

void ThemeManager::ResetToDefaults() {
    ApplyDefaults();
    dirty_ = true;
    lastEdit_ = NowSeconds();
}

void ThemeManager::LoadFromConfig(Config& cfg) {
    for (int i = 0; i < kThemeSlotCount; ++i) {
        const std::string def = HexOf(kThemeDefaults[i]);
        unsigned hex = 0;
        if (ParseHex(cfg.GetString(kSection, ThemeSlotName(i), def), hex)) {
            ThemeSlotRef(cur_, i) = hex;
            RgbToHsv(static_cast<float>((hex >> 16) & 0xFF) / 255.0f,
                     static_cast<float>((hex >> 8) & 0xFF) / 255.0f,
                     static_cast<float>(hex & 0xFF) / 255.0f, hsv_[i][0], hsv_[i][1],
                     hsv_[i][2]);
        }
    }
    dirty_ = false;
}

void ThemeManager::SaveToConfig(Config& cfg) const {
    for (int i = 0; i < kThemeSlotCount; ++i) {
        cfg.SetString(kSection, ThemeSlotName(i), HexOf(ThemeSlotColor(cur_, i)));
    }
    dirty_ = false;
}

void ThemeManager::TickAutoSave() {
    if (!dirty_ || NowSeconds() - lastEdit_ < kAutoSaveDelay) {
        return;
    }
    Config& cfg = Config::Instance();
    SaveToConfig(cfg);
    cfg.Save();
}

} // namespace sibalhook::ui
