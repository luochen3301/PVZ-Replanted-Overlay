# 架构 / Architecture

## 中文

```
injector.exe                ROH.dll（注入进游戏进程）
    │                            │
    │ CreateRemoteThread         ├─ dllmain        初始化线程：il2cpp 解析 → 配置加载
    ▼                            ├─ dx11_hook      Present 钩子（MinHook，dummy 交换链取 vtable）
 游戏进程                        ├─ imgui          菜单与 ESP 绘制（FreeType 字体）
                                 ├─ game_data      每帧快照 + 修改类作弊（Present 线程）
                                 └─ render/ui_*    自绘控件 / 主题 / 双语
```

### 关键设计

- **il2cpp 按名反射**：所有类/字段/方法通过 `GameAssembly.dll` 导出的 il2cpp 运行时 API 按名称解析并缓存偏移，不硬编码任何偏移地址，游戏小版本更新可自愈。
- **双缓冲快照**：`game::on_present` 在 Present 线程每帧填充 A/B 两份 `Snapshot`（僵尸/植物/金币/投射物/卡槽），原子指针发布，绘制侧只读。
- **修改类作弊**：与快照同线程（Present），SEH 保护的内存直写；涉及游戏内部状态机的操作（如收集入账）走 `runtime_invoke` 调用游戏自身方法（`Coin.ScoreCoin`）。
- **菜单绘制**：不用 ImGui 默认皮肤，`render/ui_widgets.cpp` 用 ImDrawList 自绘全部控件（多段渐变/斜面/悬浮态），支持绝对坐标布局系统。
- **主题系统**：`UITheme` 六色（边框/背景/面板/标签页/文字/控件），所有 UI 颜色由六色经固定通道偏移派生，编辑器实时生效；游戏 ESP 颜色与 UI 主题完全隔离。
- **双语**：`ui::TR()` 英文键查表返回中文，漏项自动回退英文；ini 键与控件 ID 恒用英文，避免语言切换破坏配置与控件状态。
- **注入器**：无参数时自动选注入器目录内最新的 `ROH*.dll`；等待游戏进程出现；跳过已注入实例（防双注入）；结束时暂停便于双击使用。
- **构建回退**：`ROH.dll` 被运行中游戏锁定时 `build.bat` 自动改链 `ROH_pending.dll`，`sync_dll.bat` 后台等待锁释放后同步。

### 目录

```
src/
  dllmain.cpp        入口、初始化顺序
  dx11_hook.cpp      Present 钩子、ImGui 帧、ESP 绘制
  game_data.cpp/h    il2cpp 反射、快照、作弊、名字表
  il2cpp_api.cpp/h   GameAssembly 运行时 API 绑定
  injector/          独立注入器
  render/            菜单 UI、控件、主题、双语
  config/            INI 配置、持久化、按键解析
vendor/              imgui / freetype / minhook
```

---

## English

- **Name-based il2cpp reflection**: every class/field/method is resolved by name via the il2cpp runtime API exported by `GameAssembly.dll` — no hardcoded offsets, self-healing across minor game updates.
- **Double-buffered snapshots**: `game::on_present` fills two alternating `Snapshot` structs per frame on the Present thread and publishes via an atomic pointer; the draw side is read-only.
- **Memory-write cheats** run on the Present thread with SEH-guarded reads/writes; state-machine operations (e.g. collecting coins) call the game's own methods via `runtime_invoke` (`Coin.ScoreCoin`).
- **Custom-drawn menu**: all widgets are hand-drawn with ImDrawList (multi-stop gradients, bevels, hover states) instead of the default ImGui skin.
- **Theme system**: six editable colors derive every UI color through fixed channel offsets; live preview; game (ESP) colors and UI theme are fully independent.
- **Localization**: `ui::TR()` maps English keys to Chinese with graceful fallback; ini keys and widget IDs always stay English.
- **Injector**: picks the newest `ROH*.dll` by mtime, waits for the game, skips already-injected instances, pauses at exit for double-click use.
- **Build fallback**: when `ROH.dll` is locked by a running game, `build.bat` links `ROH_pending.dll` and `sync_dll.bat` syncs it back once the lock releases.
