#include "framework.h"
#include "dx11_hook.h"
#include "il2cpp_api.h"
#include "game_data.h"
#include "render/menu.h"
#include "render/menu_state.h"
#include "render/menu_ui.h"
#include "app/app.h"
#include "config/hotkeys.h"

#include <d3d11.h>
#include <dxgi.h>

#include "MinHook.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include "imgui_impl_win32.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg,
                                                             WPARAM wParam, LPARAM lParam);

namespace dx11 {

typedef long(__stdcall* PresentFn)(IDXGISwapChain*, UINT, UINT);

static PresentFn oPresent = nullptr;
static ID3D11Device* g_device = nullptr;
static ID3D11DeviceContext* g_ctx = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;
static HWND g_hwnd = nullptr;
static WNDPROC oWndProc = nullptr;
static bool g_imguiReady = false;
static bool g_unloadTriggered = false;
static float g_lastRtvW = 0, g_lastRtvH = 0;

// ---------------- ESP 绘制 ----------------
namespace esp {

static ImU32 EspU32(const sibalhook::EspColor& color) {
    return ImGui::ColorConvertFloat4ToU32(ImVec4(color[0], color[1], color[2], color[3]));
}

static ImU32 lerpColor(float t) {   // t: 1满血 -> 0空血  绿->黄->红
    int r, g;
    if (t > 0.5f) { r = (int)((1.0f - t) * 2 * 255); g = 255; }
    else          { r = 255; g = (int)(t * 2 * 255); }
    return IM_COL32(r, g, 60, 255);
}

static void DrawHpBar(ImDrawList* dl, float x, float y, float w, float h,
                      int body, int bodyMax, int helm, int helmMax,
                      int shield, int shieldMax, bool byRatio) {
    int totalMax = bodyMax + helmMax + shieldMax;
    if (totalMax <= 0 || w <= 4) return;
    dl->AddRectFilled(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + h + 1),
                      IM_COL32(0, 0, 0, 200));
    float cx = x;
    auto seg = [&](int hp, int hpMax, ImU32 col) {
        if (hpMax <= 0 || hp <= 0) return;
        float sw = w * (hp / (float)totalMax);
        dl->AddRectFilled(ImVec2(cx, y), ImVec2(cx + sw, y + h), col);
        cx += sw;
    };
    float ratio = totalMax ? (body + helm + shield) / (float)totalMax : 0;
    ImU32 bodyCol = byRatio ? lerpColor(ratio) : IM_COL32(80, 220, 80, 255);
    seg(body, bodyMax, bodyCol);
    seg(helm, helmMax, IM_COL32(80, 150, 255, 255));
    seg(shield, shieldMax, IM_COL32(200, 200, 200, 255));
}

static void DrawCornerBox(ImDrawList* dl, const ImVec2& a, const ImVec2& b, ImU32 col, float th) {
    float len = (b.x - a.x) * 0.25f;
    len = (len < 4) ? 4 : len;
    // 左上
    dl->AddLine(ImVec2(a.x, a.y), ImVec2(a.x + len, a.y), col, th);
    dl->AddLine(ImVec2(a.x, a.y), ImVec2(a.x, a.y + len), col, th);
    // 右上
    dl->AddLine(ImVec2(b.x - len, a.y), ImVec2(b.x, a.y), col, th);
    dl->AddLine(ImVec2(b.x, a.y), ImVec2(b.x, a.y + len), col, th);
    // 左下
    dl->AddLine(ImVec2(a.x, b.y - len), ImVec2(a.x, b.y), col, th);
    dl->AddLine(ImVec2(a.x, b.y), ImVec2(a.x + len, b.y), col, th);
    // 右下
    dl->AddLine(ImVec2(b.x, b.y - len), ImVec2(b.x, b.y), col, th);
    dl->AddLine(ImVec2(b.x - len, b.y), ImVec2(b.x, b.y), col, th);
}

static void DrawZombieSkeleton(ImDrawList* dl, const std::vector<game::BoneSnap>& bones,
                               float lineW, const sibalhook::MenuState& s,
                               const game::ZombieSnap& z) {
    auto findBone = [&](const char* name) -> const game::BoneSnap* {
        for (const auto& bone : bones)
            if (strcmp(bone.name, name) == 0) return &bone;
        return nullptr;
    };
    // The Spine rig parents most body parts directly to root. Those links are
    // animation dependencies, not anatomical joints, so build a visible pose.
    static const char* chains[][2] = {
        { "zombie_body", "zombie_neck" },
        { "zombie_neck", "anim_head1" },
        { "zombie_body", "zombie_outerarm_upper" },
        { "zombie_outerarm_upper", "zombie_outerarm_lower" },
        { "zombie_outerarm_lower", "zombie_outerarm_hand" },
        { "zombie_body", "anim_innerarm1" },
        { "anim_innerarm1", "anim_innerarm2" },
        { "anim_innerarm2", "anim_innerarm3" },
        { "zombie_body", "zombie_outerleg_upper" },
        { "zombie_outerleg_upper", "zombie_outerleg_lower" },
        { "zombie_outerleg_lower", "zombie_outerleg_foot" },
        { "zombie_body", "zombie_innerleg_upper" },
        { "zombie_innerleg_upper", "zombie_innerleg_lower" },
        { "zombie_innerleg_lower", "zombie_innerleg_foot" }
    };
    ImU32 color = EspU32(s.zombieSkeletonColor);
    for (const auto& chain : chains) {
        const auto* a = findBone(chain[0]);
        const auto* b = findBone(chain[1]);
        if (!a || !b) continue;
        dl->AddLine(ImVec2(a->x, a->y), ImVec2(b->x, b->y), color, lineW);
        dl->AddCircleFilled(ImVec2(a->x, a->y), 2.2f, color, 8);
        dl->AddCircleFilled(ImVec2(b->x, b->y), 2.2f, color, 8);
    }
    if (s.zombieHeadDot && z.hasHead)
        dl->AddCircleFilled(ImVec2(z.headX, z.headY), 4.0f, EspU32(s.zombieHeadDotColor));
}

static void Draw() {
    auto& s = sibalhook::MenuState::Instance();
    auto& menu = sibalhook::Menu::Instance();
    if (menu.panicHidden) return;

    const game::Snapshot* snap = game::current();
    ImDrawList* dl = ImGui::GetForegroundDrawList();

    // 水印
    if (s.watermark) {
        static float fps = 60.0f;
        fps = fps * 0.95f + ImGui::GetIO().Framerate * 0.05f;
        char wm[180];
        int nz = snap ? (int)snap->zombies.size() : 0;
        int nsun = snap ? snap->sun : 0;
        int df = game::debug_flags();
        _snprintf(wm, sizeof(wm), "PvZ Overlay | PvZ Replanted | %.0f fps | z:%d sun:%d | board:%d cam:%d aff:%d",
                  fps, nz, nsun, (df & 1) ? 1 : 0, (df & 2) ? 1 : 0, (df & 4) ? 1 : 0);
        dl->AddText(ImVec2(9, 9), IM_COL32(0, 0, 0, 220), wm);
        dl->AddText(ImVec2(8, 8), IM_COL32(255, 60, 60, 255), wm);
    }

    if (!snap || !snap->valid) return;

    // ---- 僵尸 ----
    if (s.zombieEsp) {
        for (const auto& z : snap->zombies) {
            if (z.dead) continue;
            const ImVec2 view = ImGui::GetIO().DisplaySize;
            if (!z.hasBox || z.bx1 < 0 || z.by1 < 0 ||
                z.bx0 > view.x || z.by0 > view.y) continue;
            char name[48];
            game::zombie_name_for(z.type, name, sizeof(name));

            if (z.hasBox && s.zombieBox && s.boxStyle < 2) {
                ImVec2 a(z.bx0, z.by0), b(z.bx1, z.by1);
                ImU32 col = z.mindControlled ? EspU32(s.mindControlledBoxColor) : EspU32(s.zombieBoxColor);
                if (s.boxStyle == 1) DrawCornerBox(dl, a, b, col, s.lineW);
                else dl->AddRect(a, b, col, 0.0f, 0, s.lineW);

                if (s.zombieHpBar) {
                    DrawHpBar(dl, a.x, b.y + 3.0f, b.x - a.x, 4.0f,
                              z.hp, z.hpMax, z.helm, z.helmMax, z.shield, z.shieldMax,
                              s.hpColorByRatio);
                }
                // 文本堆叠
                float ty = a.y - 16.0f;
                if (s.zombieName) {
                    char label[80];
                    if (s.zombieRowText) _snprintf(label, sizeof(label), "%s %s%d", name,
                                               s.zhText ? "行" : "r", z.row);
                    else strcpy_s(label, name);
                    ImVec2 ts = ImGui::CalcTextSize(label);
                    dl->AddText(ImVec2((a.x + b.x) * 0.5f - ts.x * 0.5f, ty), IM_COL32(255, 255, 255, 230), label);
                    ty -= 14.0f;
                }
                if (s.zombieHpText) {
                    char hp[48];
                    _snprintf(hp, sizeof(hp), "%d/%d%s%s",
                              z.hp + z.helm + z.shield, z.hpMax + z.helmMax + z.shieldMax,
                              z.helm > 0 ? (s.zhText ? " +盾" : " +Sh") : "",
                              z.mindControlled ? (s.zhText ? " [魅惑]" : " [MC]") : "");
                    ImVec2 ts = ImGui::CalcTextSize(hp);
                    dl->AddText(ImVec2((a.x + b.x) * 0.5f - ts.x * 0.5f, ty), lerpColor(
                        (z.hpMax + z.helmMax + z.shieldMax) ? (z.hp + z.helm + z.shield) / (float)(z.hpMax + z.helmMax + z.shieldMax) : 1), hp);
                }
            }
            // 射线从屏幕上沿向僵尸头部照射。
            if (s.zombieTracer && z.hasBox) {
                ImGuiIO& io = ImGui::GetIO();
                float tx = z.hasHead ? z.headX : (z.bx0 + z.bx1) * 0.5f;
                float ty = z.hasHead ? z.headY : z.by0;
                dl->AddLine(ImVec2(io.DisplaySize.x * 0.5f, 2.0f),
                            ImVec2(tx, ty), EspU32(s.zombieTracerColor), 1.4f);
            }

            if (s.zombieSkeleton && !z.bones.empty())
                DrawZombieSkeleton(dl, z.bones, s.lineW * 1.1f, s, z);
            // 头点（无骨骼时用框顶中心）
            if (s.zombieHeadDot && !s.zombieSkeleton && z.hasBox) {
                dl->AddCircleFilled(z.hasHead ? ImVec2(z.headX, z.headY) :
                                    ImVec2((z.bx0 + z.bx1) * 0.5f, z.by0),
                                    3.0f, EspU32(s.zombieHeadDotColor));
            }
        }
    }

    // ---- 植物 ----
    if (s.plantEsp) {
        for (const auto& p : snap->plants) {
            if (p.dead || !p.hasBox) continue;
            ImVec2 a(p.bx0, p.by0), b(p.bx1, p.by1);
            dl->AddRect(a, b, EspU32(s.plantBoxColor), 0.0f, 0, 1.2f);
            if (s.plantHpBar && p.hpMax > 0) {
                float ratio = p.hp / (float)p.hpMax;
                dl->AddRectFilled(ImVec2(a.x - 1, b.y + 2), ImVec2(b.x + 1, b.y + 5), IM_COL32(0, 0, 0, 180));
                dl->AddRectFilled(ImVec2(a.x, b.y + 3), ImVec2(a.x + (b.x - a.x) * ratio, b.y + 4),
                                  lerpColor(ratio));
            }
            if (s.plantName) {
                char name[48];
                game::plant_name_for(p.type, name, sizeof(name));
                if (p.asleep) strcat_s(name, s.zhText ? " [睡]" : " [Zzz]");
                ImVec2 ts = ImGui::CalcTextSize(name);
                dl->AddText(ImVec2((a.x + b.x) * 0.5f - ts.x * 0.5f, a.y - 15.0f),
                            IM_COL32(120, 255, 160, 230), name);
            }
        }
    }

    // ---- 阳光/金币 ----
    for (const auto& c : snap->coins) {
        if (!c.hasPt) continue;
        bool isSun = (c.type >= 4 && c.type <= 6);
        if (isSun && !s.sunEsp) continue;
        if (!isSun && !s.coinEsp) continue;
        if (isSun) {
            dl->AddCircle(ImVec2(c.sx, c.sy), 10.0f, EspU32(s.sunRingColor), 12, 2.0f);
        } else {
            dl->AddCircle(ImVec2(c.sx, c.sy), 7.0f, EspU32(s.coinRingColor), 10, 1.6f);
        }
        if (s.coinValueText) {
            char name[32];
            game::coin_name_for(c.type, name, sizeof(name));
            dl->AddText(ImVec2(c.sx - 12, c.sy - 26), IM_COL32(255, 240, 160, 230), name);
        }
    }

    // ---- 弹道 ----
    if (s.projectileTrajectory) {
        const ImU32 col = EspU32(s.projectileTrajectoryColor);
        for (const auto& pr : snap->projectiles) {
            if (!pr.hasEnd) continue;
            dl->AddLine(ImVec2(pr.sx, pr.sy), ImVec2(pr.ex, pr.ey), col, 1.2f);
            dl->AddCircleFilled(ImVec2(pr.sx, pr.sy), 2.0f, col, 8);
        }
    }

    // ---- 卡槽冷却 ----
    if (s.seedCooldownHud && !snap->seeds.empty()) {
        float x = 12.0f, y = 60.0f;
        for (const auto& sd : snap->seeds) {
            char name[48];
            game::plant_name_for(sd.type, name, sizeof(name));
            // counter 往上数到 refreshTime：充能式进度（空→满，满=就绪）
            float ratio = 1.0f;
            if (sd.refreshing && sd.refreshTime > 0)
                ratio = sd.refresh / (float)sd.refreshTime;
            if (ratio < 0) ratio = 0;
            if (ratio > 1) ratio = 1;
            dl->AddText(ImVec2(x, y), IM_COL32(230, 230, 230, 220), name);
            dl->AddRectFilled(ImVec2(x + 130, y + 3), ImVec2(x + 230, y + 7), IM_COL32(0, 0, 0, 190));
            dl->AddRectFilled(ImVec2(x + 130, y + 3), ImVec2(x + 130 + 100 * ratio, y + 7),
                              ratio >= 1.0f ? IM_COL32(120, 255, 120, 255) : IM_COL32(255, 180, 60, 255));
            y += 16.0f;
        }
    }

    // ---- 波次 ----
    if (s.waveHud && snap->numWaves > 0) {
        ImGuiIO& io = ImGui::GetIO();
        float cx = io.DisplaySize.x * 0.5f;
        char t[64];
        _snprintf(t, sizeof(t), s.zhText ? "第 %d / %d 波" : "Wave %d / %d",
                  snap->wave, snap->numWaves);
        ImVec2 ts = ImGui::CalcTextSize(t);
        dl->AddText(ImVec2(cx - ts.x * 0.5f, 34), IM_COL32(255, 255, 255, 240), t);
        dl->AddRectFilled(ImVec2(cx - 100, 52), ImVec2(cx + 100, 57), IM_COL32(0, 0, 0, 190));
        dl->AddRectFilled(ImVec2(cx - 100, 52), ImVec2(cx - 100 + 200 * (1.0f - snap->waveProgress), 57),
                          IM_COL32(255, 80, 80, 255));
    }
}

} // namespace esp

