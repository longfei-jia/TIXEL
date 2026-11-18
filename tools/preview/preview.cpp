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

#include "lumen/config.hpp"
#include "lumen/theme.hpp"
#include "lumen/ui.hpp"
#include "lumen_demo/apps.hpp"

namespace {

uint32_t gNowMs = 0;
uint8_t gKeys = 0;
lumen::Ui gUi;
std::string gOutDir = "out";
int gShot = 0;

uint32_t portMillis(void*) { return gNowMs; }
uint8_t portReadKeys(void*) { return gKeys; }
void portHaptic(void*, uint16_t) {}
void portFlush(void*, const uint8_t*, lumen::PageMask) {}

void frames(int count) {
    for (int i = 0; i < count; i++) {
        gNowMs += 16;
        gUi.tick();
    }
}

void seconds(float s) { frames(static_cast<int>(s * 62.5f + 0.5f)); }

void press(lumen::Key key, int holdFrames = 3) {
    gKeys = lumen::keyBit(key);
    frames(holdFrames);
    gKeys = 0;
    frames(2);
}

// 3-key layout: a hold between 400 ms and 1200 ms maps to BACK on release.
void back() {
    press(lumen::Key::Ok, 32);
    frames(40);
}

void expect(const char* page) {
    lumen::Page* top = gUi.top();
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
    std::fprintf(f, "P5\n%d %d\n255\n", lumen::kWidth, lumen::kHeight);
    for (int y = 0; y < lumen::kHeight; y++)
        for (int x = 0; x < lumen::kWidth; x++)
            std::fputc(lumen::bufGet(fb, x, y) ? 255 : 0, f);
    std::fclose(f);
    std::printf("  %s\n", path);
}

}  // namespace

int main(int argc, char** argv) {
    if (argc > 1) gOutDir = argv[1];

    lumen::Port port;
    port.millis = portMillis;
    port.readKeys = portReadKeys;
    port.haptic = portHaptic;
    port.flush = portFlush;
    port.keyLayout = lumen::KeyLayout::Three;

    gUi.init(port, demo::kDefaults, demo::kSettingCount);
    demo::start(gUi);
    std::printf("lumen preview %dx%d, %d pages, %d B/frame\n", lumen::kWidth, lumen::kHeight,
                lumen::kPages, lumen::kFbBytes);
    {
        struct Role { const char* name; const lumen::Font* f; } roles[] = {
            {"ui", &lumen::theme::kUi}, {"tiny", &lumen::theme::kTiny}, {"mid", &lumen::theme::kMid},
            {"big", &lumen::theme::kBig}, {"logo", &lumen::theme::kLogo}};
        for (const Role& r : roles)
            std::printf("  font %-5s ascent %2d line %2d  LUMEN %3d  Brightness %3d  Wi-Fi %3d\n",
                        r.name, r.f->ascent(), r.f->lineHeight(), r.f->width("LUMEN"),
                        r.f->width("Brightness"), r.f->width("Wi-Fi"));
    }

    // boot splash
    seconds(0.55f);
    shot("boot_flash");
    seconds(0.75f);
    shot("boot_letters");
    seconds(0.9f);
    shot("boot_loader");
    seconds(2.4f);   // splash hands off to home
    frames(40);
    expect("Home");
    shot("home");

    // home -> clock
    press(lumen::Key::Next);
    frames(30);
    shot("home_next");
    press(lumen::Key::Prev);
    frames(30);
    press(lumen::Key::Ok);
    frames(45);
    expect("Clock");
    shot("clock");
    press(lumen::Key::Next);
    frames(40);
    shot("clock_stopwatch");
    press(lumen::Key::Ok);
    seconds(1.0f);
    shot("clock_running");
    back();
    expect("Home");

    // music
    press(lumen::Key::Next);
    frames(30);
    press(lumen::Key::Ok);
    frames(45);
    expect("Music");
    shot("music");
    press(lumen::Key::Ok);   // play
    seconds(1.4f);
    shot("music_playing");
    back();

    // monitor
    press(lumen::Key::Next);
    frames(30);
    press(lumen::Key::Ok);
    frames(45);
    expect("Monitor");
    shot("monitor");
    press(lumen::Key::Next);   // RAM tab
    frames(40);
    shot("monitor_ram");
    press(lumen::Key::Ok);     // pause badge
    frames(40);
    shot("monitor_paused");
    press(lumen::Key::Ok);
    frames(20);
    back();

    // settings
    press(lumen::Key::Next);
    frames(30);
    press(lumen::Key::Ok);
    frames(45);
    expect("Settings");
    shot("settings");
    press(lumen::Key::Ok);   // brightness sheet
    frames(45);
    shot("sheet");
    press(lumen::Key::Next);
    press(lumen::Key::Next);
    press(lumen::Key::Next);
    frames(45);
    shot("sheet_changed");
    press(lumen::Key::Ok);   // close
    frames(30);
    press(lumen::Key::Next);   // Sound
    press(lumen::Key::Next);   // Vibration
    press(lumen::Key::Next);   // Wi-Fi
    press(lumen::Key::Ok);     // async busy phase
    frames(20);
    shot("wifi_busy");
    seconds(1.4f);
    shot("toast");
    press(lumen::Key::Next);   // Display submenu
    press(lumen::Key::Ok);
    frames(45);
    expect("Display");
    shot("display_menu");
    press(lumen::Key::Ok);     // invert toggle
    frames(45);
    shot("display_inverted");
    press(lumen::Key::Ok);
    frames(45);
    back();                    // -> settings
    expect("Settings");
    press(lumen::Key::Next);   // Reset
    press(lumen::Key::Ok);
    frames(45);
    shot("dialog");
    press(lumen::Key::Next);
    frames(20);
    shot("dialog_reset");
    press(lumen::Key::Ok);
    frames(30);
    shot("dialog_busy");
    seconds(1.6f);
    shot("toast_reset");
    back();   // -> home
    expect("Home");

    // about
    press(lumen::Key::Next);
    frames(30);
    press(lumen::Key::Ok);
    frames(60);
    expect("About");
    shot("about");
    press(lumen::Key::Next);
    frames(30);
    shot("about_scrolled");

    std::printf("fps ~%.1f, frames %u, depth %u\n", gUi.fps(), gUi.frames(), gUi.depth());
    return 0;
}
