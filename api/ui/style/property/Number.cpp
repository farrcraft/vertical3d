/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Number.h"

#include <string>

namespace v3d::ui::style::prop {

Number::Number(const std::string& name, float value) : Property(name), value_(value) {
}

Number::~Number() {
}

float Number::value() const noexcept {
    return value_;
}

void Number::value(float v) noexcept {
    value_ = v;
}

};  // namespace v3d::ui::style::prop
