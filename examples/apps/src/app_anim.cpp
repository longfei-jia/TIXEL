// Animation playground: a menu with three demo groups. Each group cycles
// through several examples with Up/Down - every switch restarts that
// example's clock - and a hold on Ok returns to the menu. One example per
// screen, never stacked.
//
//   Layout: card->screen morph / drop-down drawer / sliding tab underline
//   Widget: disc<->square / ring<->bar cross-fade / toggle<->check / rolling digits
//   Frame : noise dissolve / radial wipe / blinds wipe between two patterns
#include <cmath>
#include <cstdio>
#include <cstring>

#include "tixel/math.hpp"
#include "tixel/menu.hpp"
#include "tixel/theme.hpp"
#include "tixel/widgets.hpp"
#include "apps/apps.hpp"

namespace tixel::apps {
namespace {

// 0 -> 1 -> 0 over `period` seconds; apply the easing curve at the call site.
float pingPong(float t, float period) {
    const float p = std::fmodf(t, period) / (period * 0.5f);
    return p < 1.f ? p : 2.f - p;
}

bool bufBit(const uint8_t* buf, int x, int y) {
    return (buf[(y >> 3) * tixel::kWidth + x] >> (y & 7)) & 1;
}

// Shared chrome for a demo screen: localized title, index badge, hint line.
void demoChrome(tixel::Canvas& g, tixel::Ui& ui, const char* name, int idx, int count) {
    g.setFont(tixel::theme::kUi);
    g.textCenter(64, 16, tr(ui, name));
    g.setFont(tixel::theme::kTiny);
    char b[24];   // worst case for two ints: 11 + 1 + 11 + NUL = 24 (xtensa GCC counts ranges)
    std::snprintf(b, sizeof(b), "%d/%d", idx + 1, count);
    g.text(124 - static_cast<float>(g.textWidth(b)), 18, b);   // right-aligned inside the frame
    g.textCenter(64, 116, tr(ui, "up-down / hold=back|上下切换 · 长按返回"));
}

// ---- Layout group -----------------------------------------------------------

class AnimLayout : public tixel::Page {
public:
    bool key(const tixel::Event& e) override {
        if ((e.key == tixel::Key::Prev || e.key == tixel::Key::Next) && tixel::isStep(e)) {
            ex_ = (ex_ + (e.key == tixel::Key::Next ? 1 : -1) + 3) % 3;
            t_ = 0;   // restart the example's clock
            return true;
        }
        return false;   // hold falls through to the BACK gesture
    }
    void update(float dt) override { t_ += dt; }
    void render(tixel::Canvas& g) override;

private:
    void renderCard(tixel::Canvas& g);
    void renderDrawer(tixel::Canvas& g);
    void renderTabs(tixel::Canvas& g);

