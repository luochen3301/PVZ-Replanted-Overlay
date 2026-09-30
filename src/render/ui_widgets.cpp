#include "framework.h"
#include "render/ui_widgets.h"
#include "render/ui_theme.h"
#include "render/input_block.h"
#include "config/hotkeys.h"

#include "imgui.h"
#include "imgui_internal.h"

#include <algorithm>
#include <cstdio>

namespace sibalhook::ui {
namespace {

// A dropdown whose popup must be drawn after all rows (so it overlays them).
struct PendingPopup {
    ImGuiID id;
    ImVec2 pos;
    float width;
    std::vector<std::string> items;
    int* current;
    // Captured at push time: FlushPopups runs after the owning window is closed,
    // so GetStateStorage() there would resolve to the wrong window.
    ImGuiStorage* store;
    bool justOpened;
};
std::vector<PendingPopup> g_popups;

// Last frame's dropdown rects. The menu is drawn on the foreground list, so
// InvisibleButtons in the rows below still win hit-testing.
struct PopupHit {
    ImVec2 min;
    ImVec2 max;
};
std::vector<PopupHit> g_popupHits;

bool MouseOverPopup() {
    const ImVec2 m = ImGui::GetIO().MousePos;
    for (const auto& r : g_popupHits) {
        if (m.x >= r.min.x && m.x < r.max.x && m.y >= r.min.y && m.y < r.max.y) {
            return true;
        }
    }
    return false;
}

void SuppressWidgetUnderPopup() {
    if (MouseOverPopup() && ImGui::IsItemActive()) {
        ImGui::ClearActiveID();
    }
}

// Exclusive keybind capture. Polled at BeginPanel so the bind click is eaten
// before later rows see IsItemClicked.
ImGuiID g_hotkeyCapture = 0;
std::string* g_hotkeyTarget = nullptr;
bool g_hotkeyIgnoreUp = false;
bool g_hotkeyAteClick = false;

bool IsMouseBindVk(int vk) {
    switch (vk & 0xFF) {
    case VK_LBUTTON:
    case VK_RBUTTON:
    case VK_MBUTTON:
    case VK_XBUTTON1:
    case VK_XBUTTON2:
        return true;
    default:
        return false;
    }
}

int FirstKeyBindVk() {
    for (int vk = 1; vk <= 0xFE; ++vk) {
        if (IsMouseBindVk(vk)) {
            continue;
        }
        if (InputBlock::QueryKeyDown(vk)) {
            return vk;
        }
    }
    return 0;
}

int FirstMouseBindVk() {
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        return VK_LBUTTON;
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
        return VK_RBUTTON;
    }
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Middle)) {
        return VK_MBUTTON;
    }
    if (ImGui::IsMouseClicked(3)) {
        return VK_XBUTTON1;
    }
    if (ImGui::IsMouseClicked(4)) {
        return VK_XBUTTON2;
    }
    return 0;
}

bool AnyBindHeld() {
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) ||
        ImGui::IsMouseDown(ImGuiMouseButton_Right) ||
        ImGui::IsMouseDown(ImGuiMouseButton_Middle)) {
        return true;
    }
    return FirstKeyBindVk() != 0;
}

bool WidgetBlocked() { return MouseOverPopup() || g_hotkeyAteClick; }

void ClearHotkeyCapture() {
    g_hotkeyCapture = 0;
    g_hotkeyTarget = nullptr;
    g_hotkeyIgnoreUp = false;
}

void PollHotkeyCapture() {
    g_hotkeyAteClick = false;
    if (!g_hotkeyCapture || !g_hotkeyTarget) {
        return;
    }
    if (g_hotkeyIgnoreUp) {
        if (!AnyBindHeld()) {
            g_hotkeyIgnoreUp = false;
        }
        return;
    }
    if (InputBlock::QueryKeyDown(VK_ESCAPE)) {
        *g_hotkeyTarget = "none";
        g_hotkeyAteClick = true;
        ClearHotkeyCapture();
        return;
    }
    int vk = FirstKeyBindVk();
    if (vk == 0) {
        vk = FirstMouseBindVk();
    }
    if (vk == 0) {
        return;
    }
    *g_hotkeyTarget = Hotkeys::VkName(vk);
    g_hotkeyAteClick = true;
    ClearHotkeyCapture();
}

ImU32 LerpColor(ImU32 a, ImU32 b, float t) {
    const ImVec4 ca = ImGui::ColorConvertU32ToFloat4(a);
    const ImVec4 cb = ImGui::ColorConvertU32ToFloat4(b);
    return ImGui::ColorConvertFloat4ToU32(ImVec4(ca.x + (cb.x - ca.x) * t, ca.y + (cb.y - ca.y) * t,
                                                 ca.z + (cb.z - ca.z) * t,
                                                 ca.w + (cb.w - ca.w) * t));
}

ImU32 SampleStops(const GradStop* stops, int count, float t) {
    if (count <= 0) {
        return 0;
    }
    if (t <= stops[0].pos) {
        return stops[0].color;
    }
    for (int i = 1; i < count; ++i) {
        if (t <= stops[i].pos) {
            const float span = stops[i].pos - stops[i - 1].pos;
            const float k = span > 0.0f ? (t - stops[i - 1].pos) / span : 0.0f;
            return LerpColor(stops[i - 1].color, stops[i].color, k);
        }
    }
    return stops[count - 1].color;
}

void GradientBand(ImDrawList* dl, float x0, float x1, float y0, float y1, ImU32 c0, ImU32 c1) {
    if (y1 <= y0) {
        return;
    }
    if (c0 == c1) {
        dl->AddRectFilled(ImVec2(x0, y0), ImVec2(x1, y1), c0);
    } else {
        dl->AddRectFilledMultiColor(ImVec2(x0, y0), ImVec2(x1, y1), c0, c0, c1, c1);
    }
}

// Draw text clipped/aligned inside a rect.
void TextIn(ImDrawList* dl, const ImVec2& min, const ImVec2& max, const char* text, ImU32 color,
            float alignX, float padX = 0.0f) {
    const ImVec2 size = ImGui::CalcTextSize(text);
    const float x = min.x + padX + (max.x - min.x - padX * 2.0f - size.x) * alignX;
    const float y = min.y + (max.y - min.y - size.y) * 0.5f;
    dl->PushClipRect(min, max, true);
    dl->AddText(ImVec2(IM_ROUND(x), IM_ROUND(y)), color, text);
    dl->PopClipRect();
}

} // namespace