// ---------------- ImGui 初始化 ----------------
static bool ensure_rtv(IDXGISwapChain* sc) {
    ID3D11Texture2D* bb = nullptr;
    if (FAILED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&bb))) return false;
    D3D11_TEXTURE2D_DESC desc;
    bb->GetDesc(&desc);
    if (g_rtv && desc.Width == g_lastRtvW && desc.Height == g_lastRtvH) {
        bb->Release();
        return true;
    }
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
    if (FAILED(g_device->CreateRenderTargetView(bb, nullptr, &g_rtv))) {
        bb->Release();
        return false;
    }
    g_lastRtvW = (float)desc.Width;
    g_lastRtvH = (float)desc.Height;
    bb->Release();
    return true;
}

static bool load_fonts() {
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->Clear();
    ImFontConfig cfg;
    // FreeType handles glyph hinting; stb_truetype oversampling is not used.
    static const ImWchar cnRanges[] = {
        0x0020, 0x00FF,   // ASCII + Latin
        0x2000, 0x206F,
        0x3000, 0x30FF,   // CJK 符号
        0x31F0, 0x31FF,
        0x4e00, 0x9FAF,   // CJK 统一
        0xFF00, 0xFFEF,
        0,
    };
    const char* fonts[] = {"C:\\Windows\\Fonts\\msyh.ttc",
                           "C:\\Windows\\Fonts\\simhei.ttf",
                           "C:\\Windows\\Fonts\\simsun.ttc"};
    ImFont* f = nullptr;
    for (auto p : fonts) {
        if (GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES) {
            f = io.Fonts->AddFontFromFileTTF(p, 15.0f, &cfg, cnRanges);
            if (f) break;
        }
    }
    if (!f) f = io.Fonts->AddFontDefault();
    if (!io.Fonts->Build()) {
        roh::log("[dx11] fonts: FreeType v35 atlas build failed");
        return false;
    }
    roh::log("[dx11] fonts: FreeType TrueType interpreter v35, built %p", (void*)f);
    // 注意：此处 DX11 后端尚未 Init，不能调用 InvalidateDeviceObjects（空指针崩溃）
    return true;
}

