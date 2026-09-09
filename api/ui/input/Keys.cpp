/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Keys.h"

#include <api/ui/Component.h>
#include <api/ui/Engine.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/component/TabBar.h>
#include <api/ui/component/TextBox.h>
#include <api/ui/component/Type.h>

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

Keys::Keys(const boost::shared_ptr<Engine>& ui, const boost::shared_ptr<entt::dispatcher>& dispatcher) :
    ui_(ui),
    dispatcher_(dispatcher) {
}

bool Keys::press(std::string_view key, bool shifted) {
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
    return act(focused, key);
}

bool Keys::text(std::string_view utf8) {
    if (!ui_ || utf8.empty()) {
        return false;
    }
    const boost::shared_ptr<Component> focused = ui_->focused();
    if (!focused || focused->type() != component::Type::TextBox) {
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

bool Keys::act(const boost::shared_ptr<Component>& component, std::string_view key) {
    switch (component->type()) {
        case component::Type::TextBox:
            return edit(boost::dynamic_pointer_cast<component::TextBox>(component), key);
        case component::Type::SelectList:
            return choose(boost::dynamic_pointer_cast<component::SelectList>(component), key);
        case component::Type::TabBar:
            return turn(boost::dynamic_pointer_cast<component::TabBar>(component), key);
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
        case component::Type::Scrollbar:
        case component::Type::TabPage:
        case component::Type::Toolbar:
        case component::Type::Undefined:
        case component::Type::VerticalBox:
            // nothing here steps through anything it holds, so the only key it answers is the
            // one that activates it - which is what falls out of the switch. A scrollbar is
            // the exception worth naming: it holds a position a key could move, and takes none
            break;
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

bool Keys::edit(const boost::shared_ptr<component::TextBox>& box, std::string_view key) {
    if (!box) {
        return false;
    }

    if (key == "backspace") {
        box->backspace();
    } else if (key == "delete") {
        box->erase();
    } else if (key == "arrow_left") {
        box->left();
    } else if (key == "arrow_right") {
        box->right();
    } else if (key == "home") {
        box->home();
    } else if (key == "end") {
        box->end();
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

void Keys::send(const boost::shared_ptr<Component>& component) const {
    send(command(component));
}

void Keys::send(const v3d::event::Event& event) const {
    if (dispatcher_ && event.context()) {
        dispatcher_->trigger(event);
    }
}

};  // namespace v3d::ui::input