void VertGradient(ImDrawList* dl, const ImVec2& a, const ImVec2& b, const GradStop* stops,
                  int count, float rounding, ImDrawFlags flags) {
    const float h = b.y - a.y;
    const float w = b.x - a.x;
    if (h <= 0.0f || w <= 0.0f || count <= 0) {
        return;
    }
    if (rounding > 0.0f) {
        dl->PushClipRect(a, b, true);
    }

    // One quad per stop pair (CSS linear-gradient). A 580px body with 3 stops
    // is 2 rects instead of 580 scanlines.
    if (stops[0].pos > 0.0f) {
        GradientBand(dl, a.x, b.x, a.y, a.y + stops[0].pos * h, stops[0].color, stops[0].color);
    }
    for (int i = 0; i < count - 1; ++i) {
        GradientBand(dl, a.x, b.x, a.y + stops[i].pos * h, a.y + stops[i + 1].pos * h,
                     stops[i].color, stops[i + 1].color);
    }
    if (stops[count - 1].pos < 1.0f) {
        GradientBand(dl, a.x, b.x, a.y + stops[count - 1].pos * h, b.y, stops[count - 1].color,
                     stops[count - 1].color);
    }

    if (rounding > 0.0f) {
        dl->PopClipRect();
        dl->AddRect(a, b, SampleStops(stops, count, 0.5f), rounding, flags, 0.0f);
    }
}

void VertGradientRounded(ImDrawList* dl, const ImVec2& a, const ImVec2& b, const GradStop* stops,
                         int count, float rounding) {
    const float h = b.y - a.y;
    const float w = b.x - a.x;
    if (h <= 0.0f || w <= 0.0f) {
        return;
    }
    const float r = ImMin(rounding, ImMin(w, h) * 0.5f);
    const int steps = static_cast<int>(h);
    for (int i = 0; i < steps; ++i) {
        const float y = a.y + static_cast<float>(i);
        const ImU32 c = SampleStops(stops, count, static_cast<float>(i) / h);

        // Inset this scanline to follow the corner arc. The row's *outer* edge
        // decides coverage: using the centre would let the fill bleed past the
        // arc and light up the pixels outside it. Measuring from the corner
        // centre and rounding up keeps every painted pixel inside the border.
        const float cyTop = a.y + r;
        const float cyBot = b.y - r;
        float dy = 0.0f;
        if (y < cyTop) {
            dy = cyTop - y;                    // distance to the top arc centre
        } else if (y + 1.0f > cyBot) {
            dy = (y + 1.0f) - cyBot;           // distance to the bottom arc centre
        }
        if (dy > r) {
            continue; // this row lies entirely outside the rounded silhouette
        }
        const float inset = (dy > 0.0f) ? ImCeil(r - ImSqrt(r * r - dy * dy)) : 0.0f;

        dl->AddRectFilled(ImVec2(a.x + inset, y), ImVec2(b.x - inset, y + 1.0f), c);
    }
}

void HorzGradient(ImDrawList* dl, const ImVec2& a, const ImVec2& b, const GradStop* stops,
                  int count, float rounding) {
    const float w = b.x - a.x;
    if (w <= 0.0f) {
        return;
    }
    if (rounding > 0.0f) {
        dl->PushClipRect(a, b, true);
    }
    const int steps = static_cast<int>(w);
    for (int i = 0; i < steps; ++i) {
        const float t0 = static_cast<float>(i) / w;
        const ImU32 c = SampleStops(stops, count, t0);
        dl->AddRectFilled(ImVec2(a.x + static_cast<float>(i), a.y),
                          ImVec2(a.x + static_cast<float>(i) + 1.0f, b.y), c);
    }
    if (rounding > 0.0f) {
        dl->PopClipRect();
    }
}

namespace {
// The widget layer's own layout cursor (screen space).
float g_curX = 0.0f;
float g_curY = 0.0f;
float g_curW = 0.0f;
} // namespace

void SetCursor(float x, float y, float width) {
    g_curX = x;
    g_curY = y;
    g_curW = width;
}
float CursorX() { return g_curX; }
float CursorY() { return g_curY; }
float CursorWidth() { return g_curW; }
void AdvanceY(float dy) { g_curY += dy; }

RowLayout BeginRow(const char* label, float rowHeight) {
    RowLayout out;
    out.rowMin = ImVec2(g_curX, g_curY);
    out.rowMax = ImVec2(g_curX + g_curW, g_curY + rowHeight);
    out.widgetMin = ImVec2(out.rowMax.x - kWidgetW, g_curY);

    if (label && label[0]) {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        TextIn(dl, out.rowMin, ImVec2(out.widgetMin.x, out.rowMax.y), label, TextCol(), 0.0f);
    }
    return out;
}

