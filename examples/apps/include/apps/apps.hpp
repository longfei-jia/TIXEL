// TIXEL sample apps.
//
// The seven pages that demo the framework, ported from the upstream reference
// sample: boot splash, icon carousel, clock, music, system monitor, settings
// menus and about page. No platform code lives here; a port supplies time,
// keys and a display sink.
#pragma once

#include <cstdint>

#include "tixel/canvas.hpp"
#include "tixel/page.hpp"
#include "tixel/ui.hpp"

namespace tixel::apps {

// app-level settings extend the framework ids
enum : int { kSettingSound = tixel::kSettingUser, kSettingWifi, kSettingCount };
extern const int32_t kDefaults[kSettingCount];

// page factories (each page is a static singleton)
tixel::Page* bootPage();
tixel::Page* homePage(bool intro);
tixel::Page* clockPage();
tixel::Page* musicPage();
tixel::Page* monitorPage();
tixel::Page* settingsPage();
tixel::Page* aboutPage();

void statusBar(tixel::Ui& ui, tixel::Canvas& g, float y);

// boot splash -> home
inline void start(tixel::Ui& ui) { ui.start(bootPage()); }

}  // namespace tixel::apps
