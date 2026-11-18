// Settings tree: two declarative menus + the reset dialog and brightness sheet.
#include <cstdio>

#include "lumen/menu.hpp"
#include "lumen/overlay.hpp"
#include "lumen_demo/apps.hpp"

namespace demo {
namespace {

// ---- brightness sheet ---------------------------------------------------
void onBrightness(lumen::Ui& ui, int32_t value, void*) {
    ui.setSetting(lumen::kSettingBrightness, value);
}

void openBrightness(lumen::Menu& menu, int) {
    lumen::SheetDesc d;
    d.title = "Brightness";
    d.value = menu.ui->setting(lumen::kSettingBrightness);
    d.lo = 10;
    d.hi = 100;
    d.step = 10;
    d.onChange = onBrightness;
    lumen::sheet(*menu.ui, d);
}

void formatBrightness(lumen::Ui& ui, char* out, int n) {
    std::snprintf(out, static_cast<size_t>(n), "%d%%", static_cast<int>(ui.setting(lumen::kSettingBrightness)));
}

// ---- reset dialog -------------------------------------------------------
void resetDone(lumen::Ui& ui, int choice, void*) {
    if (choice != 1) return;
    for (int k = 0; k < kSettingCount; k++) ui.setSetting(k, kDefaults[k]);   // observers animate
    lumen::toast(ui, "Settings reset", lumen::ToastIcon::Check);
}

void openReset(lumen::Menu& menu, int) {
    float r[4];
    menu.focusRect(r);
    lumen::DialogDesc d;
    d.title = "Reset settings";
    d.message = "Restore defaults?";
    d.buttons[0] = "Cancel";
    d.buttons[1] = "Reset";
    d.defaultButton = 0;
    d.busyButton = 1;
    d.busySeconds = 1.1f;
    d.busyText = "Resetting";
    d.originX = r[0] + r[2] / 2;
    d.originY = r[1] + r[3] / 2;
    d.callback = resetDone;
    lumen::dialog(*menu.ui, d);
}

void wifiDone(lumen::Ui& ui, int32_t on) {
    if (on) lumen::toast(ui, "Connected", lumen::ToastIcon::Wifi);
}

// ---- tables (const: live in flash) --------------------------------------
const lumen::Option kSleepOptions[] = {{15, "15s"}, {30, "30s"}, {60, "60s"}, {0, "Never"}};
const lumen::Option kMotionOptions[] = {{100, "Normal"}, {50, "Slow"}, {150, "Fast"}};

const lumen::MenuItem kDisplayItems[] = {
    LUMEN_ITEM_TOGGLE("Invert", lumen::kSettingInvert),
    LUMEN_ITEM_OPTION("Auto sleep", lumen::kSettingAutosleep, kSleepOptions),
    LUMEN_ITEM_OPTION("Motion", lumen::kSettingMotion, kMotionOptions),
};
lumen::Menu sDisplay("Display", kDisplayItems);

const lumen::MenuItem kSettingsItems[] = {
    LUMEN_ITEM_SHEET("Brightness", openBrightness, formatBrightness),
    LUMEN_ITEM_TOGGLE("Sound", kSettingSound),
    LUMEN_ITEM_TOGGLE("Vibration", lumen::kSettingVibrate),
    LUMEN_ITEM_TOGGLE_ASYNC("Wi-Fi", kSettingWifi, 1100, wifiDone),
    LUMEN_ITEM_SUBMENU("Display", &sDisplay),
    LUMEN_ITEM_ACTION("Reset", openReset),
};
lumen::Menu sSettings("Settings", kSettingsItems);

}  // namespace

lumen::Page* settingsPage() { return &sSettings; }

}  // namespace demo
