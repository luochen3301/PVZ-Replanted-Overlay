#pragma once

namespace dx11 {
bool install();       // 挂 IDXGISwapChain::Present
void shutdown();      // 卸载钩子（安全时机调用）
}
