/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Button.h"


namespace v3d::ui::style {

Button::Button(const std::string& str, v3d::ui::component::Button::ButtonState s) : Style(str, "button"), state_(s) {
}

Button::~Button() {
}

v3d::ui::component::Button::ButtonState Button::state() const {
	return state_;
}

};  // namespace v3d::ui::style
