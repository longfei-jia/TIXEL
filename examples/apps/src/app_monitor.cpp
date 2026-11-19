// System monitor: tabbed live graph. Each of the points is its own
// second-order system whose frequency falls off towards older samples, so a
// tab switch sweeps across the curve like a wave instead of cutting.
//
// Layout: upstream shares one row for tabs + value; at 2x type the value is
// wider than the remaining row, so the tabs moved to their own line.
#include <cstdio>
#include <cstring>

#include "tixel/math.hpp"
#include "tixel/theme.hpp"
#include "tixel/widgets.hpp"
#include "apps/apps.hpp"

namespace tixel::apps {
namespace {

constexpr int kPoints = 34;
constexpr float kStep = 4.f;
constexpr float kSample = 0.25f;
constexpr int kTabs = 3;
const char* kTabNames[kTabs] = {"CPU", "RAM", "TEMP"};

constexpr float kGraphTop = 48.f;
constexpr float kGraphH = 76.f;
constexpr float kTabsY = 30.f;
constexpr float kUnderlineY = 43.f;

class Monitor : public tixel::Page {
public:
    void enter() override;
    bool key(const tixel::Event& e) override;
    void update(float dt) override;
    void render(tixel::Canvas& g) override;
    const tixel::Spring* probe(const char** label) override;

private:
    float sample(int k, float t);
    void format(char* b, float v);
    float tabX(tixel::Canvas& g, int i);
    void placeUnderline(bool snap);

