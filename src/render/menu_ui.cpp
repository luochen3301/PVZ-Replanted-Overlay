#include "framework.h"
#include "render/menu_ui.h"
#include "render/menu.h"
#include "render/menu_state.h"
#include "render/ui_theme.h"
#include "render/ui_widgets.h"
#include "render/ui_lang.h"
#include "app/app.h"
#include "config/persist.h"
#include "game_data.h"

#include "imgui.h"
#include "imgui_internal.h"

namespace sibalhook {
namespace {

using namespace ui;

const char* const kTabsEn[] = {"Visuals", "Cheats", "Rage", "World", "Settings", "GUI"};
const char* const kTabsZh[] = {"视觉", "作弊", "狂暴", "世界", "设置", "界面"};
constexpr int kTabCount = 6;

// 僵尸类型下拉项（按当前语言生成；索引=生成器用的原始枚举值）
void BuildZombieTypeItems(char names[][40], const char* items[], int count) {
    for (int i = 0; i < count; ++i) {
        game::zombie_enum_name(i, names[i], 40);
        items[i] = names[i];
    }
}

// Column bookkeeping: panels are a 1fr 1fr grid with a 16px gap.
struct ColumnLayout {
    float width;
    float leftX;
    float rightX;
    float top;
};

ColumnLayout BeginColumns(float originX, float originY, float totalWidth) {
    ColumnLayout c;
    c.width = (totalWidth - kColumnGap) * 0.5f;
    c.leftX = originX;
    c.rightX = originX + c.width + kColumnGap;
    c.top = originY;
    return c;
}

float g_columnBottom = 0.0f;

void EnterColumn(const ColumnLayout& c, bool right) {
    g_columnBottom = ImMax(g_columnBottom, CursorY());
    SetCursor(right ? c.rightX : c.leftX, c.top, c.width);
}

// --------------------------------------------------------------- Visuals ----
void DrawVisuals(MenuState& s, const ColumnLayout& c) {
    EnterColumn(c, false);
    BeginGroup(TR("Zombie ESP"));
    RowCheckbox(TR("Zombie ESP"), &s.zombieEsp);
    RowCheckbox(TR("Box"), &s.zombieBox);
    RowColor(TR("Box Color"), s.zombieBoxColor.data());
    RowColor(TR("Controlled Box"), s.mindControlledBoxColor.data());
    RowCheckbox(TR("Chams"), &s.zombieChams);
    RowColor(TR("Chams Color"), s.zombieChamsColor.data());
    {
        const char* const items[] = {TR("Corners"), TR("Full"), TR("Off")};
        RowCombo(TR("Box Style"), &s.boxStyle, items, IM_ARRAYSIZE(items));
    }
    RowCheckbox(TR("Health Bar"), &s.zombieHpBar);
    RowCheckbox(TR("Health Text"), &s.zombieHpText);
    RowCheckbox(TR("Zombie Name"), &s.zombieName);
    RowCheckbox(TR("Row Text"), &s.zombieRowText);
    RowCheckbox(TR("Health Color By Ratio"), &s.hpColorByRatio);
    EndGroup();

    BeginGroup(TR("Skeleton"));
    RowCheckbox(TR("Draw Skeleton"), &s.zombieSkeleton);
    RowColor(TR("Skeleton Color"), s.zombieSkeletonColor.data());
    RowCheckbox(TR("Head Bone Dot"), &s.zombieHeadDot);
    RowColor(TR("Head Dot Color"), s.zombieHeadDotColor.data());
    RowCheckbox(TR("Tracer Lines"), &s.zombieTracer);
    RowColor(TR("Tracer Color"), s.zombieTracerColor.data());
    RowSlider(TR("Line Width"), &s.lineW, 0.5f, 4.0f, "%.1f");
    EndGroup();

    BeginGroup(TR("Projectiles"));
    RowCheckbox(TR("Trajectory"), &s.projectileTrajectory);
    RowColor(TR("Trajectory Color"), s.projectileTrajectoryColor.data());
    EndGroup();

    EnterColumn(c, true);
    BeginGroup(TR("Plant ESP"));
    RowCheckbox(TR("Plant ESP"), &s.plantEsp);
    RowColor(TR("Plant Box Color"), s.plantBoxColor.data());
    RowCheckbox(TR("Health Bar"), &s.plantHpBar);
    RowCheckbox(TR("Plant Name"), &s.plantName);
    EndGroup();

    BeginGroup(TR("Pickups"));
    RowCheckbox(TR("Sun ESP"), &s.sunEsp);
    RowColor(TR("Sun Ring Color"), s.sunRingColor.data());
    RowCheckbox(TR("Coin ESP"), &s.coinEsp);
    RowColor(TR("Coin Ring Color"), s.coinRingColor.data());
    RowCheckbox(TR("Value Text"), &s.coinValueText);
    EndGroup();

    BeginGroup(TR("HUD"));
    RowCheckbox(TR("Seed Packet Cooldown"), &s.seedCooldownHud);
    RowCheckbox(TR("Wave Progress Bar"), &s.waveHud);
    RowCheckbox(TR("Watermark"), &s.watermark);
    EndGroup();
}

// ---------------------------------------------------------------- Cheats ----
void DrawCheats(MenuState& s, const ColumnLayout& c) {
    EnterColumn(c, false);
    BeginGroup(TR("Economy"));
    RowCheckbox(TR("Infinite Sun"), &s.infiniteSun);
    RowSliderInt(TR("Sun Amount"), &s.sunAmount, 10, 9990, "%d");
    EndGroup();

    BeginGroup(TR("Seed Packets"));
    RowCheckbox(TR("No Cooldown"), &s.noCooldown);
    RowCheckbox(TR("Free Planting (Any Cost)"), &s.freePlanting);
    EndGroup();

    EnterColumn(c, true);
    BeginGroup(TR("Combat"));
    RowCheckbox(TR("Instant Kill"), &s.instantKill);
    RowCheckbox(TR("Freeze Zombies"), &s.freezeZombies);
    RowCheckbox(TR("Plant God Mode"), &s.plantGod);
    EndGroup();

    BeginGroup(TR("Automation"));
    RowCheckbox(TR("Auto Collect Sun & Coins"), &s.autoCollect);
    EndGroup();
}

// ----------------------------------------------------------------- World ----
void DrawWorld(MenuState& s, const ColumnLayout& c) {
    EnterColumn(c, false);
    BeginGroup(TR("Game Speed"));
    RowSlider(TR("Time Scale"), &s.timeScale, 0.1f, 5.0f, "%.2fx");
    {
        const float avail = CursorWidth();
        const float bw = (avail - 2.0f * 6.0f) / 3.0f;
        const ImVec2 start(CursorX(), CursorY());
        const char* const names[] = {"1x", "2x", "0.5x"};
        const float vals[] = {1.0f, 2.0f, 0.5f};
        for (int i = 0; i < 3; ++i) {
            SetCursor(start.x + (bw + 6.0f) * static_cast<float>(i), start.y, bw);
            if (Button(names[i], bw)) s.timeScale = vals[i];
        }
        SetCursor(start.x, start.y + kButtonH + 4.0f, avail);
    }
    EndGroup();

    BeginGroup(TR("Environment"));
    RowCheckbox(TR("No Fog"), &s.noFog);
    EndGroup();

    EnterColumn(c, true);
    BeginGroup(TR("Wave Control"));
    {
        const float avail = CursorWidth();
        if (Button(TR("Skip To Next Wave"), avail)) game::request_skip_wave();
        AdvanceY(kButtonH + 4.0f);
    }
    EndGroup();

    BeginGroup(TR("Zombie Spawner (Experimental)"));
    {
        constexpr int kZTypeCount = 38;
        static char zNames[kZTypeCount][40];
        static const char* zItems[kZTypeCount];
        BuildZombieTypeItems(zNames, zItems, kZTypeCount);
        RowCombo(TR("Zombie Type"), &s.spawnType, zItems, kZTypeCount);
    }
    RowSliderInt(TR("Row (0-5)"), &s.spawnRow, 0, 5, "%d");
    RowSliderInt(TR("Count"), &s.spawnCount, 1, 10, "%d");
    {
        const float avail = CursorWidth();
        if (Button(TR("Spawn!"), avail)) {
            for (int i = 0; i < s.spawnCount; ++i)
                game::request_spawn(s.spawnType, s.spawnRow);
        }
        AdvanceY(kButtonH + 4.0f);
    }
    EndGroup();
}

// -------------------------------------------------------------- Settings ----
void DrawSettings(MenuState& s, const ColumnLayout& c) {
    EnterColumn(c, false);
    BeginGroup(TR("Configuration"));
    RowInput(TR("Config Name"), &s.configName);
    {
        const float avail = CursorWidth();
        const float bw = (avail - 3.0f * 6.0f) / 4.0f;
        const ImVec2 start(CursorX(), CursorY());
        const char* const names[] = {TR("Load"), TR("Save"), TR("Reset"), TR("New")};
        for (int i = 0; i < 4; ++i) {
            SetCursor(start.x + (bw + 6.0f) * static_cast<float>(i), start.y, bw);
            if (Button(names[i], bw)) {
                if (i == 0) s.LoadFromConfig();
                else if (i == 1) PersistFlushNow();
                else if (i == 2) s = MenuState();   // 重置为默认
            }
        }
        SetCursor(start.x, start.y + kButtonH + 4.0f, avail);
    }
    EndGroup();

    BeginGroup(TR("Keybinds"));
    RowHotkey(TR("Menu Toggle"), &s.menuToggleKey);
    RowHotkey(TR("Panic (Hide All)"), &s.panicKey);
    EndGroup();

    BeginGroup(TR("Language"));
    RowCheckbox("Chinese Text  中文", &s.zhText);
    EndGroup();

    EnterColumn(c, true);
    BeginGroup(TR("About Lawnbox"));
    {
        ImDrawList* dl = ImGui::GetWindowDrawList();
        const char* const lines[][2] = {{TR("Software:"), " Lawnbox"},
                                        {TR("Target:"), " PvZ Replanted (Unity IL2CPP x64)"},
                                        {TR("Build:"), " v0.5 dev"},
                                        {TR("Engine:"), " Direct3D 11 Native Hook"}};
        const float lh = ImGui::GetTextLineHeight() * 1.6f;
        ImVec2 p(CursorX(), CursorY() + 4.0f);
        for (const auto& ln : lines) {
            dl->AddText(ImVec2(IM_ROUND(p.x + 2.0f), IM_ROUND(p.y)), AboutTextCol(), ln[0]);
            const float w2 = ImGui::CalcTextSize(ln[0]).x;
            dl->AddText(ImVec2(IM_ROUND(p.x + 2.0f + w2), IM_ROUND(p.y)), AboutTextCol(), ln[1]);
            p.y += lh;
        }
        AdvanceY(lh * 4.0f + 6.0f);
    }
    EndGroup();

    AdvanceY(2.0f);
    if (Button(TR("Unload DLL"), 100.0f)) {
        App::Instance().RequestUnload();
    }
}

// -------------------------------------------------------------------- GUI ----
// One theme color block: [hue/sat field][brightness slider][preview], editing
// the live theme so the whole menu recolors on the next frame (no Apply).
void ThemeColorBlock(int slot) {
    ThemeManager& tm = ThemeManager::Instance();
    float* hsv = tm.hsv(slot);
    const float w = CursorWidth();
    const float sliderW = 16.0f;
    const float previewW = 100.0f;
    const float gap = 6.0f;
    const float blockH = 104.0f;
    const float fieldW = w - sliderW - previewW - gap * 2.0f;

    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddText(ImVec2(CursorX(), CursorY()), TextCol(), ThemeSlotDisplayName(slot));
    AdvanceY(ImGui::GetTextLineHeight() + 3.0f);

    const float y = CursorY();
    const float x = CursorX();
    ImGui::SetCursorScreenPos(ImVec2(x, y));
    // 控件 ID 用英文 ThemeSlotName（语言切换不断 ID）
    bool changed = HueSatField(ThemeSlotName(slot), &hsv[0], &hsv[1], ImVec2(fieldW, blockH));
    ImGui::SetCursorScreenPos(ImVec2(x + fieldW + gap, y));
    changed |= BrightnessSliderV(ThemeSlotName(slot), &hsv[2], hsv[0], hsv[1],
                                 ImVec2(sliderW, blockH));

    const ImVec2 pa(x + fieldW + sliderW + gap * 2.0f, y);
    const ImVec2 pb(pa.x + previewW, y + blockH);
    dl->AddRectFilled(pa, pb, Rgb(ThemeSlotColor(tm.theme(), slot)));
    dl->AddRect(pa, pb, GroupBorder(), 0.0f, 0, 1.0f);

    SetCursor(x, y + blockH + 10.0f, w);
    if (changed) {
        tm.CommitHSV(slot); // live update + autosave mark
    }
}

// 第五个 Tab：整页主题编辑卡片，2×3 网格，Reset 在右上角。
void DrawGUI(const ColumnLayout& c) {
    SetCursor(c.leftX, c.top, c.width * 2.0f + kColumnGap);
    BeginGroup(TR("UI Theme"));
    {
        const float w = CursorWidth();
        const ImVec2 start(CursorX(), CursorY());
        const float rw = 64.0f;
        SetCursor(start.x + w - rw, start.y, rw);
        if (Button(TR("Reset"), rw)) {
            ThemeManager::Instance().ResetToDefaults();
        }

        const float gridTop = start.y + kButtonH + 10.0f;
        const float colW = (w - kColumnGap) * 0.5f;
        const float rowH = ImGui::GetTextLineHeight() + 3.0f + 104.0f + 10.0f;
        for (int i = 0; i < kThemeSlotCount; ++i) {
            SetCursor(start.x + static_cast<float>(i % 2) * (colW + kColumnGap),
                      gridTop + static_cast<float>(i / 2) * rowH, colW);
            ThemeColorBlock(i);
        }
        SetCursor(start.x, gridTop + 3.0f * rowH, w);
    }
    EndGroup();
}

// ------------------------------------------------------------------- Rage ----
// 第六个 Tab：Rage 独立页面（攻击性作弊单独一页）。
void DrawRage(const ColumnLayout& c) {
    SetCursor(c.leftX, c.top, c.width * 2.0f + kColumnGap);
    BeginGroup(TR("Rage"));
    RowCheckbox(TR("Magic Bullets"), &MenuState::Instance().magicBullets);
    EndGroup();
}

// Draws the translucent red chassis + title bar + inner container.
void DrawChassis(const ImVec2& winMin, const ImVec2& winMax) {
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const ImVec2 glassMin(winMin.x + 1.0f, winMin.y + 1.0f);
    const ImVec2 glassMax(winMax.x - 1.0f, winMax.y - 1.0f);
    dl->AddRectFilled(glassMin, glassMax, ChassisBody(), kOuterNeonRounding);
    dl->AddRect(winMin, winMax, ChassisOuterDark(), kWindowRounding, 0, 1.0f);
    dl->AddRect(glassMin, glassMax, ChassisNeon(), kOuterNeonRounding, 0, 1.0f);

    const ImVec2 tp(winMin.x + 6.0f, winMin.y);
    const char* title = "Lawnbox for PvZ Replanted";
    const ImVec2 ts = ImGui::CalcTextSize(title);
    const ImVec2 at(IM_ROUND(tp.x), IM_ROUND(tp.y + (kTitleBarH - ts.y) * 0.5f));
    dl->AddText(ImVec2(at.x, at.y + 1.0f), Black(0.95f), title);
    dl->AddText(at, TitleTextCol(), title);
}

} // namespace

void MenuUi::Draw() {
    MenuState& s = MenuState::Instance();
    ui::ThemeManager::Instance().TickAutoSave();

    static const int kTabIcons[] = {ui::kIconEye,   ui::kIconSliders, ui::kIconBolt,
                                    ui::kIconGlobe, ui::kIconGear,   ui::kIconPalette};

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, kWindowRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0.0f, 0.0f));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, 0);

    ImGui::SetNextWindowSize(ImVec2(kWindowW, kWindowH), ImGuiCond_FirstUseEver);
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(
        ImVec2(vp->WorkPos.x + (vp->WorkSize.x - kWindowW) * 0.5f,
               vp->WorkPos.y + (vp->WorkSize.y - kWindowH) * 0.5f),
        ImGuiCond_FirstUseEver);

    const ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar |
                                   ImGuiWindowFlags_NoScrollWithMouse |
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;

    if (ImGui::Begin("Lawnbox for PvZ Replanted", nullptr, flags)) {
        const ImVec2 winMin = ImGui::GetWindowPos();
        const ImVec2 winMax(winMin.x + ImGui::GetWindowSize().x,
                            winMin.y + ImGui::GetWindowSize().y);

        DrawChassis(winMin, winMax);

        const ImVec2 frameMin(winMin.x + kInnerInset, winMin.y + kTitleBarH);
        const ImVec2 frameMax(winMax.x - kInnerInset, winMax.y - kInnerInset);
        const ImVec2 innerMin(frameMin.x + 1.0f, frameMin.y + 1.0f);
        const ImVec2 innerMax(frameMax.x - 1.0f, frameMax.y - 1.0f);

        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(innerMin, innerMax, TabBarBg(), kInnerRounding);
        dl->AddRect(frameMin, frameMax, InnerDark(), kInnerRounding, 0, 1.0f);
        dl->AddRect(ImVec2(frameMin.x - 1.0f, frameMin.y - 1.0f),
                    ImVec2(frameMax.x + 1.0f, frameMax.y + 1.0f), ChassisNeon(), kInnerRounding,
                    0, 1.0f);

        // ---- tab bar ----
        const float innerW = innerMax.x - innerMin.x;
        ImGui::SetCursorScreenPos(innerMin);
        const char* const* tabs = s.zhText ? kTabsZh : kTabsEn;
        TabBar(tabs, kTabCount, &s.activeTab, innerW, kTabBarH, kTabIcons);

        // ---- window body ----
        const ImVec2 bodyMin(innerMin.x, innerMin.y + kTabBarH);
        const ImVec2 bodyMax(innerMax.x, innerMax.y);
        const GradStop body[] = {{0.0f, BodyTop()}, {0.5f, BodyMid()}, {1.0f, BodyBot()}};
        dl->PushClipRect(bodyMin, bodyMax, true);
        VertGradient(dl, bodyMin, bodyMax, body, 3, 0.0f);
        for (int i = 0; i < static_cast<int>(kBodyBevel); ++i) {
            const float t = 1.0f - static_cast<float>(i) / kBodyBevel;
            const float fi = static_cast<float>(i);
            dl->AddRectFilled(ImVec2(bodyMin.x, bodyMax.y - 1.0f - fi),
                              ImVec2(bodyMax.x, bodyMax.y - fi), Black(0.20f * t));
            dl->AddRectFilled(ImVec2(bodyMin.x + fi, bodyMin.y),
                              ImVec2(bodyMin.x + fi + 1.0f, bodyMax.y), Black(0.18f * t));
            dl->AddRectFilled(ImVec2(bodyMax.x - 1.0f - fi, bodyMin.y),
                              ImVec2(bodyMax.x - fi, bodyMax.y), Black(0.18f * t));
        }
        dl->PopClipRect();

        // ---- scrolling panel ----
        ImGui::SetCursorScreenPos(bodyMin);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, 0);
        ImGui::PushStyleColor(ImGuiCol_ScrollbarBg, ScrollTrack());
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrab, ScrollThumbM());
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabHovered, ScrollThumbL());
        ImGui::PushStyleColor(ImGuiCol_ScrollbarGrabActive, ScrollThumbR());
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, kScrollbarW);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 1.0f);

        if (ImGui::BeginChild("##panel", ImVec2(bodyMax.x - bodyMin.x, bodyMax.y - bodyMin.y),
                              ImGuiChildFlags_None, ImGuiWindowFlags_NoBackground)) {
            const ImVec2 childPos = ImGui::GetWindowPos();
            const float childW = ImGui::GetWindowWidth();
            const bool hasScroll = ImGui::GetScrollMaxY() > 0.0f;
            const float contentW =
                childW - kPanelPad * 2.0f - (hasScroll ? kScrollbarW : 0.0f);

            BeginPanel();
            g_columnBottom = 0.0f;
            const ColumnLayout cols =
                BeginColumns(childPos.x + kPanelPad, childPos.y + kPanelPad - ImGui::GetScrollY(),
                             contentW);
            switch (s.activeTab) {
            case 0: DrawVisuals(s, cols); break;
            case 1: DrawCheats(s, cols); break;
            case 2: DrawRage(cols); break;
            case 3: DrawWorld(s, cols); break;
            case 5: DrawGUI(cols); break;
            default: DrawSettings(s, cols); break;
            }
            EndPanel();

            const float bottom = ImMax(g_columnBottom, CursorY());
            const float contentH = bottom - (childPos.y - ImGui::GetScrollY()) + 10.0f;
            ImGui::SetCursorScreenPos(ImVec2(childPos.x, childPos.y - ImGui::GetScrollY()));
            ImGui::Dummy(ImVec2(contentW, contentH));
        }
        ImGui::EndChild();

        ImGui::PopStyleVar(2);
        ImGui::PopStyleColor(5);

        FlushPopups();
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(4);
}

} // namespace sibalhook
