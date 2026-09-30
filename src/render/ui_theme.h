#pragma once

#include "imgui.h"

#include <string>

// UI theme system. The six editable colors below (Border / Background / Panel /
// Tab / Text / Control) are the single source of truth for every UI surface.
// Each widget derives its exact palette (gradients, bevels, hovers) from them
// at draw time via the palette functions in namespace sibalhook::ui.
//
// Game/ESP colors are a completely separate system (esp_color.h + MenuState
// fields persisted under [ui.visuals]) and never read from here.
//
// Derivation rules use fixed per-channel offsets tuned so that the default
// theme reproduces the previous hand-tuned constants bit-exactly; the offsets
// keep their meaning across recolors (e.g. TabInactiveTop = Tab + 0x1A lightness).
namespace sibalhook {

class Config; // config/config.h

namespace ui {

// ---- helpers ----------------------------------------------------------------
// 0xRRGGBB -> ImU32 (ImGui packs as ABGR).
constexpr ImU32 Rgb(unsigned hex, float alpha = 1.0f) {
    return (static_cast<ImU32>(alpha * 255.0f + 0.5f) << 24) | ((hex & 0xFFu) << 16) |
           (((hex >> 8) & 0xFFu) << 8) | ((hex >> 16) & 0xFFu);
}
constexpr ImU32 Black(float alpha) { return Rgb(0x000000, alpha); }
constexpr ImU32 White(float alpha) { return Rgb(0xFFFFFF, alpha); }

// ---- window chassis ---------------------------------------------------------
// .aimware-window: 796x684, padding 27px top / 5px sides+bottom.
constexpr float kWindowW = 796.0f;
constexpr float kWindowH = 684.0f;
constexpr float kTitleBarH = 27.0f;
// L/R/B to the inner 1px black border:
// 1px outer dark + 1px chassis red + 3px glass + 1px inner red.
// Tab/body start 1px inside that black, then 2px window-body inset.
constexpr float kBezelGlass = 3.0f;
constexpr float kInnerInset = 1.0f + 1.0f + kBezelGlass + 1.0f;
constexpr float kBodyBevel = 2.0f;
constexpr float kWindowRounding = 4.0f;
constexpr float kOuterNeonRounding = kWindowRounding - 1.0f;
constexpr float kInnerRounding = 1.0f;

// ---- tab bar ----------------------------------------------------------------
// Icon-above-text tab layout: 16px icon, 4px gap, text line.
constexpr float kTabBarH = 54.0f;
constexpr float kTabIconR = 8.0f;
constexpr float kTabIconTextGap = 4.0f;

// ---- window body ------------------------------------------------------------
constexpr float kPanelPad = 15.0f;
constexpr float kColumnGap = 16.0f;

// ---- groupbox ---------------------------------------------------------------
constexpr float kGroupPadX = 8.0f;
constexpr float kGroupPadTop = 5.0f;
constexpr float kGroupPadBot = 7.0f;
constexpr float kGroupSpacing = 7.0f;
constexpr float kGroupLegendLead = 12.0f;
constexpr float kGroupLegendPad = 4.0f;

// ---- rows / widgets ---------------------------------------------------------
constexpr float kRowH = 22.0f;
constexpr float kRowGap = 2.0f;
constexpr float kWidgetW = 166.0f;
constexpr float kHotkeyW = 65.0f;
constexpr float kCheckSize = 16.0f;
constexpr float kSliderTrackH = 8.0f;
constexpr float kSliderThumbW = 8.0f;
constexpr float kSliderThumbH = 12.0f;
constexpr float kFieldH = 19.0f;
constexpr float kMenuRounding = 2.0f;
constexpr float kMenuShadowOffY = 2.0f;
constexpr float kMenuShadowBlur = 6.0f;
constexpr float kMenuShadowAlpha = 0.18f;
constexpr float kComboSplitW = 18.0f;
constexpr float kSubTabH = 23.0f;
constexpr float kWeaponBarH = 42.0f;
constexpr float kButtonH = 22.0f;
constexpr float kScrollbarW = 12.0f;

// ---- editable theme ---------------------------------------------------------
struct UITheme {
    unsigned Border;     // window chassis frame, derived groupbox/focus border grays
    unsigned Background; // page body behind the groupboxes
    unsigned Panel;      // groupbox fill (and the neutral widget surfaces inside)
    unsigned Tab;        // tab bar background + tab gradients
    unsigned Text;       // labels; dark-surface text auto-derives as the complement
    unsigned Control;    // accent: checkmark, slider fill, tab underline/highlights
};

constexpr int kThemeSlotCount = 6;
// Defaults = the exact look this menu shipped with.
constexpr unsigned kThemeDefaults[kThemeSlotCount] = {
    0xD41313, // Border
    0xDEDEDE, // Background
    0xE0E0E0, // Panel
    0x282828, // Tab
    0x111111, // Text
    0xCD0000, // Control
};
const char* ThemeSlotName(int slot);          // 英文名（ini 键 + 控件 ID 用，勿翻译）
const char* ThemeSlotDisplayName(int slot);   // 界面显示名（跟随语言）
unsigned ThemeSlotColor(const UITheme& t, int slot);
unsigned& ThemeSlotRef(UITheme& t, int slot);

// Live theme. Load happens before the DX11 hook installs; every later access is
// on the render thread, so no lock (same policy as MenuState).
class ThemeManager {
public:
    static ThemeManager& Instance();