static LRESULT CALLBACK hkWndProcStub(HWND h, UINT msg, WPARAM w, LPARAM l);

static bool init_imgui(IDXGISwapChain* sc) {
    if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), (void**)&g_device))) {
        roh::log("[dx11] init: GetDevice failed");
        return false;
    }
    g_device->GetImmediateContext(&g_ctx);
    if (!ensure_rtv(sc)) {
        roh::log("[dx11] init: ensure_rtv failed");
        return false;
    }

    DXGI_SWAP_CHAIN_DESC sd;
    sc->GetDesc(&sd);
    g_hwnd = sd.OutputWindow;
    if (!g_hwnd) {
        roh::log("[dx11] init: no OutputWindow");
        return false;
    }

    ImGui::CreateContext();
    roh::log("[dx11] init: context created");
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    if (!load_fonts()) {
        ImGui::DestroyContext();
        return false;
    }
    roh::log("[dx11] init: fonts loaded");
    ImGui_ImplWin32_Init(g_hwnd);
    ImGui_ImplDX11_Init(g_device, g_ctx);
    roh::log("[dx11] init: backends inited");

    oWndProc = (WNDPROC)SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)hkWndProcStub);
    g_imguiReady = true;
    roh::log("[dx11] imgui ready, hwnd=%p", g_hwnd);
    return true;
}

