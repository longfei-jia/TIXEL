// TIXEL C++ - small float helpers.
//
// Only sqrtf/fabsf/floorf/ceilf come from libm; trig is polynomial so the core
// does not pull the full sin/cos/atan2 implementations into flash.
#pragma once

#include <cmath>
#include <cstdint>

namespace tixel {

inline constexpr float kPi = 3.14159265f;
inline constexpr float kTau = 6.28318531f;

inline float clampf(float v, float lo, float hi) { return v < lo ? lo : v > hi ? hi : v; }
inline float clamp01(float v) { return clampf(v, 0.f, 1.f); }
inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }
inline float absf(float v) { return v < 0 ? -v : v; }
inline float minf(float a, float b) { return a < b ? a : b; }
inline float maxf(float a, float b) { return a > b ? a : b; }
inline int mini(int a, int b) { return a < b ? a : b; }
inline int maxi(int a, int b) { return a > b ? a : b; }

// round half away from zero -> int
inline int roundi(float v) { return static_cast<int>(v < 0 ? v - 0.5f : v + 0.5f); }

inline float remap(float v, float a0, float a1, float b0, float b1) {
    float t = (a1 != a0) ? (v - a0) / (a1 - a0) : 1.f;
    return b0 + (b1 - b0) * clamp01(t);
}

float sinf(float x);              // |err| < 1e-3
float cosf(float x);
float atan2f(float y, float x);   // |err| < 5e-3 rad

// deterministic PRNG (xorshift32) for demo data
struct Rng {
    uint32_t s = 0;
    uint32_t next() {
        uint32_t x = s ? s : 0x9E3779B9u;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        return s = x;
    }
    float nextFloat() { return static_cast<float>(next() >> 8) * (1.f / 16777216.f); }
};

}  // namespace tixel
