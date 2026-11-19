// TIXEL C++ - rasteriser.
//
// Every primitive is reduced to horizontal spans in screen space; span()
// applies clip, mode and ordered dither, so behaviour is identical for shapes
// and glyphs.
#include "tixel/canvas.hpp"

#include <cstring>

namespace tixel {
namespace {

// 4x4 Bayer thresholds 0..15; a pixel is kept when BAYER < alpha (0..16).
constexpr uint8_t kBay[4][4] = {
    {0, 8, 2, 10},
    {12, 4, 14, 6},
    {3, 11, 1, 9},
    {15, 7, 13, 5},
};

inline void putPixel(uint8_t* buf, int x, int y, Mode m) {
    uint8_t* p = &buf[(y >> 3) * kWidth + x];
    const uint8_t bit = static_cast<uint8_t>(1u << (y & 7));
    if (m == Mode::Set) *p |= bit;
    else if (m == Mode::Clear) *p &= static_cast<uint8_t>(~bit);
    else *p ^= bit;
}

int alpha16(float a) {
    int v = roundi(a * 16.f);
    return v < 0 ? 0 : v > 16 ? 16 : v;
}

uint8_t colMask(int xmod4, int a16) {
    uint8_t m = 0;
    for (int r = 0; r < 8; r++)
        if (kBay[r & 3][xmod4] < a16) m |= static_cast<uint8_t>(1u << r);
    return m;
}

// Left inset of row ry of a w*h rounded rect with radius r: same corner test
// as the reference (pixel centre inside the quarter disc).
int rrInset(int w, int h, int r, int ry) {
    (void)w;
    if (r <= 0) return 0;
    int cy;
    if (ry < r) cy = ry;
    else if (ry >= h - r) cy = h - 1 - ry;
    else return 0;
    const float dy = static_cast<float>(cy) + 0.5f - static_cast<float>(r);
    const float lim = static_cast<float>(r * r) + 0.5f;
    for (int xx = 0; xx < r; xx++) {
        const float dx = static_cast<float>(xx) + 0.5f - static_cast<float>(r);
        if (dx * dx + dy * dy <= lim) return xx;
    }
    return r;
}

int rrClamp(int w, int h, float r) {
    int ri = roundi(r);
    ri = mini(ri, mini(w / 2, h / 2));
    return ri < 0 ? 0 : ri;
}

// pixels whose centre lies within the half-width hw of centre c
inline void rowExtent(float c, float hw, int* a, int* b) {
    *a = static_cast<int>(std::ceil(c - hw - 0.5f));
    *b = static_cast<int>(std::floor(c + hw - 0.5f)) + 1;   // exclusive
}

}  // namespace

// ---------------------------------------------------------------- pixels
bool bufGet(const uint8_t* buf, int x, int y) {
    if (static_cast<unsigned>(x) >= static_cast<unsigned>(kWidth) ||
        static_cast<unsigned>(y) >= static_cast<unsigned>(kHeight))
        return false;
    return (buf[(y >> 3) * kWidth + x] >> (y & 7)) & 1u;
}

void bufPut(uint8_t* buf, int x, int y, bool on) {
    if (static_cast<unsigned>(x) >= static_cast<unsigned>(kWidth) ||
        static_cast<unsigned>(y) >= static_cast<unsigned>(kHeight))
        return;
    putPixel(buf, x, y, on ? Mode::Set : Mode::Clear);
}

void Canvas::span(int x0, int x1, int y) {
    const Clip* c = &clip_;
    if (y < c->y0 || y >= c->y1) return;
    if (x0 < c->x0) x0 = c->x0;
    if (x1 > c->x1) x1 = c->x1;
    if (x0 >= x1 || alpha_ == 0) return;

    uint8_t* row = target_ + (y >> 3) * kWidth;
    const uint8_t bit = static_cast<uint8_t>(1u << (y & 7));
    if (alpha_ >= 16) {
        if (mode_ == Mode::Set) {
            for (int x = x0; x < x1; x++) row[x] |= bit;
        } else if (mode_ == Mode::Clear) {
            for (int x = x0; x < x1; x++) row[x] &= static_cast<uint8_t>(~bit);
        } else {
            for (int x = x0; x < x1; x++) row[x] ^= bit;
        }
        return;
    }
    const uint8_t* b = kBay[y & 3];
    for (int x = x0; x < x1; x++) {
        if (b[x & 3] >= alpha_) continue;
        if (mode_ == Mode::Set) row[x] |= bit;
        else if (mode_ == Mode::Clear) row[x] &= static_cast<uint8_t>(~bit);
        else row[x] ^= bit;
    }
}

void Canvas::plot(int x, int y) { span(x, x + 1, y); }

// ---------------------------------------------------------------- context
void Canvas::init() {
    std::memset(screen_, 0, sizeof(screen_));
    target_ = screen_;
    mode_ = Mode::Set;
    alpha_ = 16;
    ox_ = oy_ = 0;
    clip_ = Clip{0, 0, kWidth, kHeight};
    clipSp_ = 0;
    offSp_ = 0;
}

void Canvas::reset(uint8_t* target) {
    target_ = target;
    mode_ = Mode::Set;
    alpha_ = 16;
    ox_ = oy_ = 0;
    clip_ = Clip{0, 0, kWidth, kHeight};
    clipSp_ = 0;
    offSp_ = 0;
}

void Canvas::clear() { std::memset(target_, 0, kFbBytes); }

void Canvas::setAlpha(float a) { alpha_ = static_cast<uint8_t>(alpha16(a)); }

void Canvas::clipPush(float x, float y, float w, float h) {
    if (clipSp_ < kGfxClipDepth) clipStack_[clipSp_] = clip_;
    clipSp_++;
    const int x0 = roundi(x) + ox_, y0 = roundi(y) + oy_;
    const int x1 = x0 + roundi(w), y1 = y0 + roundi(h);
    Clip* c = &clip_;
    c->x0 = static_cast<int16_t>(maxi(c->x0, x0));
    c->y0 = static_cast<int16_t>(maxi(c->y0, y0));
    c->x1 = static_cast<int16_t>(mini(c->x1, x1));
    c->y1 = static_cast<int16_t>(mini(c->y1, y1));
    if (c->x1 < c->x0) c->x1 = c->x0;
    if (c->y1 < c->y0) c->y1 = c->y0;
}

void Canvas::clipPop() {
    if (!clipSp_) return;
    clipSp_--;
    if (clipSp_ < kGfxClipDepth) clip_ = clipStack_[clipSp_];
}

void Canvas::offsetPush(float dx, float dy) {
    if (offSp_ < kGfxOffsetDepth) {
        offStack_[offSp_][0] = ox_;
        offStack_[offSp_][1] = oy_;
    }
    offSp_++;
    ox_ = static_cast<int16_t>(ox_ + roundi(dx));
    oy_ = static_cast<int16_t>(oy_ + roundi(dy));
}

void Canvas::offsetPop() {
    if (!offSp_) return;
    offSp_--;
    if (offSp_ < kGfxOffsetDepth) {
        ox_ = offStack_[offSp_][0];
        oy_ = offStack_[offSp_][1];
    }
}

// ---------------------------------------------------------------- rects
void Canvas::pixel(float x, float y) { plot(roundi(x) + ox_, roundi(y) + oy_); }

void Canvas::hline(float x, float y, float w) {
    const int xi = roundi(x) + ox_, wi = roundi(w);
    if (wi > 0) span(xi, xi + wi, roundi(y) + oy_);
}

void Canvas::vline(float x, float y, float h) {
    const int xi = roundi(x) + ox_, yi = roundi(y) + oy_, hi = roundi(h);
    for (int i = 0; i < hi; i++) plot(xi, yi + i);
}

void Canvas::box(float x, float y, float w, float h) {
    const int xi = roundi(x) + ox_, yi = roundi(y) + oy_;
    const int wi = roundi(w), hi = roundi(h);
    if (wi <= 0 || hi <= 0) return;
    for (int r = 0; r < hi; r++) span(xi, xi + wi, yi + r);
}

void Canvas::frame(float x, float y, float w, float h) {
    const int xi = roundi(x), yi = roundi(y), wi = roundi(w), hi = roundi(h);
    if (wi <= 0 || hi <= 0) return;
    hline(static_cast<float>(xi), static_cast<float>(yi), static_cast<float>(wi));
    if (hi > 1) hline(static_cast<float>(xi), static_cast<float>(yi + hi - 1), static_cast<float>(wi));
    if (hi > 2) {
        vline(static_cast<float>(xi), static_cast<float>(yi + 1), static_cast<float>(hi - 2));
        if (wi > 1) vline(static_cast<float>(xi + wi - 1), static_cast<float>(yi + 1), static_cast<float>(hi - 2));
    }
}

void Canvas::rbox(float x, float y, float w, float h, float r) {
    const int xi = roundi(x) + ox_, yi = roundi(y) + oy_;
    const int wi = roundi(w), hi = roundi(h);
    if (wi <= 0 || hi <= 0) return;
    const int ri = rrClamp(wi, hi, r);
    for (int ry = 0; ry < hi; ry++) {
        const int in = rrInset(wi, hi, ri, ry);
        span(xi + in, xi + wi - in, yi + ry);
    }
}

void Canvas::rframe(float x, float y, float w, float h, float r) {
    const int xi = roundi(x) + ox_, yi = roundi(y) + oy_;
    const int wi = roundi(w), hi = roundi(h);
    if (wi <= 0 || hi <= 0) return;
    const int ro = rrClamp(wi, hi, r);
    const int iw = wi - 2, ih = hi - 2;
    const int rin = (iw > 0 && ih > 0) ? rrClamp(iw, ih, static_cast<float>(ro - 1)) : 0;
    for (int ry = 0; ry < hi; ry++) {
        const int a = rrInset(wi, hi, ro, ry);
        const int x0 = xi + a, x1 = xi + wi - a;
        const int iry = ry - 1;
        if (iw > 0 && ih > 0 && iry >= 0 && iry < ih) {
            const int b = rrInset(iw, ih, rin, iry);
            const int i0 = xi + 1 + b, i1 = xi + 1 + iw - b;
            if (i0 < i1) {
                span(x0, i0, yi + ry);
                span(i1, x1, yi + ry);
                continue;
            }
        }
        span(x0, x1, yi + ry);
    }
}

// ---------------------------------------------------------------- round
void Canvas::disc(float cx, float cy, float r) {
    if (r <= 0.f) return;
    const float sx = cx + ox_, sy = cy + oy_;
    const int y0 = static_cast<int>(std::floor(sy - r)), y1 = static_cast<int>(std::ceil(sy + r));
    for (int y = y0; y <= y1; y++) {
        const float dy = static_cast<float>(y) + 0.5f - sy;
        const float d = r * r - dy * dy;
        if (d < 0.f) continue;
        int a, b;
        rowExtent(sx, std::sqrt(d), &a, &b);
        span(a, b, y);
    }
}

void Canvas::ring(float cx, float cy, float r, float thick) {
    if (r <= 0.f) return;
    const float ri = r - thick;
    const float sx = cx + ox_, sy = cy + oy_;
    const int y0 = static_cast<int>(std::floor(sy - r)), y1 = static_cast<int>(std::ceil(sy + r));
    for (int y = y0; y <= y1; y++) {
        const float dy = static_cast<float>(y) + 0.5f - sy;
        const float d = r * r - dy * dy;
        if (d < 0.f) continue;
        int a, b;
        rowExtent(sx, std::sqrt(d), &a, &b);
        const float di = ri * ri - dy * dy;
        if (ri > 0.f && di > 0.f) {
            int ia, ib;
            rowExtent(sx, std::sqrt(di), &ia, &ib);
            span(a, ia, y);
            span(ib, b, y);
        } else {
            span(a, b, y);
        }
    }
}

void Canvas::arc(float cx, float cy, float r, float a0, float a1, float thick) {
    if (r <= 0.f || a1 <= a0) return;
    const float spanAngle = a1 - a0;
    const bool full = spanAngle >= kTau;
    const float ri = r - thick;
    const float sx = cx + ox_, sy = cy + oy_;
    const int x0 = static_cast<int>(std::floor(sx - r)), x1 = static_cast<int>(std::ceil(sx + r));
    const int y0 = static_cast<int>(std::floor(sy - r)), y1 = static_cast<int>(std::ceil(sy + r));
    a0 = a0 - kTau * std::floor(a0 / kTau);
    for (int y = y0; y <= y1; y++) {
        const float dy = static_cast<float>(y) + 0.5f - sy;
        for (int x = x0; x <= x1; x++) {
            const float dx = static_cast<float>(x) + 0.5f - sx;
            const float d2 = dx * dx + dy * dy;
            if (d2 > r * r || (ri > 0.f && d2 <= ri * ri)) continue;
            if (!full) {
                float ang = atan2f(dx, -dy);   // clockwise from 12
                if (ang < 0) ang += kTau;
                float rel = ang - a0;
                if (rel < 0) rel += kTau;
                if (rel > spanAngle) continue;
            }
            plot(x, y);
        }
    }
}

// ---------------------------------------------------------------- lines
void Canvas::line(float fx0, float fy0, float fx1, float fy1) {
    int x0 = roundi(fx0) + ox_, y0 = roundi(fy0) + oy_;
    int x1 = roundi(fx1) + ox_, y1 = roundi(fy1) + oy_;
    const int dx = x1 > x0 ? x1 - x0 : x0 - x1, dy = y1 > y0 ? y0 - y1 : y1 - y0;
    const int sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
    int err = dx + dy;
    for (;;) {
        plot(x0, y0);
        if (x0 == x1 && y0 == y1) break;
        const int e2 = 2 * err;
        if (e2 >= dy) {
            err += dy;
            x0 += sx;
        }
        if (e2 <= dx) {
            err += dx;
            y0 += sy;
        }
    }
}

void Canvas::capsule(float x0, float y0, float x1, float y1, float thick) {
    if (thick <= 1.05f) {
        line(x0, y0, x1, y1);
        return;
    }
    const float r = thick * 0.5f;
    x0 += ox_;
    x1 += ox_;
    y0 += oy_;
    y1 += oy_;
    const int bx0 = static_cast<int>(std::floor(minf(x0, x1) - r)) - 1;
    const int bx1 = static_cast<int>(std::ceil(maxf(x0, x1) + r)) + 1;
    const int by0 = static_cast<int>(std::floor(minf(y0, y1) - r)) - 1;
    const int by1 = static_cast<int>(std::ceil(maxf(y0, y1) + r)) + 1;
    const float vx = x1 - x0, vy = y1 - y0, l2 = vx * vx + vy * vy;
    for (int y = by0; y < by1; y++) {
        const float py = static_cast<float>(y) + 0.5f;
        int run = -1;
        for (int x = bx0; x <= bx1; x++) {
            bool in = false;
            if (x < bx1) {
                const float px = static_cast<float>(x) + 0.5f;
                const float t = l2 < 1e-9f ? 0.f : clamp01(((px - x0) * vx + (py - y0) * vy) / l2);
                const float qx = x0 + t * vx - px, qy = y0 + t * vy - py;
                in = qx * qx + qy * qy <= r * r;
            }
            if (in && run < 0) run = x;
            if (!in && run >= 0) {
                span(run, x, y);
                run = -1;
            }
        }
    }
}

void Canvas::poly(const float* xy, int n) {
    if (n < 3) return;
    float miny = 1e9f, maxy = -1e9f;
    for (int i = 0; i < n; i++) {
        miny = minf(miny, xy[2 * i + 1]);
        maxy = maxf(maxy, xy[2 * i + 1]);
    }
    const int y0 = static_cast<int>(std::floor(miny)), y1 = static_cast<int>(std::ceil(maxy));
    float xs[48];
    for (int y = y0; y <= y1; y++) {
        const float py = static_cast<float>(y) + 0.5f;
        int k = 0;
        for (int i = 0; i < n && k < 48; i++) {
            const float xa = xy[2 * i], ya = xy[2 * i + 1];
            const float xb = xy[2 * ((i + 1) % n)], yb = xy[2 * ((i + 1) % n) + 1];
            if (ya == yb || ((ya > py) == (yb > py))) continue;
            xs[k++] = xa + (py - ya) * (xb - xa) / (yb - ya);
        }
        for (int i = 1; i < k; i++) {   // insertion sort
            const float v = xs[i];
            int j = i - 1;
            while (j >= 0 && xs[j] > v) {
                xs[j + 1] = xs[j];
                j--;
            }
            xs[j + 1] = v;
        }
        for (int i = 0; i + 1 < k; i += 2) {
            // pixel centre px+0.5 in (xs[i], xs[i+1]] -> even-odd inside
            const int a = static_cast<int>(std::ceil(xs[i] - 0.5f));
            const int b = static_cast<int>(std::ceil(xs[i + 1] - 0.5f));
            span(a + ox_, b + ox_, y + oy_);
        }
    }
}

void Canvas::dottedHLine(float x, float y, float w, int step) {
    const int xi = roundi(x), yi = roundi(y), wi = roundi(w);
    for (int i = 0; i < wi; i += step) pixel(static_cast<float>(xi + i), static_cast<float>(yi));
}

void Canvas::dottedVLine(float x, float y, float h, int step) {
    const int xi = roundi(x), yi = roundi(y), hi = roundi(h);
    for (int i = 0; i < hi; i += step) pixel(static_cast<float>(xi), static_cast<float>(yi + i));
}

// ---------------------------------------------------------------- text
int Canvas::text(float x, float y, const char* s) {
    const int lx = roundi(x), ly = roundi(y);
    const int end = font_.draw(*this, lx + ox_, ly + oy_, s);
    return end - ox_;
}

int Canvas::textWidth(const char* s) { return font_.width(s); }

int Canvas::textCenter(float cx, float y, const char* s) {
    return text(cx - static_cast<float>(textWidth(s)) * 0.5f, y, s);
}

int Canvas::textRight(float rx, float y, const char* s) {
    return text(rx - static_cast<float>(textWidth(s)), y, s);
}

// ---------------------------------------------------------------- buffers
uint8_t ditherCol(int xmod4, float a) { return colMask(xmod4 & 3, alpha16(a)); }

void bufFade(uint8_t* buf, float keep) {
    const int a = alpha16(keep);
    if (a >= 16) return;
    uint8_t m[4];
    for (int i = 0; i < 4; i++) m[i] = colMask(i, a);
    for (int p = 0; p < kPages; p++)
        for (int x = 0; x < kWidth; x++) buf[p * kWidth + x] &= m[x & 3];
}

void bufXorAll(uint8_t* buf) {
    for (int i = 0; i < kFbBytes; i++) buf[i] ^= 0xFF;
}

void bufBlit(uint8_t* dst, const uint8_t* src, int dx, int dy, float alpha) {
    const int a = alpha16(alpha);
    if (a <= 0) return;
    uint8_t m[4];
    for (int i = 0; i < 4; i++) m[i] = a >= 16 ? 0xFF : colMask(i, a);
    if (dy == 0) {
        // fast path (horizontal slides): whole bytes, dither anchored to the
        // source so it travels with the content instead of shimmering
        const int x0 = maxi(0, dx), x1 = mini(kWidth, kWidth + dx);
        for (int p = 0; p < kPages; p++)
            for (int x = x0; x < x1; x++) {
                const int sx = x - dx;
                dst[p * kWidth + x] |= src[p * kWidth + sx] & m[sx & 3];
            }
        return;
    }
    for (int y = maxi(0, dy); y < mini(kHeight, kHeight + dy); y++)
        for (int x = maxi(0, dx); x < mini(kWidth, kWidth + dx); x++) {
            const int sx = x - dx, sy = y - dy;
            if (bufGet(src, sx, sy) && kBay[sy & 3][sx & 3] < a) putPixel(dst, x, y, Mode::Set);
        }
}

void bufBlitRegion(uint8_t* dst, const uint8_t* src,
                   float sx, float sy, float sw, float sh,
                   float dx, float dy, float dw, float dh,
                   float r, float alpha, bool opaque) {
    const int x0 = roundi(dx), y0 = roundi(dy), w = roundi(dw), h = roundi(dh);
    if (w <= 0 || h <= 0) return;
    const int a = alpha16(alpha);
    if (a <= 0 && !opaque) return;
    const int ri = rrClamp(w, h, r);
    const float kx = sw / static_cast<float>(w), ky = sh / static_cast<float>(h);
    for (int py = maxi(0, y0); py < mini(kHeight, y0 + h); py++) {
        const int ry = py - y0;
        const int syi = static_cast<int>(std::floor(sy + (static_cast<float>(ry) + 0.5f) * ky));
        const int in = rrInset(w, h, ri, ry);
        for (int px = maxi(0, x0 + in); px < mini(kWidth, x0 + w - in); px++) {
            const int sxi = static_cast<int>(std::floor(sx + (static_cast<float>(px - x0) + 0.5f) * kx));
            const bool on = bufGet(src, sxi, syi) && kBay[py & 3][px & 3] < a;
            if (opaque) putPixel(dst, px, py, on ? Mode::Set : Mode::Clear);
            else if (on) putPixel(dst, px, py, Mode::Set);
        }
    }
}

}  // namespace tixel
