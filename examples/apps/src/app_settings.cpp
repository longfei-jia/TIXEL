// Settings tree: two declarative menus + the reset dialog and brightness sheet.
#include <cstdio>

#include "tixel/menu.hpp"
#include "tixel/overlay.hpp"
#include "apps/apps.hpp"

namespace tixel::apps {
namespace {

// ---- brightness sheet ---------------------------------------------------
void onBrightness(tixel::Ui& ui, int32_t value, void*) {
    ui.setSetting(tixel::kSettingBrightness, value);
}

void openBrightness(tixel::Menu& menu, int) {
    tixel::SheetDesc d;
    d.title = "Brightness";
    d.value = menu.ui->setting(tixel::kSettingBrightness);
    d.lo = 10;
    d.hi = 100;
    d.step = 10;
    d.onChange = onBrightness;
    tixel::sheet(*menu.ui, d);
}

void formatBrightness(tixel::Ui& ui, char* out, int n) {
    std::snprintf(out, static_cast<size_t>(n), "%d%%", static_cast<int>(ui.setting(tixel::kSettingBrightness)));
}

// ---- reset dialog -------------------------------------------------------
void resetDone(tixel::Ui& ui, int choice, void*) {
    if (choice != 1) return;
    for (int k = 0; k < kSettingCount; k++) ui.setSetting(k, kDefaults[k]);   // observers animate
    tixel::toast(ui, "Settings reset", tixel::ToastIcon::Check);
}

void openReset(tixel::Menu& menu, int) {
    float r[4];
    menu.focusRect(r);
    tixel::DialogDesc d;
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
    tixel::dialog(*menu.ui, d);
}

void wifiDone(tixel::Ui& ui, int32_t on) {
    if (on) tixel::toast(ui, "Connected", tixel::ToastIcon::Wifi);
}

// ---- tables (const: live in flash) --------------------------------------
const tixel::Option kSleepOptions[] = {{15, "15s"}, {30, "30s"}, {60, "60s"}, {0, "Never"}};
const tixel::Option kMotionOptions[] = {{100, "Normal"}, {50, "Slow"}, {150, "Fast"}};

const tixel::MenuItem kDisplayItems[] = {
    TIXEL_ITEM_TOGGLE("Invert", tixel::kSettingInvert),
    TIXEL_ITEM_OPTION("Auto sleep", tixel::kSettingAutosleep, kSleepOptions),
    TIXEL_ITEM_OPTION("Motion", tixel::kSettingMotion, kMotionOptions),
};
tixel::Menu sDisplay("Display", kDisplayItems);

const tixel::MenuItem kSettingsItems[] = {
    TIXEL_ITEM_SHEET("Brightness", openBrightness, formatBrightness),
    TIXEL_ITEM_TOGGLE("Sound", kSettingSound),
    TIXEL_ITEM_TOGGLE("Vibration", tixel::kSettingVibrate),
    TIXEL_ITEM_TOGGLE_ASYNC("Wi-Fi", kSettingWifi, 1100, wifiDone),
    TIXEL_ITEM_SUBMENU("Display", &sDisplay),
    TIXEL_ITEM_ACTION("Reset", openReset),
};
tixel::Menu sSettings("Settings", kSettingsItems);

}  // namespace

tixel::Page* settingsPage() { return &sSettings; }

}  // namespace tixel::apps
