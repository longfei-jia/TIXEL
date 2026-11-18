// Lumen C++ - settings store with observers.
//
// The store owns values; views read them, and anything that must react to a
// change (panel contrast, invert wipe, persistence, an app's own state)
// subscribes instead of being called from the widget that changed it.
// Framework ids come first; apps extend with ids from kSettingUser upwards.
#pragma once

#include <cstdint>

#include "lumen/config.hpp"

namespace lumen {

enum SettingId : int {
    kSettingBrightness = 0,   // 10..100 %
    kSettingInvert,           // 0/1
    kSettingAutosleep,        // seconds, 0 = never
    kSettingMotion,           // animation speed, % (100 = normal)
    kSettingVibrate,          // 0/1
    kSettingUser              // first app-defined id
};

using SettingCallback = void (*)(void* ctx, int id, int32_t oldValue, int32_t newValue);

class Settings {
public:
    void init(const int32_t* defaults, int count);

    int32_t get(int id) const {
        return (id >= 0 && id < kMaxSettings) ? v[id] : 0;
    }
    void set(int id, int32_t value);   // notifies on change
    int subscribe(SettingCallback cb, void* ctx);

    int32_t v[kMaxSettings] = {};
    uint8_t dirty = 0;   // set on change; the UI flushes to storage

private:
    struct Sub {
        SettingCallback cb = nullptr;
        void* ctx = nullptr;
    };
    Sub sub_[kMaxSubscribers] = {};
    uint8_t nsub_ = 0;
};

}  // namespace lumen
