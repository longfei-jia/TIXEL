#include "tixel/menu.hpp"

#include <cstdio>
#include <cstring>

#include "tixel/math.hpp"
#include "tixel/ui.hpp"

namespace tixel {
namespace {

constexpr int kRowH = scaled(12);
constexpr int kTop = scaled(13);
constexpr int kVisibleRows = (kHeight - kTop) / kRowH;
constexpr int kCursorX = scaled(2);
constexpr int kLabelX = scaled(6);
constexpr int kRight = scaled(7);

}  // namespace

void Menu::set(const char* title, const MenuItem* items, int count) {
    this->title = title;
    items_ = items;
    count_ = static_cast<uint8_t>(count > kMenuMaxItems ? kMenuMaxItems : count);
    sel_ = top_ = 0;
}

void Menu::valueText(int i, char* out, int n) {
    const MenuItem* it = &items_[i];
    out[0] = 0;
    if (it->kind == MenuItemKind::Option) {
        const int32_t value = ui->setting(it->setting);
        for (int k = 0; k < it->optionCount; k++)
            if (it->options[k].value == value) {
                std::snprintf(out, static_cast<size_t>(n), "%s", it->options[k].label);
                return;
            }
        std::snprintf(out, static_cast<size_t>(n), "%d", static_cast<int>(value));
    } else if (it->format) {
        it->format(*ui, out, n);
    }
}

float Menu::labelWidth(int i) {
    Canvas& g = ui->canvas();
    g.setFont(theme::kUi);
    return static_cast<float>(g.textWidth(items_[i].label)) + 9.f;
}

void Menu::counterText(char* out) const {
    std::snprintf(out, 8, "%d/%d", sel_ + 1, count_);
}

void Menu::enter() {
    sel_ = top_ = 0;
    if (!count_) return;
    // jelly cursor: leading edge snappy, trailing edge follows it
    cur_.init(kMotionSnappy, 0, static_cast<float>(kRowH));
    trail_.init(kMotionFollow, 0, static_cast<float>(kRowH));
    curW_.init(kMotionSnappy, 0, 1);
    curW_.set(labelWidth(0));
    scroll_.init(kMotionSmooth, 0, static_cast<float>(kRowH));
    press_.init(kMotionPress, 0, 6);

    char b[8];
    counterText(b);
    counter_.init(theme::kTiny, b, 0.3f, 0.04f);

    for (int i = 0; i < count_; i++) {
        Row& r = rows_[i];
        const MenuItem* it = &items_[i];
        const float on = (it->kind == MenuItemKind::Toggle && ui->setting(it->setting)) ? 1.f : 0.f;
        r.sw.init(kMotionToggle, on, 6);
        char v[kRollMax + 1];
        valueText(i, v, sizeof(v));
        r.value.init(theme::kUi, v, 0.32f, 0.03f);
        r.busy = 0;
    }
}

void Menu::move(int d) {
    int n = sel_ + d;
    if (n < 0 || n >= count_) {   // wrap: the long ride is the feedback
        n = (n + count_) % count_;
        ui->haptic();
    }
    sel_ = static_cast<uint8_t>(n);
    if (n < top_) top_ = static_cast<uint8_t>(n);
    else if (n >= top_ + kVisibleRows) top_ = static_cast<uint8_t>(n - kVisibleRows + 1);
    cur_.set(static_cast<float>(n));
    curW_.set(labelWidth(n));
    scroll_.set(static_cast<float>(top_));
    char b[8];
    counterText(b);
    counter_.set(b, d);
}

void Menu::activate(int i) {
    const MenuItem* it = &items_[i];
    Row& r = rows_[i];
    switch (it->kind) {
        case MenuItemKind::Toggle:
            if (r.busy > 0) return;
            if (it->busyMs && !ui->setting(it->setting)) {
                r.busy = static_cast<float>(it->busyMs) / 1000.f;
                return;
            }
            ui->setSetting(it->setting, !ui->setting(it->setting));
            ui->haptic();
            if (it->done) it->done(*ui, ui->setting(it->setting));
            break;
        case MenuItemKind::Option: {
            const int32_t value = ui->setting(it->setting);
            int k = 0;
            for (; k < it->optionCount; k++)
                if (it->options[k].value == value) break;
            ui->setSetting(it->setting, it->options[(k + 1) % it->optionCount].value);
            break;
        }
        case MenuItemKind::Sheet:
        case MenuItemKind::Action:
            if (it->action) it->action(*this, i);
            break;
        case MenuItemKind::Submenu:
            if (it->page) ui->push(it->page, Transition::Slide);
            break;
    }
}

bool Menu::key(const Event& e) {
    if ((e.key == Key::Prev || e.key == Key::Next) && isStep(e)) {
        move(e.key == Key::Prev ? -1 : 1);
        return true;
    }
    if (e.key == Key::Ok && e.kind == Kind::Press) {
        press_.poke(1.f);
        activate(sel_);
        return true;
    }
    return false;
}

void Menu::update(float dt) {
    trail_.set(cur_.value);   // chain: trail follows the output
    cur_.update(dt);
    trail_.update(dt);
    curW_.update(dt);
    scroll_.update(dt);
    press_.update(dt);
    counter_.update(dt);
    for (int i = 0; i < count_; i++) {
        const MenuItem* it = &items_[i];
        Row& r = rows_[i];
        if (r.busy > 0) {
            r.busy -= dt;
            if (r.busy <= 0) {
                r.busy = 0;
                ui->setSetting(it->setting, 1);
                ui->haptic();
                if (it->done) it->done(*ui, 1);
            }
        }
        if (it->kind == MenuItemKind::Toggle) r.sw.set(ui->setting(it->setting) ? 1.f : 0.f);
        r.sw.update(dt);
        char v[kRollMax + 1];
        valueText(i, v, sizeof(v));
        r.value.set(v, 1);
        r.value.update(dt);
    }
}

void Menu::drawWidget(Canvas& g, int i, float rowY) {
    const MenuItem* it = &items_[i];
    Row& r = rows_[i];
    const float rx = static_cast<float>(kWidth - kRight);
    if (it->kind == MenuItemKind::Toggle) {
        if (r.busy > 0)
            drawSpinner(g, rx - scaled(7), rowY + scaledf(4.5f), scaledf(4.5f), age, 1.8f);
        else
            drawToggle(g, rx - scaled(15), rowY + scaledf(1.f), r.sw.value, r.sw.vel);
    } else if (it->kind == MenuItemKind::Option || it->kind == MenuItemKind::Sheet) {
        g.setFont(theme::kUi);
        const float y = rowY + static_cast<float>((kRowH - theme::kUi.lineHeight()) / 2);
        r.value.draw(g, rx - static_cast<float>(r.value.width(g)), y);
    } else if (it->kind == MenuItemKind::Submenu) {
        drawChevron(g, rx - scaled(2), rowY + scaledf(5.f), true, scaled(2));
    }
}

void Menu::render(Canvas& g) {
    // title bar
    drawChevron(g, scaled(3), scaledf(5.f), false, scaled(2));
    g.setFont(theme::kUi);
    g.text(scaled(10), scaled(1), title);
    g.setFont(theme::kTiny);
    counter_.draw(g, static_cast<float>(kWidth - scaled(3) - counter_.width(g)), scaled(3));
    g.dottedHLine(0, scaled(11), static_cast<float>(kWidth), 2);

    const float sc = scroll_.value;
    const int fontH = theme::kUi.lineHeight();
    g.clipPush(0, scaled(12), static_cast<float>(kWidth), static_cast<float>(kHeight - scaled(12)));
    for (int i = 0; i < count_; i++) {
        const float ry = static_cast<float>(kTop) + (static_cast<float>(i) - sc) * kRowH;
        if (ry < -kRowH || ry > kHeight) continue;
        // staggered entrance: slide in from the right and fade up
        const float e = curve(kCurveDecel, (age - 0.08f - static_cast<float>(i - top_) * 0.055f) / 0.42f);
        const float dx = (1 - e) * static_cast<float>(scaled(28));
        // focus indent follows the cursor output continuously
        const float ind = static_cast<float>(scaled(2)) * clamp01(1.f - absf(cur_.value - static_cast<float>(i)));
        g.setAlpha(clamp01(e * 1.3f));
        g.offsetPush(dx, 0);
        g.setFont(theme::kUi);
        g.text(static_cast<float>(kLabelX) + ind, ry + static_cast<float>((kRowH - fontH) / 2),
               items_[i].label);
        drawWidget(g, i, ry);
        g.offsetPop();
        g.setAlpha(1.f);
    }
    // jelly cursor: XOR pill spanning trailing..leading edge
    const float pr = press_.value;
    const float a = minf(cur_.value, trail_.value), b = maxf(cur_.value, trail_.value);
    float ya = static_cast<float>(kTop) + (a - sc) * kRowH;
    float yb = static_cast<float>(kTop) + (b - sc) * kRowH + kRowH - 1;
    const float cap = static_cast<float>(kRowH) * 2.2f;   // long wrap rides stay compact
    if (yb - ya > cap) {
        if (cur_.value > trail_.value) ya = yb - cap;
        else yb = ya + cap;
    }
    g.setMode(Mode::Xor);
    g.rbox(static_cast<float>(kCursorX) + 1.5f * pr, ya + 0.5f * pr,
           maxf(0.f, curW_.value - 3 * pr), yb - ya - pr, static_cast<float>(scaled(3)));
    g.setMode(Mode::Set);
    g.clipPop();
    drawScrollbar(g, static_cast<float>(kWidth - scaled(3)), static_cast<float>(kTop),
                  static_cast<float>(kHeight - kTop - 1), sc,
                  static_cast<float>(kVisibleRows), static_cast<float>(count_));
}

void Menu::focusRect(float r[4]) {
    r[0] = static_cast<float>(kCursorX);
    r[1] = static_cast<float>(kTop) + (cur_.value - scroll_.value) * kRowH;
    r[2] = labelWidth(sel_);
    r[3] = static_cast<float>(kRowH - 1);
}

void Menu::focus(float r[4]) { focusRect(r); }

const Spring* Menu::probe(const char** label) {
    *label = "cursor  snappy / trail follow";
    return &cur_;
}

}  // namespace tixel