bool Checkbox(const char* id, bool* value) {
    ImGui::PushID(id);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec2 size(kCheckSize, kCheckSize);
    ImGui::InvisibleButton("##cb", size);
    SuppressWidgetUnderPopup();
    const bool hovered = !MouseOverPopup() && ImGui::IsItemHovered();
    bool changed = false;
    if (ImGui::IsItemClicked() && !WidgetBlocked()) {
        *value = !*value;
        changed = true;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = p;
    const ImVec2 b(p.x + kCheckSize, p.y + kCheckSize);
    constexpr float kR = 3.0f;

    // box-shadow: 0 1px 1px rgba(0,0,0,0.35)
    dl->AddRectFilled(ImVec2(a.x, a.y + 1.0f), ImVec2(b.x, b.y + 1.0f), Black(0.35f), kR);

    const GradStop stops[] = {{0.00f, CheckTop()},
                              {0.25f, CheckTop()},
                              {0.45f, CheckMid()},
                              {0.70f, CheckMid2()},
                              {1.00f, CheckBot()}};
    VertGradientRounded(dl, a, b, stops, IM_ARRAYSIZE(stops), kR);

    const ImU32 side = hovered ? CheckBorderHover() : CheckBorder();
    const ImU32 bot = hovered ? CheckBorderHoverBot() : CheckBorderBot();
    dl->AddLine(ImVec2(a.x + kR, a.y + 0.5f), ImVec2(b.x - kR, a.y + 0.5f), CheckBorderTop(),
                1.0f);
    dl->AddLine(ImVec2(a.x + kR, b.y - 0.5f), ImVec2(b.x - kR, b.y - 0.5f), bot, 1.0f);
    dl->AddLine(ImVec2(a.x + kR + 1.0f, a.y + 1.5f), ImVec2(b.x - kR - 1.0f, a.y + 1.5f), White(1.0f),
                1.0f);
    dl->AddRect(a, b, side, kR, 0, 1.0f);

    if (*value) {
        const float s = kCheckSize / 16.0f;
        const ImVec2 p0(a.x + 3.5f * s, a.y + 7.5f * s);
        const ImVec2 p1(a.x + 6.5f * s, a.y + 10.5f * s);
        const ImVec2 p2(a.x + 12.5f * s, a.y + 3.5f * s);
        dl->PathLineTo(p0);
        dl->PathLineTo(p1);
        dl->PathLineTo(p2);
        dl->PathStroke(CheckMark(), 0, 2.6f * s);
    }

    ImGui::PopID();
    return changed;
}

namespace {

// Shared slider body. Track + fill + thumb + centered value caption below.
bool SliderCore(const char* id, float* value, float vMin, float vMax, const char* caption) {
    ImGui::PushID(id);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const float w = kWidgetW;

    // .slider-track-wrap margin: 3px 0 1px 0 -> track sits 3px below the row top.
    const ImVec2 trackA(IM_ROUND(p.x), IM_ROUND(p.y + 3.0f));
    const ImVec2 trackB(trackA.x + w, trackA.y + kSliderTrackH);

    ImGui::InvisibleButton("##sl", ImVec2(w, kSliderTrackH + 4.0f));
    SuppressWidgetUnderPopup();
    const bool hovered = !MouseOverPopup() && ImGui::IsItemHovered();
    bool changed = false;
    if (ImGui::IsItemActive() && !WidgetBlocked() && vMax > vMin) {
        const float mx = ImGui::GetIO().MousePos.x;
        float t = (mx - trackA.x) / (trackB.x - trackA.x);
        t = ImClamp(t, 0.0f, 1.0f);
        const float nv = vMin + t * (vMax - vMin);
        if (nv != *value) {
            *value = nv;
            changed = true;
        }
    }

    const float t = (vMax > vMin) ? ImClamp((*value - vMin) / (vMax - vMin), 0.0f, 1.0f) : 0.0f;

    ImDrawList* dl = ImGui::GetWindowDrawList();

    // Track gradient derived from the panel base.
    const GradStop track[] = {{0.00f, TrackTop()},
                              {0.25f, TrackQ1()},
                              {0.50f, TrackMid()},
                              {0.75f, TrackQ3()},
                              {1.00f, TrackBot()}};
    VertGradientRounded(dl, trackA, trackB, track, IM_ARRAYSIZE(track), 2.5f);

    // Fill gradient derived from the control accent. Painted over the full
    // rounded track, then clipped to the filled portion so its left end keeps
    // the track's rounding and its right end stays square.
    const float fillW = IM_ROUND((trackB.x - trackA.x) * t);
    if (fillW > 0.0f) {
        const GradStop fill[] = {{0.00f, FillTop()},
                                 {0.20f, FillHi()},
                                 {0.45f, FillMid()},
                                 {0.75f, FillQ3()},
                                 {1.00f, FillBot()}};
        dl->PushClipRect(trackA, ImVec2(trackA.x + fillW, trackB.y), true);
        VertGradientRounded(dl, trackA, trackB, fill, IM_ARRAYSIZE(fill), 2.5f);
        dl->AddLine(ImVec2(trackA.x + 1.0f, trackA.y + 0.5f),
                    ImVec2(trackA.x + fillW, trackA.y + 0.5f), White(0.35f), 1.0f);
        dl->PopClipRect();
    }

    dl->AddRect(trackA, trackB, TrackBorder(), 2.5f, 0, 1.0f);
    dl->AddLine(ImVec2(trackA.x + 1.0f, trackB.y - 0.5f), ImVec2(trackB.x - 1.0f, trackB.y - 0.5f),
                TrackBorderBot(), 1.0f);

    // Thumb 8x12, radius 4, centered on the fill's right edge.
    const float cx = trackA.x + fillW;
    const float cy = (trackA.y + trackB.y) * 0.5f;
    const ImVec2 thA(IM_ROUND(cx - kSliderThumbW * 0.5f), IM_ROUND(cy - kSliderThumbH * 0.5f));
    const ImVec2 thB(thA.x + kSliderThumbW, thA.y + kSliderThumbH);
    const GradStop thumb[] = {{0.00f, ThumbTop()}, {0.42f, ThumbTop()},  {0.55f, ThumbMid()},
                              {0.70f, ThumbLow()}, {0.90f, ThumbLow()}, {1.00f, ThumbBot()}};
    VertGradientRounded(dl, thA, thB, thumb, IM_ARRAYSIZE(thumb), 4.0f);
    dl->AddRect(thA, thB, hovered ? ThumbBorderHover() : ThumbBorder(), 4.0f, 0, 1.0f);

    // Value caption centered under the track (.slider-val, margin-top 1px).
    if (caption && caption[0]) {
        const ImVec2 ts = ImGui::CalcTextSize(caption);
        dl->AddText(ImVec2(IM_ROUND(p.x + (w - ts.x) * 0.5f), IM_ROUND(trackB.y + 2.0f)),
                    TextCol(), caption);
    }

    ImGui::PopID();
    return changed;
}

} // namespace

bool SliderFloat(const char* id, float* value, float vMin, float vMax, const char* format) {
    char buf[64]{};
    snprintf(buf, sizeof(buf), format ? format : "%.1f", *value);
    return SliderCore(id, value, vMin, vMax, buf);
}

bool SliderInt(const char* id, int* value, int vMin, int vMax, const char* format) {
    float fv = static_cast<float>(*value);
    char buf[64]{};
    snprintf(buf, sizeof(buf), format ? format : "%d", *value);
    const bool changed = SliderCore(id, &fv, static_cast<float>(vMin), static_cast<float>(vMax),
                                    buf);
    if (changed) {
        *value = static_cast<int>(fv + 0.5f);
    }
    return changed;
}

bool Combo(const char* id, int* current, const char* const* items, int count) {
    ImGui::PushID(id);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec2 a = p;
    const ImVec2 b(p.x + kWidgetW, p.y + kFieldH);

    ImGui::InvisibleButton("##dd", ImVec2(kWidgetW, kFieldH));
    SuppressWidgetUnderPopup();
    const bool hovered = !MouseOverPopup() && ImGui::IsItemHovered();
    const ImGuiID myId = ImGui::GetItemID();
    ImGuiStorage* store = ImGui::GetStateStorage();
    bool open = store->GetBool(myId, false);
    // IsItemClicked fires on mouse-down, the same frame FlushPopups runs. Record
    // that frame so the popup does not treat its own opening click as "outside".
    // A click on an option that overlaps the next row's combo must not toggle it.
    bool openedThisFrame = false;
    if (ImGui::IsItemClicked() && !WidgetBlocked()) {
        open = !open;
        store->SetBool(myId, open);
        openedThisFrame = open;
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const GradStop field[] = {{0.00f, FieldTop()}, {0.45f, FieldMid()}, {1.00f, FieldBot()}};
    const ImU32 border = (hovered || open) ? FieldBorderHover() : FieldBorder();
    VertGradientRounded(dl, a, b, field, IM_ARRAYSIZE(field), 3.0f);

    // Split: [ text | v ]. Fill first, outline last, or the gray eats the border.
    const float splitX = IM_ROUND(b.x - kComboSplitW);
    {
        const GradStop btn[] = {{0.0f, ComboBtnTop()}, {1.0f, ComboBtnBot()}};
        dl->PushClipRect(ImVec2(splitX + 1.0f, a.y + 1.0f), ImVec2(b.x - 1.0f, b.y - 1.0f), true);
        VertGradientRounded(dl, a, b, btn, 2, 3.0f);
        dl->PopClipRect();
    }
    dl->AddRectFilled(ImVec2(splitX, a.y + 1.0f), ImVec2(splitX + 1.0f, b.y - 1.0f), border);
    const float btnL = splitX + 1.0f;
    const float btnR = b.x - 1.0f;
    const float left = IM_ROUND((btnL + btnR) * 0.5f - 2.5f) - 1.0f;
    const float top = IM_ROUND(a.y + kFieldH * 0.5f) - 1.0f;
    for (int i = 0; i < 3; ++i) {
        const float inset = static_cast<float>(open ? (2 - i) : i);
        const float y = top + static_cast<float>(i);
        dl->AddRectFilled(ImVec2(left + inset, y), ImVec2(left + 5.0f - inset, y + 1.0f),
                          ComboArrow());
    }
    dl->AddRect(a, b, border, 3.0f, 0, 1.0f);

    if (*current >= 0 && *current < count) {
        TextIn(dl, a, ImVec2(splitX, b.y), items[*current], TextCol(), 0.0f, 6.0f);
    }

    if (open) {
        PendingPopup pp;
        pp.id = myId;
        pp.pos = ImVec2(a.x, b.y + 2.0f);
        pp.width = kWidgetW;
        pp.current = current;
        pp.store = store;
        pp.justOpened = openedThisFrame;
        for (int i = 0; i < count; ++i) {
            pp.items.emplace_back(items[i]);
        }
        g_popups.push_back(std::move(pp));
    }

    ImGui::PopID();
    return false;
}

void FlushPopups() {
    g_popupHits.clear();
    if (g_popups.empty()) {
        return;
    }
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImVec2 mouse = ImGui::GetIO().MousePos;
    const bool clicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);

    for (auto& pp : g_popups) {
        const float rowH = 17.0f; // .dropdown-option padding 3px + 11px line
        const float h = rowH * static_cast<float>(pp.items.size());
        const ImVec2 a = pp.pos;
        const ImVec2 b(a.x + pp.width, a.y + h);
        g_popupHits.push_back({a, b});

        // Stacked rects standing in for a blurred drop shadow.
        constexpr int kShadowLayers = 8;
        for (int i = kShadowLayers; i >= 1; --i) {
            const float expand =
                kMenuShadowBlur * static_cast<float>(i) / static_cast<float>(kShadowLayers);
            dl->AddRectFilled(ImVec2(a.x - expand, a.y - expand + kMenuShadowOffY),
                              ImVec2(b.x + expand, b.y + expand + kMenuShadowOffY),
                              Black(kMenuShadowAlpha / static_cast<float>(kShadowLayers)),
                              kMenuRounding + expand * 0.5f);
        }
        dl->AddRectFilled(a, b, MenuBg(), kMenuRounding);
        dl->AddRect(a, b, MenuBorder(), kMenuRounding, 0, 1.0f);

        bool insideAny = false;
        for (size_t i = 0; i < pp.items.size(); ++i) {
            const ImVec2 ra(a.x + 1.0f, a.y + rowH * static_cast<float>(i));
            const ImVec2 rb(b.x - 1.0f, ra.y + rowH);
            const bool hov = mouse.x >= ra.x && mouse.x <= rb.x && mouse.y >= ra.y &&
                             mouse.y <= rb.y;
            if (hov) {
                insideAny = true;
                dl->AddRectFilled(ra, rb, OptionHover());
                if (clicked) {
                    *pp.current = static_cast<int>(i);
                    pp.store->SetBool(pp.id, false);
                }
            }
            TextIn(dl, ra, rb, pp.items[i].c_str(), hov ? OptionHoverText() : TextCol(), 0.0f,
                   6.0f);
        }
        // Click outside closes the menu (but not the click that just opened it).
        if (clicked && !insideAny && !pp.justOpened) {
            pp.store->SetBool(pp.id, false);
        }
    }
    g_popups.clear();
}

bool InputText(const char* id, std::string* text) {
    ImGui::PushID(id);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec2 a = p;
    const ImVec2 b(p.x + kWidgetW, p.y + kFieldH);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const GradStop field[] = {{0.00f, FieldTop()}, {0.45f, FieldMid()}, {1.00f, FieldBot()}};
    VertGradientRounded(dl, a, b, field, IM_ARRAYSIZE(field), 3.0f);

    // Transparent ImGui input on top so text editing/caret still work.
    char buf[128]{};
    snprintf(buf, sizeof(buf), "%s", text->c_str());
    ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, 0);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, 0);
    ImGui::PushStyleColor(ImGuiCol_Text, TextCol());
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.0f, 3.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
    ImGui::SetNextItemWidth(kWidgetW);
    const bool changed = ImGui::InputText("##in", buf, sizeof(buf));
    const bool active = ImGui::IsItemActive();
    const bool hovered = ImGui::IsItemHovered();
    ImGui::PopStyleVar(2);
    ImGui::PopStyleColor(4);
    if (changed) {
        *text = buf;
    }

    dl->AddRect(a, b, (hovered || active) ? FieldBorderHover() : FieldBorder(), 3.0f, 0, 1.0f);
    ImGui::PopID();
    return changed;
}

