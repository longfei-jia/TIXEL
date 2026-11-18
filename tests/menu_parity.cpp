// Menu page parity: the C++ Menu vs the reference lm_menu.c.
//
// Both menus get the same items, the same settings and the same key script at
// the reference geometry; every frame is compared byte for byte. This covers
// the page layer on top of the canvas parity in parity.cpp.
#include <cstdio>
#include <cstring>

#include "lumen/menu.hpp"
#include "lumen/settings.hpp"
#include "lumen/ui.hpp"

extern "C" {
#include "lm_menu.h"
#include "lm_ui.h"
}

namespace {

const int32_t kDefaults[] = {80, 0, 30, 100, 1, 1};
enum { kSound = LM_S_USER, kWifi, kSettingCount };

int gChecks = 0;
int gFailures = 0;

// ---- upstream side ------------------------------------------------------
const lm_option_t kSleepOpts[] = {{15, "15s"}, {30, "30s"}, {60, "60s"}, {0, "Never"}};
const lm_option_t kMotionOpts[] = {{100, "Normal"}, {50, "Slow"}, {150, "Fast"}};

void formatPercent(lm_ui_t* ui, char* out, int n) {
    std::snprintf(out, static_cast<size_t>(n), "%d%%", static_cast<int>(lm_setting(ui, LM_S_BRIGHTNESS)));
}
void noopAction(lm_menu_t*, int) {}

const lm_menu_item_t kItems[] = {
    {.label = "Brightness", .kind = LM_MI_SHEET, .setting = -1, .options = nullptr, .noptions = 0,
     .action = noopAction, .fmt = formatPercent, .page = nullptr, .busy_ms = 0, .done = nullptr},
    {.label = "Sound", .kind = LM_MI_TOGGLE, .setting = kSound, .options = nullptr, .noptions = 0,
     .action = nullptr, .fmt = nullptr, .page = nullptr, .busy_ms = 0, .done = nullptr},
    {.label = "Auto sleep", .kind = LM_MI_OPTION, .setting = LM_S_AUTOSLEEP, .options = kSleepOpts,
     .noptions = 4, .action = nullptr, .fmt = nullptr, .page = nullptr, .busy_ms = 0, .done = nullptr},
    {.label = "Motion", .kind = LM_MI_OPTION, .setting = LM_S_MOTION, .options = kMotionOpts,
     .noptions = 3, .action = nullptr, .fmt = nullptr, .page = nullptr, .busy_ms = 0, .done = nullptr},
    {.label = "Wi-Fi", .kind = LM_MI_TOGGLE, .setting = kWifi, .options = nullptr, .noptions = 0,
     .action = nullptr, .fmt = nullptr, .page = nullptr, .busy_ms = 0, .done = nullptr},
    {.label = "Reset", .kind = LM_MI_ACTION, .setting = -1, .options = nullptr, .noptions = 0,
     .action = noopAction, .fmt = nullptr, .page = nullptr, .busy_ms = 0, .done = nullptr},
};

LM_MENU_DEFINE(sRefMenu, "Settings", kItems);
lm_ui_t sRefUi;

// ---- C++ side -----------------------------------------------------------
void formatPercentCpp(lumen::Ui& ui, char* out, int n) {
    std::snprintf(out, static_cast<size_t>(n), "%d%%", static_cast<int>(ui.setting(lumen::kSettingBrightness)));
}
void noopActionCpp(lumen::Menu&, int) {}

const lumen::Option kSleepOptsCpp[] = {{15, "15s"}, {30, "30s"}, {60, "60s"}, {0, "Never"}};
const lumen::Option kMotionOptsCpp[] = {{100, "Normal"}, {50, "Slow"}, {150, "Fast"}};

const lumen::MenuItem kItemsCpp[] = {
    LUMEN_ITEM_SHEET("Brightness", noopActionCpp, formatPercentCpp),
    LUMEN_ITEM_TOGGLE("Sound", kSound),
    LUMEN_ITEM_OPTION("Auto sleep", lumen::kSettingAutosleep, kSleepOptsCpp),
    LUMEN_ITEM_OPTION("Motion", lumen::kSettingMotion, kMotionOptsCpp),
    LUMEN_ITEM_TOGGLE("Wi-Fi", kWifi),
    LUMEN_ITEM_ACTION("Reset", noopActionCpp),
};

lumen::Menu sCppMenu("Settings", kItemsCpp);
lumen::Ui sCppUi;

void frame(const char* what) {
    const uint8_t* expect = lm_gfx_screen(&sRefUi.gfx);
    const uint8_t* actual = sCppUi.canvas().screen();
    gChecks++;
    if (std::memcmp(expect, actual, static_cast<size_t>(lumen::kFbBytes)) == 0) return;
    int first = -1;
    for (int i = 0; i < lumen::kFbBytes; i++)
        if (expect[i] != actual[i]) {
            first = i;
            break;
        }
    std::printf("FAIL %-28s first diff byte %d (page %d, x %d, y %d)\n", what, first,
                first / lumen::kWidth, first % lumen::kWidth, (first / lumen::kWidth) * 8);
    gFailures++;
}

void step(float dt) {
    // upstream: what lm_ui_step() does around a page render
    sRefMenu.base.age += dt;
    if (sRefMenu.base.age > 3600.f) sRefMenu.base.age -= 3000.f;
    lm_menu_ops.update(&sRefMenu.base, dt);
    lm_gfx_target(&sRefUi.gfx, lm_gfx_screen(&sRefUi.gfx));
    lm_gfx_clear(&sRefUi.gfx);
    lm_menu_ops.render(&sRefMenu.base, &sRefUi.gfx);

    // C++ side
    sCppMenu.age += dt;
    if (sCppMenu.age > 3600.f) sCppMenu.age -= 3000.f;
    sCppMenu.update(dt);
    sCppUi.canvas().reset(sCppUi.canvas().screen());
    sCppUi.canvas().clear();
    sCppMenu.render(sCppUi.canvas());
}

void keys(lm_key_t key, lm_kind_t kind) {
    lm_event_t e = {static_cast<uint8_t>(key), static_cast<uint8_t>(kind)};
    lm_menu_ops.key(&sRefMenu.base, e);
    sCppMenu.key(lumen::Event{static_cast<lumen::Key>(key), static_cast<lumen::Kind>(kind)});
}

}  // namespace

