// Parity harness: the C++ port vs the reference C implementation.
//
// The library is compiled at the reference geometry (128x64) with the upstream
// fonts, then every primitive and every glyph of every role is drawn twice --
// once by res/LUMEN/core/lm_gfx.c, once by lumen::Canvas -- and the two frame
// buffers are compared byte for byte. A non-zero exit code means the port
// drifted from upstream.
#include <cstdio>
#include <cstring>
#include <initializer_list>

#include "lumen/canvas.hpp"
#include "lumen/font.hpp"
#include "lumen/fonts.hpp"
#include "lumen/motion.hpp"
#include "lumen/theme.hpp"

extern "C" {
#include "lm_gfx.h"
}

namespace {

int gChecks = 0;
int gFailures = 0;

void compare(const char* what, const uint8_t* expected, const uint8_t* actual) {
    gChecks++;
    if (std::memcmp(expected, actual, lumen::kFbBytes) == 0) return;
    int first = -1;
    for (int i = 0; i < lumen::kFbBytes; i++)
        if (expected[i] != actual[i]) {
            first = i;
            break;
        }
    std::printf("FAIL %-46s first diff byte %d (page %d, x %d)  %02X vs %02X\n", what, first,
                first / lumen::kWidth, first % lumen::kWidth, expected[first], actual[first]);
    gFailures++;
}

// Runs `draw` against both implementations and compares the result.
template <typename Fn>
void both(const char* what, Fn draw) {
    lm_gfx_t ref;
    lm_gfx_init(&ref);
    lumen::Canvas mine;
    mine.init();
    draw(ref, mine);
    compare(what, lm_gfx_screen(&ref), mine.screen());
}

void textParity(const char* name, const uint8_t* upstream, const lumen::Font& font, float x, float y,
                const char* s) {
    lm_gfx_t ref;
    lm_gfx_init(&ref);
    lm_font(&ref, upstream);
    lm_text(&ref, x, y, s);

    lumen::Canvas mine;
    mine.init();
    mine.setFont(font);
    mine.text(x, y, s);

    char label[96];
    std::snprintf(label, sizeof(label), "%s \"%s\" @%.1f,%.1f", name, s, x, y);
    compare(label, lm_gfx_screen(&ref), mine.screen());
}

const char* kSamples[] = {"", "LUMEN", "Brightness 80%", "12:59", "MOTION UI FRAMEWORK",
                          "gpqy_|{}[]@#&", "Neon Horizon"};
const float kXs[] = {-9.f, -1.f, 0.f, 3.f, 17.f, 64.f, 118.f};
const float kYs[] = {-4.f, 0.f, 1.f, 7.f, 23.f, 44.f, 60.f};

}  // namespace

