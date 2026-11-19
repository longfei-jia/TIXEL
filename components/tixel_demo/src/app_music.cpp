// Music player: spinning record with inertia, morphing play/pause, VU-style
// spectrum (fast attack, slow release) with falling peak caps.
#include <cstdio>

#include "tixel/math.hpp"
#include "tixel/theme.hpp"
#include "tixel/widgets.hpp"
#include "tixel_demo/apps.hpp"

namespace demo {
namespace {

constexpr int kBands = 13;

struct Track {
    const char* title;
    const char* artist;
    int duration;
};

const Track kTracks[] = {
    {"Neon Horizon", "SYNTHWAVE FM", 214},
    {"Midnight Circuit", "PIXEL DRIVE", 187},
    {"Glass Garden", "LOFI ROOM", 243},
};
constexpr int kTrackCount = static_cast<int>(sizeof(kTracks) / sizeof(kTracks[0]));

constexpr float kDiscX = 64.f;
constexpr float kDiscY = 54.f;
constexpr float kDiscR = 22.f;
constexpr float kControlsY = 86.f;
constexpr float kSpectrumBase = 110.f;
constexpr float kProgressY = 113.f;
constexpr float kTimesY = 117.f;

void drawDisc(tixel::Canvas& g, float angle, float cx, float cy, float r) {
    g.disc(cx, cy, r);
    g.setMode(tixel::Mode::Clear);
    g.ring(cx, cy, r - 1.5f, 1);
    g.ring(cx, cy, r - 3.5f, 1);   // grooves
    g.ring(cx, cy, r - 6, 1);
    for (int k = 0; k < 2; k++) {   // rotating light reflection
        const float a = angle + static_cast<float>(k) * tixel::kPi;
        for (int j = 0; j < 3; j++) {
            const float aa = a + static_cast<float>(j - 1) * 0.12f;
            g.line(cx + tixel::sinf(aa) * r * 0.42f, cy - tixel::cosf(aa) * r * 0.42f,
                   cx + tixel::sinf(aa) * (r - 2.5f), cy - tixel::cosf(aa) * (r - 2.5f));
        }
    }
    g.disc(cx, cy, r * 0.34f);
    g.setMode(tixel::Mode::Set);
    g.ring(cx, cy, r * 0.34f, 1);
    g.disc(cx, cy, 1.2f);
    g.disc(cx + tixel::sinf(angle) * r * 0.2f, cy - tixel::cosf(angle) * r * 0.2f, 0.9f);
}

// play triangle's halves slide/stretch into the two pause bars
void drawPlayPause(tixel::Canvas& g, float cx, float cy, float morph, float flash) {
    const float u = static_cast<float>(tixel::kScale);
    const float s = (1 + 0.25f * flash) * u;
    const float h = 4.5f * s;
    const float k = tixel::clampf(morph, -0.2f, 1.2f);
    const float l[8] = {cx - 3.5f * s, cy - h,
                        tixel::lerpf(cx + 0.25f * u, cx - 1.f * s, k), tixel::lerpf(cy - h / 2, cy - h, k),
                        tixel::lerpf(cx + 0.25f * u, cx - 1.f * s, k), tixel::lerpf(cy + h / 2, cy + h, k),
                        cx - 3.5f * s, cy + h};
    const float r[8] = {tixel::lerpf(cx - 0.25f * u, cx + 1.f * s, k), tixel::lerpf(cy - h / 2, cy - h, k),
                        tixel::lerpf(cx + 4.5f * s, cx + 3.5f * s, k), tixel::lerpf(cy, cy - h, k),
                        tixel::lerpf(cx + 4.5f * s, cx + 3.5f * s, k), tixel::lerpf(cy, cy + h, k),
                        tixel::lerpf(cx - 0.25f * u, cx + 1.f * s, k), tixel::lerpf(cy + h / 2, cy + h, k)};
    g.poly(l, 4);
    g.poly(r, 4);
}

void drawSkip(tixel::Canvas& g, float cx, float cy, float dir, float flash) {
    const float u = static_cast<float>(tixel::kScale);
    const float s = (1 + 0.35f * flash) * u;
    const float dx = dir * 1.5f * flash * u;
    for (int k = 0; k < 2; k++) {
        const float ox = cx + dx + dir * (static_cast<float>(k) * 3 - 1.5f) * s;
        const float t[6] = {ox - dir * 1.5f * s, cy - 2.5f * s, ox + dir * 2 * s, cy,
                            ox - dir * 1.5f * s, cy + 2.5f * s};
        g.poly(t, 3);
    }
}

class Music : public tixel::Page {
public:
    void enter() override;
    bool key(const tixel::Event& e) override;
    void update(float dt) override;
    void render(tixel::Canvas& g) override;
    const tixel::Spring* probe(const char** label) override;

private:
    void change(int d);