bool Hotkey(const char* id, std::string* text) {
    ImGui::PushID(id);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    const ImVec2 a = p;
    const ImVec2 b(p.x + kHotkeyW, p.y + kFieldH);

    ImGui::InvisibleButton("##hk", ImVec2(kHotkeyW, kFieldH));
    SuppressWidgetUnderPopup();
    const bool hovered = !MouseOverPopup() && ImGui::IsItemHovered();
    const ImGuiID myId = ImGui::GetItemID();
    bool capturing = (g_hotkeyCapture == myId);
    bool changed = false;

    if (ImGui::IsItemClicked() && !WidgetBlocked() && !capturing) {
        g_hotkeyCapture = myId;
        g_hotkeyTarget = text;
        g_hotkeyIgnoreUp = true;
        capturing = true;
    }
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && !WidgetBlocked() && !capturing) {
        if (*text != "none") {
            *text = "none";
            changed = true;
        }
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const GradStop field[] = {{0.00f, FieldTop()}, {0.45f, FieldMid()}, {1.00f, FieldBot()}};
    VertGradientRounded(dl, a, b, field, IM_ARRAYSIZE(field), 3.0f);
    dl->AddRect(a, b, (hovered || capturing) ? FieldBorderHover() : FieldBorder(), 3.0f, 0, 1.0f);

    const char* label = capturing ? "..." : (text->empty() ? "none" : text->c_str());
    TextIn(dl, a, b, label, TextCol(), 0.5f, 3.0f);

    ImGui::PopID();
    return changed;
}

bool Button(const char* label, float width) {
    const ImVec2 p(g_curX, g_curY);
    ImGui::SetCursorScreenPos(p);
    ImGui::PushID(label);
    ImGui::InvisibleButton("##btn", ImVec2(width, kButtonH));
    SuppressWidgetUnderPopup();
    const bool blocked = WidgetBlocked();
    const bool hovered = !blocked && ImGui::IsItemHovered();
    const bool held = !blocked && ImGui::IsItemActive();
    const bool pressed = !blocked && ImGui::IsItemDeactivated() && hovered;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImVec2 a = p;
    const ImVec2 b(p.x + width, p.y + kButtonH);
    // :active inverts the gradient.
    const GradStop normal[] = {{0.00f, BtnTop()}, {0.50f, BtnMid()}, {1.00f, BtnBot()}};
    const GradStop hover[] = {{0.00f, BtnTop()}, {0.50f, BtnHoverMid()}, {1.00f, BtnHoverBot()}};
    const GradStop active[] = {{0.00f, BtnBot()}, {0.50f, BtnMid()}, {1.00f, BtnTop()}};
    const GradStop* stops = held ? active : (hovered ? hover : normal);

    VertGradientRounded(dl, a, b, stops, 3, 3.0f);
    dl->AddRect(a, b, hovered ? FieldBorderHover() : BtnBorder(), 3.0f, 0, 1.0f);
    TextIn(dl, a, b, label, TextCol(), 0.5f);

    ImGui::PopID();
    return pressed;
}

