// Lumen demo application.
//
// The upstream LUMEN sample, ported to the C++ component: boot splash, icon
// carousel, clock, music, system monitor, settings menus and about page. No
// platform code lives here; a port supplies time, keys and a display sink.
#pragma once

#include <cstdint>

#include "lumen/canvas.hpp"
#include "lumen/page.hpp"
#include "lumen/ui.hpp"

namespace demo {

// app-level settings extend the framework ids
enum : int { kSettingSound = lumen::kSettingUser, kSettingWifi, kSettingCount };
extern const int32_t kDefaults[kSettingCount];

// page factories (each page is a static singleton)
lumen::Page* bootPage();
lumen::Page* homePage(bool intro);
lumen::Page* clockPage();
lumen::Page* musicPage();
lumen::Page* monitorPage();
lumen::Page* settingsPage();
lumen::Page* aboutPage();

void statusBar(lumen::Ui& ui, lumen::Canvas& g, float y);

// boot splash -> home
inline void start(lumen::Ui& ui) { ui.start(bootPage()); }

}  // namespace demo
