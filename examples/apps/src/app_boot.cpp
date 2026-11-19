// Boot splash: CRT flash -> letters thrown along arcs -> underline ->
// typewriter subtitle -> shine -> loader -> dissolve to home.
#include <cstring>

#include "tixel/math.hpp"
#include "tixel/theme.hpp"
#include "apps/apps.hpp"

namespace tixel::apps {
namespace {

// The choreography is authored on a 3.35 s timeline; kBootSpeed plays it faster
// (1.6 -> ~2.1 s to home). Any key skips it.
constexpr float kBootSpeed = 0.8f;
constexpr float kDoneAt = 3.35f;
const char kWord[] = "TIXEL";
const char kSub[] = "MOTION UI FRAMEWORK|动效 UI 框架";

class Boot : public tixel::Page {
public:
    void enter() override { handed_ = false; }
    bool key(const tixel::Event& e) override {
        if (e.kind == tixel::Kind::Press) handOff();   // skip the splash
        return true;
    }
    void update(float) override {
        if (age * kBootSpeed >= kDoneAt) handOff();
    }
    void render(tixel::Canvas& g) override;

private:
    void handOff() {
        if (handed_) return;
        handed_ = true;
        ui->replace(homePage(true), tixel::Transition::Fade);
    }

    bool handed_ = false;
};

Boot sBoot;

void Boot::render(tixel::Canvas& g) {
    const float u = static_cast<float>(tixel::kScale);
    const float t = age * kBootSpeed;
    if (t < 0.95f) {   // flash dissolving away
        g.box(0, 0, static_cast<float>(tixel::kWidth), static_cast<float>(tixel::kHeight));
        tixel::bufFade(g.target(), tixel::remap(t, 0.55f, 0.95f, 1.f, 0.f));
        if (t < 0.6f) return;
    }

    g.setFont(tixel::theme::kLogo);
    const int fh = g.fontHeight();
    int widths[5];
    int total = 0;
    for (int i = 0; i < 5; i++) {
        widths[i] = g.charWidth(kWord[i]);
        total += widths[i];
    }
    float x = tixel::kWidth / 2.f - static_cast<float>(total) / 2.f;
    const float baseY = 22 * u;
    char s[2] = {0, 0};
    for (int i = 0; i < 5; i++) {
        // thrown from below-centre along a quadratic Bezier that crests above
        // its slot, lands with a small settle hop
        const float q = tixel::clamp01((t - 0.72f - static_cast<float>(i) * 0.07f) / 0.55f);
        if (q > 0) {
            const float x0 = tixel::kWidth / 2.f - static_cast<float>(widths[i]) / 2.f;
            const float y0 = static_cast<float>(tixel::kHeight) + 2 * u;
            const float cx = tixel::lerpf(x0, x, 1.25f), cy = baseY - 22 * u;
            float px, py;
            tixel::qbez(x0, y0, cx, cy, x, baseY, tixel::curve(tixel::kCurveDecel, q), &px, &py);
            py += -2.5f * u * tixel::sinf(tixel::kPi * tixel::clamp01((q - 0.7f) / 0.3f));
            s[0] = kWord[i];
            g.text(px, py, s);
        }
        x += static_cast<float>(widths[i]);
    }
    const float ly = baseY + static_cast<float>(fh) + 3 * u;
    const float ul = tixel::curve(tixel::kCurveStandard, (t - 1.35f) / 0.5f);
    if (ul > 0)
        g.hline(tixel::kWidth / 2.f - static_cast<float>(total) / 2.f * ul, ly,
                static_cast<float>(total) * ul);
    if (t > 1.6f) {
        g.setFont(tixel::theme::kTiny);
        // Reveal by codepoint, not byte: half a Chinese glyph looks like noise.
        const char* sub = tr(*ui, kSub);
        const int subChars = utf8Count(sub);
        const int k = static_cast<int>(tixel::clamp01((t - 1.65f) / 0.55f) * static_cast<float>(subChars));
        const int kBytes = utf8BytesFor(sub, k);
        char buf[64];
        std::memcpy(buf, sub, static_cast<size_t>(kBytes));
        buf[kBytes] = 0;
        const float sx = tixel::kWidth / 2.f - static_cast<float>(g.textWidth(sub)) / 2.f;
        const int ex = g.text(sx, ly + 2 * u, buf);
        if ((k < subChars || static_cast<int>(t * 4) % 2 == 0) && t < 2.8f)
            g.box(static_cast<float>(ex), ly + 2 * u, 3 * u, static_cast<float>(g.fontHeight()));
    }
    const float sh = tixel::clamp01((t - 2.05f) / 0.45f);   // diagonal glint through the letters
    if (sh > 0 && sh < 1) {
        const float cx = tixel::lerpf(tixel::kWidth / 2.f - total / 2.f - 20 * u,
                                      tixel::kWidth / 2.f + total / 2.f + 20 * u,
                                      tixel::curve(tixel::kCurveInOut, sh));
        g.clipPush(0, baseY, static_cast<float>(tixel::kWidth), static_cast<float>(fh));
        g.setMode(tixel::Mode::Clear);   // XOR would light the background at this glyph size
        for (int k = 0; k < 5; k++) g.line(cx + k - 8 * u, baseY + fh, cx + k + 8 * u, baseY);
        g.setMode(tixel::Mode::Set);
        g.clipPop();
    }
    const float lp = tixel::clamp01((t - 2.25f) / 0.85f);
    if (lp > 0) {
        const float bw = 40 * u, bx = tixel::kWidth / 2.f - bw / 2.f;
        const float e = lp < .5f ? 4 * lp * lp * lp
                                 : 1 - (-2 * lp + 2) * (-2 * lp + 2) * (-2 * lp + 2) / 2;
        g.rframe(bx, 56 * u, bw, 4 * u, 2 * u);
        g.hline(bx + 1 * u, 57 * u, (bw - 2 * u) * e);
        g.hline(bx + 1 * u, 58 * u, (bw - 2 * u) * e);
    }
}

}  // namespace

tixel::Page* bootPage() {
    sBoot.title = "Boot";
    return &sBoot;
}

}  // namespace tixel::apps
