// Lumen C++ - spring / bezier / tween.
//
// The tables and the integrator are transcribed from the reference
// implementation on purpose: identical numbers keep the motion identical, and
// the host tests diff frames against the original C library.
#include "lumen/motion.hpp"

#include "lumen/config.hpp"

namespace lumen {

#define LM_CURVE_COUNT kCurveCount
#include "curves.inc"
#undef LM_CURVE_COUNT

// clang-format off
const Motion kMotions[kMotionCount] = {
    /*                    f      z      r   */
    motionDef(           3.6f,  0.82f,  1.0f),   // snappy
    motionDef(           2.6f,  0.78f,  1.0f),   // carousel
    motionDef(           2.2f,  1.00f,  0.6f),   // smooth
    motionDef(           1.9f,  0.90f,  0.0f),   // follow
    motionDef(           2.8f,  0.50f,  1.0f),   // bouncy
    motionDef(           3.2f,  0.62f,  1.3f),   // toggle
    motionDef(           1.2f,  1.00f,  0.0f),   // gentle
    motionDef(           2.3f,  0.78f, -0.8f),   // zoom
    motionDef(           6.0f,  0.45f,  0.0f),   // press
    motionDef(           9.0f,  0.22f,  0.0f),   // wobble
    motionDef(           2.8f,  1.00f, -0.5f),   // exit
    motionDef(           9.0f,  0.75f,  1.0f),   // vu attack
    motionDef(           2.4f,  1.00f,  0.0f),   // vu release
    motionDef(           0.6f,  1.00f,  0.0f),   // platter
};
// clang-format on

// ------------------------------------------------------------------ spring
void Spring::initCustom(const Motion* motion, float v, float scale) {
    value = target = xp = v;
    vel = 0.f;
    m = motion;
    eps = 0.35f / (scale > 1e-6f ? scale : 1e-6f);
    rest = true;
}

void Spring::init(MotionId id, float v, float scale) { initCustom(&kMotions[id], v, scale); }

void Spring::set(float t) {
    if (t != target) {
        target = t;
        rest = false;
    }
}

void Spring::snap(float v) {
    value = target = xp = v;
    vel = 0.f;
    rest = true;
}

void Spring::kick(float velocity) {
    vel += velocity;
    rest = false;
}

void Spring::poke(float v) {
    value = v;
    rest = false;
}

void Spring::update(float dt) {
    if (dt <= 0.f) return;
    float x = target;
    float xd = (x - xp) / dt;   // step input -> impulse through k3
    xp = x;
    if (rest && xd == 0.f) return;

    const Motion* motion = m;
    int steps = static_cast<int>(dt * kSpringHz + 0.999f);
    if (steps < 1) steps = 1;
    float h = dt / static_cast<float>(steps);
    // stability: never let the effective k2 fall below what explicit
    // sub-steps can integrate (t3ssel8r, "Giving Personality to Procedural
    // Animations")
    float k2 = motion->k2;
    float k2min = maxf(h * h / 2.f + h * motion->k1 / 2.f, h * motion->k1);
    if (k2 < k2min) k2 = k2min;
    float y = value, v = vel;
    for (int i = 0; i < steps; i++) {
        y += h * v;
        v += h * (x + motion->k3 * xd - y - motion->k1 * v) / k2;
    }
    value = y;
    vel = v;
    if (absf(y - x) < eps && absf(v) < eps * 8.f) {
        value = x;
        vel = 0.f;
        rest = true;
    }
}

// ------------------------------------------------------------------ curves
float curve(Curve c, float t) {
    if (t <= 0.f) return 0.f;
    if (t >= 1.f) return 1.f;
    const int16_t* q = lm_curve_lut[c];
    float f = t * LM_CURVE_N;
    int i = static_cast<int>(f);
    float a = q[i], b = q[i + 1];
    return (a + (b - a) * (f - static_cast<float>(i))) * (1.f / 4096.f);
}

static float bz(float u, float p1, float p2) {
    float m = 1.f - u;
    return 3.f * m * m * u * p1 + 3.f * m * u * u * p2 + u * u * u;
}

float bezier(float x1, float y1, float x2, float y2, float t) {
    if (t <= 0.f) return 0.f;
    if (t >= 1.f) return 1.f;
    float u = t;
    for (int i = 0; i < 6; i++) {   // Newton on x(u) = t
        float m = 1.f - u;
        float dx = 3.f * m * m * x1 + 6.f * m * u * (x2 - x1) + 3.f * u * u * (1.f - x2);
        float ex = bz(u, x1, x2) - t;
        if (absf(ex) < 1e-4f) break;
        if (absf(dx) < 1e-5f) break;
        u = clamp01(u - ex / dx);
    }
    float lo = 0.f, hi = 1.f;   // bisection polish
    for (int i = 0; i < 8 && absf(bz(u, x1, x2) - t) > 1e-4f; i++) {
        if (bz(u, x1, x2) < t) lo = u; else hi = u;
        u = 0.5f * (lo + hi);
    }
    return bz(u, y1, y2);
}

// ------------------------------------------------------------------ tween
void Tween::start(float from, float to, float duration, Curve c, float delay) {
    a = from;
    b = to;
    dur = duration > 1e-6f ? duration : 1e-6f;
    curve = c;
    t = -delay;
}

float Tween::value() const { return lerpf(a, b, lumen::curve(curve, progress())); }

}  // namespace lumen
