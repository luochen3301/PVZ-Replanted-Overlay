#pragma once

// UI 本地化：英文原文即键，查表得中文。查不到原样返回英文（漏翻自动降级）。
// 语言开关：MenuState.zhText（Settings -> Language -> Chinese Text）。
namespace sibalhook::ui {

const char* TR(const char* en);

} // namespace sibalhook::ui
