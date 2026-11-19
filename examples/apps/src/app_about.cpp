// About: spring-scrolled sheet with rubber-band ends and typewriter rows.
#include <cstdio>
#include <cstring>

#include "tixel/math.hpp"
#include "tixel/theme.hpp"
#include "tixel/widgets.hpp"
#include "apps/apps.hpp"

namespace tixel::apps {
namespace {

constexpr float kLogoH = 34.f;                  // logisoso34 cap height
constexpr float kTagY = 12.f + kLogoH + 8.f;
constexpr float kRow0 = kTagY + 20.f;
constexpr float kRowH = 22.f;
constexpr float kStep = 22.f;
constexpr int kRows = 7;

class About : public tixel::Page {
public:
    void enter() override;
    bool key(const tixel::Event& e) override;
    void update(float dt) override;
    void render(tixel::Canvas& g) override;
    const tixel::Spring* probe(const char** label) override;

private:
    void row(int i, const char** key, char* value, int n);
    float maxScroll() const { return kRow0 + kRows * kRowH + 2 - static_cast<float>(tixel::kHeight); }

    tixel::Spring scroll_;
    float target_ = 0;
};

About sAbout;

void About::row(int i, const char** key, char* value, int n) {
    static const char* keys[kRows] = {"Display", "Buffer", "Refresh", "Motion", "Uptime", "Stack", "Core RAM"};
    *key = keys[i];
    const uint32_t up = ui->ms() / 1000;
    switch (i) {
        case 0: std::snprintf(value, static_cast<size_t>(n), "128x128"); break;
        case 1: std::snprintf(value, static_cast<size_t>(n), "%d B", tixel::kFbBytes); break;
        case 2: std::snprintf(value, static_cast<size_t>(n), "%d FPS", tixel::roundi(ui->fps())); break;
        case 3: std::snprintf(value, static_cast<size_t>(n), "spring"); break;
        case 4: std::snprintf(value, static_cast<size_t>(n), "%02u:%02u", static_cast<unsigned>(up / 60 % 100), static_cast<unsigned>(up % 60)); break;
        case 5: std::snprintf(value, static_cast<size_t>(n), "%d pages", ui->depth()); break;
        default: std::snprintf(value, static_cast<size_t>(n), "%u B", static_cast<unsigned>(sizeof(tixel::Ui))); break;
    }
}

void About::enter() {
    scroll_.init(tixel::kMotionSmooth, 0, 1);
    target_ = 0;
}

bool About::key(const tixel::Event& e) {
    if ((e.key == tixel::Key::Prev || e.key == tixel::Key::Next) && tixel::isStep(e)) {
        const float d = e.key == tixel::Key::Prev ? -1.f : 1.f;
        float n = target_ + d * kStep;
        if (n < 0 || n > maxScroll()) {
            n = tixel::clampf(n, 0, maxScroll());
            scroll_.kick(d * 110.f);   // rubber band
            ui->haptic();
        }
        target_ = n;
        scroll_.set(n);
        return true;
    }
    return false;
}

void About::update(float dt) { scroll_.update(dt); }

void About::render(tixel::Canvas& g) {
    const float u = static_cast<float>(tixel::kScale);
    g.offsetPush(0, -scroll_.value);
    g.setFont(tixel::theme::kLogo);
    // caps-only wordmark: its visual height is the ascent, not ascent+descent
    const float lw = static_cast<float>(g.textWidth("TIXEL")), lx = tixel::kWidth / 2.f - lw / 2;
    const float ly = 6 * u - 20 * u * (1 - tixel::curve(tixel::kCurveOvershoot, (age - 0.1f) / 0.5f));
    g.text(lx, ly, "TIXEL");
    float ph = age - 0.8f;
    if (ph > 0) {
        ph -= 3.2f * std::floor(ph / 3.2f);
        if (ph < 0.6f) {   // periodic shine
            const float cx = tixel::lerpf(lx - 14 * u, lx + lw + 14 * u, tixel::curve(tixel::kCurveInOut, ph / 0.6f));
            g.clipPush(0, ly, static_cast<float>(tixel::kWidth), kLogoH);
            g.setMode(tixel::Mode::Clear);
            for (int k = 0; k < 4; k++) g.line(cx + k - 7 * u, ly + kLogoH, cx + k + 7 * u, ly);
            g.setMode(tixel::Mode::Set);
            g.clipPop();
        }
    }
    g.setFont(tixel::theme::kTiny);
    g.textCenter(tixel::kWidth / 2.f, kTagY, "V1.0  C++ / IDF");

    for (int i = 0; i < kRows; i++) {
        const char* key = nullptr;
        char value[24];
        row(i, &key, value, sizeof(value));
        const float y = kRow0 + static_cast<float>(i) * kRowH;
        const float t = tixel::clamp01((age - 0.35f - static_cast<float>(i) * 0.08f) / 0.35f);
        const size_t n = static_cast<size_t>(static_cast<float>(std::strlen(value)) * t);
        value[n] = 0;
        g.setFont(tixel::theme::kUi);
        g.text(8, y, key);
        g.textRight(tixel::kWidth - 14, y, value);
        if (t < 1) g.box(static_cast<float>(tixel::kWidth - 12), y, 6, 14);
        g.dottedHLine(8, y + 18, static_cast<float>(tixel::kWidth - 22), 3);
    }
    g.offsetPop();
    tixel::drawScrollbar(g, static_cast<float>(tixel::kWidth - 3), 4,
                         static_cast<float>(tixel::kHeight - 8), scroll_.value,
                         static_cast<float>(tixel::kHeight),
                         static_cast<float>(tixel::kHeight) + maxScroll());
}

const tixel::Spring* About::probe(const char** label) {
    *label = "scroll  smooth";
    return &scroll_;
}

}  // namespace

tixel::Page* aboutPage() {
    sAbout.title = "About";
    return &sAbout;
}

}  // namespace tixel::apps
