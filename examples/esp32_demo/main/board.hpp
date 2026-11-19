// ESP32 demo board port.
//
// Everything platform specific for the example lives here: monotonic time,
// three buttons, and a display sink. The component is panel-agnostic - a board
// plugs its own panel driver in as the sink and receives page-format frames.
#pragma once

#include <cstdint>

#include "tixel/port.hpp"
#include "tixel/ui.hpp"

namespace demo_port {

// Pages are dirty-page masked (bit p = page p), buffer layout = SSD1306 pages.
using PanelSink = void (*)(void* ctx, const uint8_t* fb, uint16_t pages);

// GPIOs of the three keys (active low, internal pull-ups). Override with -D.
#ifndef TIXEL_KEY_UP_GPIO
#define TIXEL_KEY_UP_GPIO 4
#endif
#ifndef TIXEL_KEY_ENTER_GPIO
#define TIXEL_KEY_ENTER_GPIO 5
#endif
#ifndef TIXEL_KEY_DOWN_GPIO
#define TIXEL_KEY_DOWN_GPIO 6
#endif

class Board {
public:
    void begin(PanelSink sink = nullptr, void* ctx = nullptr);
    void tick();   // one UI frame; call from your task
    tixel::Ui& ui() { return ui_; }

private:
    tixel::Ui ui_;
};

}  // namespace demo_port