    const UITheme& theme() const { return cur_; }
    float* hsv(int slot) { return hsv_[slot]; } // editor cache: h/s/v in 0..1
    void CommitHSV(int slot);                   // hsv_[slot] -> color, mark dirty
    void ResetToDefaults();
    void LoadFromConfig(Config& cfg);           // section [ui.theme]
    void SaveToConfig(Config& cfg) const;       // writes keys, clears dirty
    void TickAutoSave();                        // debounced full config flush

private:
    ThemeManager();
    void ApplyDefaults();
    UITheme cur_{};
    float hsv_[kThemeSlotCount][3]{};
    mutable bool dirty_ = false;
    double lastEdit_ = 0.0;
};

const UITheme& ActiveTheme(); // fast path for the palette functions below

// ---- color math (0xRRGGBB) --------------------------------------------------
inline unsigned ShiftC(unsigned rgb, int dr, int dg, int db) {
    int r = static_cast<int>((rgb >> 16) & 0xFF) + dr;
    int g = static_cast<int>((rgb >> 8) & 0xFF) + dg;
    int b = static_cast<int>(rgb & 0xFF) + db;
    r = r < 0 ? 0 : r > 255 ? 255 : r;
    g = g < 0 ? 0 : g > 255 ? 255 : g;
    b = b < 0 ? 0 : b > 255 ? 255 : b;
    return (static_cast<unsigned>(r) << 16) | (static_cast<unsigned>(g) << 8) |
           static_cast<unsigned>(b);
}
inline unsigned MixC(unsigned a, unsigned b, float t) {
    const float u = 1.0f - t;
    const int r = static_cast<int>((((a >> 16) & 0xFF) * u + ((b >> 16) & 0xFF) * t) + 0.5f);
    const int g = static_cast<int>((((a >> 8) & 0xFF) * u + ((b >> 8) & 0xFF) * t) + 0.5f);
    const int bl = static_cast<int>(((a & 0xFF) * u + (b & 0xFF) * t) + 0.5f);
    return (static_cast<unsigned>(r) << 16) | (static_cast<unsigned>(g) << 8) |
           static_cast<unsigned>(bl);
}
inline float LumC(unsigned rgb) {
    return (0.299f * static_cast<float>((rgb >> 16) & 0xFF) +
            0.587f * static_cast<float>((rgb >> 8) & 0xFF) +
            0.114f * static_cast<float>(rgb & 0xFF)) /
           255.0f;
}
inline unsigned GrayFromLum(float l, float a, float b) {
    float g = a + b * l;
    g = g < 0.0f ? 0.0f : g > 1.0f ? 1.0f : g;
    const unsigned v = static_cast<unsigned>(g * 255.0f + 0.5f);
    return (v << 16) | (v << 8) | v;
}

// HSV helpers for the theme editor.
ImU32 HsvToU32(float h, float s, float v);

// ---- derived palette (all UI drawing reads through these) -------------------

// Text. Light surfaces take Text directly; dark surfaces (tab bar, chassis
// title, subtabs) use the per-channel complement so a dark default Text reads
// white there, and both always move together when the theme changes.
inline ImU32 TextCol() { return Rgb(ActiveTheme().Text); }
inline ImU32 InvTextCol() { return Rgb(0xFFFFFFu - ActiveTheme().Text); }
inline ImU32 TabTextCol() { return InvTextCol(); }
inline ImU32 TitleTextCol() { return InvTextCol(); }
inline ImU32 SubTextCol() {
    const unsigned c = 0xFFFFFFu - ActiveTheme().Text;
    return Rgb(ShiftC(c, -0x22, -0x22, -0x22));
}
inline ImU32 SubTextActiveCol() {
    const unsigned c = 0xFFFFFFu - ActiveTheme().Text;
    return Rgb(ShiftC(c, 0x11, 0x11, 0x11));
}
inline ImU32 AboutTextCol() {
    return Rgb(ShiftC(ActiveTheme().Text, 0x22, 0x22, 0x22));
}

// Border.
inline ImU32 ChassisNeon() { return Rgb(ActiveTheme().Border); }
inline ImU32 ChassisBody() { return Rgb(MixC(ActiveTheme().Border, 0x000000, 0.32f), 0.6f); }
inline ImU32 ChassisOuterDark() { return Rgb(MixC(ActiveTheme().Border, 0x000000, 0.85f)); }
inline ImU32 InnerDark() { return Rgb(MixC(ActiveTheme().Border, 0x000000, 0.88f)); }
// Groupbox/focus borders stay neutral gray; default red maps to #B4B4B4 / #666666.
inline ImU32 GroupBorder() {
    return Rgb(GrayFromLum(LumC(ActiveTheme().Border), 0.488f, 0.727f));
}
inline ImU32 FieldBorderHover() {
    return Rgb(GrayFromLum(LumC(ActiveTheme().Border), 0.10f, 1.0f));
}

// Background (page body).
inline ImU32 BodyTop() { return Rgb(ShiftC(ActiveTheme().Background, 0x10, 0x10, 0x10)); }
inline ImU32 BodyMid() { return Rgb(ActiveTheme().Background); }
inline ImU32 BodyBot() { return Rgb(ShiftC(ActiveTheme().Background, -0x0A, -0x0A, -0x0A)); }
inline ImU32 ScrollTrack() {
    return Rgb(ShiftC(ActiveTheme().Background, -0x18, -0x18, -0x18));
}
inline ImU32 ScrollTrackBorder() {
    return Rgb(ShiftC(ActiveTheme().Background, -0x42, -0x42, -0x42));
}
inline ImU32 ScrollThumbL() {
    return Rgb(ShiftC(ActiveTheme().Background, 0x21, 0x21, 0x21));
}
inline ImU32 ScrollThumbM() { return Rgb(ActiveTheme().Background); }
inline ImU32 ScrollThumbR() {
    return Rgb(ShiftC(ActiveTheme().Background, -0x1A, -0x1A, -0x1A));
}
inline ImU32 ScrollThumbBorder() {
    return Rgb(ShiftC(ActiveTheme().Background, -0x73, -0x73, -0x73));
}
inline ImU32 ScrollThumbDark() {
    return Rgb(ShiftC(ActiveTheme().Background, -0x94, -0x94, -0x94));
}

// Panel (groupbox fill + neutral widget surfaces inside).
inline ImU32 GroupTop() { return Rgb(ShiftC(ActiveTheme().Panel, -0x0B, -0x0B, -0x0B)); }
inline ImU32 GroupBot() { return Rgb(ShiftC(ActiveTheme().Panel, 0x0C, 0x0C, 0x0C)); }
inline ImU32 CheckTop() { return Rgb(ShiftC(ActiveTheme().Panel, 0x1F, 0x1F, 0x1F)); }
inline ImU32 CheckMid() { return Rgb(ShiftC(ActiveTheme().Panel, 0x14, 0x14, 0x14)); }
inline ImU32 CheckMid2() { return Rgb(ShiftC(ActiveTheme().Panel, 0x0C, 0x0C, 0x0C)); }
inline ImU32 CheckBot() { return Rgb(ShiftC(ActiveTheme().Panel, -0x08, -0x08, -0x08)); }
inline ImU32 CheckBorder() { return Rgb(ShiftC(ActiveTheme().Panel, -0x3A, -0x3A, -0x3A)); }
inline ImU32 CheckBorderTop() { return Rgb(ShiftC(ActiveTheme().Panel, -0x22, -0x22, -0x22)); }
inline ImU32 CheckBorderBot() { return Rgb(ShiftC(ActiveTheme().Panel, -0x8A, -0x8A, -0x8A)); }
inline ImU32 CheckBorderHover() {
    return Rgb(ShiftC(ActiveTheme().Panel, -0x58, -0x58, -0x58));
}
inline ImU32 CheckBorderHoverBot() {
    return Rgb(ShiftC(ActiveTheme().Panel, -0x9C, -0x9C, -0x9C));
}
inline ImU32 TrackTop() { return Rgb(ShiftC(ActiveTheme().Panel, -0x0B, -0x0B, -0x0B)); }
inline ImU32 TrackQ1() { return Rgb(ShiftC(ActiveTheme().Panel, 0x0C, 0x0C, 0x0C)); }
inline ImU32 TrackMid() { return Rgb(ShiftC(ActiveTheme().Panel, 0x1F, 0x1F, 0x1F)); }
inline ImU32 TrackQ3() { return Rgb(ShiftC(ActiveTheme().Panel, 0x0D, 0x0D, 0x0D)); }
inline ImU32 TrackBot() { return Rgb(ShiftC(ActiveTheme().Panel, -0x05, -0x05, -0x05)); }
inline ImU32 TrackBorder() { return Rgb(ShiftC(ActiveTheme().Panel, -0x53, -0x53, -0x53)); }
inline ImU32 TrackBorderBot() { return Rgb(ShiftC(ActiveTheme().Panel, 0x1F, 0x1F, 0x1F)); }
inline ImU32 ThumbTop() { return Rgb(ShiftC(ActiveTheme().Panel, 0x1F, 0x1F, 0x1F)); }
inline ImU32 ThumbMid() { return Rgb(ShiftC(ActiveTheme().Panel, 0x05, 0x05, 0x05)); }
inline ImU32 ThumbLow() { return Rgb(ShiftC(ActiveTheme().Panel, -0x23, -0x23, -0x23)); }
inline ImU32 ThumbBot() { return Rgb(ShiftC(ActiveTheme().Panel, -0x66, -0x66, -0x66)); }
inline ImU32 ThumbBorder() { return Rgb(ShiftC(ActiveTheme().Panel, -0x62, -0x62, -0x62)); }
inline ImU32 ThumbBorderTop() { return Rgb(ShiftC(ActiveTheme().Panel, -0x18, -0x18, -0x18)); }
inline ImU32 ThumbBorderHover() {
    return Rgb(ShiftC(ActiveTheme().Panel, -0x8B, -0x8B, -0x8B));
}
inline ImU32 FieldTop() { return Rgb(ShiftC(ActiveTheme().Panel, 0x1F, 0x1F, 0x1F)); }
inline ImU32 FieldMid() { return Rgb(ShiftC(ActiveTheme().Panel, 0x1B, 0x1B, 0x1B)); }
inline ImU32 FieldBot() { return Rgb(ShiftC(ActiveTheme().Panel, 0x08, 0x08, 0x08)); }
inline ImU32 FieldBorder() { return Rgb(ShiftC(ActiveTheme().Panel, -0x47, -0x47, -0x47)); }
inline ImU32 ComboArrow() { return Rgb(ShiftC(ActiveTheme().Panel, -0x8B, -0x8B, -0x8B)); }
inline ImU32 ComboBtnTop() { return Rgb(ActiveTheme().Panel); }
inline ImU32 ComboBtnBot() { return Rgb(ShiftC(ActiveTheme().Panel, -0x1A, -0x1A, -0x1A)); }
inline ImU32 MenuBg() { return Rgb(ShiftC(ActiveTheme().Panel, 0x1F, 0x1F, 0x1F)); }
inline ImU32 MenuBorder() { return Rgb(ShiftC(ActiveTheme().Panel, -0x58, -0x58, -0x58)); }
inline ImU32 BtnTop() { return Rgb(ShiftC(ActiveTheme().Panel, 0x1F, 0x1F, 0x1F)); }
inline ImU32 BtnMid() { return Rgb(ShiftC(ActiveTheme().Panel, 0x0C, 0x0C, 0x0C)); }
inline ImU32 BtnBot() { return Rgb(ShiftC(ActiveTheme().Panel, -0x08, -0x08, -0x08)); }
inline ImU32 BtnBorder() { return Rgb(ShiftC(ActiveTheme().Panel, -0x52, -0x52, -0x52)); }
inline ImU32 BtnHoverMid() { return Rgb(ShiftC(ActiveTheme().Panel, 0x14, 0x14, 0x14)); }
inline ImU32 BtnHoverBot() { return Rgb(ShiftC(ActiveTheme().Panel, 0x02, 0x02, 0x02)); }

// Control (accent).
inline ImU32 CheckMark() { return Rgb(ShiftC(ActiveTheme().Control, 0x08, 0, 0)); }
inline ImU32 FillTop() { return Rgb(ShiftC(ActiveTheme().Control, 0x04, 0x8E, 0x8E)); }
inline ImU32 FillHi() { return Rgb(ShiftC(ActiveTheme().Control, -0x06, 0, 0)); }
inline ImU32 FillMid() { return Rgb(ActiveTheme().Control); }
inline ImU32 FillQ3() { return Rgb(ShiftC(ActiveTheme().Control, -0x11, 0, 0)); }
inline ImU32 FillBot() { return Rgb(ShiftC(ActiveTheme().Control, -0x2F, 0x08, 0x13)); }
inline ImU32 OptionHover() { return Rgb(MixC(ActiveTheme().Control, 0x000000, 0.55f)); }
inline ImU32 OptionHoverText() { return White(1.0f); }

// Tab bar.
inline ImU32 TabBarBg() { return Rgb(ShiftC(ActiveTheme().Tab, -0x17, -0x17, -0x17)); }
inline ImU32 TabInactiveTop() { return Rgb(ShiftC(ActiveTheme().Tab, 0x1A, 0x1A, 0x1A)); }
inline ImU32 TabInactiveM2() { return Rgb(ShiftC(ActiveTheme().Tab, 0x08, 0x08, 0x08)); }
inline ImU32 TabInactiveMid() { return Rgb(ActiveTheme().Tab); }
inline ImU32 TabInactiveM4() { return Rgb(ShiftC(ActiveTheme().Tab, -0x08, -0x08, -0x08)); }
inline ImU32 TabInactiveM5() { return Rgb(ShiftC(ActiveTheme().Tab, -0x12, -0x12, -0x12)); }
inline ImU32 TabInactiveBot() { return Rgb(ShiftC(ActiveTheme().Tab, -0x1B, -0x1B, -0x1B)); }
inline ImU32 TabHoverTop() { return Rgb(ShiftC(ActiveTheme().Tab, 0x26, 0x26, 0x26)); }
inline ImU32 TabHoverM2() { return Rgb(ShiftC(ActiveTheme().Tab, 0x10, 0x10, 0x10)); }
inline ImU32 TabHoverMid() { return Rgb(ShiftC(ActiveTheme().Tab, 0x08, 0x08, 0x08)); }
inline ImU32 TabHoverM4() { return Rgb(ShiftC(ActiveTheme().Tab, -0x02, -0x02, -0x02)); }
inline ImU32 TabHoverM5() { return Rgb(ShiftC(ActiveTheme().Tab, -0x0C, -0x0C, -0x0C)); }
inline ImU32 TabHoverBot() { return Rgb(ShiftC(ActiveTheme().Tab, -0x17, -0x17, -0x17)); }
inline ImU32 TabActiveTop() { return Rgb(ShiftC(ActiveTheme().Tab, 0x0D, 0x0D, 0x0D)); }
inline ImU32 TabActiveShadow() { return Rgb(ShiftC(ActiveTheme().Tab, -0x0E, -0x0E, -0x0E)); }
inline ImU32 TabActQ4() { return Rgb(ShiftC(ActiveTheme().Tab, -0x0C, -0x0C, -0x0C)); }
inline ImU32 TabActiveMid() { return Rgb(ShiftC(ActiveTheme().Tab, -0x06, -0x06, -0x06)); }
inline ImU32 TabActP45() { return Rgb(ShiftC(ActiveTheme().Tab, 0x01, 0x01, 0x01)); }
inline ImU32 TabActP70() { return Rgb(ShiftC(ActiveTheme().Tab, 0x09, 0x09, 0x09)); }
inline ImU32 TabActiveBot() { return Rgb(ShiftC(ActiveTheme().Tab, 0x14, 0x14, 0x14)); }
inline ImU32 TabSepLight() { return Rgb(ShiftC(ActiveTheme().Tab, 0x05, 0x05, 0x05)); }
inline ImU32 TabSepDark() { return Rgb(ShiftC(ActiveTheme().Tab, -0x1F, -0x1F, -0x1F)); }
inline ImU32 TabActSepL() { return Rgb(ShiftC(ActiveTheme().Tab, -0x10, -0x10, -0x10)); }
inline ImU32 TabBottomBorder() { return Rgb(ShiftC(ActiveTheme().Tab, -0x20, -0x20, -0x20)); }

// Sub-tab / weapon bars (shared dark segmented control).
inline ImU32 BarFrame() { return Rgb(ShiftC(ActiveTheme().Tab, -0x17, -0x17, -0x17)); }
inline ImU32 SubTabTop() { return Rgb(ShiftC(ActiveTheme().Tab, -0x04, -0x04, -0x04)); }
inline ImU32 SubTabMid() { return Rgb(ShiftC(ActiveTheme().Tab, -0x10, -0x10, -0x10)); }
inline ImU32 SubTabBot() { return Rgb(ShiftC(ActiveTheme().Tab, -0x18, -0x18, -0x18)); }
inline ImU32 SubHoverTop() { return Rgb(ShiftC(ActiveTheme().Tab, 0x06, 0x06, 0x06)); }
inline ImU32 SubHoverMid() { return Rgb(ShiftC(ActiveTheme().Tab, -0x08, -0x08, -0x08)); }
inline ImU32 SubHoverBot() { return Rgb(ShiftC(ActiveTheme().Tab, -0x12, -0x12, -0x12)); }
inline ImU32 SubSepL() { return Rgb(ShiftC(ActiveTheme().Tab, 0x07, 0x07, 0x07)); }
inline ImU32 SubSepR() { return Rgb(ShiftC(ActiveTheme().Tab, -0x1B, -0x1B, -0x1B)); }
inline ImU32 SubTabActiveTop() { return Rgb(ShiftC(ActiveTheme().Tab, -0x13, -0x13, -0x13)); }
inline ImU32 SubTabActiveMid() { return Rgb(ShiftC(ActiveTheme().Tab, -0x08, -0x08, -0x08)); }
inline ImU32 SubTabActiveBot() { return Rgb(ShiftC(ActiveTheme().Tab, 0x04, 0x04, 0x04)); }
inline ImU32 WeaponTop() { return Rgb(ShiftC(ActiveTheme().Tab, 0x08, 0x08, 0x08)); }
inline ImU32 WeaponMid() { return Rgb(ShiftC(ActiveTheme().Tab, -0x08, -0x08, -0x08)); }
inline ImU32 WeaponBot() { return Rgb(ShiftC(ActiveTheme().Tab, -0x16, -0x16, -0x16)); }
inline ImU32 WeaponHoverTop() { return Rgb(ShiftC(ActiveTheme().Tab, 0x10, 0x10, 0x10)); }
inline ImU32 WeaponHoverMid() { return Rgb(ShiftC(ActiveTheme().Tab, -0x02, -0x02, -0x02)); }
inline ImU32 WeaponHoverBot() { return Rgb(ShiftC(ActiveTheme().Tab, -0x11, -0x11, -0x11)); }
inline ImU32 WeaponActiveTop() { return Rgb(ShiftC(ActiveTheme().Tab, -0x14, -0x14, -0x14)); }
inline ImU32 WeaponActiveMid() { return Rgb(ShiftC(ActiveTheme().Tab, -0x0A, -0x0A, -0x0A)); }
inline ImU32 WeaponActiveBot() { return Rgb(ShiftC(ActiveTheme().Tab, 0x04, 0x04, 0x04)); }
inline ImU32 WeaponSepL() { return Rgb(ShiftC(ActiveTheme().Tab, 0x14, 0x14, 0x14)); }
inline ImU32 WeaponSepR() { return Rgb(ShiftC(ActiveTheme().Tab, -0x1F, -0x1F, -0x1F)); }

} // namespace ui
} // namespace sibalhook
