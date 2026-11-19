// TIXEL C++ - toast / dialog / bottom sheet.
//
// Each is a static singleton implementing the Overlay interface. The layout is
// the upstream one doubled vertically; widths are fitted to the 128 px panel.
#include "tixel/overlay.hpp"

#include <cstdio>
#include <cstring>

#include "tixel/math.hpp"
#include "tixel/theme.hpp"
#include "tixel/widgets.hpp"

namespace tixel {
namespace {

void dimBackdrop(Canvas& g, float amount) {
    if (amount > 0.01f) bufFade(g.target(), 1.f - 0.55f * clamp01(amount));
}

}  // namespace

// ======================================================================= toast
namespace {

class Toast : public Overlay {
public:
    void show(Ui& uiRef, const char* text, ToastIcon icon);
    void update(float dt) override;
    void render(Canvas& g) override;
    void close() override;

private:
    char text_[24] = {};
    ToastIcon icon_ = ToastIcon::Check;
    float life_ = 1.8f, width_ = 0.f;
    Spring y_;
};

Toast sToast;

void toastGlyph(Canvas& g, ToastIcon icon, float x, float y) {
    const float u = static_cast<float>(kScale);
    if (icon == ToastIcon::Check) {
        g.capsule(x, y + 3 * u, x + 2 * u, y + 5 * u, u);
        g.capsule(x + 2 * u, y + 5 * u, x + 6 * u, y + 1 * u, u);
    } else if (icon == ToastIcon::Wifi) {
        g.box(x + 3 * u, y + 5 * u, u, u);
        g.box(x + 2 * u, y + 3 * u, 3 * u, u);
        g.box(x + 1 * u, y + 1 * u, 5 * u, u);
    } else {
        g.box(x + 3 * u, y, u, u);
        g.box(x + 3 * u, y + 2 * u, u, 4 * u);
    }
}

void Toast::close() {
    closing = true;
    y_.setMotion(kMotionExit);   // tiny lift, then away
    y_.set(-18.f * static_cast<float>(kScale));
}

void Toast::update(float dt) {
    y_.update(dt);
    if (!closing && age > life_) close();
    if (closing && y_.value < -15.f * static_cast<float>(kScale)) dead = true;
}

void Toast::render(Canvas& g) {
    const float u = static_cast<float>(kScale);
    const float x = static_cast<float>(roundi(kWidth / 2.f - width_ / 2.f));
    const float y = static_cast<float>(roundi(y_.value));
    // velocity stretch
    const float h = 13.f * u + static_cast<float>(static_cast<int>(clamp01(absf(y_.vel) / 60.f) * 2.f * u));
    g.setMode(Mode::Clear);
    g.rbox(x - 1, y - 1, width_ + 2, h + 2, h * 0.54f);
    g.setMode(Mode::Set);
    g.rbox(x, y, width_, h, h * 0.46f);
    g.setMode(Mode::Clear);
    g.disc(x + 8.5f * u, y + h / 2, 4.5f * u);
    g.setMode(Mode::Set);
    toastGlyph(g, icon_, x + 5 * u, y + h / 2 - 3 * u);
    g.setMode(Mode::Clear);
    g.setFont(theme::kUi);
    g.text(x + 17 * u, y + (h - static_cast<float>(g.fontHeight())) / 2, text_);
    g.setMode(Mode::Set);
}

void Toast::show(Ui& uiRef, const char* text, ToastIcon icon) {
    ui = &uiRef;
    bool up = false;
    for (int i = 0; i < ui->overlayCount(); i++) up |= ui->overlayAt(i) == this;
    modal = false;
    closing = dead = false;
    age = 0;
    std::snprintf(text_, sizeof(text_), "%s", text);
    icon_ = icon;
    life_ = 1.8f;
    ui->canvas().setFont(theme::kUi);
    width_ = static_cast<float>(ui->canvas().textWidth(text_)) + 24.f * static_cast<float>(kScale);
    if (!up) y_.init(kMotionBouncy, -16.f * static_cast<float>(kScale), 1.f);
    else y_.setMotion(kMotionBouncy);   // retarget in flight
    y_.set(3.f * static_cast<float>(kScale));
    ui->show(this);
}

}  // namespace

void toast(Ui& ui, const char* text, ToastIcon icon) { sToast.show(ui, text, icon); }

// ======================================================================= dialog
namespace {

class Dialog : public Overlay {
public:
    void open(Ui& uiRef, const DialogDesc& desc);
    void key(const Event& e) override;
    void update(float dt) override;
    void render(Canvas& g) override;
    void close() override;
    const Spring* probe(const char** label) override;

private:
    void closeWith(int result);
    void cursor(bool snap);