void DrawTabIcon(ImDrawList* dl, int icon, const ImVec2& center, float radius, ImU32 col) {
    const ImVec2 c = center;
    const float r = radius;
    switch (icon) {
    case kIconEye: {
        // Almond outline + pupil ring.
        dl->AddBezierCubic(ImVec2(c.x - r, c.y), ImVec2(c.x - r * 0.40f, c.y - r * 0.92f),
                           ImVec2(c.x + r * 0.40f, c.y - r * 0.92f), ImVec2(c.x + r, c.y), col,
                           1.6f);
        dl->AddBezierCubic(ImVec2(c.x - r, c.y), ImVec2(c.x - r * 0.40f, c.y + r * 0.92f),
                           ImVec2(c.x + r * 0.40f, c.y + r * 0.92f), ImVec2(c.x + r, c.y), col,
                           1.6f);
        dl->AddCircle(c, r * 0.34f, col, 16, 1.6f);
        break;
    }
    case kIconSliders: {
        // Three tracks with offset knobs (equalizer).
        const float half = r * 0.82f;
        const float ys[3] = {c.y - r * 0.55f, c.y, c.y + r * 0.55f};
        const float kx[3] = {c.x - r * 0.30f, c.x + r * 0.32f, c.x - r * 0.05f};
        for (int i = 0; i < 3; ++i) {
            dl->AddLine(ImVec2(c.x - half, ys[i]), ImVec2(c.x + half, ys[i]), col, 1.6f);
            dl->AddCircleFilled(ImVec2(kx[i], ys[i]), 2.6f, col);
        }
        break;
    }
    case kIconGlobe: {
        // Circle + meridian ellipse + equator.
        const float ro = r * 0.95f;
        dl->AddCircle(c, ro, col, 24, 1.6f);
        constexpr int kSteps = 24;
        dl->PathClear();
        for (int i = 0; i <= kSteps; ++i) {
            const float a = static_cast<float>(i) / kSteps * 2.0f * 3.14159265f;
            dl->PathLineTo(ImVec2(c.x + ro * 0.45f * ImCos(a), c.y + ro * ImSin(a)));
        }
        dl->PathStroke(col, 0, 1.4f);
        dl->AddLine(ImVec2(c.x - ro, c.y), ImVec2(c.x + ro, c.y), col, 1.6f);
        break;
    }
    case kIconGear: {
        // Ring + 8 radial teeth + hub.
        const float ri = r * 0.60f;
        const float ro = r * 0.95f;
        for (int i = 0; i < 8; ++i) {
            const float a = static_cast<float>(i) * (3.14159265f / 4.0f);
            dl->AddLine(ImVec2(c.x + ri * ImCos(a), c.y + ri * ImSin(a)),
                        ImVec2(c.x + ro * ImCos(a), c.y + ro * ImSin(a)), col, 3.0f);
        }
        dl->AddCircle(c, ri, col, 16, 1.6f);
        dl->AddCircle(c, r * 0.26f, col, 12, 1.4f);
        break;
    }
    case kIconPalette: {
        // Artist palette: outline + three paint dots + thumb hole.
        const float ro = r * 0.95f;
        dl->AddCircle(c, ro, col, 24, 1.6f);
        dl->AddCircleFilled(ImVec2(c.x - ro * 0.45f, c.y - ro * 0.25f), 1.7f, col);
        dl->AddCircleFilled(ImVec2(c.x, c.y - ro * 0.55f), 1.7f, col);
        dl->AddCircleFilled(ImVec2(c.x + ro * 0.45f, c.y - ro * 0.25f), 1.7f, col);
        dl->AddCircle(ImVec2(c.x + ro * 0.32f, c.y + ro * 0.38f), ro * 0.30f, col, 12, 1.4f);
        break;
    }
    case kIconBolt: {
        // Rage crosshair-X: four diagonal wedges (narrow at the center, wide at
        // the outer corners) converging on a filled center diamond.
        const float o = r * 0.96f;    // outer extent along the diagonal
        const float wOut = r * 0.34f; // half-width at the outer end
        const float wIn = r * 0.10f;  // half-width at the diamond edge
        const float cd = r * 0.30f;   // center diamond half-diagonal
        constexpr float kS = 0.70710678f;
        const float dirs[4][2] = {{kS, -kS}, {kS, kS}, {-kS, kS}, {-kS, -kS}};
        for (int i = 0; i < 4; ++i) {
            const float ux = dirs[i][0], uy = dirs[i][1];
            const float px = uy, py = -ux; // perpendicular
            dl->PathLineTo(ImVec2(c.x + ux * cd + px * wIn, c.y + uy * cd + py * wIn));
            dl->PathLineTo(ImVec2(c.x + ux * o + px * wOut, c.y + uy * o + py * wOut));
            dl->PathLineTo(ImVec2(c.x + ux * o - px * wOut, c.y + uy * o - py * wOut));
            dl->PathLineTo(ImVec2(c.x + ux * cd - px * wIn, c.y + uy * cd - py * wIn));
            dl->PathFillConvex(col);
        }
        dl->PathLineTo(ImVec2(c.x + cd, c.y));
        dl->PathLineTo(ImVec2(c.x, c.y + cd));
        dl->PathLineTo(ImVec2(c.x - cd, c.y));
        dl->PathLineTo(ImVec2(c.x, c.y - cd));
        dl->PathFillConvex(col);
        break;
    }
    default:
        break;
    }
}

