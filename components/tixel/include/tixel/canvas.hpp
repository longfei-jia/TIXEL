// TIXEL C++ - canvas.
//
// The canvas owns the frame buffer and adds what an animated 1-bit UI needs:
// float (sub-pixel) geometry, a clip and translation stack, SET/CLEAR/XOR
// modes, ordered-dither alpha, and buffer compositing for transitions.
//
// Buffer layout is the SSD1306 page format: byte = 8 vertical pixels, LSB on
// top, index = page * width + x. Any page-oriented panel driver can consume it
// directly; a row-oriented panel (Sharp memory LCD, ...) converts on flush.
#pragma once

#include <cstdint>

#include "tixel/config.hpp"
#include "tixel/font.hpp"
#include "tixel/math.hpp"

namespace tixel {

enum class Mode { Clear = 0, Set = 1, Xor = 2 };

struct Clip {
    int16_t x0 = 0, y0 = 0, x1 = kWidth, y1 = kHeight;   // x1/y1 exclusive
};

inline constexpr int kGfxClipDepth = 8;
inline constexpr int kGfxOffsetDepth = 8;

class Canvas {
public:
    Canvas() { init(); }

    void init();
    void reset(uint8_t* target);   // also resets clip/offset/mode/alpha
    void clear();

    uint8_t* screen() { return screen_; }
    const uint8_t* screen() const { return screen_; }
    uint8_t* target() { return target_; }

    void setMode(Mode m) { mode_ = m; }
    Mode mode() const { return mode_; }
    void setAlpha(float a);
    uint8_t alpha() const { return alpha_; }

    void clipPush(float x, float y, float w, float h);
    void clipPop();
    void offsetPush(float dx, float dy);
    void offsetPop();

    // ---- primitives: local float coordinates ---------------------------
    void pixel(float x, float y);
    void hline(float x, float y, float w);
    void vline(float x, float y, float h);
    void box(float x, float y, float w, float h);
    void frame(float x, float y, float w, float h);
    void rbox(float x, float y, float w, float h, float r);
    void rframe(float x, float y, float w, float h, float r);
    void disc(float cx, float cy, float r);
    void ring(float cx, float cy, float r, float thick);
    // a0/a1: radians clockwise from 12 o'clock
    void arc(float cx, float cy, float r, float a0, float a1, float thick);
    void line(float x0, float y0, float x1, float y1);
    void capsule(float x0, float y0, float x1, float y1, float thick);
    void poly(const float* xy, int n);   // even-odd fill
    void dottedHLine(float x, float y, float w, int step);
    void dottedVLine(float x, float y, float h, int step);

    // ---- text (u8g2 fonts, top-aligned) --------------------------------
    void setFont(const Font& f) { font_ = f; }
    const Font& font() const { return font_; }
    int text(float x, float y, const char* s);          // -> end x
    int textCenter(float cx, float y, const char* s);
    int textRight(float rx, float y, const char* s);
    int textWidth(const char* s);
    int charWidth(char c) { return font_.advance(static_cast<uint8_t>(c)); }
    int fontHeight() { return font_.lineHeight(); }

    // Low level: one horizontal run, clip/mode/alpha applied. This is the
    // seam u8g2 uses (ll_hvline) and the font decoder draws through.
    void span(int x0, int x1, int y);

private:
    void plot(int x, int y);

    uint8_t screen_[kFbBytes] = {};
    uint8_t* target_ = screen_;
    Mode mode_ = Mode::Set;
    uint8_t alpha_ = 16;   // 0..16 ordered-dither coverage
    int16_t ox_ = 0, oy_ = 0;
    Clip clip_{};
    Clip clipStack_[kGfxClipDepth] = {};
    uint8_t clipSp_ = 0;
    int16_t offStack_[kGfxOffsetDepth][2] = {};
    uint8_t offSp_ = 0;
    Font font_{};
};

// ---- whole-buffer operations (screen coordinates, ignore clip/offset) ---
bool bufGet(const uint8_t* buf, int x, int y);
void bufPut(uint8_t* buf, int x, int y, bool on);
// page-format column mask of the ordered dither at coverage a (0..1) for a
// column with x % 4 == xmod4: AND a page byte with it to dither it
uint8_t ditherCol(int xmod4, float a);
void bufFade(uint8_t* buf, float keep);   // keep `keep` of lit pixels
void bufXorAll(uint8_t* buf);
// OR `src` shifted by (dx, dy) into dst; `alpha` dithers it (1 = opaque)
void bufBlit(uint8_t* dst, const uint8_t* src, int dx, int dy, float alpha);
// Nearest-neighbour scale the source rect (sx,sy,sw,sh) of `src` into the
// destination rect (dx,dy,dw,dh), clipped to a rounded rect of radius r.
// opaque: destination pixels inside the rect are replaced (lit where the
// dithered source is lit, cleared elsewhere); otherwise lit pixels are ORed.
void bufBlitRegion(uint8_t* dst, const uint8_t* src,
                   float sx, float sy, float sw, float sh,
                   float dx, float dy, float dw, float dh,
                   float r, float alpha, bool opaque);

// whole-screen convenience
inline void bufBlitScaled(uint8_t* dst, const uint8_t* src,
                          float x, float y, float w, float h, float alpha) {
    bufBlitRegion(dst, src, 0, 0, kWidth, kHeight, x, y, w, h, 0, alpha, false);
}

}  // namespace tixel
