#include "tixel/input.hpp"

namespace tixel {

bool EventQueue::push(Event e) {
    const uint8_t h = head_;
    const uint8_t next = static_cast<uint8_t>((h + 1) & (kEvqSize - 1));
    if (next == tail_) return false;
    buf_[h] = e;
    head_ = next;   // publish after the payload is written
    return true;
}

bool EventQueue::pop(Event* e) {
    const uint8_t t = tail_;
    if (t == head_) return false;
    *e = buf_[t];
    tail_ = static_cast<uint8_t>((t + 1) & (kEvqSize - 1));
    return true;
}

namespace {

void emit(EventQueue& q, Key key, Kind kind) { q.push(Event{key, kind}); }

bool isNav(Key k) { return k == Key::Prev || k == Key::Next; }

}  // namespace

void Keys::init(KeyLayout layout) {
    stable_ = rawLast_ = 0;
    rawSince_ = 0;
    for (int i = 0; i < kKeyCount; i++) downAt_[i] = nextRepeat_[i] = 0;
    longFired_ = 0;
    layout_ = layout;
}

// 3-key ENTER: one physical key, three logical gestures by hold length
void Keys::enter3(bool was, bool is, uint32_t now, EventQueue& queue) {
    const int i = static_cast<int>(Key::Ok);
    const uint8_t bit = static_cast<uint8_t>(keyBit(Key::Ok));
    if (is && !was) {
        downAt_[i] = now;
        longFired_ &= static_cast<uint8_t>(~bit);
    } else if (is && was) {
        if (!(longFired_ & bit) && (now - downAt_[i]) >= kKey3HomeMs) {
            longFired_ |= bit;
            emit(queue, Key::Back, Kind::Long);   // fires while held
        }
    } else if (!is && was) {
        if (longFired_ & bit) return;             // already resolved
        const uint32_t held = now - downAt_[i];
        emit(queue, held >= kKey3BackMs ? Key::Back : Key::Ok, Kind::Press);
    }
}

void Keys::scan(uint8_t raw, uint32_t now, EventQueue& queue) {
    if (layout_ == KeyLayout::Three)
        raw &= static_cast<uint8_t>(keyBit(kKeyUp) | keyBit(kKeyDown) | keyBit(kKeyEnter));

    // debounce: a raw change must hold for kKeyDebounceMs
    if (raw != rawLast_) {
        rawLast_ = raw;
        rawSince_ = now;
    }
    const uint8_t prev = stable_;
    if (raw != prev && (now - rawSince_) >= kKeyDebounceMs) stable_ = raw;
    const uint8_t cur = stable_;

    for (int i = 0; i < kKeyCount; i++) {
        const Key key = static_cast<Key>(i);
        const uint8_t bit = static_cast<uint8_t>(1u << i);
        const bool was = (prev & bit) != 0, is = (cur & bit) != 0;

        if (layout_ == KeyLayout::Three && key == Key::Ok) {
            enter3(was, is, now, queue);
            continue;
        }

        if (is && !was) {   // key down
            downAt_[i] = now;
            longFired_ &= static_cast<uint8_t>(~bit);
            if (isNav(key)) {
                emit(queue, key, Kind::Press);
                nextRepeat_[i] = now + kKeyRepeatMs;
            }
        } else if (is && was) {   // held
            const uint32_t held = now - downAt_[i];
            if (isNav(key)) {
                if (static_cast<int32_t>(now - nextRepeat_[i]) >= 0) {
                    emit(queue, key, Kind::Repeat);
                    // accelerate after 1.5 s of holding
                    const uint32_t rate = held > 1500 ? kKeyRepeatRateMs / 2 : kKeyRepeatRateMs;
                    nextRepeat_[i] = now + rate;
                }
            } else if (!(longFired_ & bit) && held >= kKeyLongMs) {
                longFired_ |= bit;
                emit(queue, key, Kind::Long);
            }
        } else if (!is && was) {   // key up
            if (!isNav(key) && !(longFired_ & bit)) emit(queue, key, Kind::Press);
        }
    }
}

float Keys::hold(Key key, uint32_t now) const {
    const uint8_t bit = keyBit(key);
    if (!(stable_ & bit) || (longFired_ & bit)) return 0.f;
    if (isNav(key)) return 0.f;
    const uint32_t full = (layout_ == KeyLayout::Three && key == Key::Ok) ? kKey3HomeMs : kKeyLongMs;
    const float p = static_cast<float>(now - downAt_[static_cast<int>(key)]) / static_cast<float>(full);
    return p > 1.f ? 1.f : p;
}

float Keys::holdMark(Key key) const {
    if (layout_ == KeyLayout::Three && key == Key::Ok)
        return static_cast<float>(kKey3BackMs) / static_cast<float>(kKey3HomeMs);
    return 0.f;
}

}  // namespace tixel