    DialogDesc desc_{};
    int sel_ = 0, result_ = -1;
    float busy_ = -1.f;   // <0: not busy
    Spring scale_, px_, py_, curX_, curW_, shake_;
};

Dialog sDialog;

constexpr int kDialogW = kWidth - 16;
constexpr int kDialogH = scaled(50);
constexpr int kDialogX = (kWidth - kDialogW) / 2;
constexpr int kDialogY = (kHeight - kDialogH) / 2;
constexpr int kBtnGap = scaled(4);
constexpr int kBtnH = scaled(11);
constexpr int kBtnW = (kDialogW - scaled(8) - kBtnGap) / 2;

void buttonRect(int i, float r[4]) {
    r[0] = static_cast<float>(kDialogX + scaled(4) + i * (kBtnW + kBtnGap));
    r[1] = static_cast<float>(kDialogY + kDialogH - kBtnH - scaled(4));
    r[2] = static_cast<float>(kBtnW);
    r[3] = static_cast<float>(kBtnH);
}

void Dialog::cursor(bool snap) {
    float r[4];
    buttonRect(sel_, r);
    if (snap) {
        curX_.snap(r[0]);
        curW_.snap(r[2] * 0.3f);
    }
    curX_.set(r[0]);
    curW_.set(r[2]);
}

void Dialog::closeWith(int result) {
    if (closing) return;
    result_ = result;
    closing = true;
    scale_.setMotion(kMotionExit);   // swell a touch, then shrink
    scale_.set(0.f);
}

void Dialog::close() { closeWith(-1); }

void Dialog::key(const Event& e) {
    if (busy_ >= 0) return;
    if ((e.key == Key::Prev || e.key == Key::Next) && isStep(e)) {
        const int d = e.key == Key::Prev ? -1 : 1, n = sel_ + d;
        if (n >= 0 && n < 2) {
            sel_ = n;
            cursor(false);
            ui->haptic();
        } else {
            shake_.kick(static_cast<float>(d) * 90.f);
        }
    } else if (e.key == Key::Ok && e.kind == Kind::Press) {
        if (desc_.busyButton == sel_) busy_ = 0.f;
        else closeWith(sel_);
    } else if (e.key == Key::Back && e.kind == Kind::Press) {
        closeWith(-1);
    }
}

void Dialog::update(float dt) {
    Spring* springs[] = {&scale_, &px_, &py_, &curX_, &curW_, &shake_};
    for (Spring* s : springs) s->update(dt);
    if (busy_ >= 0) {
        busy_ += dt;
        if (busy_ >= desc_.busySeconds && !closing) closeWith(sel_);
    }
    if (closing && scale_.value < 0.04f) {
        dead = true;
        if (desc_.callback) desc_.callback(*ui, result_, desc_.ctx);
    }
}

void warnGlyph(Canvas& g, float x, float y) {
    const float u = static_cast<float>(kScale);
    const float tri[6] = {x + 4.5f * u, y + 0.5f * u, x + 9.5f * u, y + 9.5f * u, x - 0.5f * u, y + 9.5f * u};
    g.poly(tri, 3);
    g.setMode(Mode::Clear);
    g.vline(x + 4 * u, y + 3 * u, 3 * u);
    g.box(x + 4 * u, y + 7 * u, u, u);
    g.setMode(Mode::Set);
}

void Dialog::render(Canvas& g) {
    const float s = scale_.value;
    uint8_t* screen = g.target();
    dimBackdrop(g, s);
    if (s <= 0.02f) return;

    // content is laid out once at 1:1 into a scratch buffer, then scaled about
    // a pivot that travels from the opener to the centre
    uint8_t* buf = ui->transition().active ? nullptr : ui->scratchB();
    if (buf) {
        g.reset(buf);
        g.clear();
    }
    const float dx = shake_.value;
    g.offsetPush(dx, 0);
    g.rframe(static_cast<float>(kDialogX), static_cast<float>(kDialogY),
             static_cast<float>(kDialogW), static_cast<float>(kDialogH), static_cast<float>(scaled(5)));
    g.rframe(static_cast<float>(kDialogX + 1), static_cast<float>(kDialogY + 1),
             static_cast<float>(kDialogW - 2), static_cast<float>(kDialogH - 2),
             static_cast<float>(scaled(4)));
    g.setFont(theme::kUi);
    if (busy_ < 0.f) {
        // glyph above the centred title: at 2x type the upstream single row
        // (glyph + title + message) no longer fits the panel width
        // The glyph is 9.5*scale tall, so the title has to start below it:
        // at the old offsets the triangle base overlapped the first line.
        warnGlyph(g, kDialogX + kDialogW / 2.f - 5.f * static_cast<float>(kScale),
                  static_cast<float>(kDialogY + scaled(1)));
        g.textCenter(kDialogX + kDialogW / 2.f, static_cast<float>(kDialogY + scaled(12)), desc_.title);
        g.setFont(theme::kTiny);
        g.textCenter(kDialogX + kDialogW / 2.f, static_cast<float>(kDialogY + scaled(21)), desc_.message);
        g.setFont(theme::kUi);
        for (int i = 0; i < 2; i++) {
            float r[4];
            buttonRect(i, r);
            g.rframe(r[0], r[1], r[2], r[3], static_cast<float>(scaled(3)));
            g.textCenter(r[0] + r[2] / 2,
                         r[1] + (r[3] - static_cast<float>(g.fontHeight())) / 2, desc_.buttons[i]);
        }
        float r[4];
        buttonRect(sel_, r);
        const float cw = curW_.value;
        g.setMode(Mode::Xor);
        g.rbox(curX_.value + (r[2] - cw) / 2, r[1], cw, r[3], static_cast<float>(scaled(3)));
        g.setMode(Mode::Set);
    } else {
        drawSpinner(g, kDialogX + kDialogW / 2.f, static_cast<float>(kDialogY + scaled(18)),
                    static_cast<float>(scaled(7)), busy_, 2.f * static_cast<float>(kScale));
        char t[24];
        const int dots = 1 + static_cast<int>(busy_ * 4.f) % 3;
        std::snprintf(t, sizeof(t), "%s%.*s", desc_.busyText, dots, "...");
        g.setFont(theme::kTiny);
        g.textCenter(kDialogX + kDialogW / 2.f, static_cast<float>(kDialogY + scaled(30)), t);
        g.setFont(theme::kUi);
    }
    g.offsetPop();
    if (!buf) return;

    g.reset(screen);
    const float cx = px_.value, cy = py_.value;
    const float bx = kDialogX - 1 + dx, by = kDialogY - 1;
    const float w = (kDialogW + 2) * s, h = (kDialogH + 2) * s;
    const float x = cx + (bx - cx) * s, y = cy + (by - cy) * s;
    bufBlitRegion(screen, buf, bx, by, kDialogW + 2, kDialogH + 2, x, y, w, h,
                  6.f * s * static_cast<float>(kScale), 1.f, true);
}

const Spring* Dialog::probe(const char** label) {
    if (age < 0.9f || closing) {
        *label = "dialog scale  bouncy";
        return &scale_;
    }
    *label = "dialog cursor  snappy";
    return &curX_;
}

void Dialog::open(Ui& uiRef, const DialogDesc& desc) {
    ui = &uiRef;
    modal = true;
    closing = dead = false;
    age = 0;
    desc_ = desc;
    sel_ = desc.defaultButton;
    busy_ = -1.f;
    result_ = -1;
    scale_.init(kMotionBouncy, 0.f, static_cast<float>(kWidth));
    scale_.set(1.f);
    px_.init(kMotionCarousel, desc.originX, 1.f);
    py_.init(kMotionCarousel, desc.originY, 1.f);
    px_.set(kWidth / 2.f);
    py_.set(kHeight / 2.f);
    curX_.init(kMotionSnappy, 0, 1.f);
    curW_.init(kMotionSnappy, 0, 1.f);
    shake_.init(kMotionWobble, 0, 1.f);
    cursor(true);
    ui->show(this);
}

}  // namespace

void dialog(Ui& ui, const DialogDesc& desc) { sDialog.open(ui, desc); }

// ======================================================================= sheet
namespace {

class Sheet : public Overlay {
public:
    void open(Ui& uiRef, const SheetDesc& desc);
    void key(const Event& e) override;
    void update(float dt) override;
    void render(Canvas& g) override;
    void close() override;
    const Spring* probe(const char** label) override;

private:
    float fraction(int32_t value) const;

