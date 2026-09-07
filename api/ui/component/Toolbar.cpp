/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Toolbar.h"

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
}

/**
 **/
Toolbar::Edge Toolbar::edge() const noexcept {
    return edge_;
}

/**
 **/
void Toolbar::add(const boost::shared_ptr<Button>& button) {
    buttons_.push_back(button);
}

/**
 **/
std::size_t Toolbar::size() const noexcept {
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
        if (!button) {
            continue;
        }
        v3d::type::Bound2D bound = button->bound();
        if (bound.intersect(cursor)) {
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
    v3d::type::Bound2D bound = this->bound();
    return bound.intersect(cursor);
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
    v3d::type::Bound2D bound = this->bound();
    if (!bound.intersect(cursor)) {
        return false;
    }
    const boost::shared_ptr<Button> over = buttonAt(cursor);
    // the gap between the buttons and the edges of the strip takes the press and does
    // nothing with it, so a click that misses a button does not reach the scene under it
    if (over) {
        const v3d::event::Event event = over->event();
        // a button is only bound when its config gave both a command and a context.
        // Event::str() dereferences the context, so an unbound event must never be sent
        if (dispatcher_ && event.context()) {
            dispatcher_->trigger(event);
        }
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