    int tab_ = 0;
    bool paused_ = false;
    float t_ = 0, acc_ = 0;
    float hist_[kTabs][kPoints] = {};
    float y_[kPoints] = {}, v_[kPoints] = {}, xp_[kPoints] = {};
    tixel::Spring ulX_, ulW_, badge_;
    tixel::Tween reveal_;
    tixel::Roll value_;
    tixel::Rng rng_;
    tixel::Spring probePoint_;
};

Monitor sMonitor;

float Monitor::sample(int k, float t) {
    const float n = rng_.nextFloat();
    if (k == 0)
        return tixel::clampf(0.42f + 0.22f * tixel::sinf(t * 1.9f) + 0.18f * tixel::sinf(t * 5.3f) +
                                 0.12f * (n - 0.5f),
                             0.04f, 0.97f);
    if (k == 1)
        return tixel::clamp01(0.62f + 0.06f * tixel::sinf(t * 0.7f) + 0.03f * (n - 0.5f));
    return tixel::clamp01(0.3f + 0.08f * tixel::sinf(t * 0.9f) + 0.05f * tixel::sinf(t * 3.1f) +
                          0.02f * (n - 0.5f));
}

void Monitor::format(char* b, float v) {
    if (tab_ == 2) std::snprintf(b, 5, "%2dC", static_cast<int>(28 + v * 50));
    else std::snprintf(b, 5, "%2d%%", static_cast<int>(v * 100));
}

float Monitor::tabX(tixel::Canvas& g, int i) {
    g.setFont(tixel::theme::kTiny);
    float x = 6;
    for (int j = 0; j < i; j++) x += static_cast<float>(g.textWidth(kTabNames[j])) + 14;
    return x;
}

void Monitor::placeUnderline(bool snap) {
    tixel::Canvas& g = ui->canvas();
    const float x = tabX(g, tab_) - 2, w = static_cast<float>(g.textWidth(kTabNames[tab_])) + 4;
    if (snap) {
        ulX_.snap(x);
        ulW_.snap(w);
    }
    ulX_.set(x);
    ulW_.set(w);
}

void Monitor::enter() {
    tab_ = 0;
    paused_ = false;
    t_ = acc_ = 0;
    rng_.s = 3;
    for (int k = 0; k < kTabs; k++)
        for (int i = 0; i < kPoints; i++)
            hist_[k][i] = sample(k, -static_cast<float>(kPoints - 1 - i) * kSample);
    std::memcpy(y_, hist_[0], sizeof(y_));
    std::memcpy(xp_, hist_[0], sizeof(xp_));
    std::memset(v_, 0, sizeof(v_));
    ulX_.init(tixel::kMotionSnappy, 0, 1);
    ulW_.init(tixel::kMotionFollow, 0, 1);   // width lags: underline stretches
    badge_.init(tixel::kMotionBouncy, 0, 15);
    probePoint_.init(tixel::kMotionSmooth, 0, 40);
    reveal_.start(0, 1, 0.7f, tixel::kCurveStandard, 0.15f);
    char b[5];
    format(b, hist_[0][kPoints - 1]);
    value_.init(tixel::theme::kMid, b, 0.3f, 0.04f);
    placeUnderline(true);
}

bool Monitor::key(const tixel::Event& e) {
    if ((e.key == tixel::Key::Prev || e.key == tixel::Key::Next) && tixel::isStep(e)) {
        const int d = e.key == tixel::Key::Prev ? -1 : 1, n = tab_ + d;
        if (n >= 0 && n < kTabs) {
            tab_ = n;
            placeUnderline(false);
            char b[5];
            format(b, hist_[n][kPoints - 1]);
            value_.set(b, d);
        } else {
            ulX_.kick(static_cast<float>(d) * 60.f);
            ui->haptic();
        }
        return true;
    }
    if (e.key == tixel::Key::Ok && e.kind == tixel::Kind::Press) {
        paused_ = !paused_;
        badge_.set(paused_ ? 1.f : 0.f);
        return true;
    }
    return false;
}

void Monitor::update(float dt) {
    ulX_.update(dt);
    ulW_.update(dt);
    badge_.update(dt);
    reveal_.update(dt);
    value_.update(dt);
    if (!paused_) {
        t_ += dt;
        acc_ += dt;
        while (acc_ >= kSample) {
            acc_ -= kSample;
            for (int k = 0; k < kTabs; k++) {
                std::memmove(hist_[k], hist_[k] + 1, sizeof(float) * (kPoints - 1));
                hist_[k][kPoints - 1] = sample(k, t_);
            }
            // shift the systems with the data; re-base x' so a sample shift is
            // not seen as an input step
            std::memmove(y_, y_ + 1, sizeof(float) * (kPoints - 1));
            std::memmove(v_, v_ + 1, sizeof(float) * (kPoints - 1));
            v_[kPoints - 1] = 0;
            std::memcpy(xp_, hist_[tab_], sizeof(xp_));
            char b[5];
            format(b, hist_[tab_][kPoints - 1]);
            value_.set(b, 1);
        }
    }
    if (dt <= 0) return;
    constexpr float z = 0.72f, r = 0.5f;
    int steps = static_cast<int>(dt * tixel::kSpringHz + 0.999f);
    const float h = dt / static_cast<float>(steps < 1 ? 1 : steps);
    for (int i = 0; i < kPoints; i++) {
        const float x = hist_[tab_][i];
        const float xd = (x - xp_[i]) / dt;
        xp_[i] = x;
        const float w = tixel::kTau * tixel::lerpf(1.6f, 3.4f, static_cast<float>(i) / (kPoints - 1));
        const float k1 = 2 * z / w, k2 = 1 / (w * w), k3 = r * z / w;
        float y = y_[i], v = v_[i];
        for (int s = 0; s < steps; s++) {
            y += h * v;
            v += h * (x + k3 * xd - y - k1 * v) / k2;
        }
        y_[i] = y;
        v_[i] = v;
    }
    probePoint_.value = y_[kPoints / 2];
    probePoint_.target = hist_[tab_][kPoints / 2];
}

void Monitor::render(tixel::Canvas& g) {
    g.setFont(tixel::theme::kTiny);
    for (int i = 0; i < kTabs; i++) g.text(tabX(g, i), kTabsY, kTabNames[i]);
    g.box(ulX_.value, kUnderlineY, ulW_.value, 2);
    const float vx = static_cast<float>(tixel::kWidth - 6) - static_cast<float>(value_.width(g));
    value_.draw(g, vx, 0);

    for (int f = 1; f <= 3; f++)
        g.dottedHLine(static_cast<float>(static_cast<int>(t_ * 8) % 4),
                      kGraphTop + kGraphH * (1 - 0.25f * static_cast<float>(f)),
                      static_cast<float>(tixel::kWidth), 2);
    const float rv = reveal_.value();
    const float scroll = paused_ ? 0 : (acc_ / kSample) * kStep;
    float xs[kPoints], ys[kPoints];
    for (int i = 0; i < kPoints; i++) {
        xs[i] = static_cast<float>(i) * kStep - scroll + (tixel::kWidth - (kPoints - 1) * kStep);
        ys[i] = kGraphTop + kGraphH - 2 - y_[i] * (kGraphH - 8);
    }
    g.clipPush(0, kGraphTop, static_cast<float>(tixel::kWidth) * rv, kGraphH + 1);
    // ~30% dithered fill under the curve, then the stroke
    g.setAlpha(0.3f);
    int j = 0;
    for (int x = 0; x < tixel::kWidth; x++) {
        const float px = static_cast<float>(x);
        while (j < kPoints - 2 && xs[j + 1] < px) j++;
        const float tt = tixel::clamp01((px - xs[j]) / (xs[j + 1] - xs[j]));
        const float yc = tixel::lerpf(ys[j], ys[j + 1], tt);
        const float top = static_cast<float>(static_cast<int>(yc) + 1);
        g.vline(px, top + 1, kGraphTop + kGraphH - top - 1);
    }
    g.setAlpha(1);
    for (int i = 0; i < kPoints - 1; i++) g.line(xs[i], ys[i], xs[i + 1], ys[i + 1]);
    g.hline(0, kGraphTop + kGraphH, static_cast<float>(tixel::kWidth));
    const float hx = xs[kPoints - 1] - 1, hy = ys[kPoints - 1];
    g.setMode(tixel::Mode::Clear);
    g.disc(hx, hy, 3.6f);
    g.setMode(tixel::Mode::Set);
    g.ring(hx, hy, 3.6f, 1.5f);
    g.clipPop();

    const float b = badge_.value;
    if (b > 0.02f) {
        const float bw = 68 * static_cast<float>(tixel::kScale) / 2.f;
        const float by = tixel::lerpf(kGraphTop - 20, kGraphTop + 6, b);
        g.rbox(tixel::kWidth / 2.f - bw / 2, by, bw, 20, 6);
        g.setMode(tixel::Mode::Clear);
        g.setFont(tixel::theme::kTiny);
        g.textCenter(tixel::kWidth / 2.f + 0.5f, by + 5, "PAUSED");
        g.setMode(tixel::Mode::Set);
    }
}

const tixel::Spring* Monitor::probe(const char** label) {
    *label = "graph point  f=1.6..3.4 z=0.72 r=+0.5 x34";
    return &probePoint_;
}

}  // namespace

tixel::Page* monitorPage() {
    sMonitor.title = "Monitor";
    return &sMonitor;
}

}  // namespace tixel::apps
