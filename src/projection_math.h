#pragma once
#include <cmath>

namespace projection {
inline bool screen_to_overlay(float& x, float& y, float screenW, float screenH,
                              float overlayW, float overlayH) {
    if (!std::isfinite(x) || !std::isfinite(y) || screenW <= 1 || screenH <= 1 ||
        overlayW <= 1 || overlayH <= 1) return false;
    x *= overlayW / screenW;
    y = (screenH - y) * overlayH / screenH;
    return std::isfinite(x) && std::isfinite(y);
}

// Unity Matrix4x4 stores columns consecutively. Spine points are local XY.
inline bool transform_bone(const float* m, float x, float y, float* world) {
    for (int i = 0; i < 3; ++i) {
        world[i] = m[i] * x + m[4 + i] * y + m[12 + i];
        if (!std::isfinite(world[i])) return false;
    }
    return true;
}

inline bool calibrate_axis(float boardDelta, float worldDelta, float& scale) {
    if (std::fabs(boardDelta) <= 20) return false;
    const float candidate = worldDelta / boardDelta;
    if (!std::isfinite(candidate) || std::fabs(candidate) <= 0.01f ||
        std::fabs(candidate) >= 100) return false;
    scale = candidate;
    return true;
}
}
