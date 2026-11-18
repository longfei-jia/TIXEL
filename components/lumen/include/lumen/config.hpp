// Lumen C++ - compile-time configuration.
//
// Everything is constexpr: the rasteriser and the layout read the same values,
// and the linker can fold them. Panel geometry is overridable so the same
// sources build for the 128x64 reference (used by the parity tests) and for
// the 128x128 target.
#pragma once

#include <cstdint>

#ifndef LUMEN_PANEL_WIDTH
#define LUMEN_PANEL_WIDTH 128
#endif
#ifndef LUMEN_PANEL_HEIGHT
#define LUMEN_PANEL_HEIGHT 128
#endif

namespace lumen {

// ---- panel -------------------------------------------------------------
inline constexpr int kWidth = LUMEN_PANEL_WIDTH;
inline constexpr int kHeight = LUMEN_PANEL_HEIGHT;
inline constexpr int kPages = kHeight / 8;      // page format: 8 rows per byte
inline constexpr int kFbBytes = kWidth * kPages;

static_assert(kWidth % 8 == 0, "page format needs a width that is a multiple of 8");
static_assert(kHeight % 8 == 0, "page format needs a height that is a multiple of 8");
static_assert(kPages <= 16, "the dirty-page mask is 16 bits");

// Layout scale against the upstream 128x64 reference. The demo pages multiply
// their reference numbers by this, so at 64 they reproduce upstream exactly.
inline constexpr int kScale = (kHeight >= 64) ? kHeight / 64 : 1;

// n reference pixels -> panel pixels (integer, compile time)
constexpr int scaled(int n) { return n * kScale; }
constexpr float scaledf(float n) { return n * static_cast<float>(kScale); }

// ---- static containers -------------------------------------------------
inline constexpr int kMaxStack = 6;        // page stack depth
inline constexpr int kMaxOverlays = 4;     // toast + dialog/sheet + headroom
inline constexpr int kEvqSize = 16;        // power of two
inline constexpr int kMaxSubscribers = 8;
inline constexpr int kMaxSettings = 12;
inline constexpr int kMenuMaxItems = 10;
inline constexpr int kRollMax = 12;        // chars per rolling-text widget

// ---- input timing (ms) -------------------------------------------------
inline constexpr uint32_t kKeyDebounceMs = 12;
inline constexpr uint32_t kKeyLongMs = 600;
inline constexpr uint32_t kKeyRepeatMs = 420;
inline constexpr uint32_t kKeyRepeatRateMs = 110;
inline constexpr uint32_t kKey3BackMs = 400;   // 3-key ENTER: release after this = BACK
inline constexpr uint32_t kKey3HomeMs = 1200;  // reach this: BACK LONG (home / sleep)

// ---- motion ------------------------------------------------------------
inline constexpr int kSpringHz = 240;      // integrator sub-step rate
inline constexpr float kMaxDt = 0.05f;     // clamp frame dt (s)

// Dirty page mask handed to the port.
using PageMask = uint16_t;
inline constexpr PageMask kAllPages =
    static_cast<PageMask>(kPages >= 16 ? 0xFFFFu : ((1u << kPages) - 1u));

}  // namespace lumen
