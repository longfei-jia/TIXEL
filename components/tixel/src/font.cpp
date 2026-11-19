// TIXEL C++ - font decoding (u8g2 font format).
//
// Reference: u8g2_font.c (u8g2 font_info layout, LSB-first bit stream,
// run-length encoded glyphs). Offsets below are documented in that file.
#include "tixel/font.hpp"

#include <cstring>

#include "tixel/canvas.hpp"

namespace tixel {
namespace {

constexpr int kFontStructSize = 23;   // header bytes before the glyph table

// LSB-first bit reader over the packed glyph stream.
struct BitReader {
    const uint8_t* p;
    uint32_t bit;

    uint32_t read(int count) {
        uint32_t value = 0;
        for (int i = 0; i < count; i++) {
            value |= static_cast<uint32_t>((p[bit >> 3] >> (bit & 7)) & 1u) << i;
            bit++;
        }
        return value;
    }
    int readSigned(int count) {
        uint32_t v = read(count);
        return static_cast<int>(v) - (1 << (count - 1));
    }
};

// UTF-8 -> codepoint, one per call. A malformed lead byte is returned as-is so
// the pen still advances by one byte instead of stalling.
const char* nextCodepoint(const char* s, uint32_t* cp) {
    const auto b0 = static_cast<uint8_t>(s[0]);
    if (b0 < 0x80) {
        *cp = b0;
        return s + 1;
    }
    if ((b0 & 0xE0) == 0xC0) {
        const auto b1 = static_cast<uint8_t>(s[1]);
        if ((b1 & 0xC0) == 0x80) {
            *cp = static_cast<uint32_t>(((b0 & 0x1Fu) << 6) | (b1 & 0x3Fu));
            return s + 2;
        }
    } else if ((b0 & 0xF0) == 0xE0) {
        const auto b1 = static_cast<uint8_t>(s[1]);
        const auto b2 = static_cast<uint8_t>(s[2]);
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80) {
            *cp = static_cast<uint32_t>(((b0 & 0x0Fu) << 12) | ((b1 & 0x3Fu) << 6) | (b2 & 0x3Fu));
            return s + 3;
        }
    } else if ((b0 & 0xF8) == 0xF0) {
        const auto b1 = static_cast<uint8_t>(s[1]);
        const auto b2 = static_cast<uint8_t>(s[2]);
        const auto b3 = static_cast<uint8_t>(s[3]);
        if ((b1 & 0xC0) == 0x80 && (b2 & 0xC0) == 0x80 && (b3 & 0xC0) == 0x80) {
            *cp = static_cast<uint32_t>(((b0 & 0x07u) << 18) | ((b1 & 0x3Fu) << 12) |
                                        ((b2 & 0x3Fu) << 6) | (b3 & 0x3Fu));
            return s + 4;
        }
    }
    *cp = b0;
    return s + 1;
}

// Advance used for a codepoint the font has no glyph for. Keeps mixed text from
// collapsing onto itself; half-width for latin, full-width for CJK.
constexpr int kFallbackHalf = 6;
constexpr int kFallbackFull = 12;

}  // namespace

const uint8_t* Font::findGlyph(uint32_t codepoint) const {
    const uint8_t* font = data_ + kFontStructSize;

    if (codepoint <= 255) {
        const int startA = (data_[17] << 8) | data_[18];
        const int starta = (data_[19] << 8) | data_[20];
        if (codepoint >= 'a') {
            font += starta;
        } else if (codepoint >= 'A') {
            font += startA;
        }
        for (;;) {
            if (font[1] == 0) break;
            if (font[0] == codepoint) return font + 2;   // skip encoding + size
            font += font[1];
        }
        return nullptr;
    }

    // Unicode: an index table of (offset, first) pairs precedes the glyphs.
    const int startUnicode = (data_[21] << 8) | data_[22];
    font += startUnicode;
    const uint8_t* index = font;

    uint16_t first;
    do {
        font += static_cast<uint16_t>((index[0] << 8) | index[1]);
        first = static_cast<uint16_t>((index[2] << 8) | index[3]);
        index += 4;
    } while (first < codepoint);

    for (;;) {
        uint16_t encoding = static_cast<uint16_t>((font[0] << 8) | font[1]);
        if (encoding == 0) break;
        if (encoding == codepoint) return font + 3;   // skip encoding + size
        font += font[2];
    }
    return nullptr;
}

bool Font::glyph(uint32_t codepoint, Glyph& out) const {
    const uint8_t* data = findGlyph(codepoint);
    if (!data) return false;

    BitReader br{data, 0};
    out.data = data;
    out.width = static_cast<int>(br.read(data_[4]));
    out.height = static_cast<int>(br.read(data_[5]));
    out.x = br.readSigned(data_[6]);
    out.y = br.readSigned(data_[7]);
    out.advance = br.readSigned(data_[8]);
    return true;
}

int Font::advance(uint32_t codepoint) const {
    Glyph g;
    return glyph(codepoint, g) ? g.advance : 0;
}

// UTF-8 text, so GB2312 CJK and half-width latin can share one line. The font
// needs a unicode index table for codepoints above 255 (see findGlyph).
int Font::width(const char* s) const {
    int w = 0;
    while (*s) {
        uint32_t cp;
        s = nextCodepoint(s, &cp);
        Glyph g;
        if (glyph(cp, g)) {
            w += g.advance;
        } else {
            w += cp < 0x80 ? kFallbackHalf : kFallbackFull;
        }
    }
    return w;
}

int Font::draw(Canvas& canvas, int x, int y, const char* s) const {
    if (!data_) return x;

    const int bitsWidth = data_[4], bitsHeight = data_[5];
    const int bitsX = data_[6], bitsY = data_[7], bitsAdvance = data_[8];
    const int bits0 = data_[2], bits1 = data_[3];
    const int vref = ascent() + 1;   // u8g2 font position "top"

    int pen = x;
    while (*s) {
        uint32_t cp;
        s = nextCodepoint(s, &cp);
        Glyph g;
        if (!glyph(cp, g)) {
            pen += cp < 0x80 ? kFallbackHalf : kFallbackFull;   // missing glyph
            continue;
        }
        if (g.width > 0) {
            BitReader br{g.data, 0};
            (void)br.read(bitsWidth);
            const int height = static_cast<int>(br.read(bitsHeight));
            (void)br.readSigned(bitsX);
            (void)br.readSigned(bitsY);
            (void)br.readSigned(bitsAdvance);

            const int tx = pen + g.x;
            const int ty = y + vref - (height + g.y);
            const int glyphWidth = g.width;

            uint8_t lx = 0, ly = 0;
            // Decode one run of pixels; a run wraps at the glyph's right edge.
            auto run = [&](uint8_t len, bool foreground) {
                uint8_t left = len;
                for (;;) {
                    const uint8_t remaining = static_cast<uint8_t>(glyphWidth - lx);
                    const uint8_t current = left < remaining ? left : remaining;
                    if (foreground && current) {
                        canvas.span(tx + lx, tx + lx + current, ty + ly);
                    }
                    if (left < remaining) break;
                    left = static_cast<uint8_t>(left - remaining);
                    lx = 0;
                    ly++;
                }
                lx = static_cast<uint8_t>(lx + left);
            };

            for (;;) {
                const uint8_t zeros = static_cast<uint8_t>(br.read(bits0));
                const uint8_t ones = static_cast<uint8_t>(br.read(bits1));
                do {
                    run(zeros, false);
                    run(ones, true);
                } while (br.read(1) != 0);
                if (ly >= height) break;
            }
        }
        pen += g.advance;
    }
    return pen;
}

}  // namespace tixel
