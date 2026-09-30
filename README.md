# ROH — PvZ Replanted 菜单工具

[English](#english) | 中文

一个针对 **Plants vs. Zombies Replanted**（Unity IL2CPP 重制版）的原生 D3D11 覆盖层菜单工具：透视、骨骼、彩人、弹道、经济/战斗/狂暴类修改、变速、波次控制、僵尸生成器、实时 UI 主题编辑与完整中英文双语界面。

> ⚠️ **免责声明**：本项目仅供单机游戏的学习与技术研究（IL2CPP 反射、D3D11 钩子、内存读写），不得用于任何多人/竞技场景或商业用途。使用者自行承担一切风险。本项目与 PopCap / EA 无任何关系，请支持正版游戏。

---

## 功能总览

### 🎯 视觉 / Visuals
| 功能 | 说明 |
|---|---|
| 僵尸 ESP | 方框（四角/完整/关闭）、血条、血量文字、名字、行号 |
| 颜色系统 | Box / 魅惑方框 / 骨骼 / 头点 / 射线 / 植物框 / 阳光圈 / 金币圈 全部可自定义（含透明度） |
| 骨骼绘制 | Spine 骨骼链可视化，头部骨骼点标记 |
| 射线 | 屏幕顶部指向僵尸头部 |
| 彩人 Chams | 直接对僵尸 Spine 骨骼整体着色（游戏内本体变色，雾中也可见；魅惑僵尸自动用蓝色区分） |
| 弹道显示 | 直线投射物（豌豆系）的飞行弹道线 |
| 掉落物 | 阳光 / 金币 / 奖杯圈选 + 面值文字 |
| HUD | 卡槽冷却条、波次进度条、水印 |

### ⚡ 作弊 / Cheats
无限阳光（可设数值）、无冷却、免费种植、秒杀、冻结僵尸（游戏原生冰冻视觉）、植物无敌、自动收集阳光金币（调用游戏自身收集逻辑入账）。

### 🔥 狂暴 / Rage
魔法子弹——豌豆跨行命中：本行无目标时逐帧引导子弹飞向其他行最近的僵尸。

### 🌍 世界 / World
时间流速（0.1x–5x + 快捷按钮）、无雾、跳到下一波、僵尸生成器（类型/行/数量）。

### ⚙️ 设置 / Settings
配置存取（Load / Save / Reset / New）、按键绑定（菜单开关 / 紧急隐藏）、语言切换、关于信息。

### 🎨 界面 / GUI
完整的实时 UI 主题编辑器：**边框 / 背景 / 面板 / 标签页 / 文字 / 控件** 六色体系，二维色场 + 明度条 + 实时预览，改动即时生效无需 Apply，自动持久化，一键恢复默认。

### 🌐 双语
菜单与全部 ESP 文本支持中英文一键切换（Settings → Language → Chinese Text）。

---

## 快速开始

### 环境要求
- Windows 10/11 x64
- Plants vs. Zombies Replanted（Unity IL2CPP x64 版本）

### 使用方法
1. 从 [Releases](../../releases) 下载 `ROH.dll` 与 `injector.exe`，放在同一目录
2. 启动游戏
3. **双击 `injector.exe`**——全自动：自动选最新 DLL、自动等待游戏进程、防重复注入、多游戏实例自动选未注入的那个
4. 游戏内按 `Insert` 开关菜单，`F11` 紧急隐藏全部绘制
5. 想先开注入器再开游戏也行，它会一直等到游戏出现

### 按键
| 按键 | 功能 |
|---|---|
| `Insert` | 菜单开关（可在设置里改键） |
| `F11` | 紧急隐藏（全部绘制立即消失） |

---

## 从源码构建

1. 安装 **Visual Studio 2022 生成工具**（MSVC v143 + Windows 10 SDK）与 **CMake**（VS 自带组件即可）
2. 双击运行 `build.bat`：
   - 首次会自动用 CMake 配置并编译 `vendor/freetype-2.14.3`
   - 编译产出 `build/ROH.dll` 与 `build/injector.exe`
3. 若 `ROH.dll` 被正在运行的游戏锁定，脚本自动改链 `ROH_pending.dll` 并启动 `sync_dll.bat` 后台同步

### 技术栈
C++17 / MinHook / Dear ImGui（FreeType 字体后端）/ 直接调用 `GameAssembly.dll` 的 il2cpp 运行时 API（按名解析类与字段，游戏小版本更新可自愈）。

详见 [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)。

---

## 配置说明（config.ini）

| 分区 | 内容 |
|---|---|
| `[ui.visuals]` | 全部视觉开关与游戏颜色（`#RRGGBBAA`） |
| `[ui.cheats]` `[ui.world]` | 作弊与世界修改开关 |
| `[ui.settings]` | 配置名、按键、语言 |
| `[ui.theme]` | UI 主题六色（边框/背景/面板/标签页/文字/控件） |
| `[ui.menu]` | 上次停留的页面 |

游戏颜色（ESP）与 UI 主题是两套完全独立的配置，互不影响。

---

## 已知限制
- ESP 方框与僵尸本体的屏幕对齐存在历史性偏差（正交相机链路），本工具的方框类功能可能偏移；Chams / 修改类功能不受影响
- 仅支持 D3D11 渲染模式

## 截图

> 📷 截图待补充，放置于 `docs/images/`：
> `menu-visuals.png` / `menu-cheats.png` / `menu-rage.png` / `menu-world.png` / `menu-settings.png` / `menu-gui.png` / `esp-gameplay.png`

---

# English

# ROH — PvZ Replanted Menu Tool

A native D3D11 overlay menu for **Plants vs. Zombies Replanted** (Unity IL2CPP remake): ESP, skeleton, chams, projectile trajectories, economy/combat/rage cheats, time scale, wave control, a zombie spawner, a live UI theme editor, and full Chinese/English localization.

> ⚠️ **Disclaimer**: For single-player educational/research use only (IL2CPP reflection, D3D11 hooking, memory R/W). Not affiliated with PopCap/EA. Use at your own risk.

## Features
- **Visuals**: zombie ESP (corner/full boxes, HP bar/text, names, rows), fully customizable colors, Spine skeleton rendering, tracers, **chams** (whole-body tint via Spine skeleton color, works through fog; mind-controlled zombies tinted blue), projectile trajectory lines, pickup rings, seed-cooldown & wave HUD
- **Cheats**: infinite sun, no cooldown, free planting, instant kill, freeze (native ice visuals), plant god mode, auto-collect (invokes the game's own `Coin.ScoreCoin`)
- **Rage**: magic bullets — peas redirected to the nearest zombie on any row
- **World**: time scale, no fog, skip wave, zombie spawner
- **Settings**: config load/save/reset, keybinds, language toggle
- **GUI**: live 6-color theme editor (border/background/panel/tab/text/control) with instant preview and persistence
- **Bilingual**: every menu & ESP string switches between Chinese and English

## Usage
1. Download `ROH.dll` + `injector.exe` from [Releases](../../releases) into one folder
2. Start the game, then **double-click `injector.exe`** — fully automatic (newest DLL, waits for the game, prevents double injection, picks the uninjected instance)
3. `Insert` toggles the menu, `F11` is panic-hide

## Build
VS2022 Build Tools (MSVC v143, Win10 SDK) + CMake, then run `build.bat`. See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Known Limitations
- ESP boxes have a historical screen-alignment offset (orthographic camera chain); chams and memory-write cheats are unaffected
- D3D11 render mode only