    int ex_ = 0;
    float t_ = 0;
};

AnimLayout sAnimLayout;

const char* const kLayoutNames[3] = {"Card|卡片展开", "Drawer|抽屉", "Tabs|指示条"};

void AnimLayout::render(tixel::Canvas& g) {
    statusBar(*ui, g, 0);
    demoChrome(g, *ui, kLayoutNames[ex_], ex_, 3);
    switch (ex_) {
        case 1: renderDrawer(g); break;
        case 2: renderTabs(g); break;
        default: renderCard(g); break;
    }
}

// 0: card grows from its centre to the whole body area
void AnimLayout::renderCard(tixel::Canvas& g) {
    const float e = tixel::curve(tixel::kCurveStandard, pingPong(t_, 3.4f));
    const float w = tixel::lerpf(48, static_cast<float>(tixel::kWidth), e);
    const float h = tixel::lerpf(26, static_cast<float>(tixel::kHeight) - 26, e);
    const float x = tixel::lerpf(64 - 24, 0.f, e);
    const float y = tixel::lerpf(58 - 13, 14, e);

    g.rframe(x, y, w, h, tixel::lerpf(4, 8, e));
    g.clipPush(x + 1, y + 1, w - 2, h - 2);
    g.setAlpha(tixel::clamp01(e * 1.4f));
    const float ty = tixel::lerpf(54, 44, e);
    g.setFont(tixel::theme::kUi);
    g.textCenter(64, ty, tr(*ui, "Layout|布局级"));
    g.setFont(tixel::theme::kTiny);
    g.textCenter(64, ty + 14, tr(*ui, "card -> screen|卡片到全屏"));
    g.textCenter(64, ty + 26, tr(*ui, "one spring drives the rect|一根弹簧驱动矩形"));
    g.textCenter(64, ty + 38, tr(*ui, "contents clip to the frame|内容随框裁剪"));
    g.setAlpha(1);
    g.clipPop();
}

// 1: drawer slides down under the status bar, content rides with it
void AnimLayout::renderDrawer(tixel::Canvas& g) {
    const float e = tixel::curve(tixel::kCurveStandard, pingPong(t_, 3.6f));
    const float y = tixel::lerpf(14 - 96, 14, e);   // panel height 96

    g.rframe(0, y, static_cast<float>(tixel::kWidth), 96, 6);
    g.clipPush(0, y + 1, static_cast<float>(tixel::kWidth), 94);
    g.setAlpha(tixel::clamp01(e * 1.4f));
    g.setFont(tixel::theme::kUi);
    g.textCenter(64, y + 8, tr(*ui, "Quick set|快捷设置"));
    g.setFont(tixel::theme::kTiny);
    g.text(10, y + 22, tr(*ui, "Brightness|亮度"));
    g.text(10, y + 34, tr(*ui, "Sound|声音"));
    g.text(10, y + 46, tr(*ui, "Language|语言"));
    g.text(100, y + 22, "80%");
    g.text(100, y + 34, "On|开");
    g.text(100, y + 46, "中文");
    g.setAlpha(1);
    g.clipPop();
}

// 2: underline slides between three fixed tab slots, wraps around
void AnimLayout::renderTabs(tixel::Canvas& g) {
    const char* const tabs[3] = {"Clock|时钟", "Music|音乐", "Set|设置"};
    const float slotX[3] = {34, 64, 94};
    const float seg = std::fmodf(t_ / 1.2f, 3.f);
    const int i = static_cast<int>(seg);
    const float tt = tixel::curve(tixel::kCurveStandard, seg - static_cast<float>(i));
    const float ux = tixel::lerpf(slotX[i], slotX[(i + 1) % 3], tt);

    g.setFont(tixel::theme::kUi);
    for (int k = 0; k < 3; k++) g.textCenter(slotX[k], 44, tr(*ui, tabs[k]));
    g.hline(ux - 10, 56, 20);
}

// ---- Widget group -----------------------------------------------------------

class AnimWidget : public tixel::Page {
public:
    bool key(const tixel::Event& e) override {
        if ((e.key == tixel::Key::Prev || e.key == tixel::Key::Next) && tixel::isStep(e)) {
            ex_ = (ex_ + (e.key == tixel::Key::Next ? 1 : -1) + 4) % 4;
            t_ = 0;
            return true;
        }
        return false;
    }
    void update(float dt) override;
    void render(tixel::Canvas& g) override;

private:
    void renderShape(tixel::Canvas& g);
    void renderRingBar(tixel::Canvas& g);
    void renderToggleCheck(tixel::Canvas& g);
    void renderRollDigits(tixel::Canvas& g);

