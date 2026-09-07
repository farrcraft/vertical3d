/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RadioButton.h"

#include <string>

namespace v3d::ui::component {

RadioButton::RadioButton() :
    CheckBox(Type::RadioButton) {
}

void RadioButton::group(const std::string& name) {
    group_ = name;
}

std::string_view RadioButton::group() const {
    return group_;
}

};  // namespace v3d::ui::component
