#include "../src/projection_math.h"
#include <cassert>
#include <limits>
#include <cstdio>

static bool near(float a, float b) { return std::fabs(a - b) < 0.001f; }
int main() {
    float x = 640, y = 100;
    assert(projection::screen_to_overlay(x, y, 1280, 720, 2560, 1440));
    assert(near(x, 1280) && near(y, 1240));
    x = 0; y = 720;
    assert(projection::screen_to_overlay(x, y, 1280, 720, 1280, 720));
    assert(near(x, 0) && near(y, 0));
    assert(!projection::screen_to_overlay(x, y, 0, 720, 1280, 720));
    x = std::numeric_limits<float>::quiet_NaN();
    assert(!projection::screen_to_overlay(x, y, 1280, 720, 1280, 720));

    // 90-degree rotation, nonuniform scale (2,3), translation (10,20,30).
    float m[16] = {0,2,0,0, -3,0,0,0, 0,0,1,0, 10,20,30,1};
    float world[3]{};
    assert(projection::transform_bone(m, 4, 5, world));
    assert(near(world[0], -5) && near(world[1], 28) && near(world[2], 30));
    m[0] = -2; m[1] = 0; m[4] = 0; m[5] = 3;
    assert(projection::transform_bone(m, 4, 5, world));
    assert(near(world[0], 2) && near(world[1], 35));

    float ky = -2.82f;
    assert(!projection::calibrate_axis(0, 0, ky));
    assert(near(ky, -2.82f)); // Same-row sample must preserve the Y direction.
    assert(projection::calibrate_axis(100, -300, ky));
    assert(near(ky, -3));
    assert(!projection::calibrate_axis(100, 0, ky));
    assert(!projection::calibrate_axis(100, std::numeric_limits<float>::infinity(), ky));
    assert(near(ky, -3));
    std::puts("projection regression tests passed");
}
