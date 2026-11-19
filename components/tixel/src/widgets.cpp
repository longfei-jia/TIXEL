#include "tixel/widgets.hpp"

#include <cmath>
#include <cstring>

namespace tixel {

// ---------------------------------------------------------------- rolling
void Roll::init(const Font& font, const char* text, float dur, float stagger) {
    font_ = font;
    dur_ = dur;
    stagger_ = stagger;
    count_ = 0;
    reset(text);
}

void Roll::reset(const char* text) {
    size_t n = std::strlen(text);
    if (n > kRollMax) n = kRollMax;
    std::memcpy(text_, text, n);
    text_[n] = 0;
    count_ = static_cast<uint8_t>(n);
    for (size_t i = 0; i < n; i++) {
        cell_[i].from = cell_[i].to = text[i];
        cell_[i].t = 1e3f;
        cell_[i].dir = 1;
    }
}

void Roll::set(const char* text, int dir) {
    size_t n = std::strlen(text);
    if (n > kRollMax) n = kRollMax;
    if (n == count_ && std::memcmp(text, text_, n) == 0) return;
    if (n != count_) {
        reset(text);
        return;
    }
    int rank = 0;
    for (int i = static_cast<int>(n) - 1; i >= 0; i--) {   // rightmost rolls first
        if (cell_[i].to == text[i]) continue;
        cell_[i].from = cell_[i].to;
        cell_[i].to = text[i];
        cell_[i].dir = static_cast<int8_t>(dir < 0 ? -1 : 1);
        cell_[i].t = -static_cast<float>(rank) * stagger_;
        rank++;
    }
    std::memcpy(text_, text, n);
}

void Roll::update(float dt) {
    for (int i = 0; i < count_; i++)
        if (cell_[i].t < dur_ + 1.f) cell_[i].t += dt;
}

bool Roll::busy() const {
    for (int i = 0; i < count_; i++)
        if (cell_[i].from != cell_[i].to && cell_[i].t < dur_) return true;
    return false;
}

int Roll::width(Canvas& canvas) {
    canvas.setFont(font_);
    return canvas.textWidth(text_);
}

int Roll::draw(Canvas& canvas, float x, float y) {
    canvas.setFont(font_);
    const int fh = canvas.fontHeight();
    const float step = static_cast<float>(fh) + 2.f;
    float cx = static_cast<float>(roundi(x));
    y = static_cast<float>(roundi(y));
    canvas.clipPush(cx - 1, y, 999, static_cast<float>(fh));
    char s[2] = {0, 0};
    for (int i = 0; i < count_; i++) {
        Cell* c = &cell_[i];
        if (c->from == c->to || c->t >= dur_) {
            s[0] = c->to;
            canvas.text(cx, y, s);
        } else {
            const float p = c->t <= 0 ? 0.f : curve(kCurveOvershoot, c->t / dur_);
            const float off = p * step * c->dir;
            s[0] = c->from;
            canvas.text(cx, y - off, s);
            s[0] = c->to;
            canvas.text(cx, y + step * c->dir - off, s);
        }
        cx += static_cast<float>(canvas.charWidth(c->to));
    }
    canvas.clipPop();
    return static_cast<int>(cx);
}

// ---------------------------------------------------------------- marquee
void Marquee::init(const Font& font, const char* text, float width) {
    font_ = font;
    width_ = width;
    speed_ = 22.f * static_cast<float>(kScale);
    hold_ = 1.2f;
    gap_ = 18.f * static_cast<float>(kScale);
    set(text);
}

void Marquee::set(const char* text) {
    text_ = text;
    t_ = 0;
}

void Marquee::draw(Canvas& canvas, float x, float y) {
    canvas.setFont(font_);
    const float tw = static_cast<float>(canvas.textWidth(text_));
    canvas.clipPush(x, y - 1, width_, static_cast<float>(canvas.fontHeight()) + 3);
    if (tw <= width_) {
        canvas.text(x, y, text_);
    } else {
        const float period = (tw + gap_) / speed_;
        float t = maxf(0.f, t_ - hold_);
        t = t - (period + hold_) * std::floor(t / (period + hold_));
        const float off = minf(t, period) * speed_;
        canvas.text(x - off, y, text_);
        canvas.text(x - off + tw + gap_, y, text_);
    }
    canvas.clipPop();
}

// ---------------------------------------------------------------- toggle
void drawToggle(Canvas& canvas, float x, float y, float p, float vel) {
    const float w = static_cast<float>(scaled(15));
    const float h = static_cast<float>(scaled(9));
    const float pc = clamp01(p);
    canvas.setMode(Mode::Set);
    canvas.rframe(x, y, w, h, h * 0.44f);
    const float kx = x + h * 0.5f + (w - h) * p;
    const float cy = y + h * 0.5f;
    if (pc > 0.04f) canvas.rbox(x + 1, y + 1, kx - x, h - 2, h * 0.33f);
    // squash & stretch: the knob becomes a capsule trailing its motion
    const float tail = clampf(-vel * 6.f / 60.f * 1.4f, -3.f, 3.f) * static_cast<float>(kScale);
    canvas.setMode(Mode::Clear);
    if (absf(tail) > 0.4f)
        canvas.capsule(kx, cy, kx + tail, cy, h * 0.66f);
    canvas.disc(kx, cy, h * 0.355f);
    canvas.setMode(Mode::Set);
    const float core = h * 0.255f * (1 - pc);
    if (core > 0.6f) {
        if (absf(tail) > 0.4f)
            canvas.capsule(kx, cy, kx + tail * 0.7f, cy, core * 2);
        canvas.disc(kx, cy, core);
    }
}

void drawSpinner(Canvas& canvas, float cx, float cy, float r, float t, float thick) {
    // rotating arc whose length breathes (material indeterminate style)
    const float rot = t * kTau * 0.9f;
    const float ln = 1.3f + 3.2f * (0.5f - 0.5f * cosf(t * kTau * 0.7f));
    const float a0 = rot - kTau * std::floor(rot / kTau);
    canvas.arc(cx, cy, r, a0, a0 + ln, thick);
}

void drawScrollbar(Canvas& canvas, float x, float y, float h, float pos, float visible, float total) {
    if (total <= visible) return;
    const float w = static_cast<float>(scaled(3));
    canvas.dottedVLine(x + w * 0.33f, y, h, 2);
    float th = maxf(6.f, h * visible / total);
    float ty = y + (h - th) * clampf(pos / maxf(1.f, total - visible), -0.3f, 1.3f);
    if (ty < y) {
        th -= (y - ty);   // rubber band compresses
        ty = y;
    }
    if (ty + th > y + h) th = y + h - ty;
    th = maxf(3.f, th);
    canvas.rbox(x, ty, w, th, 1);
}

void drawChevron(Canvas& canvas, float x, float cy, bool right, int s) {
    for (int i = 0; i <= s; i++) {
        const float dx = right ? static_cast<float>(i) : static_cast<float>(s - i);
        canvas.pixel(x + dx, cy - static_cast<float>(s) + static_cast<float>(i));
        canvas.pixel(x + dx, cy + static_cast<float>(s) - static_cast<float>(i));
    }
}

void drawBattery(Canvas& canvas, float x, float y, float level) {
    const float u = static_cast<float>(kScale);
    canvas.frame(x, y, 11 * u, 6 * u);
    canvas.vline(x + 11 * u, y + 2 * u, 2 * u);
    canvas.box(x + 2 * u, y + 2 * u, static_cast<float>(roundi(7 * level)) * u, 2 * u);
}

void drawWifi(Canvas& canvas, float x, float cy, bool on) {
    const float u = static_cast<float>(kScale);
    auto px = [&](float dx, float dy) { canvas.box(x + dx * u, cy + dy * u, u, u); };
    auto row = [&](float dx, float dy, int count) { canvas.box(x + dx * u, cy + dy * u, count * u, u); };
    px(3, 2);
    if (!on) {
        canvas.capsule(x + 1 * u, cy - 2 * u, x + 5 * u, cy + 2 * u, u);
        return;
    }
    row(2, 0, 3);
    px(1, 1);
    px(5, 1);
    row(1, -2, 5);
    px(0, -1);
    px(6, -1);
}

}  // namespace tixel
