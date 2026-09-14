/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Keys.h"

#include <api/ui/Component.h>
#include <api/ui/Engine.h>
#include <api/ui/component/Scrollbar.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/component/TabBar.h>
#include <api/ui/component/TextBox.h>
#include <api/ui/component/Type.h>

#include <string>
#include <string_view>

#include "Command.h"

namespace v3d::ui::input {

namespace {

/**
 * Where a key moves a cursor through count things, starting from where.
 *
 * A list and a tab bar step through what they hold the same way and differ only in which
 * arrows read as "along": down and up for rows stacked vertically, right and left for
 * tabs laid out across. Neither wraps - running off the end of a list is how a keyboard
 * reaches the end of it, and a wrap would take the user back to the top instead.
 *
 * @param along the key that steps towards the end, and back the one that steps towards the start
 * @return where to move to, or `where` when the key moves nothing
 **/
int step(std::string_view key, int where, int count, std::string_view along, std::string_view back) {
    if (count <= 0) {
        return where;
    }
    if (key == along) {
        // nothing chosen steps onto the first rather than the second, which is what makes
        // one press of an arrow reach a list nobody has clicked in
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

bool Keys::press(std::string_view key, bool shifted, bool controlled) {
    if (!ui_ || key.empty()) {
        return false;
    }
    const boost::shared_ptr<Component> focused = ui_->focused();
    if (!focused) {
        return false;
    }
    if (key == "tab") {
        // taken whether or not it moved: a form holding one field still swallows the tab
        // rather than letting it reach a binding while somebody is typing
        ui_->focusNext(!shifted);
        return true;
    }
    if (key == "escape") {
        // leaving the box is what escape means everywhere else, and it is the only way
        // out of one with no other ui to click on
        ui_->focus(boost::shared_ptr<Component>());
        return true;
    }
    if (!usable(*focused)) {
        // a component disabled while it held the focus answers no key, and the key goes on
        // to the app's bindings the way one reaching an unfocused ui does. Tab and escape
        // are above this, so the focus is never stuck on one - ADR-0059
        return false;
    }
    return act(focused, key, shifted, controlled);
}

bool Keys::text(std::string_view utf8) {
    if (!ui_ || utf8.empty()) {
        return false;
    }
    const boost::shared_ptr<Component> focused = ui_->focused();
    if (!focused || focused->type() != component::Type::TextBox || !usable(*focused)) {
        // a box disabled while it held the focus takes no characters either, so what is
        // typed reaches the app rather than a field nobody can use - ADR-0059
        return false;
    }
    const boost::shared_ptr<component::TextBox> box =
        boost::dynamic_pointer_cast<component::TextBox>(focused);
    if (box) {
        // taken whether or not it went in: a limit that refused a paste has still answered
        // for the characters, and letting them through to the app's bindings would type
        // into the game instead
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
            // nothing here holds a place a key moves through, so the only key it answers is
            // the one that activates it - which is what falls out of the switch
            break;
    }
    if (controlled) {
        // a text box is the only thing here that answers a chord, so every other control
        // leaves one for the app - a ctrl-space must not press a focused button
        return false;
    }
    if (!activates(key)) {
        // a letter reaching a focused button is not being typed, so it goes on to the app's
        // bindings - unlike the same letter reaching a text box. Only a control that eats
        // every key can stop a game being played, and a button is not one
        return false;
    }
    // a component does not own the state it shows, so activating one sends its command and
    // marks nothing - ADR-0019. Taken either way, because a control that answers a click
    // and lets the same activation through to a binding is worse than one that does neither
    send(component);
    return true;
}

bool Keys::edit(const boost::shared_ptr<component::TextBox>& box, std::string_view key,
    bool shifted, bool controlled) {
    if (!box) {
        return false;
    }

    if (controlled) {
        // the four chords an editor is expected to answer, and no others: a chord the box
        // does not act on goes on to the app, so a ctrl-s still saves while it has the focus
        if (key == "c") {
            copySelection(box);
        } else if (key == "x") {
            cutSelection(box);
        } else if (key == "v") {
            paste(box);
        } else if (key == "a") {
            box->selectAll();
        } else {
            return false;
        }
        // taken whether or not it did anything, for the reason a refused paste is: a chord
        // the box answers must not also reach a binding
        return true;
    }

    if (key == "backspace") {
        box->backspace();
    } else if (key == "delete") {
        box->erase();
    } else if (key == "arrow_left") {
        // shift keeps the anchor where it is, so the run travelled ends up selected
        box->left(shifted);
    } else if (key == "arrow_right") {
        box->right(shifted);
    } else if (key == "home") {
        box->home(shifted);
    } else if (key == "end") {
        box->end(shifted);
    } else if (key == "return") {
        // the box owns its text and the app owns what the text means, so a return says
        // the user is done and whatever answers the command reads text() - ADR-0038. The
        // event is read off the box rather than through ui::command(), which deliberately
        // does not answer for one: a click into a box must not submit it
        send(box->event());
    } else if (key.size() == 1 || key == "space") {
        // a key that will arrive again as a character is taken here as well, so that it
        // does not also reach the app's bindings - typing "w" into a box must not walk
        // the player forward. api/input names a key one character wide when it composes
        // text and a word when it does not, so the width is the test
        return true;
    } else {
        // a function key, a modifier, a page down: nothing the box does, so the app still
        // sees it while the box has the focus
        return false;
    }
    return true;
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
    const int now = step(key, was, static_cast<int>(list->items().size()), "arrow_down", "arrow_up");
    if (now == was) {
        return false;
    }
    // the list owns which row is chosen and the app owns what being on it means, so moving
    // sends the command the same way clicking a row does - ADR-0019 and Cursor::act
    list->selected(now);
    send(list);
    return true;
}

bool Keys::turn(const boost::shared_ptr<component::TabBar>& bar, std::string_view key) {
    if (!bar) {
        return false;
    }
    const int was = bar->selected();
    const int now = step(key, was, static_cast<int>(bar->pages().size()), "arrow_right", "arrow_left");
    if (now == was) {
        // a bar carries no command, so a return on one has nothing to send and nothing to
        // take - which leaves the return for whatever the page holds
        return false;
    }
    bar->selected(now);
    return true;
}

bool Keys::nudge(const boost::shared_ptr<component::Scrollbar>& bar, std::string_view key) {
    if (!bar || !bar->scrollable()) {
        // a bar showing all of its content has nowhere to go, and a control that swallows a
        // key it could not act on is one that stops a game being played
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
        bar->offset(0.0f);
    } else if (key == "end") {
        bar->offset(bar->maximum());
    } else {
        // a bar carries no command, so a return and a space have nothing to send and are
        // left for whatever the bar scrolls to answer
        return false;
    }
    return true;
}

void Keys::copySelection(const boost::shared_ptr<component::TextBox>& box) const {
    if (clipboard_.write && box->selected()) {
        clipboard_.write(box->selection());
    }
}

void Keys::cutSelection(const boost::shared_ptr<component::TextBox>& box) const {
    // handed over before it is taken out, and not taken out at all when there is nowhere to
    // hand it: a cut that loses the run is worse than one that did not happen
    if (!clipboard_.write || !box->selected()) {
        return;
    }
    clipboard_.write(box->selection());
    box->removeSelection();
}

void Keys::paste(const boost::shared_ptr<component::TextBox>& box) const {
    if (clipboard_.read) {
        // over the selection, which is what insert() does with a run of characters - so a
        // paste and a typed character land the same way
        box->insert(clipboard_.read());
    }
}

void Keys::send(const boost::shared_ptr<Component>& component) const {
    send(command(component));
}

void Keys::send(const v3d::event::Event& event) const {
    if (dispatcher_ && event.context()) {
        dispatcher_->trigger(event);
    }
}

};  // namespace v3d::ui::input