static LRESULT CALLBACK hkWndProcStub(HWND h, UINT msg, WPARAM w, LPARAM l) {
    if (g_imguiReady) {
        ImGui_ImplWin32_WndProcHandler(h, msg, w, l);
        auto& menu = sibalhook::Menu::Instance();
        if (menu.open) {
            // 菜单打开时屏蔽游戏输入
            switch (msg) {
            case WM_KEYDOWN: case WM_KEYUP: case WM_CHAR: case WM_SYSKEYDOWN: case WM_SYSKEYUP:
            case WM_SYSCHAR: case WM_IME_CHAR:
            case WM_MOUSEMOVE: case WM_LBUTTONDOWN: case WM_LBUTTONUP:
            case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_MBUTTONDOWN: case WM_MBUTTONUP:
            case WM_XBUTTONDOWN: case WM_XBUTTONUP: case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
            case WM_INPUT: case WM_ACTIVATE:
                return 1;
            default:
                break;
            }
        }
    }
    return CallWindowProcW(oWndProc, h, msg, w, l);
}

// ---------------- Present ----------------
static long __stdcall hkPresent(IDXGISwapChain* sc, UINT sync, UINT flags) {
    // 渲染线程必须注册进 il2cpp GC（gfx-threading-mode=6 时 Present 在渲染线程，
    // 未注册线程里 runtime_invoke 的分配会触发 "Fatal error in GC"）
    static thread_local bool t_gcAttached = false;
    if (!t_gcAttached && il2cpp::g_domain && il2cpp::thread_attach) {
        __try { il2cpp::thread_attach(il2cpp::g_domain); }
        __except (EXCEPTION_EXECUTE_HANDLER) {}
        t_gcAttached = true;
        roh::log("[dx11] present thread attached to il2cpp GC");
    }

    if (!g_imguiReady) {
        if (!init_imgui(sc)) return oPresent(sc, sync, flags);
    }

    if (!sibalhook::App::Instance().IsUnloading()) {
        if (ensure_rtv(sc)) {
            ImGuiIO& io = ImGui::GetIO();
            ImGui_ImplDX11_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();
            game::on_present(io.DisplaySize.x, io.DisplaySize.y); // Current ImGui coordinate extent

            sibalhook::Menu::Instance().PollHotkeys();
            esp::Draw();
            if (sibalhook::Menu::Instance().IsOpen())
                sibalhook::MenuUi::Draw();

            ImGui::Render();
            g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }
    } else if (!g_unloadTriggered) {
        g_unloadTriggered = true;
    }

    return oPresent(sc, sync, flags);
}

