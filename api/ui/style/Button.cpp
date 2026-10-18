/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Button.h"

#include "Style.h"

#include <string>

namespace v3d::ui::style {

Button::Button(const std::string& str, State s) : Style({.name = str, .className = "button"}), state_(s) {
}

Button::~Button() {
}

Button::State Button::state() const {
    return state_;
}

};  // namespace v3d::ui::style