bool TabBar(const char* const* labels, int count, int* current, float width, float height,
            const int* icons) {
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    bool changed = false;
    int activeIdx = -1;

    dl->AddRectFilled(p, ImVec2(p.x + width, p.y + height), TabBarBg());

    const float tabW = width / static_cast<float>(count);
    for (int i = 0; i < count; ++i) {
        const ImVec2 a(p.x + tabW * static_cast<float>(i), p.y);
        const ImVec2 b(i == count - 1 ? p.x + width : a.x + tabW, p.y + height);

        ImGui::PushID(i);
        ImGui::SetCursorScreenPos(a);
        ImGui::InvisibleButton("##tab", ImVec2(b.x - a.x, height));
        SuppressWidgetUnderPopup();
        const bool hovered = !MouseOverPopup() && ImGui::IsItemHovered();
        if (ImGui::IsItemClicked() && !WidgetBlocked() && *current != i) {
            *current = i;
            changed = true;
        }
        ImGui::PopID();

        const bool act = (*current == i);
        if (act) {
            activeIdx = i;
        }
        dl->PushClipRect(a, b, true);
        if (act) {
            const float h = b.y - a.y;
            const GradStop s[] = {{0.0f, TabActiveTop()},
                                  {1.0f / h, TabActiveTop()},
                                  {2.0f / h, TabActiveShadow()},
                                  {4.0f / h, TabActQ4()},
                                  {0.20f, TabActiveMid()},
                                  {0.45f, TabActP45()},
                                  {0.70f, TabActP70()},
                                  {1.00f, TabActiveBot()}};
            VertGradient(dl, a, b, s, IM_ARRAYSIZE(s), 0.0f);
        } else {
            const float h = b.y - a.y;
            const ImU32 t = hovered ? TabHoverTop() : TabInactiveTop();
            const ImU32 m2 = hovered ? TabHoverM2() : TabInactiveM2();
            const ImU32 m3 = hovered ? TabHoverMid() : TabInactiveMid();
            const ImU32 m4 = hovered ? TabHoverM4() : TabInactiveM4();
            const ImU32 m5 = hovered ? TabHoverM5() : TabInactiveM5();
            const ImU32 bo = hovered ? TabHoverBot() : TabInactiveBot();
            const GradStop s[] = {{0.0f, t},   {1.0f / h, t}, {2.0f / h, m2},
                                  {0.20f, m3}, {0.45f, m4},   {0.70f, m5},
                                  {1.00f, bo}};
            VertGradient(dl, a, b, s, IM_ARRAYSIZE(s), 0.0f);
        }
        dl->PopClipRect();

        // Separators: left #2d2d2d (active darker), right #090909 (active lighter).
        if (i > 0) {
            dl->AddLine(ImVec2(a.x + 0.5f, a.y), ImVec2(a.x + 0.5f, b.y),
                        act ? TabActSepL() : TabSepLight(), 1.0f);
        }
        if (i < count - 1) {
            dl->AddLine(ImVec2(b.x - 0.5f, a.y), ImVec2(b.x - 0.5f, b.y),
                        act ? TabActiveBot() : TabSepDark(), 1.0f);
        }

        if (icons) {
            // Icon above label, both centered in the tab.
            const ImVec2 ts = ImGui::CalcTextSize(labels[i]);
            const float contentH = kTabIconR * 2.0f + kTabIconTextGap + ts.y;
            const float top = a.y + ((b.y - a.y) - contentH) * 0.5f;
            const ImVec2 ic((a.x + b.x) * 0.5f, top + kTabIconR);
            // Normal/hover/active icon states per the theme spec: the accent
            // takes over on hover, and stays on the active tab.
            const ImU32 iconCol = (act || hovered) ? Rgb(ActiveTheme().Control) : TabTextCol();
            DrawTabIcon(dl, icons[i], ic, kTabIconR, iconCol);
            dl->AddText(ImVec2(IM_ROUND((a.x + b.x) * 0.5f - ts.x * 0.5f),
                               IM_ROUND(top + kTabIconR * 2.0f + kTabIconTextGap)),
                        TabTextCol(), labels[i]);
        } else {
            TextIn(dl, a, b, labels[i], TabTextCol(), 0.5f);
        }
    }

    // border-bottom: 1px dark line, then the active tab's 2px accent underline.
    dl->AddLine(ImVec2(p.x, p.y + height - 0.5f), ImVec2(p.x + width, p.y + height - 0.5f),
                TabBottomBorder(), 1.0f);
    if (activeIdx >= 0) {
        const float ax = p.x + tabW * static_cast<float>(activeIdx);
        const float bx = activeIdx == count - 1 ? p.x + width : ax + tabW;
        dl->AddRectFilled(ImVec2(ax, p.y + height - 2.0f), ImVec2(bx, p.y + height),
                          Rgb(ActiveTheme().Control));
    }

    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + height));
    return changed;
}

namespace {

// Shared dark segmented bar (subtabs + weapon bar differ only in metrics/labels).
bool DarkBar(const char* const* labels, int count, int* current, float width, float height,
             bool weapon) {
    const ImVec2 p(g_curX, g_curY);
    ImDrawList* dl = ImGui::GetWindowDrawList();
    bool changed = false;

    const ImVec2 outA = p;
    const ImVec2 outB(p.x + width, p.y + height);
    dl->AddRectFilled(outA, outB, BarFrame());

    const ImVec2 inA(outA.x + 1.0f, outA.y + 1.0f);
    const ImVec2 inB(outB.x - 1.0f, outB.y - 1.0f);
    const float segW = (inB.x - inA.x) / static_cast<float>(count);

    for (int i = 0; i < count; ++i) {
        const ImVec2 a(inA.x + segW * static_cast<float>(i), inA.y);
        const ImVec2 b(i == count - 1 ? inB.x : a.x + segW, inB.y);

        ImGui::PushID(i);
        ImGui::SetCursorScreenPos(a);
        ImGui::InvisibleButton("##seg", ImVec2(b.x - a.x, b.y - a.y));
        SuppressWidgetUnderPopup();
        const bool hovered = !MouseOverPopup() && ImGui::IsItemHovered();
        if (ImGui::IsItemClicked() && !WidgetBlocked() && *current != i) {
            *current = i;
            changed = true;
        }
        ImGui::PopID();

        const bool act = (*current == i);
        dl->PushClipRect(a, b, true);
        if (weapon) {
            if (act) {
                const GradStop s[] = {{0.0f, WeaponActiveTop()},
                                      {0.40f, WeaponActiveMid()},
                                      {1.0f, WeaponActiveBot()}};
                VertGradient(dl, a, b, s, 3, 0.0f);
            } else {
                const GradStop s[] = {{0.0f, hovered ? WeaponHoverTop() : WeaponTop()},
                                      {0.5f, hovered ? WeaponHoverMid() : WeaponMid()},
                                      {1.0f, hovered ? WeaponHoverBot() : WeaponBot()}};
                VertGradient(dl, a, b, s, 3, 0.0f);
                dl->AddLine(ImVec2(a.x, a.y + 0.5f), ImVec2(b.x, a.y + 0.5f), White(0.10f), 1.0f);
            }
        } else {
            if (act) {
                const GradStop s[] = {{0.0f, SubTabActiveTop()},
                                      {0.40f, SubTabActiveMid()},
                                      {1.0f, SubTabActiveBot()}};
                VertGradient(dl, a, b, s, 3, 0.0f);
            } else {
                const GradStop s[] = {{0.0f, hovered ? SubHoverTop() : SubTabTop()},
                                      {0.5f, hovered ? SubHoverMid() : SubTabMid()},
                                      {1.0f, hovered ? SubHoverBot() : SubTabBot()}};
                VertGradient(dl, a, b, s, 3, 0.0f);
                dl->AddLine(ImVec2(a.x, a.y + 0.5f), ImVec2(b.x, a.y + 0.5f), White(0.08f), 1.0f);
            }
        }
        // Sunken active inset shadow.
        if (act) {
            dl->AddRectFilled(a, ImVec2(b.x, a.y + 3.0f), Black(0.55f));
            dl->AddRectFilled(a, ImVec2(a.x + 1.0f, b.y), Black(0.40f));
        }
        dl->PopClipRect();

        if (i > 0) {
            dl->AddLine(ImVec2(a.x + 0.5f, a.y), ImVec2(a.x + 0.5f, b.y),
                        weapon ? WeaponSepL() : SubSepL(), 1.0f);
        }
        if (i < count - 1) {
            dl->AddLine(ImVec2(b.x - 0.5f, a.y), ImVec2(b.x - 0.5f, b.y),
                        weapon ? WeaponSepR() : SubSepR(), 1.0f);
        }

        const ImU32 col =
            weapon ? InvTextCol() : (act ? SubTextActiveCol() : SubTextCol());
        if (weapon) {
            // Icon block (16px) above the label, matching .weapon-icon + gap 2px.
            const ImVec2 ts = ImGui::CalcTextSize(labels[i]);
            const float totalH = 16.0f + 2.0f + ts.y;
            const float top = a.y + ((b.y - a.y) - totalH) * 0.5f;
            const ImVec2 ic((a.x + b.x) * 0.5f, top + 8.0f);
            // Simple glyph per class, drawn as vector marks in white.
            const float r = 6.0f;
            switch (i) {
            case 0: // Melee: sword
                dl->AddLine(ImVec2(ic.x - r, ic.y + r), ImVec2(ic.x + r, ic.y - r), col, 2.0f);
                dl->AddLine(ImVec2(ic.x - r, ic.y + r), ImVec2(ic.x - r + 3.0f, ic.y + r), col,
                            2.0f);
                break;
            case 1: // Ranged: bow
                dl->PathArcTo(ImVec2(ic.x - 2.0f, ic.y), r, -1.2f, 1.2f, 12);
                dl->PathStroke(col, 0, 1.6f);
                dl->AddLine(ImVec2(ic.x - 4.0f, ic.y), ImVec2(ic.x + r, ic.y), col, 1.6f);
                break;
            case 2: // Magic: staff + spark
                dl->AddLine(ImVec2(ic.x - r, ic.y + r), ImVec2(ic.x + 2.0f, ic.y - 3.0f), col,
                            2.0f);
                dl->AddCircleFilled(ImVec2(ic.x + 4.0f, ic.y - 5.0f), 2.4f, col);
                break;
            case 3: // Summon: minion
                dl->AddCircleFilled(ImVec2(ic.x, ic.y - 2.0f), 3.2f, col);
                dl->AddTriangleFilled(ImVec2(ic.x - 5.0f, ic.y + 5.0f),
                                      ImVec2(ic.x + 5.0f, ic.y + 5.0f), ImVec2(ic.x, ic.y - 1.0f),
                                      col);
                break;
            default: // Thrown: shuriken
                dl->AddTriangleFilled(ImVec2(ic.x, ic.y - r), ImVec2(ic.x + 3.0f, ic.y),
                                      ImVec2(ic.x - 3.0f, ic.y), col);
                dl->AddTriangleFilled(ImVec2(ic.x, ic.y + r), ImVec2(ic.x + 3.0f, ic.y),
                                      ImVec2(ic.x - 3.0f, ic.y), col);
                break;
            }
            dl->AddText(ImVec2(IM_ROUND((a.x + b.x) * 0.5f - ts.x * 0.5f),
                               IM_ROUND(top + 16.0f + 2.0f)),
                        col, labels[i]);
        } else {
            TextIn(dl, a, b, labels[i], col, 0.5f);
        }
    }

    g_curY = p.y + height;
    return changed;
}

} // namespace

