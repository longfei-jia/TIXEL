#include "board.hpp"

#include "driver/gpio.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lumen_demo/apps.hpp"

namespace demo_port {
namespace {

PanelSink sSink = nullptr;
void* sSinkCtx = nullptr;

uint32_t boardMillis(void*) { return static_cast<uint32_t>(esp_timer_get_time() / 1000); }

uint8_t boardKeys(void*) {
    // Active low: 1 in the mask means "pressed". Lumen debounces internally.
    uint8_t mask = 0;
    if (gpio_get_level(static_cast<gpio_num_t>(LUMEN_KEY_UP_GPIO)) == 0)
        mask |= lumen::keyBit(lumen::kKeyUp);
    if (gpio_get_level(static_cast<gpio_num_t>(LUMEN_KEY_DOWN_GPIO)) == 0)
        mask |= lumen::keyBit(lumen::kKeyDown);
    if (gpio_get_level(static_cast<gpio_num_t>(LUMEN_KEY_ENTER_GPIO)) == 0)
        mask |= lumen::keyBit(lumen::kKeyEnter);
    return mask;
}

void boardFlush(void*, const uint8_t* fb, lumen::PageMask pages) {
    // The core hands over the frame buffer and a dirty-page mask; a real port
    // sends only those pages (I2C/SPI, DMA, Sharp memory LCD line format, ...).
    if (sSink) sSink(sSinkCtx, fb, pages);
}

// No persistence in the example: the UI keeps its defaults across boots.
bool boardLoad(void*, int32_t*, int) { return false; }
void boardSave(void*, const int32_t*, int) {}

void initKeys() {
    gpio_config_t cfg = {};
    cfg.pin_bit_mask = (1ull << LUMEN_KEY_UP_GPIO) | (1ull << LUMEN_KEY_ENTER_GPIO) |
                       (1ull << LUMEN_KEY_DOWN_GPIO);
    cfg.mode = GPIO_MODE_INPUT;
    cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&cfg);
}

}  // namespace

void Board::begin(PanelSink sink, void* ctx) {
    sSink = sink;
    sSinkCtx = ctx;
    initKeys();

    lumen::Port port;
    port.millis = boardMillis;
    port.readKeys = boardKeys;
    port.flush = boardFlush;
    port.settingsLoad = boardLoad;
    port.settingsSave = boardSave;
    port.keyLayout = lumen::KeyLayout::Three;   // UP / DOWN / ENTER

    ui_.init(port, demo::kDefaults, demo::kSettingCount);
    demo::start(ui_);
}

void Board::tick() { ui_.tick(); }

}  // namespace demo_port
