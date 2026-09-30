#include "game_data.h"
#include "projection_math.h"
#include "il2cpp_api.h"
#include "render/menu_state.h"
#include <algorithm>
#include <cctype>

using std::min;
using std::max;

namespace game {

static std::atomic<int> g_zTypeShift{ -100 };

// ================= 名字表（按 dump 顺序，Invalid=0 起；运行时校正 shift） =================
static const char* kZombieNames[] = {
    "Invalid","普通僵尸","旗帜僵尸","路障僵尸","撑杆僵尸","铁桶僵尸","读报僵尸","铁门僵尸",
    "橄榄球僵尸","舞王僵尸","伴舞僵尸","救生圈僵尸","潜水僵尸","冰车僵尸","雪橇小队","海豚骑士",
    "开盒僵尸","气球僵尸","矿工僵尸","跳跳僵尸","雪人僵尸","蹦极僵尸","搭梯僵尸","投石车",
    "巨人僵尸","小鬼僵尸","僵王博士","豌豆头","坚果头","火爆椒头","机枪头","窝瓜头",
    "高坚果头","红眼巨人","自定义僵尸","靶子","垃圾桶僵尸","墓碑","未知类型"
};
static const char* kZombieNamesEn[] = {
    "Invalid","Normal","Flag","Cone Head","Pole Vaulter","Bucket Head","Newspaper","Screen Door",
    "Football","Dancing","Backup Dancer","Ducky Tube","Snorkel","Zamboni","Bobsled","Dolphin Rider",
    "Jack-in-the-Box","Balloon","Digger","Pogo","Zombie Yeti","Bungee","Ladder","Catapult",
    "Gargantuar","Imp","Dr. Zomboss","Pea Head","Wall-nut Head","Jalapeno Head","Gatling Head","Squash Head",
    "Tall-nut Head","Redeye Gargantuar","Zombatar","Target","Trash Can","Gravestone","Unknown"
};
static const char* kSeedNames[] = {
    "豌豆射手","向日葵","樱桃炸弹","坚果墙","土豆雷","寒冰射手","大嘴花","双发射手",
    "小喷菇","阳光菇","大喷菇","墓碑吞噬者","魅惑菇","胆小菇","寒冰菇","毁灭菇",
    "荷叶","窝瓜","缠绕水草","火爆辣椒","地刺","火炬树桩","高坚果","海蘑菇",
    "路灯花","仙人掌","三叶草","分裂豌豆","杨桃","南瓜头","磁力菇","卷心菜投手",
    "花盆","玉米投手","咖啡豆","大蒜","伞叶","金盏花","西瓜投手","机枪射手",
    "双子向日葵","忧郁菇","香蒲","冰西瓜","吸金磁","钢铁地刺","玉米加农炮","模仿者",
    "爆炸坚果","巨型坚果","发芽","左发射手","种子数上限","BeghouledA","BeghouledB",
    "老虎机阳光","老虎机钻石","水族箱","水族箱奖杯"
};
static const char* kSeedNamesEn[] = {
    "Peashooter","Sunflower","Cherry Bomb","Wall-nut","Potato Mine","Snow Pea","Chomper","Repeater",
    "Puff-shroom","Sun-shroom","Fume-shroom","Grave Buster","Hypno-shroom","Scaredy-shroom","Ice-shroom","Doom-shroom",
    "Lily Pad","Squash","Tangle Kelp","Jalapeno","Spikeweed","Torchwood","Tall-nut","Sea-shroom",
    "Plantern","Cactus","Blover","Split Pea","Starfruit","Pumpkin","Magnet-shroom","Cabbage-pult",
    "Flower Pot","Kernel-pult","Coffee Bean","Garlic","Umbrella Leaf","Marigold","Melon-pult","Gatling Pea",
    "Twin Sunflower","Gloom-shroom","Cattail","Winter Melon","Gold Magnet","Spikerock","Cob Cannon","Imitater",
    "Explode-o-nut","Giant Wall-nut","Sprout","Left Repeater","More Seeds","BeghouledA","BeghouledB",
    "Slots Sun","Slots Diamond","Aquarium","Aquarium Trophy"
};
static const char* kCoinNames[] = {
    "None","银币","金币","钻石","阳光","小阳光","大阳光","终极卡片","奖杯","铲子",
    "图鉴","车钥匙","花瓶","浇水壶","塔可","纸条","卡袋","礼盒植物","钱袋","礼物",
    "钻石袋","银色向日葵","金色向日葵","巧克力","巧克力奖","小游戏礼盒","解谜礼盒","生存礼盒",
    "双倍阳光","大脑","对抗植物杯","对抗僵尸杯","RIP奖杯"
};
static const char* kCoinNamesEn[] = {
    "None","Silver Coin","Gold Coin","Diamond","Sun","Small Sun","Large Sun","Ultimate Card","Trophy","Shovel",
    "Almanac","Car Key","Vase","Watering Can","Taco","Note","Seed Pack","Gift Plant","Money Bag","Present",
    "Diamond Bag","Silver Sunflower","Gold Sunflower","Chocolate","Chocolate Coin","Mini-game Box","Puzzle Box","Survival Box",
    "Twin Sun","Brain","Vs Plants Cup","Vs Zombies Cup","RIP Trophy"
};

// 语言开关（Settings -> Language -> Chinese Text）
static bool zh_text() { return sibalhook::MenuState::Instance().zhText; }

int zombie_type_shift() {
    int s = g_zTypeShift.load();
    return s == -100 ? 0 : s;
}

void zombie_name_for(int type, char* out, size_t n) {
    int shift = zombie_type_shift();
    int idx = type + shift;
    const char* const* names = zh_text() ? kZombieNames : kZombieNamesEn;
    size_t cnt = sizeof(kZombieNames) / sizeof(kZombieNames[0]);
    if (idx >= 0 && (size_t)idx < cnt - 1) strncpy(out, names[idx], n - 1), out[n - 1] = 0;
    else _snprintf(out, n, zh_text() ? "类型#%d" : "Type #%d", type);
}
void zombie_enum_name(int idx, char* out, size_t n) {
    const char* const* names = zh_text() ? kZombieNames : kZombieNamesEn;
    size_t cnt = sizeof(kZombieNames) / sizeof(kZombieNames[0]);
    if (idx >= 0 && (size_t)idx < cnt - 1) strncpy(out, names[idx], n - 1), out[n - 1] = 0;
    else _snprintf(out, n, "#%d", idx);
}
void plant_name_for(int type, char* out, size_t n) {
    const char* const* names = zh_text() ? kSeedNames : kSeedNamesEn;
    size_t cnt = sizeof(kSeedNames) / sizeof(kSeedNames[0]);
    if (type >= 0 && (size_t)type < cnt - 1) strncpy(out, names[type], n - 1), out[n - 1] = 0;
    else _snprintf(out, n, zh_text() ? "植物#%d" : "Plant #%d", type);
}
void coin_name_for(int type, char* out, size_t n) {
    const char* const* names = zh_text() ? kCoinNames : kCoinNamesEn;
    size_t cnt = sizeof(kCoinNames) / sizeof(kCoinNames[0]);
    if (type >= 0 && (size_t)type < cnt) strncpy(out, names[type], n - 1), out[n - 1] = 0;
    else _snprintf(out, n, zh_text() ? "物品#%d" : "Item #%d", type);
}

// ================= 类与偏移缓存 =================
namespace {

int g_offExListItems = -1, g_offExListCount = -1;

struct Off { const char* cls; const char* field; int off; };

void* K_Board, * K_Zombie, * K_Plant, * K_Coin, * K_Projectile;
void* K_SeedBank, * K_SeedPacket, * K_GameplayActivity, * K_ZombieController;
void* K_PlantController;
void* K_ReloadedCharacterController, * K_CharacterAnimationController;
void* K_SkeletonRenderer, * K_SpineSkeleton, * K_SpineBone, * K_SpineBoneData;
void* K_Transform, * K_Camera, * K_Time, * K_Component, * K_Sun, * K_Renderer;
void* K_DataArrayZombie, * K_DataArrayPlant, * K_DataArrayCoin, * K_DataArrayProjectile;

// 方法
void* M_Camera_get_main, * M_Camera_WorldToScreenPoint;
void* M_Component_get_transform, * M_Transform_get_position, * M_Transform_get_lossyScale;
void* M_Transform_get_localToWorldMatrix;
void* M_Camera_get_targetTexture;
void* M_Renderer_get_bounds, * M_Renderer_get_bounds_Injected;
void* M_Camera_get_orthographic;
void* M_Time_set_timeScale;
void* M_Board_AddZombieInRow;
void* M_Coin_ScoreCoin;
void* M_Screen_get_width, * M_Screen_get_height;
void* M_Camera_get_allCameras = nullptr;
void* K_ScreenCls = nullptr;
float g_viewW = 0, g_viewH = 0;   // 交换链实际尺寸

// 偏移
int O_Z_Type, O_Z_Phase, O_Z_PosX, O_Z_PosY, O_Z_VelX, O_Z_Rect;
int O_Z_Body, O_Z_BodyMax, O_Z_Helm, O_Z_HelmMax, O_Z_Shield, O_Z_ShieldMax, O_Z_Flying, O_Z_FlyingMax;
int O_Z_Dead, O_Z_Controller, O_Z_MindCtrl, O_Z_Height;
int O_Z_Chilled, O_Z_IceTrap;        // 游戏自身冰冻/减速状态（寒冰菇同款）
int O_Base_Row, O_Base_X, O_Base_Y;
int O_P_Type, O_P_Col, O_P_State, O_P_Hp, O_P_HpMax, O_P_Rect, O_P_Dead, O_P_Asleep, O_P_Controller;
int O_C_PosX, O_C_PosY, O_C_Dead, O_C_Type, O_C_CollectX, O_C_CollectY;
int O_PR_Type, O_PR_PosX, O_PR_PosY, O_PR_VelX, O_PR_VelY, O_PR_VelZ, O_PR_Dead;
int O_PR_Motion, O_PR_Target;         // mMotionType / mTargetZombieID（猫尾草追踪弹机制）
int O_B_Zombies, O_B_Plants, O_B_Coins, O_B_Projectiles, O_B_SeedBank, O_B_App;
int O_B_NumWaves, O_B_CurrentWave, O_B_HealthNext, O_B_HealthStart, O_B_ZombieCD, O_B_HugeCD;
int O_B_FogBlown, O_B_GridFog, O_B_SunMoney;
int O_SeedBank_Packets, O_SeedBank_Num;
int O_SP_Refresh, O_SP_RefreshTime, O_SP_Type, O_SP_Active, O_SP_Refreshing;
int O_GA_Board, O_GA_EasyPlant;
int O_ZC_Renderer, O_PC_MeshRenderer;
int O_RCC_AnimationController, O_CAC_MeshRenderer, O_CAC_SkeletonAnimation;
int O_SR_Skeleton, O_SK_Bones, O_Bone_Data, O_Bone_Parent, O_Bone_WorldX, O_Bone_WorldY, O_Bone_Active;
int O_BoneData_Name;
int O_SK_R, O_SK_G, O_SK_B, O_SK_A;    // Spine.Skeleton 整体着色（= SetColor）
int O_Sun_Amount;
int O_MP_SunMain;                     // Board 内嵌 MultiplayerType<Sun>.m_mainValue
int O_SunMainAbs = -1;                // = O_B_SunMoney + O_MP_SunMain
int O_SunValuesAbs = -1;              // = O_B_SunMoney + off(m_values)


// DataArray 实例化布局
struct DAInfo { void* instClass; int offList, offCount; int itemOff, elemSize; bool ok; };
DAInfo DA_Zombie, DA_Plant, DA_Coin, DA_Proj;

bool resolve_field(int& out, void* klass, const char* name) {
    out = il2cpp::field_off(klass, name);
    if (out < 0) roh::log("[game] field missing: %s", name);
    return out >= 0;
}

bool build_da_info(DAInfo& info, void* boardFieldHolderClass, const char* boardFieldName,
                   void* elementClass) {
    memset(&info, 0, sizeof(info));
    info.instClass = il2cpp::field_type_class(boardFieldHolderClass, boardFieldName);
    if (!info.instClass) { roh::log("[game] DA inst class fail: %s", boardFieldName); return false; }
    if (!resolve_field(info.offList, info.instClass, "m_list")) return false;
    if (!resolve_field(info.offCount, info.instClass, "m_count")) return false;

    void* listArrClass = il2cpp::field_type_class(info.instClass, "m_list");
    if (!listArrClass || !il2cpp::class_get_element_class) { roh::log("[game] DA arr class fail"); return false; }
    void* elemClass = nullptr;
    __try { elemClass = il2cpp::class_get_element_class(listArrClass); } __except (1) {}
    if (!elemClass) { roh::log("[game] DA elem class fail"); return false; }
    uint32_t align = 0;
    __try { info.elemSize = il2cpp::class_value_size ? il2cpp::class_value_size(elemClass, &align) : 0; }
    __except (1) { info.elemSize = 0; }
    if (info.elemSize <= 0 || info.elemSize > 0x100) { roh::log("[game] DA elem size bad: %d", info.elemSize); return false; }

    // 找元素里类型 == elementClass 的字段（Item 引用）
    info.itemOff = -1;
    void* iter = nullptr; void* fi = nullptr;
    __try {
        while ((fi = il2cpp::class_get_fields(elemClass, &iter)) != nullptr) {
            void* ft = il2cpp::field_get_type(fi);
            void* ftc = ft ? il2cpp::class_from_type(ft) : nullptr;
            if (ftc == elementClass) {
                // Field offsets on a boxed value type include the 0x10 object
                // header. Array elements are unboxed and begin at data + i*size.
                info.itemOff = il2cpp::field_get_offset(fi) - 0x10;
                break;
            }
        }
    } __except (1) {}
    if (info.itemOff < 0 || info.itemOff + (int)sizeof(uint64_t) > info.elemSize) {
        roh::log("[game] DA item offset fail: %d / %d", info.itemOff, info.elemSize);
        return false;
    }
    info.ok = true;
    roh::log("[game] DA %s ok: list=0x%X count=0x%X item=0x%X elem=%d",
             boardFieldName, info.offList, info.offCount, info.itemOff, info.elemSize);
    return true;
}

// 日志辅助：打印类字段（调试泛型布局用）。固定缓冲，避免 C++ 对象 + __try 冲突。
static __declspec(noinline) int log_fields_append(void* klass, char* buf, size_t cap) {
    int n = 0;
    __try {
        void* iter = nullptr; void* fi = nullptr;
        while ((fi = il2cpp::class_get_fields(klass, &iter)) != nullptr &&
               (size_t)n + 2 < cap) {
            const char* fn = il2cpp::field_get_name(fi);
            if (!fn) break;
            n += _snprintf(buf + n, cap - n - 1, "%s@0x%X ", fn, il2cpp::field_get_offset(fi));
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    return n;
}

void log_fields(const char* tag, void* klass) {
    if (!klass || !il2cpp::class_get_fields) return;
    char buf[1024];
    int n = log_fields_append(klass, buf, sizeof(buf));
    buf[n] = 0;
    roh::log("[game] fields %s: %s", tag, buf);
}

} // namespace

bool init() {
    K_Board      = il2cpp::find_class("Reloaded.Gameplay", "Board");
    K_Zombie     = il2cpp::find_class("Reloaded.Gameplay", "Zombie");
    K_Plant      = il2cpp::find_class("Reloaded.Gameplay", "Plant");
    K_Coin       = il2cpp::find_class("Reloaded.Gameplay", "Coin");
    K_Projectile = il2cpp::find_class("Reloaded.Gameplay", "Projectile");
    K_SeedBank   = il2cpp::find_class("Reloaded.Gameplay", "SeedBank");
    K_SeedPacket = il2cpp::find_class("Reloaded.Gameplay", "SeedPacket");
    K_GameplayActivity = il2cpp::find_class("Reloaded.TreeStateActivities", "GameplayActivity");
    K_ZombieController = il2cpp::find_class("Source.Controllers", "ZombieController");
    K_PlantController  = il2cpp::find_class("Source.Controllers", "PlantController");
    K_ReloadedCharacterController = il2cpp::find_class("Source.Controllers", "ReloadedCharacterController");
    K_CharacterAnimationController = il2cpp::find_class("Reloaded.Characters", "CharacterAnimationController");
    K_SkeletonRenderer = il2cpp::find_class("Spine.Unity", "SkeletonRenderer");
    K_SpineSkeleton    = il2cpp::find_class("Spine", "Skeleton");
    K_SpineBone        = il2cpp::find_class("Spine", "Bone");
    K_SpineBoneData    = il2cpp::find_class("Spine", "BoneData");
    K_Transform  = il2cpp::find_class("UnityEngine", "Transform");
    K_Camera     = il2cpp::find_class("UnityEngine", "Camera");
    K_Time       = il2cpp::find_class("UnityEngine", "Time");
    K_Component  = il2cpp::find_class("UnityEngine", "Component");
    K_Renderer   = il2cpp::field_type_class(K_ZombieController, "m_renderer");
    roh::log("[projection] renderer field class=%p", K_Renderer);
    K_Sun        = il2cpp::find_class("Reloaded.Gameplay", "Sun");
    if (!K_Board || !K_Zombie || !K_Plant || !K_Coin) {
        roh::log("[game] core classes missing");
        return false;
    }

    bool ok = true;
    // Zombie
    ok &= resolve_field(O_Z_Type, K_Zombie, "mZombieType");
    ok &= resolve_field(O_Z_Phase, K_Zombie, "mZombiePhase");
    ok &= resolve_field(O_Z_PosX, K_Zombie, "mPosX");
    ok &= resolve_field(O_Z_PosY, K_Zombie, "mPosY");
    ok &= resolve_field(O_Z_VelX, K_Zombie, "mVelX");
    ok &= resolve_field(O_Z_Rect, K_Zombie, "mZombieRect");
    ok &= resolve_field(O_Z_Body, K_Zombie, "mBodyHealth");
    ok &= resolve_field(O_Z_BodyMax, K_Zombie, "mBodyMaxHealth");
    ok &= resolve_field(O_Z_Helm, K_Zombie, "mHelmHealth");
    ok &= resolve_field(O_Z_HelmMax, K_Zombie, "mHelmMaxHealth");
    ok &= resolve_field(O_Z_Shield, K_Zombie, "mShieldHealth");
    ok &= resolve_field(O_Z_ShieldMax, K_Zombie, "mShieldMaxHealth");
    ok &= resolve_field(O_Z_Flying, K_Zombie, "mFlyingHealth");
    ok &= resolve_field(O_Z_FlyingMax, K_Zombie, "mFlyingMaxHealth");
    ok &= resolve_field(O_Z_Dead, K_Zombie, "mDead");
    ok &= resolve_field(O_Z_Controller, K_Zombie, "mController");
    ok &= resolve_field(O_Z_MindCtrl, K_Zombie, "mMindControlled");
    resolve_field(O_Z_Chilled, K_Zombie, "mChilledCounter");
    resolve_field(O_Z_IceTrap, K_Zombie, "mIceTrapCounter");
    // 基类 ReloadedObject
    resolve_field(O_Base_Row, K_Zombie, "mRow");       // 非致命
    resolve_field(O_Base_X, K_Zombie, "mX");
    resolve_field(O_Base_Y, K_Zombie, "mY");
    // Plant
    ok &= resolve_field(O_P_Type, K_Plant, "mSeedType");
    ok &= resolve_field(O_P_Col, K_Plant, "mPlantCol");
    ok &= resolve_field(O_P_State, K_Plant, "mState");
    ok &= resolve_field(O_P_Hp, K_Plant, "mPlantHealth");
    ok &= resolve_field(O_P_HpMax, K_Plant, "mPlantMaxHealth");
    ok &= resolve_field(O_P_Rect, K_Plant, "mPlantRect");
    ok &= resolve_field(O_P_Dead, K_Plant, "mDead");
    resolve_field(O_P_Asleep, K_Plant, "mIsAsleep");
    // Coin
    resolve_field(O_C_PosX, K_Coin, "mPosX");
    resolve_field(O_C_PosY, K_Coin, "mPosY");
    resolve_field(O_C_Dead, K_Coin, "mDead");
    resolve_field(O_C_Type, K_Coin, "mType");
    resolve_field(O_C_CollectX, K_Coin, "mCollectX");
    resolve_field(O_C_CollectY, K_Coin, "mCollectY");
    // Projectile
    resolve_field(O_PR_Type, K_Projectile, "mProjectileType");
    resolve_field(O_PR_PosX, K_Projectile, "mPosX");
    resolve_field(O_PR_PosY, K_Projectile, "mPosY");
    resolve_field(O_PR_VelX, K_Projectile, "mVelX");
    resolve_field(O_PR_VelY, K_Projectile, "mVelY");
    resolve_field(O_PR_VelZ, K_Projectile, "mVelZ");
    resolve_field(O_PR_Dead, K_Projectile, "mDead");
    resolve_field(O_PR_Motion, K_Projectile, "mMotionType");
    resolve_field(O_PR_Target, K_Projectile, "mTargetZombieID");
    // Board
    ok &= resolve_field(O_B_Zombies, K_Board, "m_zombies");
    ok &= resolve_field(O_B_Plants, K_Board, "m_plants");
    ok &= resolve_field(O_B_Coins, K_Board, "m_coins");
    resolve_field(O_B_Projectiles, K_Board, "m_projectiles");
    resolve_field(O_B_SeedBank, K_Board, "mSeedBank");
    resolve_field(O_B_App, K_Board, "mApp");
    resolve_field(O_B_NumWaves, K_Board, "mNumWaves");
    resolve_field(O_B_CurrentWave, K_Board, "mCurrentWave");
    resolve_field(O_B_HealthNext, K_Board, "mZombieHealthToNextWave");
    resolve_field(O_B_HealthStart, K_Board, "mZombieHealthWaveStart");
    resolve_field(O_B_ZombieCD, K_Board, "mZombieCountDown");
    resolve_field(O_B_HugeCD, K_Board, "mHugeWaveCountDown");
    resolve_field(O_B_FogBlown, K_Board, "mFogBlownCountDown");
    resolve_field(O_B_GridFog, K_Board, "mGridCelFog");
    resolve_field(O_B_SunMoney, K_Board, "mSunMoney");
    // SeedBank / SeedPacket
    resolve_field(O_SeedBank_Packets, K_SeedBank, "mSeedPackets");
    resolve_field(O_SeedBank_Num, K_SeedBank, "mNumPackets");
    resolve_field(O_SP_Refresh, K_SeedPacket, "mRefreshCounter");
    resolve_field(O_SP_RefreshTime, K_SeedPacket, "mRefreshTime");
    resolve_field(O_SP_Type, K_SeedPacket, "mPacketType");
    resolve_field(O_SP_Active, K_SeedPacket, "mActive");
    resolve_field(O_SP_Refreshing, K_SeedPacket, "mRefreshing");
    // GameplayActivity
    resolve_field(O_GA_Board, K_GameplayActivity, "m_board");
    resolve_field(O_GA_EasyPlant, K_GameplayActivity, "<EasyPlantingCheat>k__BackingField");
    // ZombieController / Spine
    resolve_field(O_ZC_Renderer, K_ZombieController, "m_renderer");
    resolve_field(O_PC_MeshRenderer, K_PlantController, "m_meshRenderer");
    resolve_field(O_RCC_AnimationController, K_ReloadedCharacterController, "m_animationController");
    resolve_field(O_CAC_MeshRenderer, K_CharacterAnimationController, "m_meshRenderer");
    resolve_field(O_CAC_SkeletonAnimation, K_CharacterAnimationController, "m_skeletonAnimation");
    resolve_field(O_P_Controller, K_Plant, "mController");
    resolve_field(O_SR_Skeleton, K_SkeletonRenderer, "skeleton");
    resolve_field(O_SK_Bones, K_SpineSkeleton, "bones");
    resolve_field(O_Bone_Data, K_SpineBone, "data");
    resolve_field(O_Bone_Parent, K_SpineBone, "parent");
    resolve_field(O_Bone_WorldX, K_SpineBone, "worldX");
    resolve_field(O_Bone_WorldY, K_SpineBone, "worldY");
    resolve_field(O_Bone_Active, K_SpineBone, "active");
    resolve_field(O_BoneData_Name, K_SpineBoneData, "name");
    resolve_field(O_SK_R, K_SpineSkeleton, "r");
    resolve_field(O_SK_G, K_SpineSkeleton, "g");
    resolve_field(O_SK_B, K_SpineSkeleton, "b");
    resolve_field(O_SK_A, K_SpineSkeleton, "a");
    roh::log("[chams] skeleton rgba offs: %X %X %X %X", O_SK_R, O_SK_G, O_SK_B, O_SK_A);
    resolve_field(O_Sun_Amount, K_Sun, "Amount");

    // 方法
    if (K_Camera && il2cpp::class_get_method_from_name) {
        __try {
            M_Camera_get_main = il2cpp::class_get_method_from_name(K_Camera, "get_main", 0);
            M_Camera_WorldToScreenPoint = il2cpp::class_get_method_from_name(K_Camera, "WorldToScreenPoint", 1);
            M_Component_get_transform = il2cpp::class_get_method_from_name(K_Component, "get_transform", 0);
            M_Transform_get_position = il2cpp::class_get_method_from_name(K_Transform, "get_position", 0);
            M_Transform_get_lossyScale = il2cpp::class_get_method_from_name(K_Transform, "get_lossyScale", 0);
            M_Transform_get_localToWorldMatrix = il2cpp::class_get_method_from_name(K_Transform, "get_localToWorldMatrix", 0);
            for (void* k = K_Renderer; k && !M_Renderer_get_bounds; k = il2cpp::class_get_parent(k))
                M_Renderer_get_bounds = il2cpp::class_get_method_from_name(k, "get_bounds", 0);
            for (void* k = K_Renderer; k && !M_Renderer_get_bounds_Injected; k = il2cpp::class_get_parent(k))
                M_Renderer_get_bounds_Injected = il2cpp::class_get_method_from_name(k, "get_bounds_Injected", 1);
            M_Camera_get_orthographic = il2cpp::class_get_method_from_name(K_Camera, "get_orthographic", 0);
            M_Camera_get_targetTexture = il2cpp::class_get_method_from_name(K_Camera, "get_targetTexture", 0);
            M_Time_set_timeScale = il2cpp::class_get_method_from_name(K_Time, "set_timeScale", 1);
            M_Board_AddZombieInRow = il2cpp::class_get_method_from_name(K_Board, "AddZombieInRow", 4);
            if (K_Coin)
                M_Coin_ScoreCoin = il2cpp::class_get_method_from_name(K_Coin, "ScoreCoin", 0);
            M_Camera_get_allCameras = il2cpp::class_get_method_from_name(K_Camera, "get_allCameras", 0);
            void* K_Screen = il2cpp::find_class("UnityEngine", "Screen");
            if (K_Screen) {
                M_Screen_get_width = il2cpp::class_get_method_from_name(K_Screen, "get_width", 0);
                M_Screen_get_height = il2cpp::class_get_method_from_name(K_Screen, "get_height", 0);
            }
        } __except (1) {}
        roh::log("[game] methods: main=%p w2s=%p gt=%p gp=%p ls=%p ts=%p az=%p",
                 M_Camera_get_main, M_Camera_WorldToScreenPoint, M_Component_get_transform,
                 M_Transform_get_position, M_Transform_get_lossyScale,
                 M_Time_set_timeScale, M_Board_AddZombieInRow);
        roh::log("[projection] methods: ortho=%p target=%p localToWorld=%p",
                 M_Camera_get_orthographic, M_Camera_get_targetTexture,
                 M_Transform_get_localToWorldMatrix);
        roh::log("[projection] renderer methods: bounds=%p injected=%p offsets: anim=0x%X mesh=0x%X skeleton=0x%X",
                 M_Renderer_get_bounds, M_Renderer_get_bounds_Injected,
                 O_RCC_AnimationController, O_CAC_MeshRenderer, O_CAC_SkeletonAnimation);
    }

    // DataArray 布局
    build_da_info(DA_Zombie, K_Board, "m_zombies", K_Zombie);
    build_da_info(DA_Plant, K_Board, "m_plants", K_Plant);
    build_da_info(DA_Coin, K_Board, "m_coins", K_Coin);
    build_da_info(DA_Proj, K_Board, "m_projectiles", K_Projectile);

    // Sun 内嵌偏移：Board.mSunMoney 是 MultiplayerType<Sun> 结构体字段
    // 注意：值类型嵌入字段要去掉 0x10 对象头（embedded = structFieldOffset - 0x10）
    if (O_B_SunMoney >= 0 && K_Sun) {
        void* mp = il2cpp::field_type_class(K_Board, "mSunMoney");
        if (mp) {
            log_fields("MultiplayerType<Sun>", mp);
            int mainOff = il2cpp::field_off(mp, "m_mainValue");
            if (mainOff >= 0x10) O_SunMainAbs = O_B_SunMoney + mainOff - 0x10;
            int valsOff = il2cpp::field_off(mp, "m_values");
            if (valsOff >= 0x10) O_SunValuesAbs = O_B_SunMoney + valsOff - 0x10;
            roh::log("[game] sun embed offs: main=0x%X vals=0x%X (raw %d/%d)",
                     O_SunMainAbs, O_SunValuesAbs, mainOff, valsOff);
        }
    }

    // Spine ExposedList<Bone> 布局（按类型匹配：Items 是 Bone[]）
    if (K_SpineSkeleton && O_SK_Bones >= 0) {
        void* el = il2cpp::field_type_class(K_SpineSkeleton, "bones");
        if (el) {
            log_fields("ExposedList<Bone>", el);
            void* iter = nullptr; void* fi = nullptr;
            __try {
                while ((fi = il2cpp::class_get_fields(el, &iter)) != nullptr) {
                    void* ft = il2cpp::field_get_type(fi);
                    void* ftc = ft ? il2cpp::class_from_type(ft) : nullptr;
                    const char* fn = il2cpp::field_get_name(fi);
                    if (!fn) break;
                    int fo = il2cpp::field_get_offset(fi);
                    if (strcmp(fn, "Items") == 0 && ftc && il2cpp::class_get_element_class) {
                        void* elem = nullptr;
                        __try { elem = il2cpp::class_get_element_class(ftc); } __except (1) {}
                        if (elem == K_SpineBone) g_offExListItems = fo;
                    }
                    if (strcmp(fn, "Count") == 0) g_offExListCount = fo;
                }
            } __except (1) {}
        }
    }

    if (!ok) roh::log("[game] init PARTIAL (some fields missing)");
    else roh::log("[game] init ok");
    return ok && DA_Zombie.ok && DA_Plant.ok && DA_Coin.ok;
}

// ================= 运行时辅助 =================
namespace {

inline uint64_t klass_of(uint64_t obj) { return roh::safe_read_ptr(obj); }

// 调用方法返回对象（obj 可为 null = 静态方法）
__declspec(noinline) uint64_t invoke_get_obj(void* method, uint64_t obj) {
    if (!method) return 0;
    void* exc = nullptr; void* r = nullptr;
    __try {
        r = il2cpp::runtime_invoke(method, (void*)obj, nullptr, &exc);
        if (exc) return 0;
    } __except (1) { return 0; }
    return (uint64_t)r;
}
__declspec(noinline) bool is_orthographic(uint64_t camera) {
    uint64_t boxed = invoke_get_obj(M_Camera_get_orthographic, camera);
    if (!boxed || !il2cpp::runtime_object_unbox) return false;
    __try {
        void* data = il2cpp::runtime_object_unbox((void*)boxed);
        bool value = false;
        return data && roh::safe_read((uint64_t)data, value) && value;
    } __except (1) { return false; }
}
__declspec(noinline) bool unbox_matrix(uint64_t boxed, float* matrix) {
    if (!boxed || !il2cpp::runtime_object_unbox) return false;
    __try {
        void* data = il2cpp::runtime_object_unbox((void*)boxed);
        return data && roh::safe_readbuf((uint64_t)data, matrix, 16 * sizeof(float));
    } __except (1) { return false; }
}
// 调用无参方法返回 Vector3
__declspec(noinline) bool invoke_get_vec3(void* method, uint64_t obj, float* v3) {
    uint64_t r = invoke_get_obj(method, obj);
    if (!r || !il2cpp::runtime_object_unbox) return false;
    void* d = nullptr;
    __try { d = il2cpp::runtime_object_unbox((void*)r); } __except (1) { return false; }
    if (!d) return false;
    return roh::safe_readbuf((uint64_t)d, v3, sizeof(float) * 3);
}
// UnityEngine.Bounds is two consecutive Vector3 values: center and extents.
__declspec(noinline) bool invoke_get_bounds(uint64_t renderer, float* center, float* extents) {
    if (!renderer || !il2cpp::runtime_invoke) return false;
    float values[6]{};
    if (M_Renderer_get_bounds && il2cpp::runtime_object_unbox) {
        uint64_t boxed = invoke_get_obj(M_Renderer_get_bounds, renderer);
        if (!boxed) return false;
        __try {
            void* data = il2cpp::runtime_object_unbox((void*)boxed);
            if (!data || !roh::safe_readbuf((uint64_t)data, values, sizeof(values))) return false;
        } __except (1) { return false; }
    } else if (M_Renderer_get_bounds_Injected) {
        void* args[1] = { values };
        void* exc = nullptr;
        __try {
            il2cpp::runtime_invoke(M_Renderer_get_bounds_Injected, (void*)renderer, args, &exc);
            if (exc) return false;
        } __except (1) { return false; }
    } else return false;
    for (int i = 0; i < 3; ++i) {
        if (!std::isfinite(values[i]) || !std::isfinite(values[3 + i]) ||
            values[3 + i] < 0 || values[3 + i] > 100000) return false;
        center[i] = values[i]; extents[i] = values[3 + i];
    }
    return extents[0] + extents[1] + extents[2] > 0.1f;
}
// 相机 WorldToScreenPoint（自动做 Unity 虚拟分辨率 → 交换链分辨率换算）
static float g_virtW = 0, g_virtH = 0;
__declspec(noinline) void read_virtual_res() {
    if (!M_Screen_get_width || !M_Screen_get_height) return;
    uint64_t w = invoke_get_obj(M_Screen_get_width, 0);
    uint64_t h = invoke_get_obj(M_Screen_get_height, 0);
    if (!w || !h || !il2cpp::runtime_object_unbox) return;
    int vw = 0, vh = 0;
    __try {
        vw = *(int*)il2cpp::runtime_object_unbox((void*)w);
        vh = *(int*)il2cpp::runtime_object_unbox((void*)h);
    } __except (1) { return; }
    if (vw > 100 && vw < 20000 && vh > 100 && vh < 20000) {
        if (vw != (int)g_virtW || vh != (int)g_virtH) {
            roh::log("[game] unity virtual res: %dx%d (swapchain %.0fx%.0f)", vw, vh, g_viewW, g_viewH);
        }
        g_virtW = (float)vw; g_virtH = (float)vh;
    }
}
__declspec(noinline) bool w2s_raw(uint64_t camera, float wx, float wy, float wz, float* sx, float* sy) {
    if (!camera || !M_Camera_WorldToScreenPoint) return false;
    float v[3] = { wx, wy, wz };
    void* params[1] = { v };
    void* exc = nullptr; void* r = nullptr;
    __try {
        r = il2cpp::runtime_invoke(M_Camera_WorldToScreenPoint, (void*)camera, params, &exc);
        if (exc || !r) return false;
        void* d = il2cpp::runtime_object_unbox(r);
        if (!d) return false;
        bool ok1 = roh::safe_read((uint64_t)d, *sx);
        bool ok2 = roh::safe_read((uint64_t)d + 4, *sy);
        if (!ok1 || !ok2) return false;
        // Unity 虚拟分辨率 → 交换链分辨率
        return true;
    } __except (1) { return false; }
}

// 带虚拟分辨率换算的 w2s（对原始结果做缩放）
__declspec(noinline) bool w2s(uint64_t camera, float wx, float wy, float wz, float* sx, float* sy) {
    if (!w2s_raw(camera, wx, wy, wz, sx, sy)) return false;
    return projection::screen_to_overlay(*sx, *sy, g_virtW, g_virtH, g_viewW, g_viewH);
}

// 相机 2x2 仿射：屏幕点 = o + W*right + H*up（right/up 是每世界单位的屏幕增量向量）
// 自动处理任何轴向翻转（Unity y-up / D3D y-down / 摄像机镜像）
struct CamAffine {
    bool ok = false;
    float ox, oy;      // 世界原点的屏幕位置
    float rx, ry;      // +1 世界 X 的屏幕增量
    float ux, uy;      // +1 世界 Y 的屏幕增量
    float zx = 0, zy = 0; // +1 world Z
    float pxPerUnit;   // 像素/世界单位
};
float g_zwSample[4][2] = {{0}};   // 最近快照的僵尸世界坐标样本
int g_zwSampleN = 0;
CamAffine g_cam;

// 对候选相机打分：能把僵尸世界坐标投影进屏幕内的得分高
static int score_camera(uint64_t cam) {
    if (!cam || !is_orthographic(cam) || invoke_get_obj(M_Camera_get_targetTexture, cam)) return -1;
    float x0, y0, x1, y1, x2, y2;
    if (!w2s(cam, 0, 0, 0, &x0, &y0)) return -1;
    if (!w2s(cam, 100, 0, 0, &x1, &y1)) return -1;
    if (!w2s(cam, 0, 100, 0, &x2, &y2)) return -1;
    float rx = (x1 - x0) / 100.0f, ry = (y1 - y0) / 100.0f;
    float ux = (x2 - x0) / 100.0f, uy = (y2 - y0) / 100.0f;
    float slope = sqrtf(rx * rx + ry * ry);
    if (slope < 0.05f || slope > 20.0f) return -1;   // 斜率离谱
    int score = 0;
    float vw = g_viewW > 1 ? g_viewW : 2560.0f;
    float vh = g_viewH > 1 ? g_viewH : 1360.0f;
    for (int i = 0; i < g_zwSampleN; i++) {
        float sx = x0 + g_zwSample[i][0] * rx + g_zwSample[i][1] * ux;
        float sy = y0 + g_zwSample[i][0] * ry + g_zwSample[i][1] * uy;
        if (sx > -50 && sx < vw + 50 && sy > -50 && sy < vh + 50) score++;
    }
    return score;
}

void calibrate_camera() {
    g_cam.ok = false;
    // 收集候选相机：allCameras 数组 + Camera.main
    uint64_t cams[16]; int nCams = 0;
    if (M_Camera_get_allCameras) {
        uint64_t arr = invoke_get_obj(M_Camera_get_allCameras, 0);
        if (arr) {
            uint32_t n = il2cpp::array_len(arr);
            if (n > 16) n = 16;
            for (uint32_t i = 0; i < n; i++) {
                uint64_t c = roh::safe_read_ptr(il2cpp::array_data(arr) + (uint64_t)i * 8);
                if (c) cams[nCams++] = c;
            }
        }
    }
    uint64_t mainCam = invoke_get_obj(M_Camera_get_main, 0);
    if (mainCam) {
        bool dup = false;
        for (int i = 0; i < nCams; i++) if (cams[i] == mainCam) dup = true;
        if (!dup && nCams < 16) cams[nCams++] = mainCam;
    }
    if (nCams == 0) return;

    // 打分选最优
    int bestScore = -1; int bestIdx = -1;
    for (int i = 0; i < nCams; i++) {
        int sc = score_camera(cams[i]);
        if (sc > bestScore) { bestScore = sc; bestIdx = i; }
    }
    if (bestIdx < 0) {
        static bool warned = false;
        if (!warned) roh::log("[projection] no supported direct-screen orthographic camera (check targetTexture / projection mode)");
        warned = true;
        return;
    }
    uint64_t cam = cams[bestIdx];
    // A render texture can be composited anywhere. Do not silently treat it as the screen.
    if (invoke_get_obj(M_Camera_get_targetTexture, cam)) {
        static bool warned = false;
        if (!warned) roh::log("[projection] camera targetTexture detected; final compositing mapping required, ESP disabled");
        warned = true;
        return;
    }

    float x0, y0, x1, y1, x2, y2;
    if (!w2s(cam, 0, 0, 0, &x0, &y0)) return;
    if (!w2s(cam, 100, 0, 0, &x1, &y1)) return;
    if (!w2s(cam, 0, 100, 0, &x2, &y2)) return;
    g_cam.rx = (x1 - x0) / 100.0f;
    g_cam.ry = (y1 - y0) / 100.0f;
    g_cam.ux = (x2 - x0) / 100.0f;
    g_cam.uy = (y2 - y0) / 100.0f;
    g_cam.pxPerUnit = sqrtf(g_cam.rx * g_cam.rx + g_cam.ry * g_cam.ry);
    if (g_cam.pxPerUnit < 0.001f || g_cam.pxPerUnit > 100.0f) return;
    float upLen = sqrtf(g_cam.ux * g_cam.ux + g_cam.uy * g_cam.uy);
    if (upLen < 0.001f || upLen > 100.0f) return;
    float xz, yz;
    if (!w2s(cam, 0, 0, 100, &xz, &yz)) return;
    g_cam.zx = (xz - x0) / 100.0f;
    g_cam.zy = (yz - y0) / 100.0f;
    g_cam.ox = x0; g_cam.oy = y0;
    g_cam.ok = true;
    static uint64_t lastChosen = 0;
    static int n = 0;
    if (cam != lastChosen || ++n % 60 == 1) {
        roh::log("[game] cam chosen %llx (score %d/%d of %d cams) o=(%.1f,%.1f) right=(%.3f,%.3f) up=(%.3f,%.3f)",
                 (unsigned long long)cam, bestScore, g_zwSampleN, nCams, x0, y0, g_cam.rx, g_cam.ry, g_cam.ux, g_cam.uy);
        lastChosen = cam;
    }
}

inline void world_to_screen(float wx, float wy, float* sx, float* sy, float wz = 0) {
    *sx = g_cam.ox + wx * g_cam.rx + wy * g_cam.ux + wz * g_cam.zx;
    *sy = g_cam.oy + wx * g_cam.ry + wy * g_cam.uy + wz * g_cam.zy;
}

uint64_t visual_renderer(uint64_t controller, bool plant) {
    if (!controller) return 0;
    if (O_RCC_AnimationController >= 0 && O_CAC_MeshRenderer >= 0) {
        uint64_t anim = roh::safe_read_ptr(controller + O_RCC_AnimationController);
        uint64_t mesh = anim ? roh::safe_read_ptr(anim + O_CAC_MeshRenderer) : 0;
        if (mesh) return mesh;
    }
    int field = plant ? O_PC_MeshRenderer : O_ZC_Renderer;
    return field >= 0 ? roh::safe_read_ptr(controller + field) : 0;
}

bool renderer_box(uint64_t renderer, float* box, float* anchor) {
    if (!g_cam.ok) return false;
    float center[3]{}, extents[3]{};
    if (!invoke_get_bounds(renderer, center, extents)) return false;
    world_to_screen(center[0], center[1], &anchor[0], &anchor[1], center[2]);
    const float halfW = fabsf(g_cam.rx) * extents[0] + fabsf(g_cam.ux) * extents[1] + fabsf(g_cam.zx) * extents[2];
    const float halfH = fabsf(g_cam.ry) * extents[0] + fabsf(g_cam.uy) * extents[1] + fabsf(g_cam.zy) * extents[2];
    if (!std::isfinite(halfW) || !std::isfinite(halfH) || halfW < 2 || halfH < 2 ||
        halfW > g_viewW || halfH > g_viewH) return false;
    box[0] = anchor[0] - halfW - 4;
    box[1] = anchor[1] - halfH - 4;
    box[2] = anchor[0] + halfW + 4;
    box[3] = anchor[1] + halfH + 4;
    return true;
}

bool is_visual_bone(const char* name) {
    if (!name[0] || strcmp(name, "root") == 0 || strcmp(name, "_ground") == 0)
        return false;
    // Animation-state markers sit outside the sprite and must not expand its box.
    static const char* markers[] = {
        "anim_dance", "anim_waterdeath", "anim_eat", "anim_idle",
        "anim_walk", "anim_death", "anim_swim", "anim_superlongdeath"
    };
    for (const char* marker : markers) {
        size_t len = strlen(marker);
        if (strncmp(name, marker, len) == 0 &&
            (name[len] == '\0' || (name[len] >= '0' && name[len] <= '9')))
            return false;
    }
    return true;
}

bool skeleton_box(uint64_t controller, float* box, float* anchor,
                  std::vector<BoneSnap>* outputBones, float* head) {
    if (!controller || !g_cam.ok || O_RCC_AnimationController < 0 ||
        O_CAC_SkeletonAnimation < 0 || O_SR_Skeleton < 0 || O_SK_Bones < 0 ||
        g_offExListItems < 0 || g_offExListCount < 0) return false;
    uint64_t animation = roh::safe_read_ptr(controller + O_RCC_AnimationController);
    uint64_t spine = animation ? roh::safe_read_ptr(animation + O_CAC_SkeletonAnimation) : 0;
    uint64_t skeleton = spine ? roh::safe_read_ptr(spine + O_SR_Skeleton) : 0;
    uint64_t list = skeleton ? roh::safe_read_ptr(skeleton + O_SK_Bones) : 0;
    uint64_t items = list ? roh::safe_read_ptr(list + g_offExListItems) : 0;
    int count = 0;
    if (list) roh::safe_read(list + g_offExListCount, count);
    uint32_t length = items ? il2cpp::array_len(items) : 0;
    uint64_t transform = spine ? invoke_get_obj(M_Component_get_transform, spine) : 0;
    uint64_t boxedMatrix = transform ? invoke_get_obj(M_Transform_get_localToWorldMatrix, transform) : 0;
    float matrix[16]{};
    bool haveMatrix = unbox_matrix(boxedMatrix, matrix);
    if (!haveMatrix || count < 1 || count > 300 || (uint32_t)count > length) return false;

    float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;
    int visualCount = 0;
    if (outputBones) { outputBones->clear(); outputBones->reserve(count); }
    for (int i = 0; i < count; ++i) {
        uint64_t bone = roh::safe_read_ptr(il2cpp::array_data(items) + (uint64_t)i * 8);
        bool active = true;
        if (bone && O_Bone_Active >= 0) roh::safe_read(bone + O_Bone_Active, active);
        if (!active) continue;
        float localX = 0, localY = 0, world[3]{};
        if (!bone || !roh::safe_read(bone + O_Bone_WorldX, localX) ||
            !roh::safe_read(bone + O_Bone_WorldY, localY) ||
            !projection::transform_bone(matrix, localX, localY, world)) return false;
        float sx = 0, sy = 0;
        world_to_screen(world[0], world[1], &sx, &sy, world[2]);
        if (!std::isfinite(sx) || !std::isfinite(sy)) return false;
        BoneSnap snap{};
        snap.x = sx; snap.y = sy;
        uint64_t data = O_Bone_Data >= 0 ? roh::safe_read_ptr(bone + O_Bone_Data) : 0;
        uint64_t str = data && O_BoneData_Name >= 0 ? roh::safe_read_ptr(data + O_BoneData_Name) : 0;
        int nameLen = 0;
        if (str && roh::safe_read(str + 0x10, nameLen) && nameLen > 0 && nameLen < (int)sizeof(snap.name)) {
            wchar_t wide[sizeof(snap.name)]{};
            if (roh::safe_readbuf(str + 0x14, wide, nameLen * sizeof(wchar_t))) {
                for (int j = 0; j < nameLen; ++j)
                    snap.name[j] = wide[j] >= 32 && wide[j] < 127 ?
                        (char)tolower((unsigned char)wide[j]) : '?';
            }
        }
        if (!is_visual_bone(snap.name)) continue;
        visualCount++;
        minX = min(minX, sx); minY = min(minY, sy);
        maxX = max(maxX, sx); maxY = max(maxY, sy);
        snap.isHead = strstr(snap.name, "head") != nullptr;
        if (head && snap.isHead &&
            (!std::isfinite(head[0]) || strcmp(snap.name, "anim_head1") == 0)) {
            head[0] = sx; head[1] = sy;
        }
        if (outputBones) outputBones->push_back(snap);
    }
    const float pad = 12.0f;
    if (visualCount < 2 || maxX - minX < 4 || maxY - minY < 4 ||
        maxX - minX > g_viewW * 2 || maxY - minY > g_viewH * 2) return false;
    box[0] = minX - pad; box[1] = minY - pad;
    box[2] = maxX + pad; box[3] = maxY + pad;
    anchor[0] = (minX + maxX) * 0.5f;
    anchor[1] = (minY + maxY) * 0.5f;
    return true;
}


// 板面(board)→世界 仿射：由两个僵尸样本推导
struct BoardAffine { bool ok = false; bool scaleCalibrated = false;
                      float wx=0, wy=0, bx=0, by=0, kx=2.82f, ky=-2.82f; };
BoardAffine g_board;
inline void board_to_world(float bx, float by, float* wx, float* wy) {
    *wx = g_board.wx + (bx - g_board.bx) * g_board.kx;
    *wy = g_board.wy + (by - g_board.by) * g_board.ky;
}

// 取 Sun 对象：mSunMoney 是指向 MultiplayerType<Sun> 对象的【引用】（探针实测），
// 解引用后在其槽位里找 klass==Sun
static uint64_t get_sun_obj(uint64_t board) {
    if (!board || O_B_SunMoney < 0) return 0;
    uint64_t mt = roh::safe_read_ptr(board + O_B_SunMoney);
    if (mt < 0x10000 || mt > 0x7FFFFFFFFFFF) return 0;
    // MT 对象内部槽位（m_values@0x10 / m_mainValue@0x18 附近都扫一遍）
    for (int off = 0x10; off <= 0x28; off += 8) {
        uint64_t cand = roh::safe_read_ptr(mt + off);
        if (cand > 0x10000 && cand < 0x7FFFFFFFFFFF && klass_of(cand) == (uint64_t)K_Sun)
            return cand;
    }
    // 数组槽位展开
    for (int off = 0x10; off <= 0x28; off += 8) {
        uint64_t arr = roh::safe_read_ptr(mt + off);
        if (arr < 0x10000 || arr > 0x7FFFFFFFFFFF) continue;
        uint32_t n = il2cpp::array_len(arr);
        if (n == 0 || n > 16) continue;
        for (uint32_t i = 0; i < n; i++) {
            uint64_t e = roh::safe_read_ptr(il2cpp::array_data(arr) + (uint64_t)i * 8);
            if (e > 0x10000 && e < 0x7FFFFFFFFFFF && klass_of(e) == (uint64_t)K_Sun)
                return e;
        }
    }
    return 0;
}

__declspec(noinline) bool seh_is_assignable(void* base, void* k) {
    __try { return il2cpp::class_is_assignable_from(base, k); }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

// ================= Board 获取（内存区域扫描：找 klass==Board 且字段合理的对象） =================
uint64_t g_boardObj = 0;
uint64_t g_lastScanMs = 0;

// 关卡重开后旧 Board 仍可能留在 GC 堆里。它的 DataArray 对象还在，
// 但 m_list 已释放；仅检查 Board/DataArray 的 klass 会锁定到旧关卡。
static bool live_board_arrays(uint64_t board) {
    bool hasList = false;
    const DAInfo* infos[2] = { &DA_Zombie, &DA_Plant };
    const int offsets[2] = { O_B_Zombies, O_B_Plants };
    for (int i = 0; i < 2; ++i) {
        const DAInfo& info = *infos[i];
        if (!info.ok || offsets[i] < 0) return false;
        uint64_t da = roh::safe_read_ptr(board + offsets[i]);
        if (!da || klass_of(da) != (uint64_t)info.instClass) return false;
        int count = -1;
        if (!roh::safe_read(da + info.offCount, count) || count < 0 || count > 10000) return false;
        uint64_t arr = roh::safe_read_ptr(da + info.offList);
        if (count > 0 && !arr) return false;
        if (arr) {
            uint32_t length = il2cpp::array_len(arr);
            if (!length || length > 100000 || (uint32_t)count > length) return false;
            hasList = true;
        }
    }
    return hasList;
}

// 强校验：类型、波次、以及仍持有实体数组的活动 Board。
static bool validate_board_obj(uint64_t a) {
    if (klass_of(a) != (uint64_t)K_Board) return false;
    if (O_B_NumWaves < 0 || O_B_Zombies < 0) return false;
    int waves = 0;
    if (!roh::safe_read(a + O_B_NumWaves, waves)) return false;
    if (waves < 0 || waves > 1000) return false;
    if (!live_board_arrays(a)) return false;
    uint64_t app = roh::safe_read_ptr(a + (O_B_App >= 0 ? O_B_App : 0x310));
    if (app < 0x10000 || app > 0x7FFFFFFFFFFF) return false;
    return true;
}

// 分块扫描：在 [begin, begin+len) 里找 klass 指针；返回 Board 对象地址或 0
static __declspec(noinline) uint64_t scan_chunk_for_board(uint64_t begin, size_t len, uint64_t klass) {
    __try {
        for (uint64_t a = begin; a + 0x400 <= begin + len; a += 8) {
            if (*(uint64_t*)a != klass) continue;
            int waves = *(int*)(a + O_B_NumWaves);
            if (waves < 0 || waves > 1000) continue;
            if (!live_board_arrays(a)) continue;
            return a;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    return 0;
}

// 增量扫描器：每帧最多花 kScanBudgetMs，跨帧续扫，绝不卡帧
struct ScannerState {
    bool active = false;
    uint64_t addr = 0;
    uint64_t maxAddr = 0;
    ULONGLONG nextStart = 0;
    int regions = 0;
};
static ScannerState g_scan;
static const ULONGLONG kScanBudgetMs = 3;
static const ULONGLONG kScanRestartDelayMs = 2500;

// 扫一批（受时间预算限制）；返回 Board 地址或 0（0=还没找到，继续下帧）
static __declspec(noinline) uint64_t scan_step() {
    if (!K_Board || O_B_NumWaves < 0 || O_B_Zombies < 0) return 0;
    ULONGLONG t0 = GetTickCount64();
    MEMORY_BASIC_INFORMATION mbi;
    uint64_t addr = g_scan.addr;
    while (addr < g_scan.maxAddr) {
        if (VirtualQuery((void*)addr, &mbi, sizeof(mbi)) == 0) { addr = g_scan.maxAddr; break; }
        uint64_t base = (uint64_t)mbi.BaseAddress;
        uint64_t size = (uint64_t)mbi.RegionSize;
        bool readable = mbi.State == MEM_COMMIT &&
            (mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE)) != 0 &&
            (mbi.Protect & PAGE_GUARD) == 0;
        // 只扫 >=1MB 的私有 RW 区域（il2cpp GC 堆段），跳过模块小节
        if (readable && size >= 0x100000 && size <= 0x20000000) {
            g_scan.regions++;
            for (uint64_t off = base >= addr ? 0 : (addr - base); off + 8 <= size; off += 0x100000) {
                uint64_t chunk = base + off;
                size_t clen = (size_t)min<uint64_t>(0x100000, size - off);
                uint64_t hit = scan_chunk_for_board(chunk, clen, (uint64_t)K_Board);
                if (hit) {
                    roh::log("[game] board found @ 0x%llX (region %d)", (unsigned long long)hit, g_scan.regions);
                    return hit;
                }
                // 预算耗尽：记住断点（下一块）
                if (GetTickCount64() - t0 >= kScanBudgetMs) {
                    g_scan.addr = chunk + 0x100000;
                    return 0;
                }
            }
        }
        addr = base + size;
        if (GetTickCount64() - t0 >= kScanBudgetMs) { g_scan.addr = addr; return 0; }
    }
    // 整轮扫完没找到
    roh::log("[game] sweep miss (%d regions)", g_scan.regions);
    g_scan.active = false;
    g_scan.nextStart = GetTickCount64() + kScanRestartDelayMs;
    return 0;
}

uint64_t acquire_board() {
    if (g_boardObj && validate_board_obj(g_boardObj)) return g_boardObj;
    g_boardObj = 0;
    ULONGLONG now = GetTickCount64();
    if (!g_scan.active) {
        if (now < g_scan.nextStart) return 0;
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        g_scan.addr = (uint64_t)si.lpMinimumApplicationAddress;
        g_scan.maxAddr = (uint64_t)si.lpMaximumApplicationAddress;
        g_scan.regions = 0;
        g_scan.active = true;
    }
    g_boardObj = scan_step();
    if (g_boardObj) { g_scan.active = false; g_scan.nextStart = 0; }
    return g_boardObj;
}

// ================= DataArray 迭代 =================
template <typename F>
void for_each_dataarray_item(uint64_t board, int boardFieldOff, const DAInfo& da, F&& fn) {
    if (!da.ok || boardFieldOff < 0) return;
    uint64_t daObj = roh::safe_read_ptr(board + boardFieldOff);
    if (!daObj) return;
    uint64_t arr = roh::safe_read_ptr(daObj + da.offList);
    if (!arr) return;
    int count = 0; roh::safe_read(daObj + da.offCount, count);
    uint32_t alen = il2cpp::array_len(arr);
    if (alen > 100000) alen = 100000;
    if (count < 0 || (uint32_t)count > alen) count = (int)alen;
    uint64_t data = il2cpp::array_data(arr);
    for (int i = 0; i < count; i++) {
        uint64_t item = roh::safe_read_ptr(data + (uint64_t)i * da.elemSize + da.itemOff);
        if (!item) continue;
        fn(item);
    }
}

// ================= 快照 =================
std::atomic<bool> g_run{ false };
HANDLE g_thread = nullptr;

Snapshot g_snapA, g_snapB;
std::atomic<Snapshot*> g_publish{ nullptr };
std::atomic<bool> g_reqSkip{ false };
std::atomic<int>  g_reqSpawnType{ -1 };
std::atomic<int>  g_reqSpawnRow{ 0 };
float g_lastTimeScale = 1.0f;

Snapshot* g_filling = &g_snapA;

void snapshot_zombies(Snapshot& s) {
    g_zwSampleN = 0;
    auto& ms = sibalhook::MenuState::Instance();
    bool wantSkel = ms.zombieSkeleton;
    int sampleCount = 0;
    float b1x=0, b1y=0, w1x=0, w1y=0, b2x=0, b2y=0, w2x=0, w2y=0;

    for_each_dataarray_item(g_boardObj, O_B_Zombies, DA_Zombie, [&](uint64_t z) {
        if (klass_of(z) != (uint64_t)K_Zombie) return;
        int type = -999; roh::safe_read(z + O_Z_Type, type);
        if (type < -1 || type > 100) return;      // freelist 垃圾
        ZombieSnap zs{};
        zs.obj = z;
        zs.type = type;
        roh::safe_read(z + O_Base_Row, zs.row);
        roh::safe_read(z + O_Z_Phase, zs.phase);
        roh::safe_read(z + O_Z_PosX, zs.posX);
        roh::safe_read(z + O_Z_PosY, zs.posY);
        roh::safe_read(z + O_Z_Body, zs.hp);      roh::safe_read(z + O_Z_BodyMax, zs.hpMax);
        roh::safe_read(z + O_Z_Helm, zs.helm);    roh::safe_read(z + O_Z_HelmMax, zs.helmMax);
        roh::safe_read(z + O_Z_Shield, zs.shield);roh::safe_read(z + O_Z_ShieldMax, zs.shieldMax);
        roh::safe_read(z + O_Z_Dead, zs.dead);
        zs.dead = zs.dead || (zs.hp <= 0 && zs.helm <= 0 && zs.shield <= 0);
        roh::safe_read(z + O_Z_MindCtrl, zs.mindControlled);
        if (zs.dead && zs.hp == 0 && zs.hpMax == 0) return; // 空槽

        // 世界位置（controller -> transform -> position）
        if (g_cam.ok && O_Z_Controller >= 0 && M_Component_get_transform) {
            uint64_t ctrl = roh::safe_read_ptr(z + O_Z_Controller);
            uint64_t tr = invoke_get_obj(M_Component_get_transform, ctrl);
            float p[3];
            if (tr && invoke_get_vec3(M_Transform_get_position, tr, p)) {
                zs.hasBox = true;
                if (sampleCount == 0) { b1x = zs.posX; b1y = zs.posY; w1x = p[0]; w1y = p[1]; }
                else if (sampleCount == 1) { b2x = zs.posX; b2y = zs.posY; w2x = p[0]; w2y = p[1]; }
                sampleCount++;
                if (g_zwSampleN < 4) { g_zwSample[g_zwSampleN][0] = p[0]; g_zwSample[g_zwSampleN][1] = p[1]; g_zwSampleN++; }
                // 临时保存世界锚点
                zs.bx0 = p[0]; zs.by0 = p[1]; zs.bx1 = p[2];
            }
        }
        s.zombies.push_back(std::move(zs));
    });

    // 推导 board->world 仿射：双样本才算缩放（持久保留），单样本只更新锚点
    if (sampleCount >= 1) {
        if (sampleCount >= 2) {
            bool xOk = projection::calibrate_axis(b2x - b1x, w2x - w1x, g_board.kx);
            bool yOk = projection::calibrate_axis(b2y - b1y, w2y - w1y, g_board.ky);
            g_board.scaleCalibrated = g_board.scaleCalibrated || xOk || yOk;
        }
        g_board.bx = b1x; g_board.by = b1y;
        g_board.wx = w1x; g_board.wy = w1y;
        g_board.ok = true;
        if (!g_board.scaleCalibrated) { g_board.kx = 2.82f; g_board.ky = -2.82f; }  // 观测默认值
    }

    // ZombieController inherits ReloadedCharacterController. Its animation
    // controller owns the Spine skeleton; m_renderer is only a Unity Renderer.
    if (g_cam.ok) {
        for (auto& zs : s.zombies) {
            zs.hasBox = false;
            uint64_t ctrl = O_Z_Controller >= 0 ? roh::safe_read_ptr(zs.obj + O_Z_Controller) : 0;
            uint64_t renderer = visual_renderer(ctrl, false);
            float box[4]{}, anchor[2]{};
            float head[2] = { NAN, NAN };
            bool fromSkeleton = skeleton_box(ctrl, box, anchor, wantSkel ? &zs.bones : nullptr, head);
            if (!fromSkeleton) {
                zs.bones.clear();
                if (!renderer_box(renderer, box, anchor)) continue;
            }
            zs.bx0 = box[0]; zs.by0 = box[1]; zs.bx1 = box[2]; zs.by1 = box[3];
            zs.ax = anchor[0]; zs.ay = anchor[1];
            if (std::isfinite(head[0])) {
                zs.hasHead = true; zs.headX = head[0]; zs.headY = head[1];
            }
            zs.hasBox = true;
        }
    }
}

void snapshot_plants(Snapshot& s) {
    if (!g_cam.ok) return;
    for_each_dataarray_item(g_boardObj, O_B_Plants, DA_Plant, [&](uint64_t p) {
        if (klass_of(p) != (uint64_t)K_Plant) return;
        PlantSnap ps{};
        ps.obj = p;
        roh::safe_read(p + O_P_Type, ps.type);
        if (ps.type < 0 || ps.type > 120) return;
        roh::safe_read(p + O_Base_Row, ps.row);
        roh::safe_read(p + O_P_Col, ps.col);
        roh::safe_read(p + O_P_State, ps.state);
        roh::safe_read(p + O_P_Hp, ps.hp);
        roh::safe_read(p + O_P_HpMax, ps.hpMax);
        roh::safe_read(p + O_P_Dead, ps.dead);
        roh::safe_read(p + O_P_Asleep, ps.asleep);
        // 对象池空槽过滤：dead / 无效血量上限 的不是场上植物
        if (ps.dead || ps.hpMax <= 0 || ps.hpMax > 100000) return;
        uint64_t ctrl = O_P_Controller >= 0 ? roh::safe_read_ptr(p + O_P_Controller) : 0;
        uint64_t renderer = visual_renderer(ctrl, true);
        float box[4]{}, anchor[2]{};
        bool fromSkeleton = skeleton_box(ctrl, box, anchor, nullptr, nullptr);
        bool hasBox = fromSkeleton || renderer_box(renderer, box, anchor);
        if (!hasBox) return;
        ps.hasBox = true;
        ps.bx0 = box[0]; ps.by0 = box[1]; ps.bx1 = box[2]; ps.by1 = box[3];
        s.plants.push_back(ps);
    });
}

void snapshot_coins(Snapshot& s) {
    if (!(g_cam.ok && g_board.ok)) return;
    for_each_dataarray_item(g_boardObj, O_B_Coins, DA_Coin, [&](uint64_t c) {
        if (klass_of(c) != (uint64_t)K_Coin) return;
        CoinSnap cs{};
        cs.obj = c;
        roh::safe_read(c + O_C_Type, cs.type);
        if (cs.type < 0 || cs.type > 40) return;
        bool dead = false; roh::safe_read(c + O_C_Dead, dead);
        if (dead) return;
        roh::safe_read(c + O_C_PosX, cs.posX);
        roh::safe_read(c + O_C_PosY, cs.posY);
        float wx, wy;
        board_to_world(cs.posX, cs.posY, &wx, &wy);
        world_to_screen(wx, wy, &cs.sx, &cs.sy);
        cs.hasPt = true;
        s.coins.push_back(std::move(cs));
    });
}

// 弹道线：vel 全零（移动由动画驱动），方向由 mMotionType 决定——
// Straight(0) 向右、Backwards(6) 向左，画到草坪边缘；其余类型跳过。
void snapshot_projectiles(Snapshot& s) {
    if (!(g_cam.ok && g_board.ok)) return;
    static int dbgLogged = 0;
    for_each_dataarray_item(g_boardObj, O_B_Projectiles, DA_Proj, [&](uint64_t p) {
        if (K_Projectile && klass_of(p) != (uint64_t)K_Projectile) return;
        ProjectileSnap ps{};
        ps.obj = p;
        roh::safe_read(p + O_PR_Type, ps.type);
        bool dead = false;
        roh::safe_read(p + O_PR_Dead, dead);
        if (dead) return;
        roh::safe_read(p + O_PR_PosX, ps.posX);
        roh::safe_read(p + O_PR_PosY, ps.posY);
        int motion = -1;
        roh::safe_read(p + O_PR_Motion, motion);
        ps.velX = motion == 0 ? 1.0f : (motion == 6 ? -1.0f : 0.0f);
        ps.velY = 0;
        ps.velZ = 0;
        if (dbgLogged < 3) {
            ++dbgLogged;
            roh::log("[rage] proj type=%d motion=%d pos=(%.1f,%.1f)",
                     ps.type, motion, ps.posX, ps.posY);
        }
        float wx, wy;
        board_to_world(ps.posX, ps.posY, &wx, &wy);
        world_to_screen(wx, wy, &ps.sx, &ps.sy);
        ps.hasPt = true;
        if (ps.velX != 0.0f) {
            const float endX = ps.velX > 0.0f ? 900.0f : -100.0f;
            float ewx, ewy;
            board_to_world(endX, ps.posY, &ewx, &ewy);
            world_to_screen(ewx, ewy, &ps.ex, &ps.ey);
            ps.hasEnd = true;
        }
        s.projectiles.push_back(std::move(ps));
    });
}

void snapshot_seeds(Snapshot& s) {
    if (O_B_SeedBank < 0 || O_SeedBank_Packets < 0) return;
    uint64_t bank = roh::safe_read_ptr(g_boardObj + O_B_SeedBank);
    if (!bank) return;
    uint64_t arr = roh::safe_read_ptr(bank + O_SeedBank_Packets);
    if (!arr) return;
    uint32_t n = il2cpp::array_len(arr);
    if (n > 20) n = 20;
    for (uint32_t i = 0; i < n; i++) {
        uint64_t pk = roh::safe_read_ptr(il2cpp::array_data(arr) + (uint64_t)i * 8);
        if (!pk) continue;
        SeedSnap ss{};
        roh::safe_read(pk + O_SP_Type, ss.type);
        roh::safe_read(pk + O_SP_Refresh, ss.refresh);
        roh::safe_read(pk + O_SP_RefreshTime, ss.refreshTime);
        roh::safe_read(pk + O_SP_Active, ss.active);
        roh::safe_read(pk + O_SP_Refreshing, ss.refreshing);
        s.seeds.push_back(ss);
    }
}

// Chams（彩人）：写 Spine Skeleton 的 r/g/b/a（等同 Skeleton.SetColor），
// Spine 每帧重建网格时整体着色。纯内存写，不依赖屏幕坐标链路。
// 魅惑僵尸用 Controlled Box 颜色，其余用 Chams Color；死亡/关闭时还原原色。
// 注意：本函数体内禁止出现带析构的 C++ 对象（safe_* 的 SEH 与 unwinding 冲突，
// C2712），原色缓存用纯 POD 定长表。
struct ChamsOrig { uint64_t sk; float rgba[4]; };
ChamsOrig g_chamsOrig[512];
int g_chamsOrigN = 0;
uint64_t g_chamsBoard = 0;

// Rage：魔法子弹——逐帧 posY 引导。日志证实：豌豆 vel=(0,0,0)、X 由动画系统
// 推进（+3.3/帧）；Homing 枚举方案无效（值不可靠）。碰撞是几何 Y 带判定，
// 所以每帧把 posY 向目标僵尸行拉近一小步（限幅），豌豆就会划出弧线命中跨行
// 目标。本行前方有目标时不干预（自然命中）。SEH 纪律：只允许 POD 局部变量。
struct MbZombie { uint64_t ptr; int id; float x, y; };
__declspec(noinline) void apply_magic_bullets(const sibalhook::MenuState& ms,
                                              const Snapshot& s) {
    if (!ms.magicBullets || !DA_Proj.ok || O_B_Projectiles < 0 || O_PR_PosX < 0 ||
        O_PR_PosY < 0 || O_PR_Motion < 0 || !DA_Zombie.ok || O_B_Zombies < 0)
        return;
    static MbZombie cands[256];
    int nCand = 0;
    {
        uint64_t daObj = roh::safe_read_ptr(g_boardObj + O_B_Zombies);
        uint64_t arr = daObj ? roh::safe_read_ptr(daObj + DA_Zombie.offList) : 0;
        if (!arr) return;
        int count = 0;
        roh::safe_read(daObj + DA_Zombie.offCount, count);
        uint32_t alen = il2cpp::array_len(arr);
        if (alen > 100000) alen = 100000;
        if (count < 0 || (uint32_t)count > alen) count = (int)alen;
        uint64_t data = il2cpp::array_data(arr);
        for (int i = 0; i < count && nCand < 256; i++) {
            uint64_t slot = data + (uint64_t)i * DA_Zombie.elemSize;
            uint64_t z = roh::safe_read_ptr(slot + DA_Zombie.itemOff);
            if (!z || klass_of(z) != (uint64_t)K_Zombie) continue;
            bool dead = false, mc = false;
            roh::safe_read(z + O_Z_Dead, dead);
            roh::safe_read(z + O_Z_MindCtrl, mc);
            if (dead || mc) continue;
            float zx = 0, zy = 0;
            roh::safe_read(z + O_Z_PosX, zx);
            roh::safe_read(z + O_Z_PosY, zy);
            cands[nCand].ptr = z;
            cands[nCand].id = 0;
            cands[nCand].x = zx;
            cands[nCand].y = zy;
            nCand++;
        }
    }
    if (nCand == 0) return;
    constexpr float kSteer = 2.5f;    // 每帧垂直引导速度（板坐标）
    static int dbgLogged = 0;
    for_each_dataarray_item(g_boardObj, O_B_Projectiles, DA_Proj, [&](uint64_t p) {
        if (K_Projectile && klass_of(p) != (uint64_t)K_Projectile) return;
        int motion = -1;
        roh::safe_read(p + O_PR_Motion, motion);
        if (motion != 0) return;               // 只引导 Straight（豌豆系）
        bool dead = false;
        roh::safe_read(p + O_PR_Dead, dead);
        if (dead) return;
        float px = 0, py = 0;
        roh::safe_read(p + O_PR_PosX, px);
        roh::safe_read(p + O_PR_PosY, py);
        const MbZombie* best = nullptr;
        float bestD = 1e9f;
        bool sameLane = false;
        for (int i = 0; i < nCand; i++) {
            const float d = cands[i].x - px;
            if (d <= 0.5f) continue;
            if (fabsf(cands[i].y - py) < 45.0f) { sameLane = true; break; }
            if (d < bestD) {
                bestD = d;
                best = &cands[i];
            }
        }
        if (sameLane || !best) return;
        // 逐帧把 Y 拉向目标行（限幅防跳变）；X 交给游戏动画推进
        const float dy = best->y - py;
        float ny = py + (dy > kSteer ? kSteer : (dy < -kSteer ? -kSteer : dy));
        if (ny != py) roh::safe_write(p + O_PR_PosY, &ny, 4);
        if (dbgLogged < 3) {
            ++dbgLogged;
            roh::log("[rage] mb: steer py=%.1f -> %.1f (target y=%.1f d=%.0f)",
                     py, ny, best->y, bestD);
        }
    });
}

__declspec(noinline) void apply_chams(const sibalhook::MenuState& ms) {
    if (O_Z_Controller < 0 || O_RCC_AnimationController < 0 || O_CAC_SkeletonAnimation < 0 ||
        O_SR_Skeleton < 0 || O_SK_R < 0 || O_SK_G < 0 || O_SK_B < 0 || O_SK_A < 0)
        return;
    if (g_boardObj != g_chamsBoard) {
        g_chamsOrigN = 0;
        g_chamsBoard = g_boardObj;
    }
    const int rgbaOff[4] = {O_SK_R, O_SK_G, O_SK_B, O_SK_A};
    for_each_dataarray_item(g_boardObj, O_B_Zombies, DA_Zombie, [&](uint64_t z) {
        if (klass_of(z) != (uint64_t)K_Zombie) return;
        uint64_t ctrl = roh::safe_read_ptr(z + O_Z_Controller);
        uint64_t anim = ctrl ? roh::safe_read_ptr(ctrl + O_RCC_AnimationController) : 0;
        uint64_t spine = anim ? roh::safe_read_ptr(anim + O_CAC_SkeletonAnimation) : 0;
        uint64_t sk = spine ? roh::safe_read_ptr(spine + O_SR_Skeleton) : 0;
        if (!sk) return;
        float cur[4]{};
        for (int i = 0; i < 4; ++i) roh::safe_read(sk + rgbaOff[i], cur[i]);
        int slot = -1;
        for (int i = 0; i < g_chamsOrigN; ++i)
            if (g_chamsOrig[i].sk == sk) { slot = i; break; }
        bool dead = false;
        roh::safe_read(z + O_Z_Dead, dead);
        if (ms.zombieChams && !dead) {
            if (slot < 0 && g_chamsOrigN < 512) {
                slot = g_chamsOrigN++;
                g_chamsOrig[slot].sk = sk;
                for (int i = 0; i < 4; ++i) g_chamsOrig[slot].rgba[i] = cur[i];
            }
            bool mc = false;
            if (O_Z_MindCtrl >= 0) roh::safe_read(z + O_Z_MindCtrl, mc);
            const float* c = mc ? ms.mindControlledBoxColor.data() : ms.zombieChamsColor.data();
            const float want[4] = {c[0], c[1], c[2], 1.0f};
            for (int i = 0; i < 4; ++i) {
                if (cur[i] != want[i]) roh::safe_write(sk + rgbaOff[i], &want[i], 4);
            }
        } else if (slot >= 0) {
            // 关闭或已死：还原进入染色前记录的原色（死亡淡出动画由游戏接管）
            for (int i = 0; i < 4; ++i) {
                if (cur[i] != g_chamsOrig[slot].rgba[i])
                    roh::safe_write(sk + rgbaOff[i], &g_chamsOrig[slot].rgba[i], 4);
            }
            g_chamsOrig[slot] = g_chamsOrig[--g_chamsOrigN];
        }
    });
}

void apply_cheats(Snapshot& s) {
    auto& ms = sibalhook::MenuState::Instance();
    // 阳光
    if (ms.infiniteSun && O_Sun_Amount >= 0) {
        uint64_t sunObj = get_sun_obj(g_boardObj);
        if (sunObj) {
            int v = ms.sunAmount;
            roh::safe_write(sunObj + O_Sun_Amount, &v, 4);
            s.sun = v;
        }
    } else if (O_Sun_Amount >= 0) {
        uint64_t sunObj = get_sun_obj(g_boardObj);
        if (sunObj) roh::safe_read(sunObj + O_Sun_Amount, s.sun);
    }
    // 免冷却：mRefreshCounter 往上数到 mRefreshTime 才算好——直接写满，
    // 并停掉 refreshing 状态（v40 之前清零 counter 是反的，永远冷却中）
    if (ms.noCooldown) {
        if (O_B_SeedBank >= 0 && O_SeedBank_Packets >= 0) {
            uint64_t bank = roh::safe_read_ptr(g_boardObj + O_B_SeedBank);
            uint64_t arr = bank ? roh::safe_read_ptr(bank + O_SeedBank_Packets) : 0;
            if (arr) {
                uint32_t n = min(il2cpp::array_len(arr), 20u);
                for (uint32_t i = 0; i < n; i++) {
                    uint64_t pk = roh::safe_read_ptr(il2cpp::array_data(arr) + (uint64_t)i * 8);
                    if (!pk) continue;
                    int rt = 0;
                    roh::safe_read(pk + O_SP_RefreshTime, rt);
                    int full = rt > 0 ? rt : 1000000;
                    roh::safe_write(pk + O_SP_Refresh, &full, 4);
                    bool f = false;
                    roh::safe_write(pk + O_SP_Refreshing, &f, 1);
                }
            }
        }
    }
    // 免费种植（游戏自带作弊开关）
    if (ms.freePlanting && O_B_App >= 0 && O_GA_EasyPlant >= 0) {
        uint64_t app = roh::safe_read_ptr(g_boardObj + O_B_App);
        if (app) {
            bool t = true;
            roh::safe_write(app + O_GA_EasyPlant, &t, 1);
        }
    }
    // 僵尸处理
    if (ms.freezeZombies || ms.instantKill) {
        for_each_dataarray_item(g_boardObj, O_B_Zombies, DA_Zombie, [&](uint64_t z) {
            if (klass_of(z) != (uint64_t)K_Zombie) return;
            if (ms.instantKill) {
                int zero = 0;
                roh::safe_write(z + O_Z_Body, &zero, 4);
                roh::safe_write(z + O_Z_Helm, &zero, 4);
                roh::safe_write(z + O_Z_Shield, &zero, 4);
            } else if (ms.freezeZombies) {
                // 冻结 = 写游戏自身的冰冻/减速状态（寒冰菇同款字段）。
                // 只冻已上草坪的僵尸（posX<800）：屏外刷新点刚生成的若被钉死，
                // 永远走不进场，最后一波永不结算。
                float zx = 0;
                roh::safe_read(z + O_Z_PosX, zx);
                if (zx < 800.0f) {
                    if (O_Z_IceTrap >= 0) {
                        int ice = 10000;
                        roh::safe_write(z + O_Z_IceTrap, &ice, 4);
                    }
                    if (O_Z_Chilled >= 0) {
                        int chill = 10000;
                        roh::safe_write(z + O_Z_Chilled, &chill, 4);
                    }
                    float z0 = 0.0f;
                    roh::safe_write(z + O_Z_VelX, &z0, 4);
                }
            }
        });
    }
    // Chams（彩人）：见 apply_chams
    apply_chams(ms);
    // Rage：魔法子弹（跨行命中）
    apply_magic_bullets(ms, s);
    // 植物无敌
    if (ms.plantGod) {
        for_each_dataarray_item(g_boardObj, O_B_Plants, DA_Plant, [&](uint64_t p) {
            if (klass_of(p) != (uint64_t)K_Plant) return;
            int hp = 0, hpMax = 0;
            roh::safe_read(p + O_P_Hp, hp);
            roh::safe_read(p + O_P_HpMax, hpMax);
            if (hpMax > 0 && hp != hpMax) roh::safe_write(p + O_P_Hp, &hpMax, 4);
        });
    }
    // 自动收集：调用游戏自身的 Coin.ScoreCoin() 直接入账（阳光加钱+音效），
    // 然后 mDead 置位让 DataArray 回收。旧做法只传送坐标，永远不触发收集。
    if (ms.autoCollect) {
        for_each_dataarray_item(g_boardObj, O_B_Coins, DA_Coin, [&](uint64_t c) {
            bool dead = false; roh::safe_read(c + O_C_Dead, dead);
            if (dead) return;
            int type = 0; roh::safe_read(c + O_C_Type, type);
            // 4=Sun 5=SmallSun 6=LargeSun；金币也收（1/2）
            if (type < 1 || type > 6) return;
            if (M_Coin_ScoreCoin) {
                invoke_get_obj(M_Coin_ScoreCoin, c);
                static int dbgN = 0;
                if (dbgN < 3) {
                    ++dbgN;
                    roh::log("[autocollect] ScoreCoin type=%d", type);
                }
            }
            bool d = true;
            roh::safe_write(c + O_C_Dead, &d, 1);
        });
    }
    // 无雾
    if (ms.noFog) {
        if (O_B_FogBlown >= 0) {
            int big = 99999999;
            roh::safe_write(g_boardObj + O_B_FogBlown, &big, 4);
        }
        if (O_B_GridFog >= 0) {
            uint64_t arr = roh::safe_read_ptr(g_boardObj + O_B_GridFog);
            if (arr) {
                uint32_t n = il2cpp::array_len(arr);
                if (n > 0 && n < 100000) {
                    uint64_t d = il2cpp::array_data(arr);
                    static std::vector<int> zeros;
                    if (zeros.size() < n) zeros.assign(n, 0);
                    roh::safe_write(d, zeros.data(), n * 4);
                }
            }
        }
    }
    // 变速
    if (M_Time_set_timeScale) {
        float want = ms.timeScale;
        if (fabsf(want - g_lastTimeScale) > 0.001f) {
            float v = want;
            void* params[1] = { &v };
            void* exc = nullptr;
            __try { il2cpp::runtime_invoke(M_Time_set_timeScale, nullptr, params, &exc); }
            __except (1) {}
            g_lastTimeScale = want;
        }
    }
    // 跳波
    if (g_reqSkip.exchange(false)) {
        int one = 1;
        roh::safe_write(g_boardObj + O_B_ZombieCD, &one, 4);
        roh::safe_write(g_boardObj + O_B_HugeCD, &one, 4);
    }
    // 生成僵尸（实验）
    int st = g_reqSpawnType.exchange(-1);
    if (st >= 0 && M_Board_AddZombieInRow) {
        int type = st, row = g_reqSpawnRow.load(), a3 = 0, a4 = 0;
        void* params[4] = { &type, &row, &a3, &a4 };
        void* exc = nullptr;
        __try { il2cpp::runtime_invoke(M_Board_AddZombieInRow, (void*)g_boardObj, params, &exc); }
        __except (1) { roh::log("[game] spawn threw"); }
    }
}

// ZombieType shift 校准：hpMax==270 的普通僵尸最常见
void calibrate_type_shift(Snapshot& s) {
    if (g_zTypeShift.load() != -100) return;
    std::map<int, int> countByType;   // type -> 出现次数（hpMax==270）
    for (auto& z : s.zombies)
        if (z.hpMax == 270) countByType[z.type]++;
    int bestType = -999, bestCnt = 0;
    for (auto& kv : countByType)
        if (kv.second > bestCnt) { bestCnt = kv.second; bestType = kv.first; }
    if (bestCnt >= 1) {
        // dump 顺序里 Normal=1（Invalid=0）；若 Invalid 实际是 -1，则 shift=+1
        int shift = 1 - bestType;   // 使 bestType 对应索引 1 (Normal)
        g_zTypeShift.store(shift);
        roh::log("[game] zombie type shift calibrated: type %d -> shift %+d", bestType, shift);
    }
}

} // namespace

// Present/render-thread loop. GC attachment does not establish Unity main-thread safety.
static int s_frame = 0;
void on_present(float viewW, float viewH) {
    if (viewW > 1) g_viewW = viewW;
    if (viewH > 1) g_viewH = viewH;
    static int vrFrame = 0;
    ++vrFrame;
    read_virtual_res();
    Snapshot& s = *g_filling;
    s.zombies.clear(); s.plants.clear(); s.coins.clear(); s.seeds.clear(); s.projectiles.clear();
    s.valid = false;
    s_frame++;

    uint64_t board = acquire_board();
    if (!board) {
        g_board = BoardAffine{}; g_cam.ok = false; g_zwSampleN = 0;
        g_publish.store(&s);
        g_filling = (g_filling == &g_snapA) ? &g_snapB : &g_snapA;
        return;
    }
    s.boardObj = board;
    calibrate_camera();

    snapshot_zombies(s);
    snapshot_plants(s);
    snapshot_coins(s);
    snapshot_projectiles(s);
    snapshot_seeds(s);
    calibrate_type_shift(s);

    if (O_B_NumWaves >= 0) roh::safe_read(board + O_B_NumWaves, s.numWaves);
    if (O_B_CurrentWave >= 0) roh::safe_read(board + O_B_CurrentWave, s.wave);
    int hn = 0, hs = 0;
    roh::safe_read(board + O_B_HealthNext, hn);
    roh::safe_read(board + O_B_HealthStart, hs);
    if (hs > 0) s.waveProgress = hn / (float)hs;

    apply_cheats(s);
    s.valid = true;
    g_publish.store(&s);
    g_filling = (g_filling == &g_snapA) ? &g_snapB : &g_snapA;

    // 每秒一次状态日志（诊断用）
    static ULONGLONG lastDbg = 0;
    ULONGLONG nowMs = GetTickCount64();
    if (nowMs - lastDbg > 1000) {
        lastDbg = nowMs;
        char sunInfo[128] = "sunChain: n/a";
        if (s.boardObj && O_B_SunMoney >= 0) {
            uint64_t mpBase = s.boardObj + O_B_SunMoney;
            uint64_t mainV = O_SunMainAbs >= 0 ? roh::safe_read_ptr(s.boardObj + O_SunMainAbs) : 0;
            uint64_t varr = O_SunValuesAbs >= 0 ? roh::safe_read_ptr(s.boardObj + O_SunValuesAbs) : 0;
            uint32_t vlen = varr ? il2cpp::array_len(varr) : 0;
            uint64_t v0 = varr && vlen > 0 ? roh::safe_read_ptr(il2cpp::array_data(varr)) : 0;
            uint64_t v1 = varr && vlen > 1 ? roh::safe_read_ptr(il2cpp::array_data(varr) + 8) : 0;
            _snprintf(sunInfo, sizeof(sunInfo),
                      "sunChain: mp=%llx main=%llx arr=%llx len=%u e0=%llx e1=%llx",
                      (unsigned long long)mpBase, (unsigned long long)mainV,
                      (unsigned long long)varr, vlen,
                      (unsigned long long)v0, (unsigned long long)v1);
        }
        char zInfo[160] = "";
        if (!s.zombies.empty()) {
            const auto& z0 = s.zombies[0];
            float asx = 0, asy = 0;
            if (g_cam.ok) world_to_screen(z0.bx0, z0.by0, &asx, &asy);  // 注意 bx0/by0 此时已是屏幕框，这里复用锚点前值不可得，输出框即可
            _snprintf(zInfo, sizeof(zInfo), " z0[type=%d hp=%d/%d box=(%.0f,%.0f)-(%.0f,%.0f)]",
                      z0.type, z0.hp, z0.hpMax, z0.bx0, z0.by0, z0.bx1, z0.by1);
        }
        // Sun 探针：扫 mSunMoney 前后 8 字节槽位，klass==Sun 的就是答案
        static bool sunProbed = false;
        if (!sunProbed && s.boardObj && O_B_SunMoney >= 0 && K_Sun && il2cpp::class_get_name) {
            sunProbed = true;
            char buf[512]; int n = 0;
            for (int off = -0x10; off <= 0x30; off += 8) {
                uint64_t cand = roh::safe_read_ptr(s.boardObj + O_B_SunMoney + off);
                if (cand < 0x10000 || cand > 0x7FFFFFFFFFFF) continue;
                uint64_t k = klass_of(cand);
                const char* kn = nullptr;
                if (k) { __try { kn = il2cpp::class_get_name((void*)k); } __except (1) { kn = "?"; } }
                int amount = 0;
                if (kn && strcmp(kn, "Sun") == 0) roh::safe_read(cand + 0x10, amount);
                n += _snprintf(buf + n, sizeof(buf) - n - 1, " [+%X]=%llx(%s:%d)", off,
                               (unsigned long long)cand, kn ? kn : "?", amount);
            }
            roh::log("[sun-probe] mSunMoney=0x%X%s", O_B_SunMoney, buf);
        }
        roh::log("[dbg] virt=%.0fx%.0f view=%.0fx%.0f zombies=%zu plants=%zu coins=%zu cam=%d bAff=%d(%.2f,%.2f) wave=%d/%d sun=%d%s %s",
                 g_virtW, g_virtH, g_viewW, g_viewH,
                 s.zombies.size(), s.plants.size(), s.coins.size(),
                 g_cam.ok ? 1 : 0, g_board.ok ? 1 : 0, g_board.kx, g_board.ky,
                 s.wave, s.numWaves, s.sun, zInfo, sunInfo);
    }
}

void start() {
    roh::log("[game] present-thread mode (no snapshot thread)");
}
void stop() {
    if (M_Time_set_timeScale) {
        float v = 1.0f;
        void* params[1] = { &v };
        __try { il2cpp::runtime_invoke(M_Time_set_timeScale, nullptr, params, nullptr); } __except (1) {}
    }
}
const Snapshot* current() { return g_publish.load(); }
int debug_flags() {
    return (g_boardObj ? 1 : 0) | (g_cam.ok ? 2 : 0) | (g_board.ok ? 4 : 0);
}
void request_skip_wave() { g_reqSkip.store(true); }
void request_spawn(int t, int row) { g_reqSpawnRow.store(row); g_reqSpawnType.store(t); }
void request_restore_timescale() {
    if (M_Time_set_timeScale) {
        float v = 1.0f;
        void* params[1] = { &v };
        __try { il2cpp::runtime_invoke(M_Time_set_timeScale, nullptr, params, nullptr); } __except (1) {}
    }
}

} // namespace game