bool SubTabBar(const char* const* labels, int count, int* current, float width) {
    return DarkBar(labels, count, current, width, kSubTabH, false);
}

bool WeaponBar(const char* const* labels, int count, int* current, float width) {
    return DarkBar(labels, count, current, width, kWeaponBarH, true);
}

namespace {
// Groupbox frames are drawn on EndGroup once the content height is known.
struct GroupState {
    ImVec2 start;
    float width;
    std::string title;
    int drawCmd;
};
std::vector<GroupState> g_groups;
} // namespace

void BeginGroup(const char* title) {
    GroupState gs;
    gs.start = ImVec2(g_curX, g_curY);
    gs.width = g_curW;
    gs.title = title ? title : "";
    g_groups.push_back(gs);

    // The legend straddles the frame's top edge, so content starts below the
    // whole legend line plus the CSS top padding; indent by padding-left.
    g_curY += ImGui::GetTextLineHeight() + kGroupPadTop;
    g_curX += kGroupPadX;
    g_curW -= kGroupPadX * 2.0f;
}

void EndGroup() {
    if (g_groups.empty()) {
        return;
    }
    GroupState gs = g_groups.back();
    g_groups.pop_back();

    g_curX = gs.start.x;
    g_curW = gs.width;
    g_curY += kGroupPadBot;

    // Snap so the 1px bars land on whole pixels. AddLine + AA fringes the
    // #b4b4b4 border across two pixels and it reads as a blur.
    const ImVec2 a(IM_ROUND(gs.start.x),
                   IM_ROUND(gs.start.y + ImGui::GetTextLineHeight() * 0.5f));
    const ImVec2 b(IM_ROUND(gs.start.x + gs.width), IM_ROUND(g_curY));

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->ChannelsSetCurrent(0);

    // Fill the whole frame first, including the 1px under the border. The
    // title gap then shows this gradient, not the window body.
    const GradStop s[] = {{0.0f, GroupTop()}, {1.0f, GroupBot()}};
    if (b.y > a.y && b.x > a.x) {
        VertGradient(dl, a, b, s, 2, 0.0f);
    }

    const ImU32 br = GroupBorder();
    dl->AddRectFilled(ImVec2(a.x, a.y), ImVec2(a.x + 1.0f, b.y), br);
    dl->AddRectFilled(ImVec2(a.x, b.y - 1.0f), ImVec2(b.x, b.y), br);
    dl->AddRectFilled(ImVec2(b.x - 1.0f, a.y), ImVec2(b.x, b.y), br);

    if (!gs.title.empty()) {
        const ImVec2 ts = ImGui::CalcTextSize(gs.title.c_str());
        const float gapL = a.x + kGroupLegendLead;
        const float tx = gapL + kGroupLegendPad;
        const float gapR = IM_ROUND(tx + ts.x + kGroupLegendPad);
        const float ty = IM_ROUND(gs.start.y);
        if (gapL > a.x) {
            dl->AddRectFilled(ImVec2(a.x, a.y), ImVec2(gapL, a.y + 1.0f), br);
        }
        if (b.x > gapR) {
            dl->AddRectFilled(ImVec2(gapR, a.y), ImVec2(b.x, a.y + 1.0f), br);
        }
        dl->AddText(ImVec2(tx, ty), TextCol(), gs.title.c_str());
    } else {
        dl->AddRectFilled(ImVec2(a.x, a.y), ImVec2(b.x, a.y + 1.0f), br);
    }

    dl->ChannelsSetCurrent(1);
    g_curY += kGroupSpacing;
}

void BeginPanel() {
    PollHotkeyCapture();
    // Channel 0 = groupbox frames (background), channel 1 = widgets (foreground).
    ImGui::GetWindowDrawList()->ChannelsSplit(2);
    ImGui::GetWindowDrawList()->ChannelsSetCurrent(1);
}

void EndPanel() { ImGui::GetWindowDrawList()->ChannelsMerge(); }

// ---- row wrappers -----------------------------------------------------------

