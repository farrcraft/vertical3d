/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Keys.h"

#include <string_view>

#include "Component.h"
#include "Engine.h"
#include "component/TextBox.h"
#include "component/Type.h"

namespace v3d::ui {

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
    if (component->type() != component::Type::TextBox) {
        return false;
    }
    const boost::shared_ptr<component::TextBox> box =
        boost::dynamic_pointer_cast<component::TextBox>(component);
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
        // the user is done and whatever answers the command reads text() - ADR-0038
        if (dispatcher_ && box->event().context()) {
            dispatcher_->trigger(box->event());
        }
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

};  // namespace v3d::ui
