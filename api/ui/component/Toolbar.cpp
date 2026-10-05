/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Toolbar.h"

#include <api/ui/input/Command.h>

#include <cstddef>
#include <string>

#include "Type.h"

namespace v3d::ui::component {

/**
 **/
Toolbar::Toolbar(const boost::shared_ptr<entt::dispatcher>& dispatcher, Edge edge) :
    Component(component::Type::Toolbar),
    dispatcher_(dispatcher),
    edge_(edge) {
    // a strip is a control unless a document says it is scenery, which lets a press through
    pickable(true);
}

/**
 **/
Toolbar::Edge Toolbar::edge() const noexcept {
    return edge_;
}

/**
 **/
void Toolbar::add(const boost::shared_ptr<Button>& button) {
    if (button) {
        adopt(*button);
    }
    buttons_.push_back(button);
}

/**
 **/
std::size_t Toolbar::count() const noexcept {
    return buttons_.size();
}

/**
 **/
boost::shared_ptr<Button> Toolbar::button(std::size_t index) const {
    return buttons_[index];
}

/**
 **/
boost::shared_ptr<Button> Toolbar::buttonAt(const glm::vec2& cursor) const {
    for (const boost::shared_ptr<Button>& button : buttons_) {
        // a button that cannot be used is not offered the cursor, so the strip neither
        // lights it nor sends its command
        if (!button || !usable(*button)) {
            continue;
        }
        v3d::type::geometry::Bound2D bound = button->bound();
        if (bound.contains(cursor)) {
            return button;
        }
    }
    return nullptr;
}

/**
 **/
bool Toolbar::motion(const glm::vec2& cursor) {
    const boost::shared_ptr<Button> over = buttonAt(cursor);
    for (const boost::shared_ptr<Button>& button : buttons_) {
        if (button) {
            button->state(button == over ? Button::STATE_HOVER : Button::STATE_NORMAL);
        }
    }
    v3d::type::geometry::Bound2D bound = this->bound();
    return bound.contains(cursor);
}

/**
 **/
void Toolbar::leave() {
    for (const boost::shared_ptr<Button>& button : buttons_) {
        if (button) {
            button->state(Button::STATE_NORMAL);
        }
    }
}

/**
 **/
bool Toolbar::press(const glm::vec2& cursor) {
    v3d::type::geometry::Bound2D bound = this->bound();
    if (!bound.contains(cursor)) {
        return false;
    }
    const boost::shared_ptr<Button> over = buttonAt(cursor);
    // the gap between the buttons and the edges of the strip takes the press and does
    // nothing with it, so a click that misses a button does not reach the scene under it
    if (over) {
        v3d::ui::input::send(dispatcher_.get(), over->event());
    }
    return true;
}

/**
 **/
boost::shared_ptr<Button> Toolbar::find(const std::string& command) const {
    for (const boost::shared_ptr<Button>& button : buttons_) {
        if (!button) {
            continue;
        }
        const v3d::event::Event event = button->event();
        if (event.context() && event.str() == command) {
            return button;
        }
    }
    return nullptr;
}

};  // namespace v3d::ui::component
