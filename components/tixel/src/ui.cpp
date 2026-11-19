// TIXEL C++ - core runtime.
//
// One frame: input -> dispatch -> update (motion-scaled dt) -> render ->
// post -> present.
#include "tixel/ui.hpp"

#include <cmath>
#include <cstring>

#include "tixel/math.hpp"

namespace tixel {
namespace {

// ------------------------------------------------------------ transitions
void driveInit(Drive& d, Transition kind, bool forward) {
    d = Drive{};
    if (kind == Transition::Zoom) {
        d.useSpring = true;
        // progress 0..1 spans the screen width: rest within ~0.35 px
        d.spring.init(forward ? kMotionZoom : kMotionCarousel, 0.f, static_cast<float>(kWidth));
        d.spring.set(1.f);
    } else if (kind == Transition::Fade) {
        d.tween.start(0, 1, 0.55f, kCurveInOut, 0);
    } else {
        d.tween.start(0, 1, 0.42f, kCurveStandard, 0);
    }
}

float driveValue(const Drive& d) { return d.useSpring ? d.spring.value : d.tween.value(); }
bool driveDone(const Drive& d) { return d.useSpring ? d.spring.rest : d.tween.done(); }

void driveReverse(Drive& d) {
    if (d.useSpring) {
        // heading home should not wind up again: positive zero
        d.spring.setMotion(kMotionCarousel);
        d.spring.set(0.f);
    } else {
        const float v = d.tween.value();
        const float dur = d.tween.dur * absf(v);
        d.tween.start(v, 0.f, dur < 0.16f ? 0.16f : dur, kCurveStandard, 0);
    }
}

// Strategy: each kind composes old (a) and new (b) into the screen.
void composeSlide(const TransitionState& t, float e, uint8_t* out, uint8_t* a, uint8_t* b) {
    uint8_t* under;
    uint8_t* over;
    float ux, ox, uf;
    if (t.forward) {
        under = a;
        over = b;
        ux = -40.f * e;
        ox = static_cast<float>(kWidth) * (1 - e);
        uf = 1 - 0.75f * e;
    } else {
        under = b;
        over = a;
        ux = -40.f * (1 - e);
        ox = static_cast<float>(kWidth) * e;
        uf = 0.25f + 0.75f * e;
    }
    std::memset(out, 0, kFbBytes);
    bufBlit(out, under, roundi(ux), 0, uf);
    const int oxi = roundi(ox);
    for (int p = 0; p < kPages; p++)   // the incoming sheet is opaque
        for (int x = maxi(0, oxi); x < kWidth; x++) out[p * kWidth + x] = 0;
    bufBlit(out, over, oxi, 0, 1.f);
    if (oxi > 0 && oxi < kWidth)
        for (int y = oxi & 1; y < kHeight; y += 2) bufPut(out, oxi - 1, y, true);
}

void composeZoom(const TransitionState& t, float e, uint8_t* out, uint8_t* a, uint8_t* b) {
    // e may leave [0,1]: below 0 is the anticipation squeeze, above 1 the
    // page landing a little past full screen
    uint8_t* bg = a;
    uint8_t* fg = b;
    if (!t.forward) {
        e = 1 - e;
        bg = b;
        fg = a;
    }
    const float rx = t.rect[0], ry = t.rect[1], rw = t.rect[2], rh = t.rect[3];
    const float pcx = rx + rw / 2, pcy = ry + rh / 2;
    const float s = 1 + 0.45f * e;
    std::memset(out, 0, kFbBytes);
    // camera flies into the icon: background scales about the icon centre
    bufBlitScaled(out, bg, pcx - pcx * s, pcy - pcy * s,
                  static_cast<float>(kWidth) * s, static_cast<float>(kHeight) * s,
                  clamp01(1 - e * 1.4f));
    const float x = lerpf(rx, 0, e), y = lerpf(ry, 0, e);
    const float w = lerpf(rw, static_cast<float>(kWidth), e);
    const float h = lerpf(rh, static_cast<float>(kHeight), e);
    const float r = lerpf(6, 0, clamp01(e * 1.15f));
    const float lvl = clamp01(e * 3.f);
    if (lvl > 0 && w >= 1 && h >= 1)
        bufBlitRegion(out, fg, 0, 0, static_cast<float>(kWidth), static_cast<float>(kHeight),
                      x, y, w, h, r, lvl, true);
    if (e < 0.97f && w >= 1 && h >= 1 && t.newPage && t.newPage->ui) {
        Canvas& c = t.newPage->ui->canvas();
        c.reset(out);
        c.rframe(static_cast<float>(roundi(x)), static_cast<float>(roundi(y)),
                 static_cast<float>(roundi(w)), static_cast<float>(roundi(h)), r);
    }
}

void composeFade(const TransitionState& t, float e, uint8_t* out, uint8_t* a, uint8_t* b) {
    (void)t;
    // ordered dissolve: new page where the dither mask is on, old elsewhere
    uint8_t m[4];
    for (int i = 0; i < 4; i++) m[i] = ditherCol(i, e);
    for (int p = 0; p < kPages; p++)
        for (int x = 0; x < kWidth; x++) {
            const int k = p * kWidth + x;
            out[k] = static_cast<uint8_t>((b[k] & m[x & 3]) | (a[k] & static_cast<uint8_t>(~m[x & 3])));
        }
}

using ComposeFn = void (*)(const TransitionState&, float, uint8_t*, uint8_t*, uint8_t*);
const ComposeFn kCompose[] = {nullptr, composeSlide, composeZoom, composeFade};

// ------------------------------------------------------------ post effects
void xorSpan(uint8_t* buf, int y, int x0, int x1) {
    x0 = maxi(0, x0);
    x1 = mini(kWidth, x1);
    const uint8_t bit = static_cast<uint8_t>(1u << (y & 7));
    uint8_t* row = buf + (y >> 3) * kWidth;
    for (int x = x0; x < x1; x++) row[x] ^= bit;
}

// invert, with a circular wipe from the centre while it changes
float inCubic(float t) { return t * t * t; }
float inoutCubic(float t) {
    return t < .5f ? 4 * t * t * t
                   : 1 - ((-2 * t + 2) * (-2 * t + 2) * (-2 * t + 2)) / 2;
}

// CRT power-off/on: squash to a line, shrink to a dot, fade. c: 0 on .. 1 off
float postCrt(uint8_t* buf, uint8_t* tmp, float c) {
    if (c <= 0.f) return 1.f;
    std::memcpy(tmp, buf, kFbBytes);
    std::memset(buf, 0, kFbBytes);
    const int cy = kHeight / 2;
    if (c < 0.5f) {
        const float s = 1 - inCubic(c / 0.5f);
        const int h = maxi(1, roundi(static_cast<float>(kHeight) * s));
        const int y0 = cy - h / 2;
        // the collapsing beam saturates into a solid bright bar
        const float fill = s < 0.35f ? remap(s, 0.35f, 0.05f, 0, 1) : 0.f;
        uint8_t fm[4];
        for (int i = 0; i < 4; i++) fm[i] = ditherCol(i, fill);
        for (int i = 0; i < h; i++) {
            const int sy = mini(kHeight - 1, static_cast<int>((static_cast<float>(i) + 0.5f) * kHeight / static_cast<float>(h)));
            const int y = y0 + i;
            if (y < 0 || y >= kHeight) continue;
            for (int x = 0; x < kWidth; x++) {
                const bool on = bufGet(tmp, x, sy) || ((fm[x & 3] >> (y & 7)) & 1u);
                if (on) bufPut(buf, x, y, true);
            }
        }
        return 1.f;
    }
    if (c < 0.82f) {
        const float e = inoutCubic((c - 0.5f) / 0.32f);
        const int w = maxi(2, roundi(static_cast<float>(kWidth) * (1 - e)));
        const int x0 = kWidth / 2 - w / 2;
        for (int x = x0; x < x0 + w; x++) {
            bufPut(buf, x, cy - 1, true);
            bufPut(buf, x, cy, true);
        }
        return 1.f;
    }
    const float e = (c - 0.82f) / 0.18f;
    if (e < 1.f) {
        const float r = 2.2f * (1 - e) + 0.6f;
        for (int y = cy - 4; y <= cy + 4; y++)
            for (int x = kWidth / 2 - 4; x <= kWidth / 2 + 4; x++) {
                const float dx = static_cast<float>(x) + 0.5f - kWidth / 2.f;
                const float dy = static_cast<float>(y) + 0.5f - static_cast<float>(cy);
                if (dx * dx + dy * dy <= r * r) bufPut(buf, x, y, true);
            }
        return 1.f - e * 0.6f;
    }
    return 0.f;
}

uint32_t fnv(const uint8_t* p, int n) {
    uint32_t h = 2166136261u;
    for (int i = 0; i < n; i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

}  // namespace

// invert, with a circular wipe from the centre while it changes
void Ui::postInvert(uint8_t* buf) {
    const bool inv = settings_.get(kSettingInvert) != 0;
    if (!invertActive_) {
        if (inv) bufXorAll(buf);
        return;
    }
    const float p = invertTween_.value();
    const float cx = kWidth / 2.f, cy = kHeight / 2.f;
    const float R = std::sqrt(cx * cx + cy * cy) + 2.f;
    const float r = p * R, r2 = r + 2.5f;
    const bool ring = p < 0.98f;
    const bool from = invertFrom_;
    for (int y = 0; y < kHeight; y++) {
        const float dy = static_cast<float>(y) + 0.5f - cy;
        // outside: inv_from, inside: inv, ring: flipped
        if (from) xorSpan(buf, y, 0, kWidth);
        const float d = r * r - dy * dy;
        int a = kWidth / 2, b = kWidth / 2;
        if (d > 0) {
            const float hw = std::sqrt(d);
            a = static_cast<int>(std::ceil(cx - hw - 0.5f));
            b = static_cast<int>(std::floor(cx + hw - 0.5f)) + 1;
        }
        if (inv != from && b > a) xorSpan(buf, y, a, b);
        if (ring) {
            const float d2 = r2 * r2 - dy * dy;
            if (d2 > 0) {
                const float hw = std::sqrt(d2);
                const int a2 = static_cast<int>(std::ceil(cx - hw - 0.5f));
                const int b2 = static_cast<int>(std::floor(cx + hw - 0.5f)) + 1;
                if (b > a) {
                    xorSpan(buf, y, a2, a);
                    xorSpan(buf, y, b, b2);
                } else {
                    xorSpan(buf, y, a2, b2);
                }
            }
        }
    }
}

// ======================================================================= ui
void Ui::init(const Port& port, const int32_t* defaults, int defaultCount) {
    port_ = port;
    canvas_.init();
    settings_.init(defaults, defaultCount);
    if (port.settingsLoad) {
        int32_t v[kMaxSettings];
        std::memcpy(v, settings_.v, sizeof(v));
        if (port.settingsLoad(port.ctx, v, kMaxSettings)) std::memcpy(settings_.v, v, sizeof(v));
    }
    settings_.dirty = 0;
    settings_.subscribe(settingsChanged, this);

    const float b = static_cast<float>(settings_.get(kSettingBrightness)) / 100.f;
    bright_.init(kMotionGentle, b, 500.f);
    dim_.init(kMotionGentle, 1.f, 500.f);

    depth_ = 0;
    overlayCount_ = 0;
    transition_ = TransitionState{};
    events_.clear();
    deferred_.clear();
    keys_.init(port.keyLayout);

    time_ = 0.f;
    ms_ = lastMs_ = 0;
    started_ = false;
    frames_ = 0;
    fps_ = 0.f;
    stillFrames_ = 0;
    forceFull_ = true;

    power_ = Power::Off;
    powerTween_.start(1, 0, 0.01f, kCurveLinear, 0);
    idleTime_ = 0.f;
    dimmed_ = false;
    invertTween_ = Tween{};
    invertActive_ = false;
    invertFrom_ = false;
    contrastSent_ = 0xFF;
    for (int i = 0; i < kPages; i++) pageHash_[i] = 0;
    saveTimer_ = 0.f;
    hapticAt_ = -9.f;
    clockBase_ = DateTime{};
    clockBase_.year = 2025;
    clockBase_.month = 6;
    clockBase_.day = 18;
    clockBase_.wday = 2;
    clockBase_.hour = 12;
    clockBase_.min = 59;
    panelPower(false);
}

void Ui::start(Page* root) {
    push(root, Transition::None);
    wake();
}

// ---------------------------------------------------------------- services
void Ui::haptic() {
    if (!settings_.get(kSettingVibrate)) return;
    hapticAt_ = time_;
    if (port_.haptic) port_.haptic(port_.ctx, 18);
}

void Ui::sleep() {
    if (power_ == Power::On || power_ == Power::TurningOn) {
        power_ = Power::TurningOff;
        powerTween_.start(0, 1, 0.55f, kCurveLinear, 0);
    }
}

void Ui::wake() {
    if (power_ == Power::Off || power_ == Power::TurningOff) {
        power_ = Power::TurningOn;
        powerTween_.start(1, 0, 0.6f, kCurveLinear, 0);
        idleTime_ = 0;
        panelPower(true);
        forceFull_ = true;
    }
}

void Ui::panelPower(bool on) {
    if (port_.setPower) port_.setPower(port_.ctx, on);
}

namespace {
bool isLeap(int y) { return (y % 4 == 0 && y % 100 != 0) || y % 400 == 0; }
}  // namespace

void Ui::now(DateTime* dt) const {
    if (port_.rtcGet && port_.rtcGet(port_.ctx, dt)) return;
    static const uint8_t kMonthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    DateTime d = clockBase_;
    uint32_t t = ms_ + d.ms;
    d.ms = static_cast<uint16_t>(t % 1000);
    t /= 1000;
    t += d.sec + 60u * (d.min + 60u * d.hour);
    uint32_t days = t / 86400u;
    t %= 86400u;
    d.hour = static_cast<uint8_t>(t / 3600);
    d.min = static_cast<uint8_t>(t / 60 % 60);
    d.sec = static_cast<uint8_t>(t % 60);
    while (days--) {
        d.wday = static_cast<uint8_t>((d.wday + 1) % 7);
        const int md = kMonthDays[d.month - 1] + (d.month == 2 && isLeap(d.year));
        if (++d.day > md) {
            d.day = 1;
            if (++d.month > 12) {
                d.month = 1;
                d.year++;
            }
        }
    }
    *dt = d;
}

float Ui::holdProgress(Key k) const { return keys_.hold(k, ms_); }

bool Ui::modalOpen() const {
    for (int i = 0; i < overlayCount_; i++)
        if (overlays_[i]->modal && !overlays_[i]->closing) return true;
    return false;
}

Overlay* Ui::show(Overlay* overlay) {
    overlay->ui = this;
    for (int i = 0; i < overlayCount_; i++)
        if (overlays_[i] == overlay) return overlay;   // singleton already up
    if (overlayCount_ < kMaxOverlays) overlays_[overlayCount_++] = overlay;
    return overlay;
}

void Ui::settingsChanged(void* ctx, int id, int32_t oldValue, int32_t newValue) {
    auto* ui = static_cast<Ui*>(ctx);
    if (id == kSettingBrightness) {
        ui->bright_.set(static_cast<float>(newValue) / 100.f);
    } else if (id == kSettingInvert) {
        ui->startInvert(oldValue != 0);
    }
}

// ---------------------------------------------------------------- transitions
void Ui::beginTransition(Transition kind, Page* oldPage, Page* newPage, bool forward) {
    TransitionState& t = transition_;
    if (kind == Transition::None || !oldPage) {
        t.active = false;
        return;
    }
    t.active = true;
    t.kind = kind;
    t.oldPage = oldPage;
    t.newPage = newPage;
    t.forward = forward;
    t.reversed = false;
    t.rect[0] = static_cast<float>(kWidth) / 2 - scaled(12);
    t.rect[1] = static_cast<float>(kHeight) / 2 - scaled(12);
    t.rect[2] = static_cast<float>(scaled(24));
    t.rect[3] = static_cast<float>(scaled(24));
    Page* anchor = forward ? oldPage : newPage;   // the page owning the icon
    anchor->focus(t.rect);
    driveInit(t.drive, kind, forward);
}

// ---------------------------------------------------------------- navigation
void Ui::attach(Page* p) {
    p->ui = this;
    p->age = 0;
}

void Ui::push(Page* p, Transition how) {
    if (depth_ >= kMaxStack) return;
    Page* old = top();
    attach(p);
    p->enter();
    stack_[depth_].page = p;
    stack_[depth_].how = how;
    depth_++;
    if (old) beginTransition(how, old, p, true);
}

void Ui::replace(Page* p, Transition how) {
    Page* old = top();
    attach(p);
    p->enter();
    if (!depth_) {
        push(p, Transition::None);
        return;
    }
    stack_[depth_ - 1].page = p;
    stack_[depth_ - 1].how = Transition::None;
    old->leave();
    beginTransition(how, old, p, true);
}

void Ui::pop() {
    if (depth_ <= 1) return;
    depth_--;
    Page* old = stack_[depth_].page;
    const Transition how = stack_[depth_].how;
    Page* nw = top();
    nw->resume();
    beginTransition(how != Transition::None ? how : Transition::Slide, old, nw, false);
    old->leave();
}

void Ui::home() {
    if (depth_ <= 1) return;
    Page* old = top();
    const Transition how = stack_[1].how;
    for (int i = depth_ - 1; i >= 1; i--) stack_[i].page->leave();
    depth_ = 1;
    Page* nw = top();
    nw->resume();
    beginTransition(how != Transition::None ? how : Transition::Slide, old, nw, false);
}

// ---------------------------------------------------------------- input
void Ui::dispatch(const Event& e) {
    for (int i = overlayCount_ - 1; i >= 0; i--) {   // 1. modal overlay
        Overlay* o = overlays_[i];
        if (o->modal && !o->closing) {
            o->key(e);
            return;
        }
    }
    if (e.key == Key::Back && e.kind == Kind::Long) {   // 2. global gesture
        if (depth_ > 1) home();
        else sleep();
        return;
    }
    Page* t = top();                                    // 3. page
    if (t && t->key(e)) return;
    if (e.key == Key::Back && e.kind == Kind::Press && depth_ > 1) pop();   // 4. default
}

void Ui::handleKey(const Event& e) {
    idleTime_ = 0;
    if (power_ == Power::Off || power_ == Power::TurningOff) {
        wake();
        return;
    }
    if (dimmed_) {   // first key only wakes the panel
        dimmed_ = false;
        dim_.set(1.f);
        return;
    }
    TransitionState& t = transition_;
    if (t.active) {
        if (e.key == Key::Back && e.kind == Kind::Press && t.forward && !t.reversed &&
            t.newPage == top() && depth_ > 1 && !modalOpen()) {
            // changed their mind mid-flight: run the same motion backwards
            // from where it is, with its current velocity
            depth_--;
            t.oldPage->resume();
            t.newPage->leave();
            t.reversed = true;
            driveReverse(t.drive);
            deferred_.clear();
            return;
        }
        deferred_.push(e);   // never dropped: replayed on landing
        return;
    }
    dispatch(e);
}

void Ui::post(Key key, Kind kind) { events_.push(Event{key, kind}); }

// ---------------------------------------------------------------- update
void Ui::pageUpdate(Page* p, float dt) {
    p->age += dt;
    if (p->age > 3600.f) p->age -= 3000.f;   // keep float phases precise
    p->update(dt);
}

void Ui::update(float dt) {
    const float adt = dt * static_cast<float>(settings_.get(kSettingMotion)) / 100.f;

    powerTween_.update(dt);
    if (power_ == Power::TurningOff && powerTween_.done()) {
        power_ = Power::Off;
        panelPower(false);
    } else if (power_ == Power::TurningOn && powerTween_.done()) {
        power_ = Power::On;
    }

    if (power_ == Power::On) {
        idleTime_ += dt;
        const float lim = static_cast<float>(settings_.get(kSettingAutosleep));
        if (lim > 0 && !dimmed_ && idleTime_ > maxf(lim - 5.f, lim * 0.6f)) {
            dimmed_ = true;
            dim_.set(0.25f);
        }
        if (lim > 0 && idleTime_ > lim) {
            dimmed_ = false;
            dim_.set(1.f);
            sleep();
        }
    }
    if (power_ == Power::Off) return;   // frozen while off

    TransitionState& t = transition_;
    Page* topPage = top();
    if (t.active) {
        if (t.drive.useSpring) t.drive.spring.update(adt);
        else t.drive.tween.update(adt);
        if (t.oldPage != topPage) pageUpdate(t.oldPage, adt);   // leaving pages keep moving
        if (t.newPage != topPage) pageUpdate(t.newPage, adt);
    }
    if (topPage) pageUpdate(topPage, adt);

    for (int i = 0; i < overlayCount_; i++) {
        Overlay* o = overlays_[i];
        o->age += adt;
        o->update(adt);
    }
    int n = 0;
    for (int i = 0; i < overlayCount_; i++)
        if (!overlays_[i]->dead) overlays_[n++] = overlays_[i];
    overlayCount_ = static_cast<uint8_t>(n);

    if (t.active && driveDone(t.drive)) {
        t.active = false;
        Event e;
        while (!t.active && deferred_.pop(&e)) dispatch(e);
    }

    bright_.update(dt);
    dim_.update(dt);
    if (invertActive_) {
        invertTween_.update(dt);
        if (invertTween_.done()) invertActive_ = false;
    }
    if (settings_.dirty) {
        saveTimer_ += dt;
        if (saveTimer_ > 1.0f) {   // debounce flash writes
            settings_.dirty = 0;
            saveTimer_ = 0;
            if (port_.settingsSave) port_.settingsSave(port_.ctx, settings_.v, kMaxSettings);
        }
    }
}

// ---------------------------------------------------------------- render
void Ui::render() {
    Canvas& g = canvas_;
    uint8_t* screen = g.screen();
    TransitionState& t = transition_;
    if (t.active) {
        g.reset(bufA_);
        g.clear();
        if (t.oldPage) t.oldPage->render(g);
        g.reset(bufB_);
        g.clear();
        if (t.newPage) t.newPage->render(g);
        kCompose[static_cast<int>(t.kind)](t, driveValue(t.drive), screen, bufA_, bufB_);
        g.reset(screen);
    } else {
        g.reset(screen);
        g.clear();
        Page* topPage = top();
        if (topPage) topPage->render(g);
    }
    for (int i = 0; i < overlayCount_; i++) {
        Overlay* o = overlays_[i];
        g.reset(screen);
        o->render(g);
    }
    g.reset(screen);
    // hold hint: a bar grows from the centre of the bottom edge while a
    // hold-sensitive key is down. With an intermediate stage (3-key ENTER:
    // release here = BACK) tick marks show where it begins.
    const float hp = maxf(holdProgress(Key::Back), holdProgress(Key::Ok));
    const float mark = keys_.holdMark(Key::Ok);
    const float show = mark > 0 ? mark * 0.5f : 0.15f;   // don't flash on quick taps
    if (hp > show) {
        const float w = kWidth * curve(kCurveStandard, (hp - show) / (1.f - show));
        g.setMode(Mode::Xor);
        g.hline(kWidth / 2.f - w / 2, kHeight - 1, w);
        if (mark > 0) {
            const float mw = kWidth * curve(kCurveStandard, (mark - show) / (1.f - show));
            g.vline(kWidth / 2.f - mw / 2, kHeight - 3, 2);
            g.vline(kWidth / 2.f + mw / 2, kHeight - 3, 2);
        }
        g.setMode(Mode::Set);
    }
}

void Ui::present(float level) {
    const int c = roundi(clamp01(level) * 255.f);
    if (c != contrastSent_) {
        if (port_.setContrast) port_.setContrast(port_.ctx, static_cast<uint8_t>(c));
        contrastSent_ = static_cast<uint8_t>(c);
    }
    const uint8_t* buf = canvas_.screen();
    PageMask dirty = 0;
    for (int p = 0; p < kPages; p++) {
        const uint32_t h = fnv(buf + p * kWidth, kWidth);
        if (forceFull_ || h != pageHash_[p]) {
            pageHash_[p] = h;
            dirty |= static_cast<PageMask>(1u << p);
        }
    }
    if (dirty && port_.flush) port_.flush(port_.ctx, buf, dirty);   // only dirty pages
    forceFull_ = false;
    stillFrames_ = dirty ? 0 : static_cast<uint16_t>(mini(stillFrames_ + 1, 60000));
}

void Ui::step(float dt) {
    if (dt > kMaxDt) dt = kMaxDt;
    if (dt < 0) dt = 0;
    ms_ += static_cast<uint32_t>(dt * 1000.f + 0.5f);
    time_ += dt;
    if (time_ > 3600.f) {
        time_ -= 3600.f;
        hapticAt_ -= 3600.f;
    }
    frames_++;
    if (dt > 0) fps_ = fps_ * 0.95f + 0.05f / dt;

    if (port_.readKeys) keys_.scan(port_.readKeys(port_.ctx), ms_, events_);
    Event e;
    while (events_.pop(&e)) handleKey(e);

    update(dt);
    if (power_ == Power::Off) {
        stillFrames_ = static_cast<uint16_t>(mini(stillFrames_ + 1, 60000));
        return;
    }

    render();
    uint8_t* screen = canvas_.screen();
    postInvert(screen);
    float lvl = 1.f;
    if (power_ != Power::On) lvl = postCrt(screen, bufA_, powerTween_.value());
    present(clamp01(bright_.value * dim_.value) * lvl);
}

void Ui::tick() {
    const uint32_t nowMs = port_.millis ? port_.millis(port_.ctx) : 0;
    if (!started_) {
        started_ = true;
        lastMs_ = nowMs;
    }
    const float dt = static_cast<float>(nowMs - lastMs_) / 1000.f;
    lastMs_ = nowMs;
    step(dt);
}

bool Ui::idle() const {
    return !transition_.active && stillFrames_ > 30 && power_ != Power::TurningOn &&
           power_ != Power::TurningOff;
}

const void* Ui::probe(const char** label, float* value, float* target) {
    static const char* kTrNames[] = {"", "slide  bezier(.2,0,0,1)", "zoom  2nd f=2.3 z=.78 r=-.8",
                                     "fade  bezier(.65,0,.35,1)"};
    const Spring* s = nullptr;
    *label = "";
    if (transition_.active) {   // the driver itself is the identity
        *label = transition_.reversed ? "transition reversed" : kTrNames[static_cast<int>(transition_.kind)];
        *value = driveValue(transition_.drive);
        *target = transition_.drive.useSpring ? transition_.drive.spring.target : transition_.drive.tween.b;
        return &transition_;
    }
    for (int i = overlayCount_ - 1; i >= 0; i--) {
        Overlay* o = overlays_[i];
        if (o->modal) {
            s = o->probe(label);
            if (s) break;
        }
    }
    Page* topPage = top();
    if (!s && topPage) s = topPage->probe(label);
    if (s) {
        *value = s->value;
        *target = s->target;
    }
    return s;
}

void Ui::startInvert(bool wasInverted) {
    invertFrom_ = wasInverted;
    invertActive_ = true;
    invertTween_.start(0, 1, 0.6f, kCurveInOut, 0);
}

}  // namespace tixel
