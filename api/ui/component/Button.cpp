/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Button.h"

#include <vector>

namespace v3d::ui::component {

    Button::Button() : state_(STATE_NORMAL), Component(component::Type::BUTTON) {
    }

    void Button::label(const std::string& str) {
        label_ = str;
    }

    std::string_view Button::label() const {
        return label_;
    }

    Button::ButtonState Button::state() const {
        return state_;
    }

    void Button::state(ButtonState s) {
        state_ = s;
    }

};  // namespace v3d::ui::component
