#pragma once

#include <string>
#include "render/esp_color.h"

namespace sibalhook {

// PvZ Replanted 功能状态。持久化到 config.ini [ui.*]
struct MenuState {
    static MenuState& Instance();

    int activeTab = 0;    // Visuals

    // ---- Visuals / Zombie ESP ----
    bool zombieEsp = true;
    bool zombieBox = true;
    int  boxStyle = 1;          // 0=Off 1=Corners 2=Full
    bool zombieHpBar = true;
    bool zombieHpText = true;
    bool zombieName = true;
    bool zombieSkeleton = false;
    bool zombieHeadDot = true;
    bool zombieTracer = true;      // 射线：屏幕顶部指向僵尸头部
    bool zombieRowText = false;
    bool hpColorByRatio = true;
    bool zombieChams = false;   // 彩人：直接给僵尸 Spine 骨骼整体着色
    EspColor zombieBoxColor{1.0f, 70.0f / 255.0f, 70.0f / 255.0f, 1.0f};
    EspColor mindControlledBoxColor{120.0f / 255.0f, 220.0f / 255.0f, 1.0f, 1.0f};
    EspColor zombieChamsColor{1.0f, 40.0f / 255.0f, 40.0f / 255.0f, 1.0f};
    EspColor zombieSkeletonColor{1.0f, 1.0f, 120.0f / 255.0f, 220.0f / 255.0f};
    EspColor zombieHeadDotColor{1.0f, 80.0f / 255.0f, 80.0f / 255.0f, 1.0f};
    EspColor zombieTracerColor{1.0f, 70.0f / 255.0f, 70.0f / 255.0f, 200.0f / 255.0f};

    // ---- Visuals / Plant ESP ----
    bool plantEsp = true;
    bool plantHpBar = true;
    bool plantName = false;
    bool plantStateIcon = false;
    EspColor plantBoxColor{80.0f / 255.0f, 220.0f / 255.0f, 120.0f / 255.0f, 220.0f / 255.0f};

    // ---- Visuals / Pickups ----
    bool coinEsp = true;
    bool sunEsp = true;
    bool coinValueText = true;
    EspColor sunRingColor{1.0f, 230.0f / 255.0f, 90.0f / 255.0f, 1.0f};
    EspColor coinRingColor{1.0f, 200.0f / 255.0f, 60.0f / 255.0f, 1.0f};

    // ---- Visuals / HUD ----
    bool seedCooldownHud = true;
    bool waveHud = true;
    bool watermark = true;
    float lineW = 1.5f;

    // ---- Visuals / Projectiles ----
    bool projectileTrajectory = false;   // 弹道线（直线平飞投射物）
    EspColor projectileTrajectoryColor{0.30f, 1.0f, 0.45f, 0.55f};

    // ---- Cheats / Rage ----
    bool magicBullets = false;           // 魔法子弹：豌豆跨行命中

    // ---- Cheats / Economy ----
    bool infiniteSun = false;
    int  sunAmount = 9990;       // 10..9990
    bool noCooldown = false;
    bool freePlanting = false;

    // ---- Cheats / Combat ----
    bool instantKill = false;
    bool freezeZombies = false;
    bool plantGod = false;
    bool autoCollect = false;

    // ---- World ----
    float timeScale = 1.0f;      // 0.1..5
    bool noFog = false;
    int  spawnType = 2;          // 路障
    int  spawnRow = 0;           // 0..5
    int  spawnCount = 1;         // 1..10

    // ---- Settings ----
    std::string configName = "default";
    std::string menuToggleKey = "insert";
    std::string panicKey = "f11";
    bool zhText = true;   // ESP 文本语言：true=中文 false=English

    void LoadFromConfig();
    void SaveToConfig() const;
};

} // namespace sibalhook
