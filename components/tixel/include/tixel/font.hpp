// TIXEL C++ - u8g2-format font decoder.
//
// The demo builds ship u8g2 font blobs (see fonts.hpp) but not the u8g2
// library: this class decodes that format directly and pushes horizontal runs
// into the canvas, which is exactly what u8g2's ll_hvline hook does in the
// reference, so glyph placement matches it pixel for pixel.
#pragma once

#include <cstdint>

namespace tixel {

class Canvas;

class Font {
public:
    constexpr Font() = default;
    constexpr explicit Font(const uint8_t* data) : data_(data) {}

    bool valid() const { return data_ != nullptr; }
    const uint8_t* data() const { return data_; }

    // Font-wide metrics (same fields u8g2 exposes).
    int ascent() const { return static_cast<int8_t>(data_[13]); }   // capital A
    int descent() const { return static_cast<int8_t>(data_[14]); }  // lower g
    int lineHeight() const { return ascent() - descent(); }
    int maxGlyphWidth() const { return data_[9]; }
    int maxGlyphHeight() const { return data_[10]; }

    int advance(uint32_t codepoint) const;   // delta x of one glyph
    int width(const char* s) const;          // sum of advances (lm_text_w)

    // Draws with the top of the reference glyph at y; returns the end x.
    int draw(Canvas& canvas, int x, int y, const char* s) const;

private:
    struct Glyph {
        const uint8_t* data = nullptr;
        int width = 0, height = 0, x = 0, y = 0, advance = 0;
    };

    const uint8_t* findGlyph(uint32_t codepoint) const;
    bool glyph(uint32_t codepoint, Glyph& out) const;

    const uint8_t* data_ = nullptr;
};

}  // namespace tixel
