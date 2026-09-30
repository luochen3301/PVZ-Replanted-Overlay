# 架构 / Architecture

## 中文

### 总体数据流

```
injector.exe                 主 DLL（注入进游戏进程 Replanted.exe）
    │                              │
    │ CreateRemoteThread           ├─ dllmain        初始化线程：
    │   + LoadLibraryA             │                  il2cpp 按名解析 → 配置加载 → D3D11 钩子安装
    ▼                              ├─ dx11_hook      Present 钩子 = 全部逻辑的心跳
 游戏进程                          ├─ imgui          界面与 ESP 绘制（FreeType 字体）
                                   ├─ game_data      每帧快照 + 修改类逻辑（与 Present 同线程）
                                   └─ render/ui_*    自绘控件 / 主题 / 双语 / 页面布局
```

DLL 注入后所有工作都发生在游戏的 **Present 调用链**里（渲染线程）：Unity 每帧呈现一帧，钩子在真正的 Present 之前完成「快照 → 作弊写入 → 界面绘制」三件事。没有后台线程碰 Unity 对象——这是反复踩坑后的铁律（后台线程调 Unity API 会触发 GC 崩溃）。

### 1. il2cpp 按名反射（自愈核心）

`il2cpp_api.cpp` 通过 `GetProcAddress(GameAssembly.dll, "il2cpp_*")` 绑定运行时函数，再：

1. 遍历全部程序集镜像，按「命名空间 + 类名」找到目标类（`Reloaded.Gameplay.Board` / `Zombie` / `Projectile` / `Spine.Skeleton` …）
2. 用 `class_get_field_from_name` / `class_get_method_from_name` 按名取字段偏移与方法指针，缓存后复用

**不硬编码任何偏移地址**。游戏小版本更新只要类/字段名不变，一切自动恢复。字段读取一律走 SEH 保护（`__try/__except` 包裹的安全读），对象指针失效时返回失败而不是崩游戏。

### 2. 双缓冲快照

`game_data.cpp` 维护 A/B 两份 `Snapshot` 结构（僵尸/植物/金币/投射物/卡槽五个列表 + 波次/阳光等标量）：

- 每帧填充其中一份（增量内存区域扫描定位 Board，强校验后遍历 DataArray）
- 填完用原子指针发布，下一帧轮换到另一份
- 绘制侧永远只读「已发布」的那份，读写永不同帧交叉

### 3. 修改类逻辑（与快照同线程）

三种手段按场景选用：

| 手段 | 例子 | 说明 |
|---|---|---|
| 直接写字段 | 无限阳光、冻结（冰冻状态计数）、无冷却（写满冷却计数） | SEH 保护直写，游戏自身状态机接着运转 |
| 调游戏方法 | 自动收集（`Coin.ScoreCoin`） | `runtime_invoke` 调用游戏自己的入账逻辑，零参数无类型风险 |
| 逐帧引导 | 魔法子弹（每帧微调子弹 Y 坐标） | X 由游戏动画推进，Y 向目标行限幅逼近 |

关键经验：这游戏的投射物**速度字段恒为零**（移动由动画系统驱动），因此一切基于速度过滤或速度改写的方案都无效；冻结只写速度同样无效（Update 每帧从类型基础速度重算），必须写游戏自身的冰冻状态字段。

### 4. 界面绘制管线

```
hkPresent
  ├─ ensure_rtv（拿后台缓冲 RTV）
  ├─ ImGui NewFrame（DX11 + Win32 后端）
  ├─ game::on_present（快照 + 作弊）
  ├─ 菜单热键轮询（GetAsyncKeyState 边沿检测）
  ├─ esp::Draw（前台绘制列表：ESP/HUD，永远在菜单下面）
  ├─ MenuUi::Draw（菜单窗口，打开时）
  └─ RenderDrawData → 原 Present
```

菜单不用 ImGui 默认皮肤：`render/ui_widgets.cpp` 用 ImDrawList 自绘全部控件（多段渐变、斜面高光、三态悬浮），配合绝对坐标布局系统（`SetCursor/BeginRow`）复刻经典桌面观感。

### 5. 主题与双语的隔离设计

- **主题**：六色（边框/背景/面板/标签页/文字/控件）经**固定通道偏移**派生出全部界面颜色（默认主题与经典红框风格逐位一致）；编辑器实时生效，防抖自动落盘。**ini 键与控件 ID 恒用英文**，语言切换不破坏配置与控件状态
- **双语**：`ui::TR(英文键)` 查表返回中文，漏项自动回退英文显示
- **游戏颜色（ESP）与界面主题是两套完全独立的存储与编辑路径**，互不影响

### 6. 注入器

无参数时：选注入器目录内 mtime 最新的 `*.dll` → 子串匹配找进程（可等待）→ 逐个检查模块列表跳过已注入实例 → 注入 → 校验远端返回 → 结束暂停。多游戏实例时自动挑未注入的那个。

### 7. 构建回退

输出 DLL 被运行中游戏锁定时，`build.bat` 自动改链备用文件名，`sync_dll.bat` 后台轮询，游戏关闭后自动同步回正式文件名并清理。

### 目录

```
src/
  dllmain.cpp        入口、初始化顺序
  dx11_hook.cpp      Present 钩子、ImGui 帧、ESP 绘制
  game_data.cpp/h    il2cpp 反射、快照、修改逻辑、双语名字表
  il2cpp_api.cpp/h   运行时 API 绑定
  projection_math.h  正交投影数学
  injector/          独立注入器（单文件）
  render/            菜单 UI（控件/主题/双语/布局/页面）
  config/            INI 配置、持久化、按键名解析
  app/               生命周期与卸载
vendor/              imgui / freetype / minhook
```

---

## English

### Data flow
Everything runs inside the game's **Present call chain** (render thread): each frame the hook takes a snapshot, applies cheats, then draws. No background thread ever touches Unity objects (a hard-won rule — Unity API from foreign threads triggers GC crashes).

- **Name-based il2cpp reflection**: runtime APIs are bound from `GameAssembly.dll` exports; classes/fields/methods are resolved *by name* and cached — zero hardcoded offsets, self-healing across minor game updates. All reads are SEH-guarded.
- **Double-buffered snapshots**: two alternating `Snapshot` structs (zombies/plants/coins/projectiles/seeds + scalars) filled per frame and published via an atomic pointer; the draw side only ever reads the published copy.
- **Three mutation strategies**: direct field writes (sun, freeze counters, cooldown), `runtime_invoke` of the game's own methods (`Coin.ScoreCoin` for collection), and frame-by-frame steering (magic bullets adjust Y toward the target row while the game's animation advances X).
- **Key lesson**: projectile velocity fields are always zero here (animation-driven movement) — anything velocity-based is a dead end; freezing must use the game's own ice-trap counters.
- **Custom-drawn menu**: all widgets hand-drawn with ImDrawList (multi-stop gradients, bevels, hover states) over an absolute-position layout system — no default ImGui skin.
- **Theme & localization isolation**: six editable colors derive every UI color via fixed channel offsets; ini keys and widget IDs always stay English so switching language never breaks config or widget state; `ui::TR()` maps English keys to Chinese with graceful fallback. Game (ESP) colors and the UI theme are fully independent systems.
- **Injector**: newest-DLL auto-pick by mtime, substring process match with optional waiting, skips already-injected instances (double-injection guard), remote result validation, end-of-run pause.
- **Build fallback**: when the output DLL is locked by a running game, `build.bat` links to a fallback name and `sync_dll.bat` syncs it back automatically once the lock releases.
