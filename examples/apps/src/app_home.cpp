// Home: spring carousel of app icons.
//   position: CAROUSEL token (reacts on frame one)
//   labels chain off the carousel *output* through FOLLOW (follow-through)
//   page-dot pill = leading edge (SNAPPY) .. trailing edge (FOLLOW)
//   fast motion leaves a dithered smear instead of strobing
//   intro: icons fly in along quadratic Bezier arcs
#include "tixel/icons.hpp"
#include "tixel/math.hpp"
#include "tixel/theme.hpp"
#include "apps/apps.hpp"

namespace tixel::apps {
namespace {

constexpr int kCount = 7;

// Layout: upstream 128x64 values doubled vertically; the horizontal pitch is
// fitted to the 128 px width so both neighbours stay visible.
constexpr float kCy = 56.f;
constexpr float kSpacing = 46.f;
constexpr float kBig = 44.f;
constexpr float kSmall = 22.f;
constexpr float kDotGap = 12.f;
constexpr float kLabelScale = 116.f;

struct Item {
    const char* label;
    tixel::IconFn icon;
    tixel::Page* (*open)();
};

const Item kItems[kCount] = {
    {"Clock|时钟", tixel::iconClock, clockPage},
    {"Music|音乐", tixel::iconMusic, musicPage},
    {"Monitor|监视器", tixel::iconMonitor, monitorPage},
    {"Settings|设置", tixel::iconSettings, settingsPage},
    {"About|关于", tixel::iconAbout, aboutPage},
    {"Animation|动画", tixel::iconMorph, animPage},
    {"Parallel|并行", tixel::iconParallel, parallelPage},
};

void brackets(tixel::Canvas& g, float cx, float cy, float r, float len) {
    const float x0 = cx - r, y0 = cy - r, x1 = cx + r - 1, y1 = cy + r - 1;
    g.hline(x0, y0, len);
    g.vline(x0, y0, len);
    g.hline(x1 - len + 1, y0, len);
    g.vline(x1, y0, len);
    g.hline(x0, y1, len);
    g.vline(x0, y1 - len + 1, len);
    g.hline(x1 - len + 1, y1, len);
    g.vline(x1, y1 - len + 1, len);
}

class Home : public tixel::Page {
public:
    void prepare(bool withIntro) { intro_ = withIntro; }

    void enter() override;
    void resume() override { intro_ = false; }
    bool key(const tixel::Event& e) override;
    void update(float dt) override;
    void render(tixel::Canvas& g) override;
    void focus(float r[4]) override;
    const tixel::Spring* probe(const char** label) override;

private:
    float introCurve(float delay, float dur, tixel::Curve c) const {
        return intro_ ? tixel::curve(c, (age - delay) / dur) : 1.f;
    }
    void iconXy(int i, float d, float* x, float* y, float* vis) const;

