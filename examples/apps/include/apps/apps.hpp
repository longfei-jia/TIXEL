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

// Localized text: write "English|中文", get the half for the current
// kSettingLanguage. Pass a distinct slot (0..3) for each string that must stay
// alive at the same time, e.g. a dialog's title, message and two buttons.
const char* tr(tixel::Ui& ui, const char* text, int slot = 0);

// UTF-8 helpers for reveal animations: counting codepoints (not bytes) keeps a
// typewriter effect from splitting a multi-byte glyph in half.
int utf8Count(const char* s);
int utf8BytesFor(const char* s, int codepoints);
extern const int32_t kDefaults[kSettingCount];

// page factories (each page is a static singleton)
tixel::Page* bootPage();
tixel::Page* homePage(bool intro);
tixel::Page* clockPage();
tixel::Page* musicPage();
tixel::Page* monitorPage();
tixel::Page* settingsPage();
tixel::Page* aboutPage();
tixel::Page* animPage();
tixel::Page* parallelPage();

void statusBar(tixel::Ui& ui, tixel::Canvas& g, float y);

// boot splash -> home
inline void start(tixel::Ui& ui) { ui.start(bootPage()); }

}  // namespace tixel::apps
