/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Keys.h"

#include <api/event/Event.h>
#include <api/ui/Component.h>
#include <api/ui/Engine.h>
#include <api/ui/component/Scrollbar.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/component/Slider.h>
#include <api/ui/component/TabBar.h>
#include <api/ui/component/TextBox.h>
#include <api/ui/component/Type.h>
#include <api/ui/input/LineEdit.h>

#include <algorithm>
#include <string_view>

#include "Command.h"

namespace v3d::ui::input {

namespace {

/** The key that steps towards the end, and the one that steps towards the start. **/
struct Arrows {
    std::string_view along;
    std::string_view back;
};

/**
 * Where a key moves a cursor through count things, starting from where.
 *
 * A list and a tab bar step through what they hold the same way. They differ only in which
 * arrows step: down and up for rows stacked vertically, right and left for tabs laid out
 * across. Neither wraps, so holding an arrow stops at the last item.
 *
 * @return where to move to, or `where` when the key moves nothing
 **/
int step(std::string_view key, int where, int count, const Arrows& arrows) {
    const std::string_view along = arrows.along;
    const std::string_view back = arrows.back;
    if (count <= 0) {
        return where;
    }
    if (key == along) {
        // with nothing chosen, the first step goes to the first item, not the second
        if (where < 0) {
            return 0;
        }
        return where + 1 < count ? where + 1 : where;
    }
    if (key == back) {
        return where <= 0 ? 0 : where - 1;
    }
    if (key == "home") {
        return 0;
    }
    if (key == "end") {
        return count - 1;
    }
    return where;
}

/**
 * @return whether a key is the one that activates whatever holds the focus
 **/
bool activates(std::string_view key) noexcept {
    return key == "return" || key == "space";
}

};  // namespace

Keys::Keys(const boost::shared_ptr<Engine>& ui, const boost::shared_ptr<entt::dispatcher>& dispatcher,
    const Clipboard& clipboard) :
    ui_(ui),
    dispatcher_(dispatcher),
    clipboard_(clipboard) {
}

bool Keys::press(std::string_view key, bool shifted, bool controlled, bool repeat) {
    if (!ui_ || key.empty()) {
        return false;
    }
    const boost::shared_ptr<Component> focused = ui_->focused();
    if (!focused) {
        return false;
    }
    if (usable(*focused) && !ui_->reachable(focused)) {
        // the component was hidden while it held the focus, as a dialog's field is when the
        // dialog closes. The focus is cleared, and the key goes on to the app, tab and
        // escape included
        ui_->focus(boost::shared_ptr<Component>());
        return false;
    }
    if (key == "tab") {
        // taken whether or not the focus moved, so a tab in a form with one field does not
        // reach a binding while somebody is typing
        ui_->focusNext(!shifted);
        return true;
    }
    if (key == "escape") {
        // escape takes the focus off whatever has it. With no other ui to click on, it is
        // the only way out of a text box
        ui_->focus(boost::shared_ptr<Component>());
        return true;
    }
    if (!usable(*focused)) {
        // a component disabled while it held the focus takes no key, and the key goes on to
        // the app's bindings as it would with nothing focused. Tab and escape are handled
        // above, so the focus can still be moved off it
        return false;
    }
    if (repeat && activates(key)) {
        // a repeat of return or space does nothing, so a held key sends its command once. The
        // repeat is still taken, so it reaches no binding either
        return true;
    }
    return act(focused, key, shifted, controlled);
}

bool Keys::text(std::string_view utf8) {
    if (!ui_ || utf8.empty()) {
        return false;
    }
    const boost::shared_ptr<Component> focused = ui_->focused();
    if (!focused || !component::traits(focused->type()).text || !usable(*focused)) {
        // a box disabled while it held the focus takes no characters either, so what is
        // typed reaches the app
        return false;
    }
    if (!ui_->reachable(focused)) {
        // a box hidden while it held the focus loses the focus
        ui_->focus(boost::shared_ptr<Component>());
        return false;
    }
    const boost::shared_ptr<component::TextBox> box =
        boost::dynamic_pointer_cast<component::TextBox>(focused);
    if (box) {
        // taken even when the box's length limit refused the characters, so they never reach
        // the app's bindings
        box->insert(utf8);
    }
    return true;
}

bool Keys::act(const boost::shared_ptr<Component>& component, std::string_view key,
    bool shifted, bool controlled) {
    switch (component->type()) {
        case component::Type::TextBox:
            return edit(boost::dynamic_pointer_cast<component::TextBox>(component), key,
                shifted, controlled);
        case component::Type::SelectList:
            return choose(boost::dynamic_pointer_cast<component::SelectList>(component), key);
        case component::Type::TabBar:
            return turn(boost::dynamic_pointer_cast<component::TabBar>(component), key);
        case component::Type::Scrollbar:
            return nudge(boost::dynamic_pointer_cast<component::Scrollbar>(component), key);
        case component::Type::Slider:
            return slide(boost::dynamic_pointer_cast<component::Slider>(component), key);
        case component::Type::Bar:
        case component::Type::Button:
        case component::Type::CheckBox:
        case component::Type::HorizontalBox:
        case component::Type::Icon:
        case component::Type::Label:
        case component::Type::Menu:
        case component::Type::MenuBar:
        case component::Type::MenuItem:
        case component::Type::Panel:
        case component::Type::RadioButton:
        case component::Type::TabPage:
        case component::Type::Toolbar:
        case component::Type::Undefined:
        case component::Type::VerticalBox:
            // a key does not move anything inside these, so the only keys they take are the
            // activation keys, handled after the switch
            break;
    }
    if (controlled) {
        // only a text box takes a chord. Every other control leaves it for the app, so a
        // ctrl-space does not press a focused button
        return false;
    }
    if (!activates(key)) {
        // a letter reaching a focused button goes on to the app's bindings, unlike the same
        // letter reaching a text box, so the game can still be played while a button has the
        // focus
        return false;
    }
    // a component does not own the state it shows, so activating one sends its command and
    // marks nothing. The key is taken even when there is no command, so an activation never
    // also reaches a binding
    send(component);
    return true;
}

bool Keys::edit(const boost::shared_ptr<component::TextBox>& box, std::string_view key,
    bool shifted, bool controlled) {
    if (!box) {
        return false;
    }

    if (!controlled && key == "return") {
        // a return sends the box's command, and whatever handles it reads text(). The event
        // is read from the box directly. input::command() returns none for a text box, so
        // that a click into one does not submit it
        send(box->event());
        return true;
    }
    return input::edit(box->line(), key, shifted, controlled, clipboard_);
}

bool Keys::choose(const boost::shared_ptr<component::SelectList>& list, std::string_view key) {
    if (!list) {
        return false;
    }
    if (activates(key)) {
        send(list);
        return true;
    }

    const int was = list->selected();
    const int now = step(key, was, static_cast<int>(list->items().size()), {.along = "arrow_down", .back = "arrow_up"});
    if (now == was) {
        return false;
    }
    // the list holds which row is chosen, and the app decides what that row means, so moving
    // sends the command, as clicking a row does in Cursor::act
    list->selected(now);
    send(list);
    return true;
}

bool Keys::turn(const boost::shared_ptr<component::TabBar>& bar, std::string_view key) {
    if (!bar) {
        return false;
    }
    const int was = bar->selected();
    const int now = step(key, was, static_cast<int>(bar->pages().size()), {.along = "arrow_right", .back = "arrow_left"});
    if (now == was) {
        // a tab bar carries no command, so a return on one sends nothing and is not taken.
        // The app receives it
        return false;
    }
    bar->selected(now);
    return true;
}

bool Keys::nudge(const boost::shared_ptr<component::Scrollbar>& bar, std::string_view key) {
    if (!bar || !bar->scrollable()) {
        // a bar showing all of its content cannot move, so the key is left for the app's
        // bindings
        return false;
    }
    const bool vertical = bar->direction() == component::Scrollbar::Direction::Vertical;
    if (key == (vertical ? "arrow_down" : "arrow_right")) {
        bar->scroll(bar->line());
    } else if (key == (vertical ? "arrow_up" : "arrow_left")) {
        bar->scroll(-bar->line());
    } else if (key == "pagedown") {
        bar->scroll(bar->page());
    } else if (key == "pageup") {
        bar->scroll(-bar->page());
    } else if (key == "home") {
        bar->offset(0.0F);
    } else if (key == "end") {
        bar->offset(bar->maximum());
    } else {
        // a scrollbar carries no command, so a return or a space sends nothing and is left
        // for the app
        return false;
    }
    return true;
}

bool Keys::slide(const boost::shared_ptr<component::Slider>& slider, std::string_view key) {
    if (!slider) {
        return false;
    }
    // a line is the slider's step, or a hundredth of the range for a slider with no step. A
    // page is a tenth of the range, and never less than a line
    const float span = slider->maximum() - slider->minimum();
    const float line = slider->step() > 0.0F ? slider->step() : span / 100.0F;
    const float page = std::max(line, span / 10.0F);
    float to = slider->value();
    if (key == "arrow_right") {
        to += line;
    } else if (key == "arrow_left") {
        to -= line;
    } else if (key == "pageup") {
        to += page;
    } else if (key == "pagedown") {
        to -= page;
    } else if (key == "home") {
        to = slider->minimum();
    } else if (key == "end") {
        to = slider->maximum();
    } else {
        return false;
    }
    // a key that moves nothing is not taken, as with a scrollbar, so a slider at an end
    // leaves the arrow for the app
    if (!slider->value(to)) {
        return false;
    }
    send(command(slider));
    return true;
}

void Keys::send(const boost::shared_ptr<Component>& component) const {
    send(command(component));
}

void Keys::send(const v3d::event::Event& event) const {
    input::send(dispatcher_.get(), event);
}

};  // namespace v3d::ui::input
