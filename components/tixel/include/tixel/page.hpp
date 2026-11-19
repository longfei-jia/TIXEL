// TIXEL C++ - page and overlay interfaces.
//
// Pages and overlays are objects with virtual hooks. Instances are static;
// nothing in the library allocates. Every hook has a safe default, so a page
// only overrides what it needs.
#pragma once

#include <cstdint>

#include "tixel/canvas.hpp"
#include "tixel/input.hpp"
#include "tixel/motion.hpp"

namespace tixel {

class Ui;

enum class Transition : uint8_t { None, Slide, Zoom, Fade };

class Page {
public:
    virtual ~Page() = default;

    virtual void enter() {}          // pushed: reset + start intro
    virtual void resume() {}         // page above was popped
    virtual void leave() {}          // removed from the stack
    virtual bool key(const Event&) { return false; }   // true = consumed
    virtual void update(float /*dt*/) {}
    virtual void render(Canvas& /*canvas*/) {}
    // rect a zoom transition grows from / shrinks back into (optional)
    virtual void focus(float /*r*/[4]) {}
    // optional: expose the dominant animation for tooling
    virtual const Spring* probe(const char** /*label*/) { return nullptr; }

    const char* title = "";
    Ui* ui = nullptr;
    float age = 0.f;   // seconds since enter
};

class Overlay {
public:
    virtual ~Overlay() = default;

    virtual void key(const Event&) {}
    virtual void update(float /*dt*/) {}
    virtual void render(Canvas& /*canvas*/) {}
    virtual void close() {}
    virtual const Spring* probe(const char** /*label*/) { return nullptr; }

    Ui* ui = nullptr;
    bool modal = false;     // owns input while open
    bool closing = false;   // animating out
    bool dead = false;      // remove at end of frame
    float age = 0.f;
};

}  // namespace tixel
