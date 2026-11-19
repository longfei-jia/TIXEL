// Clock with odometer digits + a stopwatch face, paged horizontally.
//
// Layout note: upstream puts HH:MM and the seconds on one line. Doubling the
// digits makes that 140 px wide, so the seconds move to their own line under
// the time (the same compromise the reference 128x128 port made).
#include <cstdio>

#include "tixel/math.hpp"
#include "tixel/theme.hpp"
#include "tixel/widgets.hpp"
#include "apps/apps.hpp"

namespace tixel::apps {
namespace {

const char* kDays[] = {"MON", "TUE", "WED", "THU", "FRI", "SAT", "SUN"};
const char* kMonths[] = {"JAN", "FEB", "MAR", "APR", "MAY", "JUN",
                         "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"};

void two(char* b, int v) {
    const unsigned u = static_cast<unsigned>(v < 0 ? 0 : v) % 100u;
    b[0] = static_cast<char>('0' + u / 10);
    b[1] = static_cast<char>('0' + u % 10);
    b[2] = 0;
}

class Clock : public tixel::Page {
public:
    void enter() override;
    bool key(const tixel::Event& e) override;
    void update(float dt) override;
    void render(tixel::Canvas& g) override;
    const tixel::Spring* probe(const char** label) override;

private:
    void resetStopwatch();
    void bigHm(tixel::Canvas& g, tixel::Roll& a, tixel::Roll& b, float y, float colon);
    void subTime(tixel::Canvas& g, tixel::Roll& r, float y);
    void faceClock(tixel::Canvas& g);
    void faceStopwatch(tixel::Canvas& g);

