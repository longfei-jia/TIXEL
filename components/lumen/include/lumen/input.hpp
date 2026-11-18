// Lumen C++ - input: raw key mask -> debounced key FSM -> gesture events.
//
// Layout Four (default)
//   PREV/NEXT  PRESS on key-down (instant), then REPEAT while held (accelerates)
//   OK/BACK    PRESS on release if short, LONG once held >= kKeyLongMs
//
// Layout Three (UP / DOWN / ENTER, no BACK key)
//   UP/DOWN    same as PREV/NEXT
//   ENTER      resolved on release by how long it was held:
//                < kKey3BackMs              OK    PRESS
//                kKey3BackMs..kKey3HomeMs   BACK  PRESS  (one level up)
//                >= kKey3HomeMs (fires while held) BACK LONG (home / sleep)
//   Resolving on release keeps the stages exclusive.
//
// Pages always see the same four logical keys, so app code is layout
// agnostic; only the gesture mapping differs.
#pragma once

#include <cstdint>

#include "lumen/config.hpp"

namespace lumen {

enum class Key : uint8_t { Prev = 0, Next = 1, Ok = 2, Back = 3 };
inline constexpr int kKeyCount = 4;

enum class Kind : uint8_t { Press = 0, Long = 1, Repeat = 2 };

inline constexpr uint8_t keyBit(Key k) { return static_cast<uint8_t>(1u << static_cast<int>(k)); }

struct Event {
    Key key = Key::Ok;
    Kind kind = Kind::Press;
};

// PRESS or REPEAT: "move once" semantics for navigation keys
inline bool isStep(const Event& e) { return e.kind == Kind::Press || e.kind == Kind::Repeat; }

// Single-producer/single-consumer ring buffer: a port may push events straight
// from an ISR.
class EventQueue {
public:
    bool push(Event e);
    bool pop(Event* e);
    void clear() {
        head_ = 0;
        tail_ = 0;
    }
    bool empty() const { return head_ == tail_; }

private:
    volatile uint8_t head_ = 0;
    volatile uint8_t tail_ = 0;
    Event buf_[kEvqSize] = {};
};

enum class KeyLayout : uint8_t { Four = 0, Three = 1 };

// 3-key layout: raw bits are UP = Prev, DOWN = Next, ENTER = Ok.
inline constexpr Key kKeyUp = Key::Prev;
inline constexpr Key kKeyDown = Key::Next;
inline constexpr Key kKeyEnter = Key::Ok;

class Keys {
public:
    void init(KeyLayout layout);
    // Feed one raw sample; generated events go to `queue`.
    void scan(uint8_t raw, uint32_t nowMs, EventQueue& queue);
    // 0..1 hold progress towards the final hold stage (LONG; in the 3-key
    // layout ENTER's "home" stage). For UI hints.
    float hold(Key key, uint32_t nowMs) const;
    // Progress at which an intermediate hold stage begins, or 0 if none.
    float holdMark(Key key) const;
    KeyLayout layout() const { return layout_; }

private:
    void enter3(bool was, bool is, uint32_t now, EventQueue& queue);

    uint8_t stable_ = 0, rawLast_ = 0;
    uint32_t rawSince_ = 0;
    uint32_t downAt_[kKeyCount] = {};
    uint32_t nextRepeat_[kKeyCount] = {};
    uint8_t longFired_ = 0;
    KeyLayout layout_ = KeyLayout::Four;
};

}  // namespace lumen