// ---------------- 安装 ----------------
static bool get_present_ptr(PresentFn& out) {
    // dummy 设备+交换链取 vtable
    DXGI_SWAP_CHAIN_DESC sd = {};
    sd.BufferCount = 2;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.Width = 640;
    sd.BufferDesc.Height = 480;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = GetDesktopWindow();
    sd.SampleDesc.Count = 1;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    IDXGISwapChain* sc = nullptr;
    ID3D11Device* dev = nullptr;
    D3D_FEATURE_LEVEL fl;
    ID3D11DeviceContext* ctx = nullptr;

    // 尝试 1: 设备+交换链（硬件）
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
        &sd, &sc, &dev, &fl, &ctx);
    roh::log("[dx11] hw device+sc hr=0x%lX", (unsigned long)hr);
    // 尝试 2: WARP
    if (FAILED(hr) || !sc) {
        if (sc) sc->Release();
        if (dev) dev->Release();
        if (ctx) ctx->Release();
        sc = nullptr; dev = nullptr; ctx = nullptr;
        hr = D3D11CreateDeviceAndSwapChain(
            nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION,
            &sd, &sc, &dev, &fl, &ctx);
        roh::log("[dx11] warp device+sc hr=0x%lX", (unsigned long)hr);
    }
    // 尝试 3: 只建设备，再用 factory 建交换链（自建隐藏窗口）
    if (FAILED(hr) || !sc) {
        if (dev) dev->Release();
        if (ctx) ctx->Release();
        dev = nullptr; ctx = nullptr;
        sc = nullptr;
        hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0,
                               D3D11_SDK_VERSION, &dev, &fl, &ctx);
        roh::log("[dx11] hw device-only hr=0x%lX", (unsigned long)hr);
        if (FAILED(hr) || !dev) {
            hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
                                   D3D11_SDK_VERSION, &dev, &fl, &ctx);
            roh::log("[dx11] warp device-only hr=0x%lX", (unsigned long)hr);
        }
        if (SUCCEEDED(hr) && dev) {
            HWND own = CreateWindowExW(0, L"STATIC", L"ROH_dummy", WS_POPUP, 0, 0, 100, 100,
                                       nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
            roh::log("[dx11] own hwnd=%p", own);
            IDXGIFactory* factory = nullptr;
            HRESULT fhr = CreateDXGIFactory1(__uuidof(IDXGIFactory), (void**)&factory);
            roh::log("[dx11] CreateDXGIFactory1 hr=0x%lX", (unsigned long)fhr);
            if (SUCCEEDED(fhr) && factory && own) {
                DXGI_SWAP_CHAIN_DESC sd2 = sd;
                sd2.OutputWindow = own;
                hr = factory->CreateSwapChain(dev, &sd2, &sc);
                roh::log("[dx11] factory CreateSwapChain hr=0x%lX", (unsigned long)hr);
                factory->Release();
            }
        }
    }
    if (!sc) {
        if (dev) dev->Release();
        if (ctx) ctx->Release();
        return false;
    }
    void** vt = *(void***)sc;
    out = (PresentFn)vt[8];
    sc->Release();
    if (dev) dev->Release();
    if (ctx) ctx->Release();
    return true;
}

