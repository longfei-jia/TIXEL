#include "tixel/settings.hpp"

namespace tixel {

void Settings::init(const int32_t* defaults, int count) {
    for (int i = 0; i < kMaxSettings; i++) v[i] = 0;
    for (int i = 0; i < nsub_; i++) sub_[i] = Sub{};
    nsub_ = 0;
    dirty = 0;
    if (count > kMaxSettings) count = kMaxSettings;
    for (int i = 0; i < count; i++) v[i] = defaults[i];
}

void Settings::set(int id, int32_t value) {
    if (id < 0 || id >= kMaxSettings || v[id] == value) return;
    const int32_t old = v[id];
    v[id] = value;
    dirty = 1;
    for (int i = 0; i < nsub_; i++) sub_[i].cb(sub_[i].ctx, id, old, value);
}

int Settings::subscribe(SettingCallback cb, void* ctx) {
    if (nsub_ >= kMaxSubscribers) return -1;
    sub_[nsub_] = Sub{cb, ctx};
    return nsub_++;
}

}  // namespace tixel
