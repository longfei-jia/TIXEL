// TIXEL C++ - motion primitives.
//
//  Spring  second-order system with a zero:
//              y + k1*y' + k2*y'' = x + k3*x'
//              k1 = 2z/w, k2 = 1/w^2, k3 = r*z/w, w = 2*pi*f
//           f: speed, z: damping ratio, r: initial response
//
//  Curve   CSS cubic-bezier timing curve, baked offline into a Q12 LUT
//          (u8g2's tables / upstream tools/gen_curves.py) so evaluation is one
//          lookup + lerp.
//
//  Motion tokens (MotionId) are the shared design language: widgets pick a
//  token, never raw numbers, so the whole UI moves consistently.
#pragma once

#include <cstdint>

#include "tixel/math.hpp"

namespace tixel {

// ---- second-order system ----------------------------------------------
struct Motion {
    float f, z, r;     // design parameters (kept for tooling/debug)
    float k1, k2, k3;  // derived coefficients
};

constexpr Motion motionDef(float f, float z, float r) {
    return Motion{f, z, r,
                  2.f * z / (6.2831853f * f),
                  1.f / ((6.2831853f * f) * (6.2831853f * f)),
                  r * z / (6.2831853f * f)};
}

enum MotionId : int {
    kMotionSnappy,     // cursors, underlines: 90% in ~85 ms
    kMotionCarousel,   // heavier content, 4% overshoot
    kMotionSmooth,     // scrolling, paging: critically damped
    kMotionFollow,     // secondary elements trailing a primary one
    kMotionBouncy,     // playful feedback, one visible bounce
    kMotionToggle,     // switch knob: leaps, overshoots, settles
    kMotionGentle,     // physical inertia (brightness, dimming)
    kMotionZoom,       // confirmed navigation: anticipation then launch
    kMotionPress,      // press squash that rings once
    kMotionWobble,     // boundary shake
    kMotionExit,       // leave the screen: lift slightly, then go
    kMotionVuAttack,   // meter ballistics: fast rise
    kMotionVuRelease,  //                  slow fall
    kMotionPlatter,    // heavy motor-driven platter
    kMotionCount
};

extern const Motion kMotions[kMotionCount];

// A spring snaps to rest once its residual is below ~0.35 px, which removes
// the 1 px flicker a damped tail would otherwise cause on a 1-bit panel.
struct Spring {
    float value = 0.f;          // output y
    float vel = 0.f;            // y'
    float target = 0.f;         // input x
    float xp = 0.f;             // previous input, for x'
    float eps = 0.f;            // rest threshold in value units
    const Motion* m = nullptr;
    bool rest = true;

    void init(MotionId id, float v, float scale);
    void initCustom(const Motion* motion, float v, float scale);
    void setMotion(MotionId id) { m = &kMotions[id]; }
    void set(float t);
    void snap(float v);
    void kick(float velocity);   // impulse
    void poke(float v);          // displace output, not target
    void update(float dt);
};

// ---- bezier timing curves ---------------------------------------------
enum Curve : int {
    kCurveStandard,   // cubic-bezier(0.2, 0, 0, 1)      move / enter
    kCurveDecel,      // cubic-bezier(0.05, 0.7, 0.1, 1) arrive
    kCurveAccel,      // cubic-bezier(0.3, 0, 0.8, 0.15) leave
    kCurveOvershoot,  // cubic-bezier(0.34, 1.45, 0.64, 1) small pops
    kCurveInOut,      // cubic-bezier(0.65, 0, 0.35, 1)  sweeps
    kCurveLinear,
    kCurveCount
};

float curve(Curve c, float t);

// Arbitrary curve evaluated at runtime (Newton on x(u)).
float bezier(float x1, float y1, float x2, float y2, float t);

// quadratic Bezier point, for arc motion paths
inline void qbez(float x0, float y0, float x1, float y1, float x2, float y2,
                 float t, float* ox, float* oy) {
    float m = 1.f - t;
    *ox = m * m * x0 + 2.f * m * t * x1 + t * t * x2;
    *oy = m * m * y0 + 2.f * m * t * y1 + t * t * y2;
}

// ---- tween -------------------------------------------------------------
struct Tween {
    float a = 0.f, b = 0.f, t = 0.f, dur = 1e-6f;
    Curve curve = kCurveLinear;

    void start(float from, float to, float duration, Curve c, float delay = 0.f);
    void update(float dt) { t += dt; }
    float value() const;
    bool done() const { return t >= dur; }
    float progress() const {
        float p = t / dur;
        return p < 0 ? 0 : p > 1 ? 1 : p;
    }
};

}  // namespace tixel
