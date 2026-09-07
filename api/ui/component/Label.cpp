/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Label.h"

#include <string>

namespace v3d::ui::component {
Label::Label() : Component(component::Type::Label) {
}

Label::~Label() {
}

void Label::text(const std::string& txt) {
    text_ = txt;
}

std::string_view Label::text(void) const {
    return text_;
}

};  // namespace v3d::ui::component