    int face_ = 0;
    bool running_ = false;
    float swTime_ = 0.f;
    tixel::Spring pager_, hint_;
    tixel::Roll hh_, mm_, ss_, swM_, swS_;
};

Clock sClock;

void Clock::enter() {
    tixel::DateTime n;
    ui->now(&n);
    char b[3];
    face_ = 0;
    pager_.init(tixel::kMotionCarousel, 0, static_cast<float>(tixel::kWidth));
    hint_.init(tixel::kMotionBouncy, 0, 2);
    two(b, n.hour);
    hh_.init(tixel::theme::kBig, b, 0.55f, 0.09f);
    two(b, n.min);
    mm_.init(tixel::theme::kBig, b, 0.55f, 0.09f);
    two(b, n.sec);
    ss_.init(tixel::theme::kMid, b, 0.32f, 0.05f);
    two(b, static_cast<int>(swTime_) / 60);
    swM_.init(tixel::theme::kBig, b, 0.4f, 0.07f);
    two(b, static_cast<int>(swTime_) % 60);
    swS_.init(tixel::theme::kBig, b, 0.4f, 0.07f);
}

void Clock::resetStopwatch() {
    running_ = false;
    swTime_ = 0;
    swM_.set("00", -1);   // digits roll back down
    swS_.set("00", -1);
    hint_.poke(1.f);
}

bool Clock::key(const tixel::Event& e) {
    if ((e.key == tixel::Key::Prev || e.key == tixel::Key::Next) && tixel::isStep(e)) {
        const int d = e.key == tixel::Key::Prev ? -1 : 1, n = face_ + d;
        if (n >= 0 && n <= 1) {
            face_ = n;
            pager_.set(static_cast<float>(n));
        } else if (face_ == 1 && d > 0 && !running_ && swTime_ > 0 && e.kind == tixel::Kind::Press) {
            resetStopwatch();   // NEXT past the last face while paused: reset
        } else {
            pager_.kick(static_cast<float>(d) * 2.2f);
            ui->haptic();
        }
        return true;
    }
    if (e.key == tixel::Key::Ok) {
        if (face_ != 1) return true;
        if (e.kind == tixel::Kind::Long) {   // 4-key: hold OK also resets
            resetStopwatch();
        } else {
            running_ = !running_;
        }
        hint_.poke(1.f);
        return true;
    }
    return false;
}

void Clock::update(float dt) {
    tixel::DateTime n;
    ui->now(&n);
    char b[3];
    two(b, n.hour);
    hh_.set(b, 1);
    two(b, n.min);
    mm_.set(b, 1);
    two(b, n.sec);
    ss_.set(b, 1);
    if (running_) swTime_ += dt * 100.f / static_cast<float>(tixel::maxi(1, ui->setting(tixel::kSettingMotion)));
    two(b, static_cast<int>(swTime_) / 60);
    swM_.set(b, 1);
    two(b, static_cast<int>(swTime_) % 60);
    swS_.set(b, 1);
    tixel::Roll* rolls[] = {&hh_, &mm_, &ss_, &swM_, &swS_};
    for (tixel::Roll* r : rolls) r->update(dt);
    pager_.update(dt);
    hint_.update(dt);
}

// HH:MM centred on its own line; returns x after the big part
void Clock::bigHm(tixel::Canvas& g, tixel::Roll& a, tixel::Roll& b, float y, float colon) {
    const float wa = static_cast<float>(a.width(g)), wb = static_cast<float>(b.width(g));
    const float cw = static_cast<float>(tixel::scaled(8));
    g.setFont(tixel::theme::kBig);
    const float bh = static_cast<float>(g.fontHeight());
    const float x = tixel::kWidth / 2.f - (wa + cw + wb) / 2.f;
    a.draw(g, x, y);
    const float cx = x + wa + cw / 2;
    g.setAlpha(colon);
    const float dot = 4.f * static_cast<float>(tixel::kScale);
    g.rbox(cx - dot / 2, y + bh * 0.28f - dot / 2, dot, dot, 1);   // square dots match logisoso
    g.rbox(cx - dot / 2, y + bh * 0.70f - dot / 2, dot, dot, 1);
    g.setAlpha(1);
    b.draw(g, x + wa + cw, y);
    (void)bh;
}

void Clock::subTime(tixel::Canvas& g, tixel::Roll& r, float y) {
    g.setFont(tixel::theme::kMid);
    const float w = static_cast<float>(r.width(g));
    r.draw(g, tixel::kWidth / 2.f - w / 2, y);
}

void Clock::faceClock(tixel::Canvas& g) {
    tixel::DateTime n;
    ui->now(&n);
    char date[16];
    std::snprintf(date, sizeof(date), "%s %02d %s", kDays[n.wday % 7], n.day, kMonths[(n.month + 11) % 12]);
    g.setFont(tixel::theme::kTiny);
    g.textCenter(tixel::kWidth / 2.f, 6, date);

    const float frac = static_cast<float>(n.ms) / 1000.f;
    const float colon = 0.25f + 0.75f * (0.5f + 0.5f * tixel::cosf(frac * tixel::kTau));
    bigHm(g, hh_, mm_, 24, colon);
    subTime(g, ss_, 76);

    // seconds ruler: passed ticks tall, the live one grows with overshoot
    const float sec = static_cast<float>(n.sec) + frac;
    const float x0 = tixel::kWidth / 2.f - 59;
    const float u = static_cast<float>(tixel::kScale);
    for (int i = 0; i < 60; i++) {
        float h;
        if (i < static_cast<int>(sec)) {
            h = (i % 5 ? 3.f : 5.f) * u;
        } else if (i == static_cast<int>(sec)) {
            h = u + tixel::curve(tixel::kCurveOvershoot, sec - static_cast<float>(i)) * (i % 5 ? 2.f : 4.f) * u;
        } else {
            h = (i % 5 ? 1.f : 2.f) * u;
        }
        g.vline(x0 + static_cast<float>(i) * 2, 112 - static_cast<float>(tixel::roundi(h)),
                static_cast<float>(tixel::roundi(h)));
    }
}

void Clock::faceStopwatch(tixel::Canvas& g) {
    g.setFont(tixel::theme::kTiny);
    g.textCenter(tixel::kWidth / 2.f, 6, "STOPWATCH");
    bigHm(g, swM_, swS_, 24, 1.f);

    char cs[4];
    two(cs, static_cast<int>((swTime_ - static_cast<float>(static_cast<int>(swTime_))) * 100.f));
    g.setFont(tixel::theme::kMid);
    const float cw = static_cast<float>(g.textWidth(cs));
    g.text(tixel::kWidth / 2.f - cw / 2, 70, cs);

    const float u = static_cast<float>(tixel::kScale);
    // short hints: at 2x body size the upstream long hint no longer fits
    const char* hint = running_ ? "OK PAUSE" : swTime_ > 0 ? "OK RESUME" : "OK START";
    g.setFont(tixel::theme::kTiny);
    const float hw = static_cast<float>(g.textWidth(hint));
    const float total = 12 * u + hw;
    const float cy = 112.f;
    const float cx = tixel::kWidth / 2.f - total / 2 + 3 * u;
    if (running_) g.disc(cx, cy, (2.2f + 0.6f * tixel::sinf(age * 8)) * u);
    else g.ring(cx, cy, 2.6f * u, u);
    g.text(cx + 6 * u, cy - 4 * u - 2 * hint_.value, hint);   // hint rises when toggled
}

void Clock::render(tixel::Canvas& g) {
    const float off = -pager_.value * tixel::kWidth;
    g.offsetPush(off, 0);
    faceClock(g);
    g.offsetPop();
    g.offsetPush(off + tixel::kWidth, 0);
    faceStopwatch(g);
    g.offsetPop();
    const float u = static_cast<float>(tixel::kScale);
    for (int i = 0; i < 2; i++) g.pixel(tixel::kWidth / 2.f - 3 * u + static_cast<float>(i) * 6 * u, 122);
    g.rbox(tixel::kWidth / 2.f - 3 * u + pager_.value * 6 * u - 1.5f * u + 0.5f, 120, 4 * u, 3 * u, 1);
}

const tixel::Spring* Clock::probe(const char** label) {
    *label = "pager  carousel";
    return &pager_;
}

}  // namespace

tixel::Page* clockPage() {
    sClock.title = "Clock";
    return &sClock;
}

}  // namespace tixel::apps
