// Lumen C++ - built-in overlays.
//
// Each is a singleton (static storage inside the module); showing one while it
// is already up re-initialises it in place.
//
//  Toast  non-modal, drops in with a bounce, auto-dismisses
//  Dialog modal confirm: pops out of the element that opened it, spring
//         cursor between buttons, optional busy phase with spinner
//  Sheet  modal bottom sheet editing an integer value with live callback
#pragma once

#include <cstdint>

#include "lumen/page.hpp"
#include "lumen/ui.hpp"

namespace lumen {

enum class ToastIcon : uint8_t { Check, Wifi, Info };

void toast(Ui& ui, const char* text, ToastIcon icon = ToastIcon::Check);

using DialogCallback = void (*)(Ui& ui, int choice, void* ctx);   // -1 = dismissed

struct DialogDesc {
    const char* title = "";
    const char* message = "";
    const char* buttons[2] = {"OK", nullptr};
    int defaultButton = 0;
    int busyButton = -1;          // button that shows a busy phase first, -1 none
    float busySeconds = 0.f;
    const char* busyText = "";
    float originX = 0.f, originY = 0.f;   // where it pops out from
    DialogCallback callback = nullptr;
    void* ctx = nullptr;
};

void dialog(Ui& ui, const DialogDesc& desc);

using SheetCallback = void (*)(Ui& ui, int32_t value, void* ctx);

struct SheetDesc {
    const char* title = "";
    int32_t value = 0, lo = 0, hi = 100, step = 1;
    SheetCallback onChange = nullptr;
    void* ctx = nullptr;
};

void sheet(Ui& ui, const SheetDesc& desc);

}  // namespace lumen