bool RowCheckbox(const char* label, bool* value) {
    const RowLayout r = BeginRow(label, kRowH);
    ImGui::SetCursorScreenPos(
        ImVec2(r.widgetMin.x, r.rowMin.y + (kRowH - kCheckSize) * 0.5f));
    const bool ch = Checkbox(label, value);
    g_curY = r.rowMax.y + kRowGap;
    return ch;
}

bool RowSlider(const char* label, float* value, float vMin, float vMax, const char* format) {
    // Slider rows are taller: track + value caption underneath.
    const float h = 3.0f + kSliderTrackH + 2.0f + ImGui::GetTextLineHeight() + 1.0f;
    const RowLayout r = BeginRow(label, h);
    ImGui::SetCursorScreenPos(r.widgetMin);
    const bool ch = SliderFloat(label, value, vMin, vMax, format);
    g_curY = r.rowMin.y + h + kRowGap;
    return ch;
}

bool RowSliderInt(const char* label, int* value, int vMin, int vMax, const char* format) {
    const float h = 3.0f + kSliderTrackH + 2.0f + ImGui::GetTextLineHeight() + 1.0f;
    const RowLayout r = BeginRow(label, h);
    ImGui::SetCursorScreenPos(r.widgetMin);
    const bool ch = SliderInt(label, value, vMin, vMax, format);
    g_curY = r.rowMin.y + h + kRowGap;
    return ch;
}

bool RowCombo(const char* label, int* current, const char* const* items, int count) {
    const RowLayout r = BeginRow(label, kRowH);
    ImGui::SetCursorScreenPos(ImVec2(r.widgetMin.x, r.rowMin.y + (kRowH - kFieldH) * 0.5f));
    const bool ch = Combo(label, current, items, count);
    g_curY = r.rowMax.y + kRowGap;
    return ch;
}

bool RowColor(const char* label, float color[4]) {
    const RowLayout r = BeginRow(label, kRowH);
    ImGui::SetCursorScreenPos(ImVec2(r.widgetMin.x, r.rowMin.y + (kRowH - kFieldH) * 0.5f));
    ImGui::PushID(label);
    const ImVec4 preview(color[0], color[1], color[2], color[3]);
    if (ImGui::ColorButton("##color", preview,
                           ImGuiColorEditFlags_AlphaPreviewHalf | ImGuiColorEditFlags_NoDragDrop,
                           ImVec2(kWidgetW, kFieldH)) && !WidgetBlocked()) {
        ImGui::OpenPopup("##color_picker");
    }
    bool changed = false;
    if (ImGui::BeginPopup("##color_picker")) {
        changed = ImGui::ColorPicker4("##rgba", color,
                                      ImGuiColorEditFlags_AlphaBar |
                                      ImGuiColorEditFlags_PickerHueWheel);
        ImGui::EndPopup();
    }
    ImGui::PopID();
    g_curY = r.rowMax.y + kRowGap;
    return changed;
}

bool RowInput(const char* label, std::string* text) {
    const RowLayout r = BeginRow(label, kRowH);
    ImGui::SetCursorScreenPos(ImVec2(r.widgetMin.x, r.rowMin.y + (kRowH - kFieldH) * 0.5f));
    const bool ch = InputText(label, text);
    g_curY = r.rowMax.y + kRowGap;
    return ch;
}

bool RowHotkey(const char* label, std::string* text) {
    const RowLayout r = BeginRow(label, kRowH);
    ImGui::SetCursorScreenPos(
        ImVec2(r.widgetMin.x, r.rowMin.y + (kRowH - kFieldH) * 0.5f));
    const bool ch = Hotkey(label, text);
    g_curY = r.rowMax.y + kRowGap;
    return ch;
}

// ---- theme editor primitives ------------------------------------------------

bool HueSatField(const char* id, float* h, float* s, const ImVec2& size) {
    ImGui::PushID(id);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##hueSat", size);
    SuppressWidgetUnderPopup();
    bool changed = false;
    if (ImGui::IsItemActive() && !WidgetBlocked()) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        const float nh = ImClamp((m.x - p.x) / ImMax(size.x - 1.0f, 1.0f), 0.0f, 1.0f);
        const float ns = ImClamp(1.0f - (m.y - p.y) / ImMax(size.y - 1.0f, 1.0f), 0.0f, 1.0f);
        if (nh != *h || ns != *s) {
            *h = nh;
            *s = ns;
            changed = true;
        }
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    // Hue varies along x, so the panel is one vertical white->hue gradient per
    // 2px column (AddRectFilledMultiColor corners: TL, TR, BL, BR).
    const float w = ImMax(2.0f, size.x);
    const int cols = static_cast<int>(w * 0.5f);
    for (int i = 0; i < cols; ++i) {
        const float x0 = p.x + 2.0f * static_cast<float>(i);
        const float x1 = ImMin(x0 + 2.0f, p.x + size.x);
        const float hue = (x0 - p.x) / ImMax(size.x - 1.0f, 1.0f);
        const ImU32 top = HsvToU32(hue, 1.0f, 1.0f);
        dl->AddRectFilledMultiColor(ImVec2(x0, p.y), ImVec2(x1, p.y + size.y), top, top,
                                    White(1.0f), White(1.0f));
    }
    dl->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), Black(0.45f), 0.0f, 0, 1.0f);

    // Cursor dot: dark halo + light core so it reads on any hue.
    const ImVec2 cur(p.x + *h * (size.x - 1.0f), p.y + (1.0f - *s) * (size.y - 1.0f));
    dl->AddCircle(cur, 4.5f, Black(0.85f), 0, 1.4f);
    dl->AddCircle(cur, 3.2f, White(1.0f), 0, 1.4f);

    ImGui::PopID();
    return changed;
}

bool BrightnessSliderV(const char* id, float* v, float h, float s, const ImVec2& size) {
    ImGui::PushID(id);
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##bright", size);
    SuppressWidgetUnderPopup();
    bool changed = false;
    if (ImGui::IsItemActive() && !WidgetBlocked()) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        const float nv = ImClamp(1.0f - (m.y - p.y) / ImMax(size.y - 1.0f, 1.0f), 0.0f, 1.0f);
        if (nv != *v) {
            *v = nv;
            changed = true;
        }
    }

    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImU32 top = HsvToU32(h, s, 1.0f);
    dl->AddRectFilledMultiColor(p, ImVec2(p.x + size.x, p.y + size.y), top, top, Black(1.0f),
                                Black(1.0f));
    dl->AddRect(p, ImVec2(p.x + size.x, p.y + size.y), Black(0.45f), 0.0f, 0, 1.0f);

    // Thumb: full-width bar with a light core and dark outline.
    const float ty = p.y + (1.0f - *v) * (size.y - 1.0f);
    dl->AddRectFilled(ImVec2(p.x, ty - 3.0f), ImVec2(p.x + size.x, ty + 3.0f), Black(0.85f));
    dl->AddRectFilled(ImVec2(p.x + 1.0f, ty - 2.0f), ImVec2(p.x + size.x - 1.0f, ty + 2.0f),
                      White(1.0f));

    ImGui::PopID();
    return changed;
}

} // namespace sibalhook::ui
