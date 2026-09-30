#include "imgui.h"

#include <cassert>
#include <cstdio>

int main() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    const ImWchar ranges[] = {0x0020, 0x007E, 0x4E00, 0x4E10, 0};
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 15.0f,
                                               nullptr, ranges);
    assert(font != nullptr);
    assert(io.Fonts->Build());
    assert(font->FindGlyphNoFallback(0x4E00) != nullptr);
    unsigned char* pixels = nullptr;
    int width = 0, height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);
    assert(pixels != nullptr && width > 0 && height > 0);
    ImGui::DestroyContext();
    std::puts("FreeType v35 ImGui font atlas test passed");
}
