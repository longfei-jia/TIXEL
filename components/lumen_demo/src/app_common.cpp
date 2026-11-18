#include <cstdio>

#include "lumen/math.hpp"
#include "lumen/theme.hpp"
#include "lumen/widgets.hpp"
#include "lumen_demo/apps.hpp"

namespace demo {

const int32_t kDefaults[kSettingCount] = {
    /* brightness */ 80,
    /* invert     */ 0,
    /* autosleep  */ 30,
    /* motion     */ 100,
    /* vibrate    */ 1,
    /* sound      */ 1,
    /* wifi       */ 0,
};

// time on the left, radios + battery on the right
void statusBar(lumen::Ui& ui, lumen::Canvas& g, float y) {
    lumen::DateTime now;
    ui.now(&now);
    char t[8];
    std::snprintf(t, sizeof(t), "%02d:%02d", now.hour, now.min);
    g.setFont(lumen::theme::kTiny);
    g.text(2 * lumen::kScale, y + lumen::kScale, t);

    const float u = static_cast<float>(lumen::kScale);
    float x = static_cast<float>(lumen::kWidth) - 2 * u - 13 * u;
    lumen::drawBattery(g, x, y + u, 0.72f);
    x -= 10 * u;
    lumen::drawWifi(g, x, y + 3 * u, ui.setting(kSettingWifi) != 0);
    if (!ui.setting(kSettingSound)) {   // muted speaker
        x -= 8 * u;
        g.box(x, y + 2 * u, 2 * u, 3 * u);
        g.line(x + 2 * u, y + 2 * u, x + 4 * u, y);
        g.line(x + 2 * u, y + 4 * u, x + 4 * u, y + 6 * u);
        g.vline(x + 4 * u, y + 1 * u, 5 * u);
        g.setMode(lumen::Mode::Xor);
        g.line(x + 2 * u, y, x + 6 * u, y + 6 * u);
        g.setMode(lumen::Mode::Set);
    }
}

}  // namespace demo
