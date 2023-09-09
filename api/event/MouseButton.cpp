/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "MouseButton.h"

namespace v3d::event {
    /**
     **/
    MouseButton::MouseButton(unsigned int button, const boost::shared_ptr<Context>& context, bool pressed) noexcept :
        Event("button", context),
        button_(button),
        pressed_(pressed) {
        EventData buttonState = pressed;
        data(buttonState);
    }

    /**
     **/
    unsigned int MouseButton::button() const noexcept {
        return button_;
    }

    /**
    **/
    bool MouseButton::pressed() const noexcept {
        return pressed_;
    }

};  // namespace v3d::event