bool install() {
    PresentFn target = nullptr;
    if (!get_present_ptr(target)) {
        roh::log("[dx11] dummy swapchain failed");
        return false;
    }
    if (MH_Initialize() != MH_OK) { /* 可能已初始化 */ }
    if (MH_CreateHook((void*)target, (void*)hkPresent, (void**)&oPresent) != MH_OK) {
        roh::log("[dx11] MH_CreateHook failed");
        return false;
    }
    if (MH_EnableHook((void*)target) != MH_OK) {
        roh::log("[dx11] MH_EnableHook failed");
        return false;
    }
    roh::log("[dx11] Present hooked @ %p", (void*)target);
    return true;
}

void shutdown() {
    if (g_hwnd && oWndProc) {
        SetWindowLongPtrW(g_hwnd, GWLP_WNDPROC, (LONG_PTR)oWndProc);
        oWndProc = nullptr;
    }
    if (g_imguiReady) {
        ImGui_ImplDX11_Shutdown();
        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        g_imguiReady = false;
    }
    if (g_rtv) { g_rtv->Release(); g_rtv = nullptr; }
    if (g_ctx) { g_ctx->Release(); g_ctx = nullptr; }
    if (g_device) { g_device->Release(); g_device = nullptr; }
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
}

} // namespace dx11
