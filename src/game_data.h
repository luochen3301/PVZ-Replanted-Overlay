#pragma once
#include "framework.h"

namespace game {

// ---- 快照数据（渲染线程只读） ----
struct BoneSnap { float x, y; char name[48]; bool isHead; };

struct ZombieSnap {
    uint64_t obj;
    int type, row, phase;
    int hp, hpMax, helm, helmMax, shield, shieldMax;
    float posX, posY;
    bool dead, mindControlled;
    // 屏幕坐标（快照线程算好）
    bool hasBox; float bx0, by0, bx1, by1;
    float ax, ay;   // 锚点屏幕坐标（调试标记用）
    float headX, headY; bool hasHead;
    std::vector<BoneSnap> bones;
};

struct PlantSnap {
    uint64_t obj;
    int type, row, col, state;
    int hp, hpMax;
    bool dead, asleep;
    bool hasBox; float bx0, by0, bx1, by1;
};

struct CoinSnap {
    uint64_t obj;
    int type;
    float posX, posY;
    bool dead;
    bool hasPt; float sx, sy;
};

struct ProjectileSnap {
    uint64_t obj;
    int type;
    float posX, posY, velX, velY, velZ;
    bool dead;
    bool hasPt; float sx, sy;    // 当前位置屏幕坐标
    bool hasEnd; float ex, ey;   // 直线弹道终点屏幕坐标
};

struct SeedSnap {
    int type;
    int refresh, refreshTime;
    bool active, refreshing;
};

struct Snapshot {
    bool valid = false;
    uint64_t boardObj = 0;
    int wave = 0, numWaves = 0;
    float waveProgress = 0.0f;   // 0..1 当前波剩余血量比例
    int sun = 0;
    float gameTime = 0.0f;
    std::vector<ZombieSnap> zombies;
    std::vector<PlantSnap> plants;
    std::vector<CoinSnap> coins;
    std::vector<ProjectileSnap> projectiles;
    std::vector<SeedSnap> seeds;
};

// ---- 初始化 / 生命周期 ----
bool init();          // 解析全部类与偏移（失败返回 false 并写日志）
void start();         // 兼容占位（主线程模式无后台线程）
void stop();
void on_present(float viewW = 0, float viewH = 0);  // Present 钩子内调用，传交换链尺寸

const Snapshot* current();   // 最新快照（可能为 nullptr 或 valid=false）
int debug_flags();           // 1=board 2=cam 4=boardAffine（水印诊断用）

// 一次性请求（按钮触发）
void request_skip_wave();
void request_spawn(int zombieType, int row);
void request_restore_timescale();

// 僵尸类型偏移校正：dump 顺序 Invalid=0/Normal=1，实际枚举可能 Invalid=-1/Normal=0
int  zombie_type_shift();
void zombie_name_for(int type, char* out, size_t n);
void zombie_enum_name(int idx, char* out, size_t n);  // 原始枚举名（无 shift 校正，生成器下拉用）
void plant_name_for(int type, char* out, size_t n);
void coin_name_for(int type, char* out, size_t n);

} // namespace game