    SheetDesc desc_{};
    Spring y_, fill_, shake_;
    Roll pct_;
};

Sheet sSheet;
constexpr int kSheetH = scaled(44);   // upstream 40, +4 for the doubled type

void formatPercent(char* b, int32_t v) { std::snprintf(b, 8, "%3d%%", static_cast<int>(v)); }

float Sheet::fraction(int32_t v) const {
    return desc_.hi > desc_.lo
               ? static_cast<float>(v - desc_.lo) / static_cast<float>(desc_.hi - desc_.lo) * 0.9f + 0.1f
               : 1.f;
}

void Sheet::close() {
    closing = true;
    y_.setMotion(kMotionSmooth);
    y_.set(static_cast<float>(kHeight) + 3.f);
}

void Sheet::key(const Event& e) {
    if ((e.key == Key::Prev || e.key == Key::Next) && isStep(e)) {
        const int d = e.key == Key::Prev ? -1 : 1;
        int32_t nv = desc_.value + d * desc_.step;
        if (nv < desc_.lo) nv = desc_.lo;
        if (nv > desc_.hi) nv = desc_.hi;
        if (nv == desc_.value) {   // at the end stop: shake
            shake_.kick(static_cast<float>(d) * 120.f);
            ui->haptic();
            return;
        }
        desc_.value = nv;
        fill_.set(fraction(nv));
        char b[8];
        formatPercent(b, nv);
        pct_.set(b, d);
        if (desc_.onChange) desc_.onChange(*ui, nv, desc_.ctx);
    } else if ((e.key == Key::Ok || e.key == Key::Back) && e.kind == Kind::Press) {
        close();
    }
}

void Sheet::update(float dt) {
    y_.update(dt);
    fill_.update(dt);
    shake_.update(dt);
    pct_.update(dt);
    if (closing && y_.value > kHeight + 1.f) dead = true;
}

void Sheet::render(Canvas& g) {
    const float u = static_cast<float>(kScale);
    const float yv = y_.value;
    dimBackdrop(g, remap(yv, static_cast<float>(kHeight), static_cast<float>(kHeight - kSheetH), 0, 1));
    const float y = static_cast<float>(roundi(yv));
    const float x0 = 2 * u, w = static_cast<float>(kWidth) - 4 * u;
    g.setMode(Mode::Clear);
    g.rbox(x0 - 1, y - 1, w + 2, kSheetH + 10, 6 * u);
    g.setMode(Mode::Set);
    g.rframe(x0, y, w, kSheetH + 8, 5 * u);
    g.dottedHLine(static_cast<float>(kWidth) / 2 - 6 * u, y + 3 * u, 13 * u, 2);

    // sun glyph whose rays grow with the value
    const float sx = x0 + 11 * u, sy = y + 21 * u, v = fill_.value;   // centred on the percentage
    g.disc(sx, sy, (2.2f + 0.8f * v) * u);
    for (int i = 0; i < 8; i++) {
        const float a = static_cast<float>(i) * kPi / 4.f;
        const float r0 = 4.5f * u, r1 = (4.5f + 2.2f * clamp01(v)) * u;
        g.line(sx + sinf(a) * r0, sy - cosf(a) * r0, sx + sinf(a) * r1, sy - cosf(a) * r1);
    }
    g.setFont(theme::kUi);
    g.text(x0 + 22 * u, y + 6 * u, desc_.title);
    g.setFont(theme::kMid);
    const int pw = g.textWidth("100%");
    pct_.draw(g, x0 + w - 8 * u - static_cast<float>(pw), y + 14 * u);

    const float bx = x0 + 8 * u + shake_.value, by = y + (kSheetH - 12 * u), bw = w - 16 * u;
    g.rframe(bx, by, bw, 7 * u, 3 * u);
    g.rbox(bx + 2 * u, by + 2 * u, maxf(3.f, (bw - 4 * u) * clampf(v, 0, 1.05f)), 3 * u, u);
    for (int i = 0; i < 11; i++) {
        const float tx = bx + 2 * u + (bw - 5 * u) * static_cast<float>(i) / 10.f;
        g.pixel(tx, by + 8.5f * u);
        if (i % 5 == 0) g.pixel(tx, by + 9.5f * u);
    }
}

const Spring* Sheet::probe(const char** label) {
    if (age < 0.6f || closing) {
        *label = "sheet y  carousel";
        return &y_;
    }
    *label = "sheet fill  snappy";
    return &fill_;
}

void Sheet::open(Ui& uiRef, const SheetDesc& desc) {
    ui = &uiRef;
    modal = true;
    closing = dead = false;
    age = 0;
    desc_ = desc;
    y_.init(kMotionCarousel, static_cast<float>(kHeight) + 2.f, 1.f);
    y_.set(static_cast<float>(kHeight - kSheetH));
    fill_.init(kMotionSnappy, fraction(desc.value), static_cast<float>(kWidth));
    shake_.init(kMotionWobble, 0, 1.f);
    char b[8];
    formatPercent(b, desc.value);
    pct_.init(theme::kMid, b, 0.35f, 0.05f);
    ui->show(this);
}

}  // namespace

void sheet(Ui& ui, const SheetDesc& desc) { sSheet.open(ui, desc); }

}  // namespace tixel