    int sel_ = 0;
    bool intro_ = false, primed_ = false;
    tixel::Spring pos_, label_, lead_, trail_, pulse_, squash_;
    tixel::Spring act_[kCount];
    float phase_[kCount] = {};
};

Home sHome;

void Home::enter() {
    if (primed_) return;   // keep carousel position across visits
    primed_ = true;
    sel_ = 0;
    pos_.init(tixel::kMotionCarousel, 0, kSpacing);
    label_.init(tixel::kMotionFollow, 0, kLabelScale);
    lead_.init(tixel::kMotionSnappy, 0, kDotGap);
    trail_.init(tixel::kMotionFollow, 0, kDotGap);
    pulse_.init(tixel::kMotionBouncy, 0, 3);
    squash_.init(tixel::kMotionPress, 0, kBig);
    for (int i = 0; i < kCount; i++) {
        act_[i].init(tixel::kMotionGentle, i == 0 ? 1.f : 0.f, 30);
        phase_[i] = 0;
    }
}

bool Home::key(const tixel::Event& e) {
    if ((e.key == tixel::Key::Prev || e.key == tixel::Key::Next) && tixel::isStep(e)) {
        const int d = e.key == tixel::Key::Prev ? -1 : 1, n = sel_ + d;
        if (n >= 0 && n < kCount) {
            sel_ = n;
            pos_.set(static_cast<float>(n));
            lead_.set(static_cast<float>(n));
            pulse_.poke(1.f);
            for (int i = 0; i < kCount; i++) act_[i].set(i == n ? 1.f : 0.f);
        } else {
            // rubber band: push past the end, the system pulls it back
            pos_.kick(static_cast<float>(d) * 4.5f);
            lead_.kick(static_cast<float>(d) * 3.f);
            pulse_.poke(-1.2f);
            ui->haptic();
        }
        return true;
    }
    if (e.key == tixel::Key::Ok && e.kind == tixel::Kind::Press) {
        squash_.poke(0.18f);
        ui->push(kItems[sel_].open(), tixel::Transition::Zoom);
        return true;
    }
    return false;
}

void Home::focus(float r[4]) {
    const float s = kBig + 16;
    r[0] = tixel::kWidth / 2.f - s / 2;
    r[1] = kCy - s / 2;
    r[2] = s;
    r[3] = s;
}

void Home::update(float dt) {
    label_.set(pos_.value);   // chains
    trail_.set(lead_.value);
    tixel::Spring* springs[] = {&pos_, &label_, &lead_, &trail_, &pulse_, &squash_};
    for (tixel::Spring* s : springs) s->update(dt);
    for (int i = 0; i < kCount; i++) {
        act_[i].update(dt);
        phase_[i] += dt * act_[i].value;
    }
}

void Home::iconXy(int i, float d, float* x, float* y, float* vis) const {
    const float u = static_cast<float>(tixel::kScale);
    const float ad = tixel::absf(d);
    *x = tixel::kWidth / 2.f + d * kSpacing;
    *y = kCy + tixel::minf(3.f * u, 2.f * u * ad * ad);
    *vis = 1.f;
    if (!intro_) return;
    const float t = tixel::clamp01((age - 0.1f - 0.075f * static_cast<float>(tixel::absf(static_cast<float>(i - sel_)))) / 0.6f);
    if (t >= 1) return;
    const float side = d < 0 ? -1.f : d > 0 ? 1.f : 0.f;
    const float q = tixel::curve(tixel::kCurveStandard, t);
    tixel::qbez(tixel::kWidth / 2.f + side * 20 * u, static_cast<float>(tixel::kHeight) + 16 * u,
                *x + side * 26 * u, *y - 20 * u, *x, *y, q, x, y);
    *vis = tixel::clamp01(t * 2.5f);
}

void Home::render(tixel::Canvas& g) {
    const float u = static_cast<float>(tixel::kScale);
    const float sb = introCurve(0.05f, 0.4f, tixel::kCurveDecel);
    g.offsetPush(0, -9.f * u * (1 - sb));
    statusBar(*ui, g, 0);
    g.offsetPop();

    const float pos = pos_.value;
    const float velPx = pos_.vel * kSpacing / 60.f;   // travel per frame
    for (int i = 0; i < kCount; i++) {
        const float d = static_cast<float>(i) - pos, ad = tixel::absf(d);
        if (ad > 2.2f) continue;
        float size = tixel::lerpf(kBig, kSmall, tixel::clamp01(ad));
        if (i == sel_) size *= 1 - squash_.value;
        float x, y, vis;
        iconXy(i, d, &x, &y, &vis);
        const float level = tixel::clamp01(1 - 0.55f * tixel::clamp01(ad)) * vis;
        if (tixel::absf(velPx) > 1.6f && !intro_) {   // motion smear
            g.setAlpha(level * 0.35f);
            kItems[i].icon(g, x + velPx * 0.55f, y, size, phase_[i]);
            g.setAlpha(level * 0.15f);
            kItems[i].icon(g, x + velPx * 1.1f, y, size, phase_[i]);
        }
        g.setAlpha(level);
        kItems[i].icon(g, x, y, size, phase_[i]);
    }
    g.setAlpha(1);

    const float e = introCurve(0.4f, 0.5f, tixel::kCurveOvershoot);
    if (e > 0) {
        const float spread = 26.f + pulse_.value * 5.f + tixel::minf(6.f, tixel::absf(pos_.vel) * 2.4f);
        brackets(g, tixel::kWidth / 2.f, kCy, spread * e * (1 - squash_.value * 0.6f), 8.f);
    }

    // labels ride the trailing `label` system and cross-dissolve
    const float lp = label_.value, le = introCurve(0.3f, 0.45f, tixel::kCurveDecel);
    g.setFont(tixel::theme::kUi);
    g.clipPush(8 * u, 92, static_cast<float>(tixel::kWidth) - 16 * u, 16);
    for (int i = 0; i < kCount; i++) {
        const float d = static_cast<float>(i) - lp;
        const float lvl = tixel::clamp01(1 - tixel::absf(d) * 1.25f) * le;
        if (tixel::absf(d) > 1.2f || lvl <= 0) continue;
        g.setAlpha(lvl);
        g.textCenter(tixel::kWidth / 2.f + d * kLabelScale, 94 + (1 - le) * 6,
                     tr(*ui, kItems[i].label));
    }
    g.setAlpha(1);
    g.clipPop();

    const float de = introCurve(0.45f, 0.4f, tixel::kCurveDecel);
    if (de > 0) {
        const float x0 = tixel::kWidth / 2.f - (kCount - 1) * kDotGap / 2;
        for (int i = 0; i < kCount; i++) g.pixel(x0 + static_cast<float>(i) * kDotGap, 120);
        const float a = tixel::minf(lead_.value, trail_.value), b = tixel::maxf(lead_.value, trail_.value);
        const float xa = x0 + a * kDotGap - 2 * u, xb = x0 + b * kDotGap + 2 * u;
        g.rbox(xa + 0.5f, 118, tixel::maxf(4 * u, xb - xa), 3 * u, 1);
    }
}

const tixel::Spring* Home::probe(const char** label) {
    *label = "carousel  f=2.6 z=0.78 r=+1";
    return &pos_;
}

}  // namespace

tixel::Page* homePage(bool withIntro) {
    sHome.title = "Home";
    sHome.prepare(withIntro);   // keep the carousel position across visits
    return &sHome;
}

}  // namespace tixel::apps
