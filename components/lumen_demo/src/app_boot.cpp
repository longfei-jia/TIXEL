// Boot splash: CRT flash -> letters thrown along arcs -> underline ->
// typewriter subtitle -> shine -> loader -> dissolve to home.
#include <cstring>

#include "lumen/math.hpp"
#include "lumen/theme.hpp"
#include "lumen_demo/apps.hpp"

namespace demo {
namespace {

// The choreography is authored on a 3.35 s timeline; kBootSpeed plays it faster
// (1.6 -> ~2.1 s to home). Any key skips it.
constexpr float kBootSpeed = 0.8f;
constexpr float kDoneAt = 3.35f;
const char kWord[] = "LUMEN";
const char kSub[] = "MOTION UI FRAMEWORK";

class Boot : public lumen::Page {
public:
    void enter() override { handed_ = false; }
    bool key(const lumen::Event& e) override {
        if (e.kind == lumen::Kind::Press) handOff();   // skip the splash
        return true;
    }
    void update(float) override {
        if (age * kBootSpeed >= kDoneAt) handOff();
    }
    void render(lumen::Canvas& g) override;

private:
    void handOff() {
        if (handed_) return;
        handed_ = true;
        ui->replace(homePage(true), lumen::Transition::Fade);
    }

    bool handed_ = false;
};

Boot sBoot;

void Boot::render(lumen::Canvas& g) {
    const float u = static_cast<float>(lumen::kScale);
    const float t = age * kBootSpeed;
    if (t < 0.95f) {   // flash dissolving away
        g.box(0, 0, static_cast<float>(lumen::kWidth), static_cast<float>(lumen::kHeight));
        lumen::bufFade(g.target(), lumen::remap(t, 0.55f, 0.95f, 1.f, 0.f));
        if (t < 0.6f) return;
    }

    g.setFont(lumen::theme::kLogo);
    const int fh = g.fontHeight();
    int widths[5];
    int total = 0;
    for (int i = 0; i < 5; i++) {
        widths[i] = g.charWidth(kWord[i]);
        total += widths[i];
    }
    float x = lumen::kWidth / 2.f - static_cast<float>(total) / 2.f;
    const float baseY = 22 * u;
    char s[2] = {0, 0};
    for (int i = 0; i < 5; i++) {
        // thrown from below-centre along a quadratic Bezier that crests above
        // its slot, lands with a small settle hop
        const float q = lumen::clamp01((t - 0.72f - static_cast<float>(i) * 0.07f) / 0.55f);
        if (q > 0) {
            const float x0 = lumen::kWidth / 2.f - static_cast<float>(widths[i]) / 2.f;
            const float y0 = static_cast<float>(lumen::kHeight) + 2 * u;
            const float cx = lumen::lerpf(x0, x, 1.25f), cy = baseY - 22 * u;
            float px, py;
            lumen::qbez(x0, y0, cx, cy, x, baseY, lumen::curve(lumen::kCurveDecel, q), &px, &py);
            py += -2.5f * u * lumen::sinf(lumen::kPi * lumen::clamp01((q - 0.7f) / 0.3f));
            s[0] = kWord[i];
            g.text(px, py, s);
        }
        x += static_cast<float>(widths[i]);
    }
    const float ly = baseY + static_cast<float>(fh) + 3 * u;
    const float ul = lumen::curve(lumen::kCurveStandard, (t - 1.35f) / 0.5f);
    if (ul > 0)
        g.hline(lumen::kWidth / 2.f - static_cast<float>(total) / 2.f * ul, ly,
                static_cast<float>(total) * ul);
    if (t > 1.6f) {
        g.setFont(lumen::theme::kTiny);
        const int k = static_cast<int>(lumen::clamp01((t - 1.65f) / 0.55f) *
                                       static_cast<float>(sizeof(kSub) - 1));
        char buf[sizeof(kSub)];
        std::memcpy(buf, kSub, static_cast<size_t>(k));
        buf[k] = 0;
        const float sx = lumen::kWidth / 2.f - static_cast<float>(g.textWidth(kSub)) / 2.f;
        const int ex = g.text(sx, ly + 5 * u, buf);
        if ((k < static_cast<int>(sizeof(kSub) - 1) || static_cast<int>(t * 4) % 2 == 0) && t < 2.8f)
            g.box(static_cast<float>(ex), ly + 5 * u, 3 * u, static_cast<float>(g.fontHeight()));
    }
    const float sh = lumen::clamp01((t - 2.05f) / 0.45f);   // diagonal glint through the letters
    if (sh > 0 && sh < 1) {
        const float cx = lumen::lerpf(lumen::kWidth / 2.f - total / 2.f - 20 * u,
                                      lumen::kWidth / 2.f + total / 2.f + 20 * u,
                                      lumen::curve(lumen::kCurveInOut, sh));
        g.clipPush(0, baseY, static_cast<float>(lumen::kWidth), static_cast<float>(fh));
        g.setMode(lumen::Mode::Clear);   // XOR would light the background at this glyph size
        for (int k = 0; k < 5; k++) g.line(cx + k - 8 * u, baseY + fh, cx + k + 8 * u, baseY);
        g.setMode(lumen::Mode::Set);
        g.clipPop();
    }
    const float lp = lumen::clamp01((t - 2.25f) / 0.85f);
    if (lp > 0) {
        const float bw = 40 * u, bx = lumen::kWidth / 2.f - bw / 2.f;
        const float e = lp < .5f ? 4 * lp * lp * lp
                                 : 1 - (-2 * lp + 2) * (-2 * lp + 2) * (-2 * lp + 2) / 2;
        g.rframe(bx, 56 * u, bw, 4 * u, 2 * u);
        g.hline(bx + 1 * u, 57 * u, (bw - 2 * u) * e);
        g.hline(bx + 1 * u, 58 * u, (bw - 2 * u) * e);
    }
}

}  // namespace

lumen::Page* bootPage() {
    sBoot.title = "Boot";
    return &sBoot;
}

}  // namespace demo
