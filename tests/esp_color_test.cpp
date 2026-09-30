#include "../src/render/esp_color.h"
#include "../src/render/menu_state.h"
#include "../src/config/config.h"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>

using sibalhook::EspColor;

int main() {
    EspColor color{1.0f, 70.0f / 255.0f, 0.0f, 200.0f / 255.0f};
    assert(sibalhook::ColorToHex(color) == "#FF4600C8");
    EspColor loaded{};
    assert(sibalhook::ColorFromHex("#1234AB80", loaded));
    assert(sibalhook::ColorToHex(loaded) == "#1234AB80");
    assert(sibalhook::ColorFromHex("a0b0c0d0", loaded));
    assert(sibalhook::ColorToHex(loaded) == "#A0B0C0D0");
    const EspColor before = loaded;
    assert(!sibalhook::ColorFromHex("#12ZZAB80", loaded));
    assert(!sibalhook::ColorFromHex("#123456", loaded));
    assert(loaded == before);
    assert(sibalhook::ColorToHex(EspColor{-1.0f, 2.0f, 0.5f, 1.0f}) == "#00FF80FF");

    auto& state = sibalhook::MenuState::Instance();
    auto& config = sibalhook::Config::Instance();
    config.SetPath(L"build\\esp_color_roundtrip.ini");
    const std::array<EspColor sibalhook::MenuState::*, 8> fields = {
        &sibalhook::MenuState::zombieBoxColor,
        &sibalhook::MenuState::mindControlledBoxColor,
        &sibalhook::MenuState::zombieSkeletonColor,
        &sibalhook::MenuState::zombieHeadDotColor,
        &sibalhook::MenuState::zombieTracerColor,
        &sibalhook::MenuState::plantBoxColor,
        &sibalhook::MenuState::sunRingColor,
        &sibalhook::MenuState::coinRingColor
    };
    std::array<std::string, 8> expected{};
    for (size_t i = 0; i < fields.size(); ++i) {
        state.*fields[i] = EspColor{static_cast<float>(i) / 8.0f, 0.25f, 0.75f,
                                    static_cast<float>(i + 1) / 8.0f};
        expected[i] = sibalhook::ColorToHex(state.*fields[i]);
    }
    state.zombieEsp = false;
    state.SaveToConfig();
    assert(config.Save());
    state = sibalhook::MenuState();
    assert(config.Load());
    state.LoadFromConfig();
    for (size_t i = 0; i < fields.size(); ++i)
        assert(sibalhook::ColorToHex(state.*fields[i]) == expected[i]);
    assert(!state.zombieEsp);
    assert(std::remove("build\\esp_color_roundtrip.ini") == 0);
    std::puts("ESP color persistence tests passed");
}