    int index_ = 0, dir_ = 1, prevIndex_ = -1;
    bool playing_ = false;
    float pos_ = 0, angle_ = 0, tick_ = 0;
    float peaks_[kBands] = {}, peakVel_[kBands] = {};
    tixel::Spring spin_, morph_, slide_, flash_[3], bars_[kBands];
    const tixel::Spring* probeSpring_ = nullptr;
    const char* probeLabel_ = "";
    tixel::Marquee title_;
    tixel::Rng rng_;
};

Music sMusic;

void Music::enter() {
    index_ = 0;
    dir_ = 1;
    prevIndex_ = -1;
    playing_ = false;
    pos_ = angle_ = tick_ = 0;
    spin_.init(tixel::kMotionPlatter, 0, 100);
    morph_.init(tixel::kMotionToggle, 0, 8);
    slide_.init(tixel::kMotionCarousel, 0, 40);
    for (int i = 0; i < 3; i++) flash_[i].init(tixel::kMotionPress, 0, 4);
    for (int i = 0; i < kBands; i++) {
        bars_[i].init(tixel::kMotionVuAttack, 0, 16);
        peaks_[i] = peakVel_[i] = 0;
    }
    title_.init(tixel::theme::kUi, kTracks[0].title, static_cast<float>(tixel::kWidth - 16));
    rng_.s = 7;
    probeSpring_ = &morph_;
    probeLabel_ = "play/pause morph  toggle";
}

void Music::change(int d) {
    prevIndex_ = index_;
    index_ = (index_ + d + kTrackCount) % kTrackCount;
    title_.set(kTracks[index_].title);
    dir_ = d;
    slide_.snap(1.f);
    slide_.set(0.f);
    pos_ = 0;
    spin_.kick(static_cast<float>(d) * 6.f);   // scratch
    flash_[d < 0 ? 0 : 2].poke(1.f);
    probeSpring_ = &slide_;
    probeLabel_ = "title slide  carousel";
}

bool Music::key(const tixel::Event& e) {
    if (e.key == tixel::Key::Ok && e.kind == tixel::Kind::Press) {
        playing_ = !playing_;
        morph_.set(playing_ ? 1.f : 0.f);
        flash_[1].poke(1.f);
        probeSpring_ = &morph_;
        probeLabel_ = "play/pause morph  toggle";
        return true;
    }
    if ((e.key == tixel::Key::Prev || e.key == tixel::Key::Next) && tixel::isStep(e)) {
        change(e.key == tixel::Key::Prev ? -1 : 1);
        return true;
    }
    return false;
}

void Music::update(float dt) {
    spin_.set(playing_ ? 1.2f : 0.f);
    spin_.update(dt);
    angle_ += spin_.value * dt * tixel::kTau * 0.55f;
    if (angle_ > 1000.f) angle_ -= tixel::kTau * 150.f;
    if (playing_) pos_ = tixel::minf(static_cast<float>(kTracks[index_].duration), pos_ + dt * 7.f);
    morph_.update(dt);
    slide_.update(dt);
    title_.update(dt);
    for (int i = 0; i < 3; i++) flash_[i].update(dt);
    tick_ += dt;
    if (tick_ > 0.09f) {
        tick_ = 0;
        const float beat = 0.5f + 0.5f * tixel::sinf(age * tixel::kTau * 2.1f);
        for (int i = 0; i < kBands; i++) {
            float v = 0;
            if (playing_) {
                const float shape = 0.9f - 0.45f * static_cast<float>(i) / kBands +
                                    0.15f * tixel::sinf(static_cast<float>(i) * 1.3f + age * 3);
                v = tixel::clampf(shape * (0.35f + 0.65f * rng_.nextFloat()) * (0.7f + 0.3f * beat), 0.05f, 1);
            }
            bars_[i].set(v);
        }
    }
    for (int i = 0; i < kBands; i++) {
        tixel::Spring* b = &bars_[i];
        b->setMotion(b->target > b->value ? tixel::kMotionVuAttack : tixel::kMotionVuRelease);
        b->update(dt);
        const float v = tixel::maxf(0, b->value);
        if (v >= peaks_[i]) {
            peaks_[i] = v;
            peakVel_[i] = 0;
        } else {
            peakVel_[i] += dt * 2.2f;
            peaks_[i] = tixel::maxf(v, peaks_[i] - peakVel_[i] * dt);
        }
    }
}

void Music::render(tixel::Canvas& g) {
    const Track* t = &kTracks[index_];
    const float u = static_cast<float>(tixel::kScale);
    const float sl = slide_.value;

    // title (marquee if long) + artist
    g.offsetPush(sl * 40.f * static_cast<float>(dir_), 0);
    g.setFont(tixel::theme::kUi);
    const float tw = static_cast<float>(g.textWidth(t->title));
    const float mx = (tw <= static_cast<float>(tixel::kWidth - 12 * tixel::kScale))
                         ? (tixel::kWidth - tw) / 2.f
                         : 6.f * u;
    title_.draw(g, mx, 2 * u);
    g.setFont(tixel::theme::kTiny);
    g.textCenter(tixel::kWidth / 2.f, 19, t->artist);
    if (prevIndex_ >= 0 && sl > 0.01f) {
        g.offsetPush((sl - 1) * 40.f * static_cast<float>(dir_), 0);
        g.setFont(tixel::theme::kUi);
        const float pw = static_cast<float>(g.textWidth(kTracks[prevIndex_].title));
        const float pxs = (pw <= static_cast<float>(tixel::kWidth - 12 * tixel::kScale))
                              ? (tixel::kWidth - pw) / 2.f
                              : 6.f * u;
        g.text(pxs, 2 * u, kTracks[prevIndex_].title);
        g.offsetPop();
    }
    g.offsetPop();

    drawDisc(g, angle_, kDiscX, kDiscY, kDiscR);
    drawSkip(g, kDiscX - 22 * u, kControlsY, -1, flash_[0].value);
    drawPlayPause(g, kDiscX, kControlsY, morph_.value, flash_[1].value);
    drawSkip(g, kDiscX + 22 * u, kControlsY, 1, flash_[2].value);

    // spectrum: 13 bars, peak caps falling behind
    const float bw = 3.f * u;   // 13 bars x 6 px on the 128 px panel
    const float x0 = tixel::kWidth / 2.f - kBands * bw / 2.f;
    for (int i = 0; i < kBands; i++) {
        const int h = tixel::roundi(tixel::clampf(bars_[i].value, 0, 1.1f) * 7 * u);
        const float x = x0 + static_cast<float>(i) * bw;
        if (h > 0) g.box(x, kSpectrumBase - static_cast<float>(h), bw - 1, static_cast<float>(h));
        g.hline(x, kSpectrumBase - static_cast<float>(tixel::roundi(peaks_[i] * 7 * u)) - 2, bw - 1);
    }
    g.hline(12, kProgressY + 1, static_cast<float>(tixel::kWidth - 24));
    const float px = 12 + (tixel::kWidth - 24) * pos_ / static_cast<float>(t->duration);
    g.box(12, kProgressY, px - 12, 3);
    g.disc(px, kProgressY + 1.5f, 2.2f);

    const int elapsed = static_cast<int>(pos_), remaining = t->duration - elapsed;
    char b[24];
    g.setFont(tixel::theme::kTiny);
    std::snprintf(b, sizeof(b), "%d:%02d", elapsed / 60, elapsed % 60);
    g.text(12, kTimesY, b);
    std::snprintf(b, sizeof(b), "-%d:%02d", remaining / 60, remaining % 60);
    g.textRight(tixel::kWidth - 12, kTimesY, b);
}

const tixel::Spring* Music::probe(const char** label) {
    *label = probeLabel_;
    return probeSpring_;
}

}  // namespace

tixel::Page* musicPage() {
    sMusic.title = "Music";
    return &sMusic;
}

}  // namespace demo
