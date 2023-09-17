/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Label.h"

namespace v3d::ui::component {
    Label::Label() : Component(component::Type::LABEL) {
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
