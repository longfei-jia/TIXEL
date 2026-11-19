// Host preview / capture tool.
//
// Drives the demo UI with a virtual clock and virtual keys, then writes the
// frame buffer as PGM images. No panel, no RTOS: this is the fastest way to
// look at a layout change, and it doubles as a smoke test for the component.
//
//   preview <out-dir> [scenario]
//
// Pipelines with tools/pgm2png.py for viewing.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "tixel/config.hpp"
#include "tixel/theme.hpp"
#include "tixel/ui.hpp"
#include "apps/apps.hpp"

namespace {

uint32_t gNowMs = 0;
uint8_t gKeys = 0;
tixel::Ui gUi;
std::string gOutDir = "out";
int gShot = 0;

uint32_t portMillis(void*) { return gNowMs; }
uint8_t portReadKeys(void*) { return gKeys; }
void portHaptic(void*, uint16_t) {}
void portFlush(void*, const uint8_t*, tixel::PageMask) {}

void frames(int count) {
    for (int i = 0; i < count; i++) {
        gNowMs += 16;
        gUi.tick();
    }
}

void seconds(float s) { frames(static_cast<int>(s * 62.5f + 0.5f)); }

void press(tixel::Key key, int holdFrames = 3) {
    gKeys = tixel::keyBit(key);
    frames(holdFrames);
    gKeys = 0;
    frames(2);
}

// 3-key layout: a hold between 400 ms and 1200 ms maps to BACK on release.
void back() {
    press(tixel::Key::Ok, 32);
    frames(40);
}

void expect(const char* page) {
    tixel::Page* top = gUi.top();
    const char* title = (top && top->title) ? top->title : "(none)";
    if (std::strcmp(title, page) != 0) std::printf("  !! expected page %s, got %s\n", page, title);
}

void shot(const char* name) {
    const uint8_t* fb = gUi.canvas().screen();
    char path[512];
    std::snprintf(path, sizeof(path), "%s/%02d_%s.pgm", gOutDir.c_str(), gShot++, name);
    FILE* f = std::fopen(path, "wb");
    if (!f) {
        std::perror(path);
        return;
    }
    std::fprintf(f, "P5\n%d %d\n255\n", tixel::kWidth, tixel::kHeight);
    for (int y = 0; y < tixel::kHeight; y++)
        for (int x = 0; x < tixel::kWidth; x++)
            std::fputc(tixel::bufGet(fb, x, y) ? 255 : 0, f);
    std::fclose(f);
    std::printf("  %s\n", path);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 1) gOutDir = argv[1];

    tixel::Port port;
    port.millis = portMillis;
    port.readKeys = portReadKeys;
    port.haptic = portHaptic;
    port.flush = portFlush;
    port.keyLayout = tixel::KeyLayout::Three;

    gUi.init(port, tixel::apps::kDefaults, tixel::apps::kSettingCount);
    tixel::apps::start(gUi);
    std::printf("tixel preview %dx%d, %d pages, %d B/frame\n", tixel::kWidth, tixel::kHeight,
                tixel::kPages, tixel::kFbBytes);
    {
        struct Role { const char* name; const tixel::Font* f; } roles[] = {
            {"ui", &tixel::theme::kUi}, {"tiny", &tixel::theme::kTiny}, {"mid", &tixel::theme::kMid},
            {"big", &tixel::theme::kBig}, {"logo", &tixel::theme::kLogo}};
        for (const Role& r : roles)
            std::printf("  font %-5s ascent %2d line %2d  TIXEL %3d  Brightness %3d  Wi-Fi %3d\n",
                        r.name, r.f->ascent(), r.f->lineHeight(), r.f->width("TIXEL"),
                        r.f->width("Brightness"), r.f->width("Wi-Fi"));
    }

    // boot splash
    seconds(0.55f);
    shot("boot_flash");
    seconds(0.75f);
    shot("boot_letters");
    seconds(0.9f);
    shot("boot_loader");
    seconds(0.35f);  // subtitle finishes typing
    shot("boot_sub");
    seconds(0.55f);  // progress bar fills
    shot("boot_bar");
    seconds(1.50f);  // splash hands off to home
    frames(40);
    expect("Home");
    shot("home");

    // home -> clock
    press(tixel::Key::Next);
    frames(30);
    shot("home_next");
    press(tixel::Key::Prev);
    frames(30);
    press(tixel::Key::Ok);
    frames(45);
    expect("Clock");
    shot("clock");
    press(tixel::Key::Next);
    frames(40);
    shot("clock_stopwatch");
    press(tixel::Key::Ok);
    seconds(1.0f);
    shot("clock_running");
    back();
    expect("Home");

    // music
    press(tixel::Key::Next);
    frames(30);
    press(tixel::Key::Ok);
    frames(45);
    expect("Music");
    shot("music");
    press(tixel::Key::Ok);   // play
    seconds(1.4f);
    shot("music_playing");
    back();

    // monitor
    press(tixel::Key::Next);
    frames(30);
    press(tixel::Key::Ok);
    frames(45);
    expect("Monitor");
    shot("monitor");
    press(tixel::Key::Next);   // RAM tab
    frames(40);
    shot("monitor_ram");
    press(tixel::Key::Ok);     // pause badge
    frames(40);
    shot("monitor_paused");
    press(tixel::Key::Ok);
    frames(20);
    back();

    // settings
    press(tixel::Key::Next);
    frames(30);
    press(tixel::Key::Ok);
    frames(45);
    expect("Settings");
    shot("settings");
    press(tixel::Key::Ok);   // brightness sheet
    frames(45);
    shot("sheet");
    press(tixel::Key::Next);
    press(tixel::Key::Next);
    press(tixel::Key::Next);
    frames(45);
    shot("sheet_changed");
    press(tixel::Key::Ok);   // close
    frames(30);
    press(tixel::Key::Next);   // Sound
    press(tixel::Key::Next);   // Vibration
    press(tixel::Key::Next);   // Wi-Fi
    press(tixel::Key::Ok);     // async busy phase
    frames(20);
    shot("wifi_busy");
    seconds(1.4f);
    shot("toast");
    press(tixel::Key::Next);   // Display submenu
    press(tixel::Key::Ok);
    frames(45);
    expect("Display");
    shot("display_menu");
    press(tixel::Key::Ok);     // invert toggle
    frames(45);
    shot("display_inverted");
    press(tixel::Key::Ok);
    frames(45);
    back();                    // -> settings
    expect("Settings");

    // language switch: the option cycles 中文 -> English -> 中文
    press(tixel::Key::Next);   // Language
    frames(30);
    shot("settings_lang");
    press(tixel::Key::Ok);
    frames(45);
    shot("settings_en");
    press(tixel::Key::Ok);
    frames(45);
    shot("settings_zh");

    press(tixel::Key::Next);   // Reset
    press(tixel::Key::Ok);
    frames(45);
    shot("dialog");
    press(tixel::Key::Next);
    frames(20);
    shot("dialog_reset");
    press(tixel::Key::Ok);
    frames(30);
    shot("dialog_busy");
    seconds(1.6f);
    shot("toast_reset");
    back();   // -> home
    frames(5);
    shot("home_return");   // mid-transition
    frames(25);
    expect("Home");

    // about
    press(tixel::Key::Next);
    frames(30);
    press(tixel::Key::Ok);
    frames(60);
    expect("About");
    shot("about");
    press(tixel::Key::Next);
    frames(30);
    shot("about_scrolled");

    std::printf("fps ~%.1f, frames %u, depth %u\n", gUi.fps(), gUi.frames(), gUi.depth());
    return 0;
}
