// TIXEL C++ - UI core: page stack, overlays, transitions, power, display.
//
// Key events flow down a chain of responsibility:
//   power/dim wake -> transition (queue or reverse) -> top modal overlay
//   -> global gestures (hold BACK) -> top page -> default BACK = pop.
//
// Transitions are strategies (slide/zoom/fade), each driven either by a bezier
// curve or a second-order system, reversible mid-flight.
#pragma once

#include <cstdint>

#include "tixel/canvas.hpp"
#include "tixel/config.hpp"
#include "tixel/input.hpp"
#include "tixel/motion.hpp"
#include "tixel/page.hpp"
#include "tixel/port.hpp"
#include "tixel/settings.hpp"

namespace tixel {

enum class Power : uint8_t { On, TurningOff, Off, TurningOn };

struct Drive {
    bool useSpring = false;
    Spring spring;   // second-order driver
    Tween tween;     // bezier driver
};

struct TransitionState {
    bool active = false, forward = false, reversed = false;
    Transition kind = Transition::None;
    Page* oldPage = nullptr;
    Page* newPage = nullptr;
    float rect[4] = {0, 0, 0, 0};
    Drive drive;
};

class Ui {
public:
    // ---- lifecycle ----------------------------------------------------
    void init(const Port& port, const int32_t* defaults, int defaultCount);
    void start(Page* root);       // push root + wake
    void tick();                  // dt from port.millis
    void step(float dt);           // explicit dt (tests / simulator)
    bool idle() const;             // nothing moving: the port may sleep
    void post(Key key, Kind kind);

    // ---- navigation ---------------------------------------------------
    void push(Page* page, Transition how);
    void replace(Page* page, Transition how);
    void pop();
    void home();
    Page* top() const { return depth_ ? stack_[depth_ - 1].page : nullptr; }

    // ---- overlays -----------------------------------------------------
    Overlay* show(Overlay* overlay);
    bool modalOpen() const;

    // ---- services -----------------------------------------------------
    void haptic();
    void sleep();
    void wake();
    void now(DateTime* dt) const;

    int32_t setting(int id) const { return settings_.get(id); }
    void setSetting(int id, int32_t value) { settings_.set(id, value); }
    float holdProgress(Key key) const;

    // Dominant animation right now (transition > modal overlay > page), for
    // tooling such as a simulator's motion scope.
    const void* probe(const char** label, float* value, float* target);

    // ---- accessors ----------------------------------------------------
    Canvas& canvas() { return canvas_; }
    const Canvas& canvas() const { return canvas_; }
    Port& port() { return port_; }
    Settings& settings() { return settings_; }
    TransitionState& transition() { return transition_; }
    uint8_t* scratchA() { return bufA_; }
    uint8_t* scratchB() { return bufB_; }
    float time() const { return time_; }
    uint32_t ms() const { return ms_; }
    float fps() const { return fps_; }
    uint8_t depth() const { return depth_; }
    Power power() const { return power_; }
    uint32_t frames() const { return frames_; }
    uint8_t overlayCount() const { return overlayCount_; }
    Overlay* overlayAt(int i) const { return overlays_[i]; }

private:
    struct Entry {
        Page* page = nullptr;
        Transition how = Transition::None;
    };

    static void settingsChanged(void* ctx, int id, int32_t oldValue, int32_t newValue);
    void startInvert(bool wasInverted);
    void beginTransition(Transition kind, Page* oldPage, Page* newPage, bool forward);
    void attach(Page* page);
    void panelPower(bool on);
    void dispatch(const Event& e);
    void handleKey(const Event& e);
    void update(float dt);
    void pageUpdate(Page* page, float dt);
    void postInvert(uint8_t* buf);
    void render();
    void present(float level);

    Port port_{};
    Canvas canvas_{};
    Settings settings_{};

    Entry stack_[kMaxStack] = {};
    uint8_t depth_ = 0;
    Overlay* overlays_[kMaxOverlays] = {};
    uint8_t overlayCount_ = 0;
    TransitionState transition_{};

    EventQueue events_{};     // from keys FSM or port ISR
    EventQueue deferred_{};   // keys that arrived mid-transition
    Keys keys_{};

    // time
    float time_ = 0.f;        // s since start, wraps hourly (phases only)
    uint32_t ms_ = 0;         // ms since start (exact, for timers/keys)
    uint32_t lastMs_ = 0;
    bool started_ = false;
    uint32_t frames_ = 0;
    float fps_ = 0.f;
    uint16_t stillFrames_ = 0;   // consecutive frames with no pixel change
    bool forceFull_ = false;     // resend every page next frame

    // display state
    Power power_ = Power::Off;
    Tween powerTween_{};
    float idleTime_ = 0.f;
    bool dimmed_ = false;
    Spring bright_{}, dim_{};
    Tween invertTween_{};
    bool invertActive_ = false, invertFrom_ = false;
    uint8_t contrastSent_ = 0xFF;
    uint32_t pageHash_[kPages] = {};
    float saveTimer_ = 0.f;
    float hapticAt_ = 0.f;      // time of the last haptic pulse
    DateTime clockBase_{};      // used when the port has no RTC

    // scratch buffers for transitions / scaled overlays
    uint8_t bufA_[kFbBytes] = {};
    uint8_t bufB_[kFbBytes] = {};
};

}  // namespace tixel
