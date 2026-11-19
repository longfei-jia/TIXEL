#include <cstdio>
#include <cstring>

#include "tixel/math.hpp"
#include "tixel/theme.hpp"
#include "tixel/widgets.hpp"
#include "apps/apps.hpp"

namespace tixel::apps {

const int32_t kDefaults[kSettingCount] = {
    /* brightness */ 80,
    /* invert     */ 0,
    /* autosleep  */ 30,
    /* motion     */ 100,
    /* vibrate    */ 1,
    /* language   */ 0,   // 0 = 中文, 1 = English
    /* sound      */ 1,
    /* wifi       */ 0,
};

const char* tr(tixel::Ui& ui, const char* text, int slot) {
    // Several strings can be alive at once (a dialog holds title + message +
    // two buttons), so each call takes its own buffer.
    static char bufs[4][64];
    const char* bar = text;
    while (*bar && *bar != '|') bar++;
    if (*bar != '|') return text;                                   // no translation
    if (ui.setting(tixel::kSettingLanguage) == 0) return bar + 1;   // 中文 half
    char* buf = bufs[static_cast<unsigned>(slot) & 3u];
    size_t len = static_cast<size_t>(bar - text);
    if (len > sizeof(bufs[0]) - 1) len = sizeof(bufs[0]) - 1;
    std::memcpy(buf, text, len);
    buf[len] = 0;
    return buf;
}

int utf8Count(const char* s) {
    int n = 0;
    for (; *s; s++)
        if ((static_cast<unsigned char>(*s) & 0xC0) != 0x80) n++;
    return n;
}

int utf8BytesFor(const char* s, int codepoints) {
    // Byte length of the first `codepoints` codepoints. The loop must step by
    // whole glyphs: incrementing per lead byte stopped right after it, which
    // cut the final multi-byte character (typewriter ended on "...框", not
    // "...框架").
    int n = 0;
    const char* p = s;
    while (*p && n < codepoints) {
        const unsigned char b = static_cast<unsigned char>(*p);
        const int len = b < 0x80 ? 1
                      : (b & 0xE0) == 0xC0 ? 2
                      : (b & 0xF0) == 0xE0 ? 3
                      : (b & 0xF8) == 0xF0 ? 4
                      : 1;   // malformed lead: consume a single byte
        p += len;
        n++;
    }
    return static_cast<int>(p - s);
}

// time on the left, radios + battery on the right
void statusBar(tixel::Ui& ui, tixel::Canvas& g, float y) {
    tixel::DateTime now;
    ui.now(&now);
    char t[8];
    std::snprintf(t, sizeof(t), "%02d:%02d", now.hour, now.min);
    g.setFont(tixel::theme::kTiny);
    g.text(2 * tixel::kScale, y + tixel::kScale, t);

    const float u = static_cast<float>(tixel::kScale);
    float x = static_cast<float>(tixel::kWidth) - 2 * u - 13 * u;
    tixel::drawBattery(g, x, y + u, 0.72f);
    x -= 10 * u;
    tixel::drawWifi(g, x, y + 3 * u, ui.setting(kSettingWifi) != 0);
    if (!ui.setting(kSettingSound)) {   // muted speaker
        x -= 8 * u;
        g.box(x, y + 2 * u, 2 * u, 3 * u);
        g.line(x + 2 * u, y + 2 * u, x + 4 * u, y);
        g.line(x + 2 * u, y + 4 * u, x + 4 * u, y + 6 * u);
        g.vline(x + 4 * u, y + 1 * u, 5 * u);
        g.setMode(tixel::Mode::Xor);
        g.line(x + 2 * u, y, x + 6 * u, y + 6 * u);
        g.setMode(tixel::Mode::Set);
    }
}

}  // namespace tixel::apps
