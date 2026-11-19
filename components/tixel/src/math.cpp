#include "tixel/math.hpp"

namespace tixel {

// Bhaskara-style parabola with one refinement step: |err| < 1e-3 over a
// period, far below one pixel at any radius used on a 128 px panel.
float sinf(float x) {
    x = x - kTau * std::floor((x + kPi) / kTau);   // -> [-pi, pi)
    float y = (4.f / kPi) * x - (4.f / (kPi * kPi)) * x * absf(x);
    return 0.225f * (y * absf(y) - y) + y;
}

float cosf(float x) { return sinf(x + kPi * 0.5f); }

float atan2f(float y, float x) {
    float ax = absf(x), ay = absf(y);
    if (ax < 1e-9f && ay < 1e-9f) return 0.f;
    float a = (ax > ay) ? ay / ax : ax / ay;
    float s = a * a;
    float r = ((-0.0464964749f * s + 0.15931422f) * s - 0.327622764f) * s * a + a;
    if (ay > ax) r = 1.57079637f - r;
    if (x < 0) r = kPi - r;
    if (y < 0) r = -r;
    return r;
}

}  // namespace tixel
