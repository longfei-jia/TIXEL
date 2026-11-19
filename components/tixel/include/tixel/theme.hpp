// TIXEL C++ - font roles.
//
// Widgets refer to roles, not concrete fonts, so a product can restyle (or
// drop Flash-heavy fonts) in one place. The 128x128 set keeps the upstream
// ratios (tiny < body < value < clock) at the sizes that fit a doubled panel;
// the 128x64 set is the upstream one, which is what the parity tests run.
#pragma once

#include "tixel/config.hpp"
#include "tixel/font.hpp"
#include "tixel/fonts.hpp"

namespace tixel {
namespace theme {

#if TIXEL_PANEL_HEIGHT >= 96
inline const Font kUi{fonts::font_ui};       // 7x13   body text
inline const Font kTiny{fonts::font_tiny};   // 6x12   status bar, labels
inline const Font kMid{fonts::font_mid};     // logisoso 28  values
inline const Font kBig{fonts::font_big};     // logisoso 42  clock digits (digits only)
inline const Font kLogo{fonts::font_logo};   // logisoso 42  wordmark
#else
inline const Font kUi{fonts::font_ui_64};       // haxrcorp4089
inline const Font kTiny{fonts::font_tiny_64};   // 4x6
inline const Font kMid{fonts::font_mid_64};     // logisoso16
inline const Font kBig{fonts::font_big_64};     // logisoso28_tn
inline const Font kLogo{fonts::font_logo_64};   // logisoso22
#endif

}  // namespace theme
}  // namespace tixel