    int ex_ = 0;
    float t_ = 0;
    tixel::Roll roll_;
    bool rollReady_ = false;
    float rollAcc_ = 0;
};

AnimWidget sAnimWidget;

const char* const kWidgetNames[4] = {"Shape|形状", "Ring/Bar|环与条", "Toggle/Check|开关勾选", "Roll|数字滚动"};

void AnimWidget::update(float dt) {
    t_ += dt;
    if (ex_ == 3) {
        if (!rollReady_) {
            roll_.init(tixel::theme::kMid, "00", 0.4f, 0.08f);
            rollReady_ = true;
        }
        rollAcc_ += dt;
        const unsigned v = static_cast<unsigned>(rollAcc_ * 3.f) % 60u;
        char b[8];
        std::snprintf(b, sizeof(b), "%02u", v);
        roll_.set(b, 1);
        roll_.update(dt);
    }
}

void AnimWidget::render(tixel::Canvas& g) {
    statusBar(*ui, g, 0);
    demoChrome(g, *ui, kWidgetNames[ex_], ex_, 4);
    switch (ex_) {
        case 1: renderRingBar(g); break;
        case 2: renderToggleCheck(g); break;
        case 3: renderRollDigits(g); break;
        default: renderShape(g); break;
    }
}

// 0: disc <-> square, the parameter-interpolation poster child
void AnimWidget::renderShape(tixel::Canvas& g) {
    const float e = tixel::curve(tixel::kCurveStandard, pingPong(t_, 3.6f));
    g.rbox(64 - 18, 30, 36, 36, tixel::lerpf(18, 4, e));
    g.setFont(tixel::theme::kTiny);
    g.textCenter(64, 100, tr(*ui, "corner radius = spring output|圆角半径即弹簧输出"));
}

// 1: one value, two presentations, cross-fading
void AnimWidget::renderRingBar(tixel::Canvas& g) {
    const float v = 0.72f;
    const float m = pingPong(t_, 3.6f);
    if (m < 0.95f) {
        g.setAlpha(tixel::clamp01(1.2f - m * 1.2f));
        g.arc(40, 76, 13, -tixel::kPi / 2, -tixel::kPi / 2 + tixel::kTau * v, 4);
    }
    if (m > 0.05f) {
        g.setAlpha(tixel::clamp01(m * 1.2f));
        g.rframe(74, 72, 46, 8, 3);
        g.rbox(76, 74, 42 * v, 4, 1);
    }
    g.setAlpha(1);
    g.setFont(tixel::theme::kUi);
    g.textCenter(40, 96, "72%");
    g.textCenter(97, 96, "72%");
}

// 2: toggle knob slides, check strokes draw in, same driver
void AnimWidget::renderToggleCheck(tixel::Canvas& g) {
    const float e = tixel::curve(tixel::kCurveStandard, pingPong(t_, 2.8f));
    drawToggle(g, 14, 98, pingPong(t_, 2.8f), 0);
    g.rframe(86, 98, 16, 16, 3);
    const float t1 = tixel::clamp01(e * 2.f), t2 = tixel::clamp01(e * 2.f - 1.f);
    if (t1 > 0) g.line(89, 106, tixel::lerpf(89, 93, t1), tixel::lerpf(106, 110, t1));
    if (t2 > 0) g.line(93, 110, tixel::lerpf(93, 99, t2), tixel::lerpf(110, 100, t2));
}

// 3: rolling digits 00 -> 59
void AnimWidget::renderRollDigits(tixel::Canvas& g) {
    if (rollReady_) {
        g.setFont(tixel::theme::kMid);
        const float x = 64 - static_cast<float>(roll_.width(g)) / 2;
        roll_.draw(g, x, 58);
    }
}

// ---- Frame group ------------------------------------------------------------

uint8_t sPatA[tixel::kFbBytes];   // disc + ring
uint8_t sPatB[tixel::kFbBytes];   // square ring + centre block
bool sPatternsReady = false;

void buildPatterns() {
    if (sPatternsReady) return;
    sPatternsReady = true;
    std::memset(sPatA, 0, sizeof(sPatA));
    std::memset(sPatB, 0, sizeof(sPatB));
    for (int y = 0; y < tixel::kHeight; y++)
        for (int x = 0; x < tixel::kWidth; x++) {
            const float dx = static_cast<float>(x - 64), dy = static_cast<float>(y - 64);
            const float d = std::sqrtf(dx * dx + dy * dy);
            if (d < 44.f || (d > 50.f && d < 55.f))
                tixel::bufPut(sPatA, x, y, true);
            const int ax = dx < 0 ? -static_cast<int>(dx) : static_cast<int>(dx);
            const int ay = dy < 0 ? -static_cast<int>(dy) : static_cast<int>(dy);
            const int m = ax > ay ? ax : ay;
            if ((m < 44 && m > 37) || m < 7)
                tixel::bufPut(sPatB, x, y, true);
        }
}

class AnimFrame : public tixel::Page {
public:
    bool key(const tixel::Event& e) override {
        if ((e.key == tixel::Key::Prev || e.key == tixel::Key::Next) && tixel::isStep(e)) {
            ex_ = (ex_ + (e.key == tixel::Key::Next ? 1 : -1) + 3) % 3;
            t_ = 0;
            return true;
        }
        return false;
    }
    void update(float dt) override { t_ += dt; }
    void render(tixel::Canvas& g) override;

private:
    int ex_ = 0;
    float t_ = 0;
};

AnimFrame sAnimFrame;

const char* const kFrameNames[3] = {"Dissolve|溶解", "Radial|径向", "Blinds|百叶窗"};

// All three wipes are the same per-pixel A/B pick with a different threshold.
void AnimFrame::render(tixel::Canvas& g) {
    buildPatterns();
    const float t = tixel::curve(tixel::kCurveStandard, pingPong(t_, 4.4f));
    const float rMax = 62.f * t;
    for (int y = 0; y < tixel::kHeight; y++)
        for (int x = 0; x < tixel::kWidth; x++) {
            bool takeB;
            if (ex_ == 0) {
                const unsigned h = static_cast<unsigned>(x) * 73856093u
                                 ^ static_cast<unsigned>(y) * 19349663u;
                takeB = static_cast<float>((h >> 8) & 255) / 255.f < t;
            } else if (ex_ == 1) {
                const float dx = static_cast<float>(x - 64), dy = static_cast<float>(y - 64);
                takeB = dx * dx + dy * dy < rMax * rMax;
            } else {
                takeB = y / 16 < static_cast<int>(t * 8.f);
            }
            if (takeB ? bufBit(sPatB, x, y) : bufBit(sPatA, x, y))
                g.pixel(static_cast<float>(x), static_cast<float>(y));
        }
}

// ---- the menu ----------------------------------------------------------------

const tixel::MenuItem kAnimItems[] = {
    TIXEL_ITEM_SUBMENU("Layout|布局级", &sAnimLayout),
    TIXEL_ITEM_SUBMENU("Widget|组件级", &sAnimWidget),
    TIXEL_ITEM_SUBMENU("Frame|帧级", &sAnimFrame),
};
tixel::Menu sAnim("Animation", kAnimItems);

}  // namespace

tixel::Page* animPage() {
    sAnim.title = "Animation";
    sAnim.setTitleText("Animation|动画");
    sAnimLayout.title = "AnimLayout";
    sAnimWidget.title = "AnimWidget";
    sAnimFrame.title = "AnimFrame";
    return &sAnim;
}

}  // namespace tixel::apps
