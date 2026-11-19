#include "tixel/icons.hpp"

#include <cmath>

#include "tixel/math.hpp"
#include "tixel/motion.hpp"

namespace tixel {
namespace {

float stroke(float s) { return maxf(1.f, s / 11.f); }

}  // namespace

void iconClock(Canvas& c, float cx, float cy, float s, float ph) {
    const float r = s * 0.46f;
    c.ring(cx, cy, r, stroke(s) * 1.1f);
    for (int i = 0; i < 12; i++) {
        if (s < 18 && i % 3) continue;
        const float a = static_cast<float>(i) * kPi / 6.f;
        const float r0 = r - (i % 3 == 0 ? s * 0.12f : s * 0.07f);
        c.pixel(cx + sinf(a) * r0 - 0.5f, cy - cosf(a) * r0 - 0.5f);
    }
    const float am = 1.05f + ph * kTau * 0.35f;              // minute hand sweeps
    const float ah = -0.95f + ph * kTau * 0.35f / 12.f;
    c.capsule(cx, cy, cx + sinf(am) * r * 0.72f, cy - cosf(am) * r * 0.72f, stroke(s));
    c.capsule(cx, cy, cx + sinf(ah) * r * 0.48f, cy - cosf(ah) * r * 0.48f, stroke(s) * 1.3f);
    c.disc(cx, cy, maxf(1.f, s * 0.06f));
}

void iconMusic(Canvas& c, float cx, float cy, float s, float ph) {
    const float b1 = absf(sinf(ph * kTau * 0.9f)) * s * 0.08f;         // notes bob
    const float b2 = absf(sinf(ph * kTau * 0.9f + 1.3f)) * s * 0.08f;
    const float hr = s * 0.13f, t = stroke(s);
    const float x1 = cx - s * 0.2f, y1 = cy + s * 0.25f - b1;
    const float x2 = cx + s * 0.24f, y2 = cy + s * 0.17f - b2;
    c.disc(x1, y1, hr * 1.1f);
    c.disc(x2, y2, hr * 1.1f);
    const float t1 = cy - s * 0.3f - b1, t2 = cy - s * 0.38f - b2;
    c.capsule(x1 + hr * 0.9f, y1, x1 + hr * 0.9f, t1, t);
    c.capsule(x2 + hr * 0.9f, y2, x2 + hr * 0.9f, t2, t);
    c.capsule(x1 + hr * 0.9f, t1, x2 + hr * 0.9f, t2, t * 2.f);
}

void iconMonitor(Canvas& c, float cx, float cy, float s, float ph) {
    static const float kBase[4] = {0.45f, 0.8f, 0.55f, 0.95f};
    const float w = s * 0.92f, h = s * 0.78f, x0 = cx - w / 2, y0 = cy - h / 2;
    c.rframe(x0, y0, w, h, maxf(1.f, s * 0.1f));
    const float bw = (w - s * 0.28f) / 4.f;
    for (int i = 0; i < 4; i++) {
        float lv = kBase[i];
        if (ph != 0.f) {
            lv = 0.35f + 0.3f * sinf(ph * 5 + static_cast<float>(i) * 1.7f) +
                 0.2f * sinf(ph * 2.3f + static_cast<float>(i)) + 0.3f;
            lv = clampf(lv, 0.15f, 0.95f);
        }
        const float bh = (h - s * 0.26f) * lv;
        c.box(x0 + s * 0.14f + static_cast<float>(i) * bw + 0.5f, y0 + h - s * 0.13f - bh,
              maxf(1.f, bw - maxf(1.f, s * 0.06f)), bh);
    }
}

void iconSettings(Canvas& c, float cx, float cy, float s, float ph) {
    const float rot = ph * 1.6f, ro = s * 0.47f, ri = s * 0.35f;
    float pts[64];
    for (int i = 0; i < 32; i++) {
        const float a = rot + static_cast<float>(i) * kTau / 32.f;
        const int k = i % 4;
        const float r = (k == 1 || k == 2) ? ro : ri;
        pts[2 * i] = cx + sinf(a) * r;
        pts[2 * i + 1] = cy - cosf(a) * r;
    }
    c.poly(pts, 32);
    const Mode m = c.mode();
    c.setMode(Mode::Clear);
    c.disc(cx, cy, s * 0.2f);
    c.setMode(m);
    c.disc(cx, cy, s * 0.08f);
}

void iconAbout(Canvas& c, float cx, float cy, float s, float ph) {
    const float r = s * 0.46f;
    c.ring(cx, cy, r, stroke(s) * 1.1f);
    const float bob = sinf(ph * 4) * s * 0.04f;
    c.disc(cx, cy - s * 0.2f + bob, maxf(1.f, s * 0.075f));
    c.capsule(cx, cy - s * 0.02f, cx, cy + s * 0.26f, stroke(s) * 1.5f);
}

// Poster child of boundary interpolation: four shapes written as radius
// functions r(theta) - circle, square, diamond, four-point star - and the
// sampled radius lerps between neighbours as `phase` advances. Every
// intermediate state is a legal shape, no bitmap frames involved.
void iconMorph(Canvas& c, float cx, float cy, float s, float ph) {
    const float R = s * 0.46f;
    auto shapeR = [](int shape, float th) {
        const float cs = fabsf(cosf(th)), sn = fabsf(sinf(th));
        switch (shape) {
            case 0: return 1.f;                                             // circle
            case 1: return 0.7071f / maxf(cs, sn);                          // square
            case 2: return 1.f / (cs + sn);                                 // diamond
            default: return 0.62f + 0.38f * powf(fabsf(cosf(2 * th)), 3.f); // star
        }
    };
    const float seg = fmodf(ph * 1.1f, 4.f);
    const int i = static_cast<int>(seg);
    const float t = curve(kCurveStandard, seg - static_cast<float>(i));
    float xy[2 * 24];
    for (int k = 0; k < 24; k++) {
        const float th = static_cast<float>(k) * kTau / 24.f;
        const float r = lerpf(shapeR(i, th), shapeR((i + 1) % 4, th), t) * R;
        xy[2 * k] = cx + sinf(th) * r;
        xy[2 * k + 1] = cy - cosf(th) * r;
    }
    c.poly(xy, 24);
    c.disc(cx, cy, maxf(1.f, s * 0.08f));
}

// Parallel group: three tiles pop in and out on one shared clock - the icon
// states the theme (many elements, one driver) rather than showing motion.
void iconParallel(Canvas& c, float cx, float cy, float s, float ph) {
    const float k = 0.45f + 0.55f * (0.5f - 0.5f * cosf(ph * kTau * 0.8f));
    const float sz = s * 0.26f * k;
    for (int i = 0; i < 3; i++) {
        const float x = cx + static_cast<float>(i - 1) * s * 0.3f;
        c.rbox(x - sz * 0.5f, cy - sz * 0.5f, sz, sz, sz * 0.3f);
    }
}

}  // namespace tixel
