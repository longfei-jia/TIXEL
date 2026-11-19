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
    d.title = tr(*menu.ui, "Brightness|亮度");
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
    tixel::toast(ui, tr(ui, "Settings reset|设置已重置"), tixel::ToastIcon::Check);
}

void openReset(tixel::Menu& menu, int) {
    float r[4];
    menu.focusRect(r);
    tixel::DialogDesc d;
    d.title = tr(*menu.ui, "Reset settings|重置设置", 0);
    d.message = tr(*menu.ui, "Restore defaults?|恢复默认？", 1);
    d.buttons[0] = tr(*menu.ui, "Cancel|取消", 2);
    d.buttons[1] = tr(*menu.ui, "Reset|重置", 3);
    d.defaultButton = 0;
    d.busyButton = 1;
    d.busySeconds = 1.1f;
    d.busyText = tr(*menu.ui, "Resetting|正在重置");
    d.originX = r[0] + r[2] / 2;
    d.originY = r[1] + r[3] / 2;
    d.callback = resetDone;
    tixel::dialog(*menu.ui, d);
}

void wifiDone(tixel::Ui& ui, int32_t on) {
    if (on) tixel::toast(ui, tr(ui, "Connected|已连接"), tixel::ToastIcon::Wifi);
}

// ---- tables (const: live in flash) --------------------------------------
const tixel::Option kSleepOptions[] = {
    {15, "15s|15秒"}, {30, "30s|30秒"}, {60, "60s|60秒"}, {0, "Never|永不"}};
const tixel::Option kMotionOptions[] = {{100, "Normal|正常"}, {50, "Slow|慢"}, {150, "Fast|快"}};
// Language names stay in their own script, which is the usual convention.
const tixel::Option kLangOptions[] = {{0, "中文"}, {1, "English"}};

const tixel::MenuItem kDisplayItems[] = {
    TIXEL_ITEM_TOGGLE("Invert|反色", tixel::kSettingInvert),
    TIXEL_ITEM_OPTION("Auto sleep|自动休眠", tixel::kSettingAutosleep, kSleepOptions),
    TIXEL_ITEM_OPTION("Motion|动效", tixel::kSettingMotion, kMotionOptions),
};
tixel::Menu sDisplay("Display", kDisplayItems);

const tixel::MenuItem kSettingsItems[] = {
    TIXEL_ITEM_SHEET("Brightness|亮度", openBrightness, formatBrightness),
    TIXEL_ITEM_TOGGLE("Sound|声音", kSettingSound),
    TIXEL_ITEM_TOGGLE("Vibration|振动", tixel::kSettingVibrate),
    TIXEL_ITEM_TOGGLE_ASYNC("Wi-Fi|无线", kSettingWifi, 1100, wifiDone),
    TIXEL_ITEM_SUBMENU("Display|显示", &sDisplay),
    TIXEL_ITEM_OPTION("Language|语言", tixel::kSettingLanguage, kLangOptions),
    TIXEL_ITEM_ACTION("Reset|重置", openReset),
};
tixel::Menu sSettings("Settings", kSettingsItems);

}  // namespace

tixel::Page* settingsPage() {
    sSettings.setTitleText("Settings|设置");
    sDisplay.setTitleText("Display|显示");
    return &sSettings;
}

}  // namespace tixel::apps
