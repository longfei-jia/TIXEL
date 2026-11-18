// ESP32-S3 demo: runs the Lumen sample on a display sink you provide.
//
// The example ships without a panel driver on purpose. Wire yours up in
// app_main() by passing a sink to Board::begin(); it receives the frame buffer
// and the dirty-page mask, in SSD1306 page format (byte = 8 vertical pixels,
// LSB on top, index = page * 128 + x).
#include <cstdio>

#include "board.hpp"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

namespace {

constexpr const char* kTag = "lumen";

// A sink that does nothing but count frames: enough to see the UI running in
// the log. Replace it with your panel's page writer.
struct NullPanel {
    uint32_t frames = 0;
    uint32_t pages = 0;
};

NullPanel gPanel;

void nullSink(void* ctx, const uint8_t* /*fb*/, uint16_t pages) {
    auto* p = static_cast<NullPanel*>(ctx);
    p->frames++;
    p->pages += static_cast<uint32_t>(__builtin_popcount(pages));
}

}  // namespace

extern "C" void app_main() {
    static demo_port::Board board;
    board.begin(nullSink, &gPanel);

    ESP_LOGI(kTag, "Lumen %dx%d, %d pages, %d B/frame", lumen::kWidth, lumen::kHeight, lumen::kPages,
             lumen::kFbBytes);

    int64_t last = esp_timer_get_time();
    uint32_t lastFrames = 0;
    while (true) {
        board.tick();                        // one UI frame
        vTaskDelay(pdMS_TO_TICKS(10));       // ~100 fps ceiling, like upstream

        const int64_t now = esp_timer_get_time();
        if (now - last >= 5000000) {
            const float fps = static_cast<float>(gPanel.frames - lastFrames) * 1e6f /
                              static_cast<float>(now - last);
            ESP_LOGI(kTag, "fps %.1f  flush frames %u  pages %u  heap %u", fps,
                     static_cast<unsigned>(gPanel.frames), static_cast<unsigned>(gPanel.pages),
                     static_cast<unsigned>(esp_get_free_heap_size()));
            last = now;
            lastFrames = gPanel.frames;
        }
    }
}
