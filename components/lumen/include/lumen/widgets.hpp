// Lumen C++ - stateless draw helpers + small stateful widgets.
//
// Widgets own their motion state and expose update()/draw(); they never touch
// global UI state.
#pragma once

#include <cstdint>

#include "lumen/canvas.hpp"
#include "lumen/config.hpp"
#include "lumen/motion.hpp"

namespace lumen {

// ---- rolling text (odometer) -------------------------------------------
// Changed characters roll vertically; with several changes the rightmost
// rolls first and carries a ripple left, like a mechanical counter.
class Roll {
public:
    void init(const Font& font, const char* text, float dur, float stagger);
    void reset(const char* text);   // jump, no motion
    void set(const char* text, int dir = 1);   // dir: +1 up, -1 down
    void update(float dt);
    int draw(Canvas& canvas, float x, float y);   // -> end x
    int width(Canvas& canvas);
    bool busy() const;

private:
    struct Cell {
        char from = 0, to = 0;
        int8_t dir = 1;
        float t = 0.f;   // <0: waiting for its stagger slot
    };

    Font font_{};
    char text_[kRollMax + 1] = {};
    Cell cell_[kRollMax] = {};
    uint8_t count_ = 0;
    float dur_ = 0.3f, stagger_ = 0.05f;
};

// ---- marquee -----------------------------------------------------------
class Marquee {
public:
    void init(const Font& font, const char* text, float width);
    void set(const char* text);
    void update(float dt) { t_ += dt; }
    void draw(Canvas& canvas, float x, float y);

private:
    const char* text_ = "";
    Font font_{};
    float width_ = 0.f, t_ = 0.f, speed_ = 0.f, hold_ = 1.2f, gap_ = 0.f;
};

// ---- draw helpers ------------------------------------------------------
// switch: p 0..1 (springs may overshoot), vel in p-units/s for stretch
void drawToggle(Canvas& canvas, float x, float y, float p, float vel);
void drawSpinner(Canvas& canvas, float cx, float cy, float r, float t, float thick);
void drawScrollbar(Canvas& canvas, float x, float y, float h, float pos, float visible, float total);
void drawChevron(Canvas& canvas, float x, float cy, bool right, int s);
void drawBattery(Canvas& canvas, float x, float y, float level);
void drawWifi(Canvas& canvas, float x, float cy, bool on);

}  // namespace lumen
