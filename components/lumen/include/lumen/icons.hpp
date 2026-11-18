// Lumen C++ - vector app icons.
//
// Procedural so they scale smoothly in the carousel and animate while focused
// (`phase` only advances for the active icon).
#pragma once

#include "lumen/canvas.hpp"

namespace lumen {

using IconFn = void (*)(Canvas& canvas, float cx, float cy, float size, float phase);

void iconClock(Canvas& c, float cx, float cy, float s, float phase);
void iconMusic(Canvas& c, float cx, float cy, float s, float phase);
void iconMonitor(Canvas& c, float cx, float cy, float s, float phase);
void iconSettings(Canvas& c, float cx, float cy, float s, float phase);
void iconAbout(Canvas& c, float cx, float cy, float s, float phase);

}  // namespace lumen
