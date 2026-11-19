// Animation playground: one menu, three demo pages - one per morphing route.
//   Layout: one spring drives a card rect from small to full screen; content
//           clips to the frame and fades in with the progress (shared-element
//           transform, the mechanism DialogDesc.origin already hints at).
//   Widget: parameter interpolation on components - disc <-> square, the same
//           progress value cross-fading between ring and bar, toggle <-> check.
//   Frame : pixel dissolve between two patterns through a fixed hash threshold.
// Demos auto-loop; the 3-key hold gesture (Ok held 0.4 s) returns to the menu.
#include <cmath>
#include <cstring>

#include "tixel/math.hpp"
#include "tixel/menu.hpp"
#include "tixel/theme.hpp"
#include "tixel/widgets.hpp"
#include "apps/apps.hpp"

namespace tixel::apps {
namespace {

// 0 -> 1 -> 0 over `period` seconds; apply the easing curve at the call site.
float pingPong(float age, float period) {
    const float p = std::fmodf(age, period) / (period * 0.5f);
    return p < 1.f ? p : 2.f - p;
}

// Pixel (x, y) of a page-format framebuffer.
bool bufBit(const uint8_t* buf, int x, int y) {
    return (buf[(y >> 3) * tixel::kWidth + x] >> (y & 7)) & 1;
}

// ---- demo 1: layout morph -------------------------------------------------

class AnimLayout : public tixel::Page {
public:
    void render(tixel::Canvas& g) override;
};

AnimLayout sAnimLayout;

void AnimLayout::render(tixel::Canvas& g) {
    statusBar(*ui, g, 0);
    const float e = tixel::curve(tixel::kCurveStandard, pingPong(age, 3.4f));

    // source card (48x26 at the centre of the body area) -> full body area
    // (below the status bar, so the morph never covers the status row)
    const float w = tixel::lerpf(48, static_cast<float>(tixel::kWidth), e);
    const float h = tixel::lerpf(26, static_cast<float>(tixel::kHeight) - 14, e);
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

// ---- demo 2: widget morphs -------------------------------------------------

class AnimWidget : public tixel::Page {
public:
    void render(tixel::Canvas& g) override;
};

AnimWidget sAnimWidget;

void AnimWidget::render(tixel::Canvas& g) {
    statusBar(*ui, g, 0);
    const float e = tixel::curve(tixel::kCurveStandard, pingPong(age, 3.6f));
    const float v = 0.72f;   // one progress value, two presentations

    // 1) poster child: disc <-> square
    g.rbox(64 - 18, 20, 36, 36, tixel::lerpf(18, 4, e));

    // 2) ring <-> bar, cross-fading on the same value
    const float m = pingPong(age, 3.6f);
    if (m < 0.95f) {
        g.setAlpha(tixel::clamp01(1.2f - m * 1.2f));
        g.arc(30, 76, 13, -tixel::kPi / 2, -tixel::kPi / 2 + tixel::kTau * v, 4);
    }
    if (m > 0.05f) {
        g.setAlpha(tixel::clamp01(m * 1.2f));
        g.rframe(62, 72, 54, 8, 3);
        g.rbox(64, 74, 50 * v, 4, 1);
    }
    g.setAlpha(1);
    g.setFont(tixel::theme::kUi);
    g.textCenter(30, 94, "72%");
    g.textCenter(89, 94, "72%");

    // 3) toggle <-> check: the knob slides with the same driver, the check
    //    strokes draw in as it settles
    drawToggle(g, 14, 106, pingPong(age, 2.8f), 0);
    g.rframe(86, 104, 16, 16, 3);
    const float t1 = tixel::clamp01(e * 2.f), t2 = tixel::clamp01(e * 2.f - 1.f);
    if (t1 > 0) g.line(89, 112, tixel::lerpf(89, 93, t1), tixel::lerpf(112, 116, t1));
    if (t2 > 0) g.line(93, 116, tixel::lerpf(93, 99, t2), tixel::lerpf(116, 106, t2));

    g.setFont(tixel::theme::kTiny);
    g.textCenter(64, 122, tr(*ui, "hold Ok = back|长按确定返回"));
}

// ---- demo 3: frame dissolve ------------------------------------------------

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
    void render(tixel::Canvas& g) override;
};

AnimFrame sAnimFrame;

void AnimFrame::render(tixel::Canvas& g) {
    buildPatterns();
    const float e = pingPong(age, 4.4f);
    // Fixed hash threshold per pixel: deterministic, no flicker between frames.
    for (int y = 0; y < tixel::kHeight; y++)
        for (int x = 0; x < tixel::kWidth; x++) {
            const unsigned h = static_cast<unsigned>(x) * 73856093u
                             ^ static_cast<unsigned>(y) * 19349663u;
            const bool takeB = static_cast<float>((h >> 8) & 255) / 255.f < e;
            if (takeB ? bufBit(sPatB, x, y) : bufBit(sPatA, x, y)) g.pixel(static_cast<float>(x), static_cast<float>(y));
        }
    g.setFont(tixel::theme::kTiny);
    g.textCenter(64, 118, tr(*ui, "hold Ok = back|长按确定返回"));
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
    sAnimLayout.title = "AnimLayout";   // plain ids: tools expect non-null titles
    sAnimWidget.title = "AnimWidget";
    sAnimFrame.title = "AnimFrame";
    return &sAnim;
}

}  // namespace tixel::apps
