/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Color.h"

#include <string>

namespace v3d::ui::style::prop {

Color::Color(const std::string& name, const glm::vec4& value) : Property(name), value_(value) {
}

Color::~Color() {
}

glm::vec4 Color::value() const noexcept {
    return value_;
}

void Color::value(const glm::vec4& v) noexcept {
    value_ = v;
}

};  // namespace v3d::ui::style::prop
