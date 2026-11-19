// TIXEL C++ - hardware port (dependency inversion).
//
// The core never includes a platform header and never talks to a panel driver.
// A port supplies time / keys / optional haptics, RTC and storage, plus the
// display sink: pages of the frame buffer in SSD1306 page format.
#pragma once

#include <cstdint>

#include "tixel/config.hpp"
#include "tixel/input.hpp"

namespace tixel {

struct DateTime {
    uint16_t year = 0;
    uint8_t month = 0, day = 0, wday = 0;   // wday: 0 = Monday
    uint8_t hour = 0, min = 0, sec = 0;
    uint16_t ms = 0;
};

struct Port {
    void* ctx = nullptr;

    uint32_t (*millis)(void* ctx) = nullptr;              // monotonic ms
    uint8_t (*readKeys)(void* ctx) = nullptr;             // key mask, or nullptr
    void (*haptic)(void* ctx, uint16_t ms) = nullptr;     // optional
    bool (*rtcGet)(void* ctx, DateTime* dt) = nullptr;    // optional
    bool (*settingsLoad)(void* ctx, int32_t* v, int n) = nullptr;     // optional
    void (*settingsSave)(void* ctx, const int32_t* v, int n) = nullptr;  // optional

    // Send pages (bit p = page p) of fb (kFbBytes, page format) to the panel.
    // May return before the transfer ends, so it must copy what it needs; fb
    // is overwritten by the next frame. nullptr: nothing is shown.
    void (*flush)(void* ctx, const uint8_t* fb, PageMask pages) = nullptr;
    void (*setContrast)(void* ctx, uint8_t level) = nullptr;   // optional, 0..255
    void (*setPower)(void* ctx, bool on) = nullptr;            // optional

    KeyLayout keyLayout = KeyLayout::Four;
};

}  // namespace tixel
