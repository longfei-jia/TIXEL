// TIXEL C++ - declarative list menu.
//
//   static const MenuItem kItems[] = {
//       TIXEL_ITEM_TOGGLE("Sound", kSound),
//       TIXEL_ITEM_SHEET("Brightness", openBrightness, formatPercent),
//       TIXEL_ITEM_SUBMENU("Display", &displayMenu),
//   };
//   Menu menu("Settings", kItems);
//
// The page renders a jelly cursor (snappy leading edge, trailing edge on a
// "follow" system), spring scroll with wrap-around, staggered row entrance and
// inline widgets bound to settings ids.
#pragma once

#include <cstdint>

#include "tixel/page.hpp"
#include "tixel/theme.hpp"
#include "tixel/widgets.hpp"

namespace tixel {

enum class MenuItemKind : uint8_t {
    Action,    // callback
    Toggle,    // bool setting; optional async busy phase
    Option,    // cycles through a value/label list
    Sheet,     // callback opens an editor, shows format() value
    Submenu    // pushes a page
};

struct Option {
    int32_t value;
    const char* label;
};

class Menu;

using MenuAction = void (*)(Menu& menu, int index);
using MenuFormat = void (*)(Ui& ui, char* out, int n);
using MenuDone = void (*)(Ui& ui, int32_t value);

struct MenuItem {
    const char* label = "";
    MenuItemKind kind = MenuItemKind::Action;
    int setting = -1;                 // Toggle / Option
    const Option* options = nullptr;  // Option
    uint8_t optionCount = 0;
    MenuAction action = nullptr;      // Action / Sheet
    MenuFormat format = nullptr;      // Sheet value text
    Page* page = nullptr;             // Submenu
    uint16_t busyMs = 0;              // Toggle: async phase before ON
    MenuDone done = nullptr;          // Toggle: after value committed
};

#define TIXEL_ITEM_ACTION(label, fn) \
    tixel::MenuItem { label, tixel::MenuItemKind::Action, -1, nullptr, 0, fn, nullptr, nullptr, 0, nullptr }
#define TIXEL_ITEM_TOGGLE(label, id) \
    tixel::MenuItem { label, tixel::MenuItemKind::Toggle, id, nullptr, 0, nullptr, nullptr, nullptr, 0, nullptr }
#define TIXEL_ITEM_TOGGLE_ASYNC(label, id, ms, done) \
    tixel::MenuItem { label, tixel::MenuItemKind::Toggle, id, nullptr, 0, nullptr, nullptr, nullptr, ms, done }
#define TIXEL_ITEM_OPTION(label, id, opts)                                                 \
    tixel::MenuItem {                                                                      \
        label, tixel::MenuItemKind::Option, id, opts,                                      \
            static_cast<uint8_t>(sizeof(opts) / sizeof((opts)[0])), nullptr, nullptr,      \
            nullptr, 0, nullptr                                                            \
    }
#define TIXEL_ITEM_SHEET(label, fn, fmt) \
    tixel::MenuItem { label, tixel::MenuItemKind::Sheet, -1, nullptr, 0, fn, fmt, nullptr, 0, nullptr }
#define TIXEL_ITEM_SUBMENU(label, page) \
    tixel::MenuItem { label, tixel::MenuItemKind::Submenu, -1, nullptr, 0, nullptr, nullptr, page, 0, nullptr }

class Menu : public Page {
public:
    Menu() = default;
    template <size_t N>
    Menu(const char* title, const MenuItem (&items)[N]) {
        set(title, items, static_cast<int>(N));
    }
    void set(const char* title, const MenuItem* items, int count);

    // Localized header text, e.g. "Settings|设置".
    void setTitleText(const char* t) { titleText_ = t; }

    void enter() override;
    bool key(const Event& e) override;
    void update(float dt) override;
    void render(Canvas& canvas) override;
    void focus(float r[4]) override;
    const Spring* probe(const char** label) override;

    // Focus rect of the selected row (dialogs pop out of it).
    void focusRect(float r[4]);

private:
    struct Row {
        Spring sw;      // toggle knob
        Roll value;     // value text
        float busy = 0.f;
    };

    // Labels and option texts may be written "English|中文". The half that
    // matches kSettingLanguage is rendered; a string without '|' is used as-is.
    const char* lang(const char* text) const;

    const MenuItem* item(int i) const { return &items_[i]; }
    void valueText(int i, char* out, int n);
    float labelWidth(int i);
    void counterText(char* out) const;
    void move(int delta);
    void activate(int i);
    void drawWidget(Canvas& canvas, int i, float rowY);

    const MenuItem* items_ = nullptr;
    // Optional localized title for the header. Page::title stays the plain
    // identifier (used by tooling), this is what gets rendered.
    const char* titleText_ = nullptr;
    int32_t langSeen_ = -1;   // last kSettingLanguage value, to re-measure on switch
    uint8_t count_ = 0, sel_ = 0, top_ = 0;
    Spring cur_, trail_, curW_, scroll_, press_;
    Roll counter_;
    Row rows_[kMenuMaxItems] = {};
    mutable char langBuf_[48] = {};   // reused for the latin half of a label
};

}  // namespace tixel