// The reference menu calls these through lm_ui.c, which is not linked here.
extern "C" void lm_haptic(lm_ui_t*) {}
extern "C" void lm_push(lm_ui_t*, lm_page_t*, lm_tr_kind_t) {}

int main() {
    std::printf("menu parity: %dx%d\n", lumen::kWidth, lumen::kHeight);

    std::memset(&sRefUi, 0, sizeof(sRefUi));
    lm_gfx_init(&sRefUi.gfx);
    lm_settings_init(&sRefUi.settings, kDefaults, kSettingCount);
    sRefMenu.base.ui = &sRefUi;
    lm_menu_ops.enter(&sRefMenu.base);

    lumen::Port port;   // no millis/flush: we drive step() by hand
    sCppUi.init(port, kDefaults, kSettingCount);
    sCppMenu.ui = &sCppUi;
    sCppMenu.enter();

    step(1.f / 60.f);
    frame("enter");

    // settle the staggered entrance
    for (int i = 0; i < 90; i++) step(1.f / 60.f);
    frame("settled");

    // move down through every row and back
    for (int i = 0; i < 7; i++) {
        keys(LM_KEY_NEXT, LM_PRESS);
        for (int f = 0; f < 12; f++) step(1.f / 60.f);
        frame("next");
    }
    for (int i = 0; i < 3; i++) {
        keys(LM_KEY_PREV, LM_REPEAT);
        for (int f = 0; f < 12; f++) step(1.f / 60.f);
        frame("prev repeat");
    }

    // toggle + option activation
    keys(LM_KEY_OK, LM_PRESS);
    for (int f = 0; f < 40; f++) step(1.f / 60.f);
    frame("toggle");
    keys(LM_KEY_NEXT, LM_PRESS);
    keys(LM_KEY_OK, LM_PRESS);
    for (int f = 0; f < 40; f++) step(1.f / 60.f);
    frame("option cycle");

    // delete the page: settle to rest, then a long idle stretch
    for (int i = 0; i < 240; i++) step(1.f / 60.f);
    frame("idle");

    std::printf("%d checks, %d failures\n", gChecks, gFailures);
    return gFailures ? 1 : 0;
}
