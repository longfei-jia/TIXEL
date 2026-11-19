#include <cstdio>

#include "tixel/math.hpp"
#include "tixel/theme.hpp"
#include "tixel/widgets.hpp"
#include "tixel_demo/apps.hpp"

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

}  // namespace demo
