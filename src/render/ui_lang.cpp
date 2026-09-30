#include "framework.h"
#include "render/ui_lang.h"
#include "render/menu_state.h"

#include <cstring>

namespace sibalhook::ui {
namespace {

struct Entry {
    const char* en;
    const char* zh;
};

// 菜单全量文案。英文=键；漏项自动回退英文显示。
constexpr Entry kTable[] = {
    // ---- tabs ----
    {"Visuals", "视觉"},
    {"Cheats", "作弊"},
    {"Rage", "狂暴"},
    {"World", "世界"},
    {"Settings", "设置"},
    {"GUI", "界面"},
    // ---- group titles ----
    {"Zombie ESP", "僵尸透视"},
    {"Skeleton", "骨骼"},
    {"Plant ESP", "植物透视"},
    {"Pickups", "掉落物"},
    {"HUD", "信息显示"},
    {"Projectiles", "弹道"},
    {"Economy", "经济"},
    {"Seed Packets", "卡槽"},
    {"Combat", "战斗"},
    {"Automation", "自动化"},
    {"Game Speed", "游戏速度"},
    {"Environment", "环境"},
    {"Wave Control", "波次控制"},
    {"Zombie Spawner (Experimental)", "僵尸生成器（实验）"},
    {"Configuration", "配置"},
    {"Keybinds", "按键绑定"},
    {"Language", "语言"},
    {"About ROH", "关于 ROH"},
    {"UI Theme", "界面主题"},
    // ---- rows: zombie ESP ----
    {"Box", "方框"},
    {"Box Color", "方框颜色"},
    {"Controlled Box", "魅惑方框色"},
    {"Box Style", "方框样式"},
    {"Health Bar", "血条"},
    {"Health Text", "血量文字"},
    {"Zombie Name", "僵尸名字"},
    {"Row Text", "行号文字"},
    {"Health Color By Ratio", "血量按比例变色"},
    {"Chams", "彩人"},
    {"Chams Color", "彩人颜色"},
    {"Draw Skeleton", "绘制骨骼"},
    {"Skeleton Color", "骨骼颜色"},
    {"Head Bone Dot", "头部骨骼点"},
    {"Head Dot Color", "头部点颜色"},
    {"Tracer Lines", "射线"},
    {"Tracer Color", "射线颜色"},
    {"Line Width", "线条宽度"},
    // ---- rows: projectiles / plants / pickups / hud ----
    {"Trajectory", "弹道线"},
    {"Trajectory Color", "弹道颜色"},
    {"Plant Box Color", "植物框颜色"},
    {"Plant Name", "植物名字"},
    {"Sun ESP", "阳光显示"},
    {"Sun Ring Color", "阳光圈颜色"},
    {"Coin ESP", "金币显示"},
    {"Coin Ring Color", "金币圈颜色"},
    {"Value Text", "面值文字"},
    {"Seed Packet Cooldown", "卡槽冷却"},
    {"Wave Progress Bar", "波次进度条"},
    {"Watermark", "水印"},
    // ---- rows: cheats / world ----
    {"Infinite Sun", "无限阳光"},
    {"Sun Amount", "阳光数量"},
    {"No Cooldown", "无冷却"},
    {"Free Planting (Any Cost)", "免费种植"},
    {"Instant Kill", "秒杀"},
    {"Freeze Zombies", "冻结僵尸"},
    {"Plant God Mode", "植物无敌"},
    {"Auto Collect Sun & Coins", "自动收集阳光金币"},
    {"Magic Bullets", "魔法子弹"},
    {"Time Scale", "时间流速"},
    {"No Fog", "无雾"},
    {"Skip To Next Wave", "跳到下一波"},
    {"Zombie Type", "僵尸类型"},
    {"Row (0-5)", "行数 (0-5)"},
    {"Count", "数量"},
    {"Spawn!", "生成！"},
    // ---- rows: settings ----
    {"Config Name", "配置名"},
    {"Load", "读取"},
    {"Save", "保存"},
    {"Reset", "重置"},
    {"New", "新建"},
    {"Menu Toggle", "菜单开关"},
    {"Panic (Hide All)", "紧急隐藏"},
    {"Unload DLL", "卸载 DLL"},
    // ---- box style combo ----
    {"Corners", "四角"},
    {"Full", "完整"},
    {"Off", "关闭"},
    // ---- about ----
    {"Software:", "软件："},
    {"Target:", "目标："},
    {"Build:", "版本："},
    {"Engine:", "引擎："},
    // ---- theme slots（显示名；ini 键仍用英文 ThemeSlotName） ----
    {"Borders", "边框"},
    {"Background", "背景"},
    {"Panels", "面板"},
    {"Tabs", "标签页"},
    {"Text", "文字"},
    {"Controls", "控件"},
};

constexpr int kTableCount = sizeof(kTable) / sizeof(kTable[0]);

} // namespace

const char* TR(const char* en) {
    if (!en || !en[0]) return en;
    if (!sibalhook::MenuState::Instance().zhText) return en;
    for (int i = 0; i < kTableCount; ++i) {
        if (strcmp(kTable[i].en, en) == 0) return kTable[i].zh;
    }
    return en;
}

} // namespace sibalhook::ui
