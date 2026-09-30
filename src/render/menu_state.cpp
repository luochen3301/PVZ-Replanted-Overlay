#include "framework.h"
#include "render/menu_state.h"
#include "render/ui_theme.h"
#include "config/config.h"

namespace sibalhook {
namespace {

constexpr const char* kSectionVisuals = "ui.visuals";
constexpr const char* kSectionCheats = "ui.cheats";
constexpr const char* kSectionWorld = "ui.world";
constexpr const char* kSectionSettings = "ui.settings";
constexpr const char* kSectionMenu = "ui.menu";

// One entry per persisted field: (section, key, member).
#define SIBAL_MENU_FIELDS(B, I, F, S)                                     \
    /* menu shell */                                                      \
    I(kSectionMenu, "active_tab", activeTab)                              \
    /* visuals */                                                         \
    B(kSectionVisuals, "zombie_esp", zombieEsp)                           \
    B(kSectionVisuals, "zombie_box", zombieBox)                           \
    I(kSectionVisuals, "box_style", boxStyle)                             \
    B(kSectionVisuals, "zombie_hp_bar", zombieHpBar)                      \
    B(kSectionVisuals, "zombie_hp_text", zombieHpText)                    \
    B(kSectionVisuals, "zombie_name", zombieName)                         \
    B(kSectionVisuals, "zombie_skeleton", zombieSkeleton)                 \
    B(kSectionVisuals, "zombie_head_dot", zombieHeadDot)                 \
    B(kSectionVisuals, "zombie_tracer", zombieTracer)                     \
    B(kSectionVisuals, "zombie_row_text", zombieRowText)                  \
    B(kSectionVisuals, "hp_color_by_ratio", hpColorByRatio)              \
    B(kSectionVisuals, "zombie_chams", zombieChams)                       \
    B(kSectionVisuals, "plant_esp", plantEsp)                             \
    B(kSectionVisuals, "plant_hp_bar", plantHpBar)                        \
    B(kSectionVisuals, "plant_name", plantName)                           \
    B(kSectionVisuals, "plant_state_icon", plantStateIcon)                \
    B(kSectionVisuals, "coin_esp", coinEsp)                               \
    B(kSectionVisuals, "sun_esp", sunEsp)                                 \
    B(kSectionVisuals, "coin_value_text", coinValueText)                  \
    B(kSectionVisuals, "seed_cooldown_hud", seedCooldownHud)              \
    B(kSectionVisuals, "wave_hud", waveHud)                               \
    B(kSectionVisuals, "watermark", watermark)                            \
    F(kSectionVisuals, "line_w", lineW)                                   \
    B(kSectionVisuals, "projectile_trajectory", projectileTrajectory)     \
    /* cheats */                                                          \
    B(kSectionCheats, "magic_bullets", magicBullets)                      \
    B(kSectionCheats, "infinite_sun", infiniteSun)                        \
    I(kSectionCheats, "sun_amount", sunAmount)                            \
    B(kSectionCheats, "no_cooldown", noCooldown)                          \
    B(kSectionCheats, "free_planting", freePlanting)                      \
    B(kSectionCheats, "instant_kill", instantKill)                        \
    B(kSectionCheats, "freeze_zombies", freezeZombies)                    \
    B(kSectionCheats, "plant_god", plantGod)                              \
    B(kSectionCheats, "auto_collect", autoCollect)                        \
    /* world */                                                           \
    F(kSectionWorld, "time_scale", timeScale)                             \
    B(kSectionWorld, "no_fog", noFog)                                     \
    I(kSectionWorld, "spawn_type", spawnType)                             \
    I(kSectionWorld, "spawn_row", spawnRow)                               \
    I(kSectionWorld, "spawn_count", spawnCount)                           \
    /* settings */                                                        \
    S(kSectionSettings, "config_name", configName)                        \
    S(kSectionSettings, "menu_toggle_key", menuToggleKey)                 \
    S(kSectionSettings, "panic_key", panicKey)                            \
    B(kSectionSettings, "zh_text", zhText)

#define SIBAL_ESP_COLORS(X)                                                  \
    X("zombie_box_color", zombieBoxColor)                                  \
    X("mind_controlled_box_color", mindControlledBoxColor)                 \
    X("zombie_chams_color", zombieChamsColor)                              \
    X("zombie_skeleton_color", zombieSkeletonColor)                        \
    X("zombie_head_dot_color", zombieHeadDotColor)                          \
    X("zombie_tracer_color", zombieTracerColor)                            \
    X("plant_box_color", plantBoxColor)                                    \
    X("sun_ring_color", sunRingColor)                                      \
    X("coin_ring_color", coinRingColor)                                    \
    X("projectile_trajectory_color", projectileTrajectoryColor)

} // namespace

MenuState& MenuState::Instance() {
    static MenuState inst;
    return inst;
}

void MenuState::LoadFromConfig() {
    Config& cfg = Config::Instance();
#define LOAD_B(sec, key, member) member = cfg.GetBool(sec, key, member);
#define LOAD_I(sec, key, member) member = cfg.GetInt(sec, key, member);
#define LOAD_F(sec, key, member) member = cfg.GetFloat(sec, key, member);
#define LOAD_S(sec, key, member) member = cfg.GetString(sec, key, member);
    SIBAL_MENU_FIELDS(LOAD_B, LOAD_I, LOAD_F, LOAD_S)
#define LOAD_COLOR(key, member) ColorFromHex(cfg.GetString(kSectionVisuals, key, ColorToHex(member)), member);
    SIBAL_ESP_COLORS(LOAD_COLOR)
#undef LOAD_COLOR
#undef LOAD_B
#undef LOAD_I
#undef LOAD_F
#undef LOAD_S
    // UI 主题（[ui.theme]），与游戏/ESP 颜色完全独立
    ui::ThemeManager::Instance().LoadFromConfig(cfg);
}

void MenuState::SaveToConfig() const {
    Config& cfg = Config::Instance();
#define SAVE_B(sec, key, member) cfg.SetBool(sec, key, member);
#define SAVE_I(sec, key, member) cfg.SetInt(sec, key, member);
#define SAVE_F(sec, key, member) cfg.SetFloat(sec, key, member);
#define SAVE_S(sec, key, member) cfg.SetString(sec, key, member);
    SIBAL_MENU_FIELDS(SAVE_B, SAVE_I, SAVE_F, SAVE_S)
#define SAVE_COLOR(key, member) cfg.SetString(kSectionVisuals, key, ColorToHex(member));
    SIBAL_ESP_COLORS(SAVE_COLOR)
#undef SAVE_COLOR
#undef SAVE_B
#undef SAVE_I
#undef SAVE_F
#undef SAVE_S
    ui::ThemeManager::Instance().SaveToConfig(cfg);
}

} // namespace sibalhook
