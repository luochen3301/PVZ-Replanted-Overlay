#pragma once

#include "imgui.h"

#include <string>
#include <vector>

// Custom-drawn widgets replicating the design mock. ImGui's stock widgets cannot
// express these multi-stop gradients / bevels, so each is drawn via ImDrawList.
namespace sibalhook::ui {

// Multi-stop vertical gradient inside a rect (stops are 0..1 positions).
// One quad per adjacent pair (AddRectFilledMultiColor), matching CSS linear-gradient.
struct GradStop {
    float pos;
    ImU32 color;
};
void VertGradient(ImDrawList* dl, const ImVec2& a, const ImVec2& b, const GradStop* stops,
                  int count, float rounding = 0.0f, ImDrawFlags flags = 0);
// Same, but confined to a rounded-rect silhouette so the corners stay unpainted
// (a rectangular clip would square off the corners of a rounded widget).
void VertGradientRounded(ImDrawList* dl, const ImVec2& a, const ImVec2& b, const GradStop* stops,
                         int count, float rounding);
void HorzGradient(ImDrawList* dl, const ImVec2& a, const ImVec2& b, const GradStop* stops,
                  int count, float rounding = 0.0f);

// Explicit layout cursor. ImGui's own cursor/indent/WorkRect rules fight the
// absolute positioning this design needs, so the widget layer owns its cursor:
// SetCursor fixes the column origin + width, and rows advance Y themselves.
void SetCursor(float x, float y, float width);
float CursorX();
float CursorY();
float CursorWidth();
void AdvanceY(float dy);

// Layout: one control row -> returns the row rect and places the label.
struct RowLayout {
    ImVec2 rowMin;
    ImVec2 rowMax;
    ImVec2 widgetMin; // left edge of the 166px widget column
};
RowLayout BeginRow(const char* label, float rowHeight);

// Widgets. Each returns true when the value changed.
bool Checkbox(const char* id, bool* value);
bool SliderFloat(const char* id, float* value, float vMin, float vMax, const char* format);
bool SliderInt(const char* id, int* value, int vMin, int vMax, const char* format);
bool Combo(const char* id, int* current, const char* const* items, int count);
bool InputText(const char* id, std::string* text);
bool Hotkey(const char* id, std::string* text);
bool Button(const char* label, float width);

// Bars.
// Monochrome vector glyphs for the main tab bar, drawn with ImDrawList strokes
// (same approach as the weapon-bar icons; no icon font/library).
enum TabIcon {
    kIconEye = 0,
    kIconSliders,
    kIconGlobe,
    kIconGear,
    kIconPalette,
    kIconBolt,
};
// Draws one glyph centered at `center` inside a ~2*radius box.
void DrawTabIcon(ImDrawList* dl, int icon, const ImVec2& center, float radius, ImU32 col);
// When `icons` is non-null each tab draws glyph-above-label and a 2px accent
// underline on the active tab ( UITheme.Control ).
bool TabBar(const char* const* labels, int count, int* current, float width, float height,
            const int* icons = nullptr);
bool SubTabBar(const char* const* labels, int count, int* current, float width);
bool WeaponBar(const char* const* labels, int count, int* current, float width);

// Groupbox: reserves the legend line and indents content by the CSS padding.
// The frame is drawn on EndGroup, once the content height is known.
void BeginGroup(const char* title);
void EndGroup();

// Splits the draw list so groupbox frames land behind their content.
// Every panel that uses BeginGroup must be wrapped in these.
void BeginPanel();
void EndPanel();

// Convenience row wrappers (label on the left, widget in the 166px column).
bool RowCheckbox(const char* label, bool* value);
bool RowSlider(const char* label, float* value, float vMin, float vMax, const char* format);
bool RowSliderInt(const char* label, int* value, int vMin, int vMax, const char* format);
bool RowCombo(const char* label, int* current, const char* const* items, int count);
bool RowColor(const char* label, float color[4]);
bool RowInput(const char* label, std::string* text);
bool RowHotkey(const char* label, std::string* text);

// Theme editor primitives. Hue/sat field: x = hue, y = saturation (full at the
// top, white at the bottom) with a draggable cursor dot; brightness slider is a
// vertical black->HSV(h,s,1) gradient with a thumb. Both mutate the floats and
// return true when dragged/clicked.
bool HueSatField(const char* id, float* h, float* s, const ImVec2& size);
bool BrightnessSliderV(const char* id, float* v, float h, float s, const ImVec2& size);

// Deferred dropdown popups are drawn last so they overlay following rows.
void FlushPopups();

} // namespace sibalhook::ui