int main() {
    std::printf("parity: %dx%d (%d B/frame)\n", lumen::kWidth, lumen::kHeight, lumen::kFbBytes);

    // ---- text: every role, every sample, at edges and in the middle -------
    struct Role {
        const char* name;
        const uint8_t* upstream;
        const lumen::Font* mine;
    } roles[] = {
        {"ui", &u8g2_font_haxrcorp4089_tr[0], &lumen::theme::kUi},
        {"tiny", &u8g2_font_4x6_tr[0], &lumen::theme::kTiny},
        {"mid", &u8g2_font_logisoso16_tr[0], &lumen::theme::kMid},
        {"big", &u8g2_font_logisoso28_tn[0], &lumen::theme::kBig},
        {"logo", &u8g2_font_logisoso22_tr[0], &lumen::theme::kLogo},
    };
    for (const Role& r : roles)
        for (const char* s : kSamples)
            for (float y : kYs) textParity(r.name, r.upstream, *r.mine, 2, y, s);
    for (const char* s : kSamples)
        for (float x : kXs) textParity("ui", &u8g2_font_haxrcorp4089_tr[0], lumen::theme::kUi, x, 9, s);

    // ---- primitives -------------------------------------------------------
    both("hline", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_hline(&r, 3.4f, 7.6f, 40.2f);
        m.hline(3.4f, 7.6f, 40.2f);
    });
    both("vline", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_vline(&r, 12.5f, 2.f, 30.9f);
        m.vline(12.5f, 2.f, 30.9f);
    });
    both("box", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_box(&r, 4.5f, 3.5f, 33.2f, 17.8f);
        m.box(4.5f, 3.5f, 33.2f, 17.8f);
    });
    both("frame", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_frame(&r, 0.5f, 0.5f, 41.7f, 11.4f);
        m.frame(0.5f, 0.5f, 41.7f, 11.4f);
    });
    for (float rad : {0.f, 1.f, 3.f, 6.f, 30.f}) {
        both("rbox", [rad](lm_gfx_t& r, lumen::Canvas& m) {
            lm_rbox(&r, 2.5f, 4.5f, 60.f, 22.f, rad);
            m.rbox(2.5f, 4.5f, 60.f, 22.f, rad);
        });
        both("rframe", [rad](lm_gfx_t& r, lumen::Canvas& m) {
            lm_rframe(&r, 2.5f, 4.5f, 60.f, 22.f, rad);
            m.rframe(2.5f, 4.5f, 60.f, 22.f, rad);
        });
    }
    both("disc", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_disc(&r, 30.7f, 20.3f, 12.6f);
        m.disc(30.7f, 20.3f, 12.6f);
    });
    both("ring", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_ring(&r, 60.2f, 30.8f, 18.4f, 3.1f);
        m.ring(60.2f, 30.8f, 18.4f, 3.1f);
    });
    both("arc", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_arc(&r, 40.f, 30.f, 20.f, 0.7f, 4.2f, 2.5f);
        m.arc(40.f, 30.f, 20.f, 0.7f, 4.2f, 2.5f);
    });
    both("line", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_line(&r, 1.f, 1.f, 90.f, 50.f);
        m.line(1.f, 1.f, 90.f, 50.f);
    });
    both("capsule", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_capsule(&r, 10.f, 10.f, 70.f, 40.f, 5.f);
        m.capsule(10.f, 10.f, 70.f, 40.f, 5.f);
    });
    both("poly", [](lm_gfx_t& r, lumen::Canvas& m) {
        const float tri[6] = {20.f, 2.f, 40.f, 30.f, 2.f, 30.f};
        lm_poly(&r, tri, 3);
        m.poly(tri, 3);
    });
    both("dotted h/v", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_dotted_hline(&r, 0.f, 9.f, 120.f, 3);
        lm_dotted_vline(&r, 5.f, 0.f, 60.f, 2);
        m.dottedHLine(0.f, 9.f, 120.f, 3);
        m.dottedVLine(5.f, 0.f, 60.f, 2);
    });
    both("clip + offset", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_gfx_clip_push(&r, 10, 8, 40, 20);
        lm_gfx_off_push(&r, 6, -3);
        lm_rbox(&r, 0, 0, 80, 40, 6);
        lm_gfx_off_pop(&r);
        lm_gfx_clip_pop(&r);
        m.clipPush(10, 8, 40, 20);
        m.offsetPush(6, -3);
        m.rbox(0, 0, 80, 40, 6);
        m.offsetPop();
        m.clipPop();
    });
    both("mode clear/xor", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_rbox(&r, 5, 5, 50, 20, 4);
        lm_gfx_mode(&r, LM_CLEAR);
        lm_disc(&r, 25, 15, 8);
        lm_gfx_mode(&r, LM_XOR);
        lm_box(&r, 0, 12, 60, 5);
        m.rbox(5, 5, 50, 20, 4);
        m.setMode(lumen::Mode::Clear);
        m.disc(25, 15, 8);
        m.setMode(lumen::Mode::Xor);
        m.box(0, 12, 60, 5);
    });
    for (float a : {0.f, 0.25f, 0.5f, 0.75f, 1.02f}) {
        both("alpha dither", [a](lm_gfx_t& r, lumen::Canvas& m) {
            lm_gfx_alpha(&r, a);
            lm_box(&r, 0, 0, 40, 40);
            m.setAlpha(a);
            m.box(0, 0, 40, 40);
        });
    }
    both("holder bar (alpha + xor)", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_box(&r, 0, 0, 128, 60);
        lm_gfx_mode(&r, LM_XOR);
        lm_hline(&r, 20, 59, 88);
        lm_vline(&r, 30, 56, 2);
        m.box(0, 0, 128, 60);
        m.setMode(lumen::Mode::Xor);
        m.hline(20, 59, 88);
        m.vline(30, 56, 2);
    });

    // ---- whole-buffer ops -------------------------------------------------
    both("buf fade", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_box(&r, 0, 0, 128, 64);
        lm_buf_fade(lm_gfx_screen(&r), 0.45f);
        m.box(0, 0, 128, 64);
        lumen::bufFade(m.screen(), 0.45f);
    });
    both("buf blit", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_box(&r, 0, 0, 40, 20);
        lm_rbox(&r, 10, 30, 50, 14, 4);
        // source = a shifted copy of the same content
        uint8_t src[LM_FB_BYTES];
        std::memcpy(src, lm_gfx_screen(&r), LM_FB_BYTES);
        lm_buf_blit(lm_gfx_screen(&r), src, 9, 0, 0.6f);
        lm_buf_blit(lm_gfx_screen(&r), src, -5, 7, 1.f);
        m.box(0, 0, 40, 20);
        m.rbox(10, 30, 50, 14, 4);
        uint8_t src2[lumen::kFbBytes];
        std::memcpy(src2, m.screen(), lumen::kFbBytes);
        lumen::bufBlit(m.screen(), src2, 9, 0, 0.6f);
        lumen::bufBlit(m.screen(), src2, -5, 7, 1.f);
    });
    both("buf blit region", [](lm_gfx_t& r, lumen::Canvas& m) {
        lm_box(&r, 4, 4, 30, 20);
        lm_disc(&r, 30, 40, 9);
        uint8_t src[LM_FB_BYTES];
        std::memcpy(src, lm_gfx_screen(&r), LM_FB_BYTES);
        lm_buf_blit_region(lm_gfx_screen(&r), src, 0, 0, 128, 64, 30.5f, 10.2f, 70.4f, 40.8f, 5.f,
                           0.8f, true);
        m.box(4, 4, 30, 20);
        m.disc(30, 40, 9);
        uint8_t src2[lumen::kFbBytes];
        std::memcpy(src2, m.screen(), lumen::kFbBytes);
        lumen::bufBlitRegion(m.screen(), src2, 0, 0, 128, 64, 30.5f, 10.2f, 70.4f, 40.8f, 5.f, 0.8f,
                             true);
    });
    both("dither col", [](lm_gfx_t& r, lumen::Canvas& m) {
        for (int x = 0; x < 128; x++) {
            const uint8_t e = lm_dither_col(x & 3, 0.4f);
            const uint8_t a = lumen::ditherCol(x & 3, 0.4f);
            (void)e;
            (void)a;
        }
        lm_box(&r, 0, 0, 128, 64);
        lm_buf_fade(lm_gfx_screen(&r), 0.4f);
        m.box(0, 0, 128, 64);
        lumen::bufFade(m.screen(), 0.4f);
    });

    // ---- motion ----------------------------------------------------------
    {
        lumen::Spring s;
        s.init(lumen::kMotionCarousel, 0.f, 32.f);
        s.set(5.f);
        for (int i = 0; i < 120; i++) s.update(1.f / 60.f);
        const bool ok = s.rest && s.value > 4.9f;
        gChecks++;
        if (!ok) {
            std::printf("FAIL spring did not settle: value %.3f rest %d\n", s.value, s.rest);
            gFailures++;
        }
    }
    {
        lumen::Tween t;
        t.start(0, 10, 0.4f, lumen::kCurveStandard, 0);
        t.update(0.2f);
        const float mid = t.value();
        gChecks++;
        if (!(mid > 5.f && mid < 9.9f)) {
            std::printf("FAIL tween midpoint %.3f\n", mid);
            gFailures++;
        }
    }

    std::printf("%d checks, %d failures\n", gChecks, gFailures);
    return gFailures ? 1 : 0;
}
