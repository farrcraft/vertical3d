/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Property.h"

#include <string>
#include <string_view>

namespace v3d::ui::style {

Property::Property(const std::string& str) : name_(str) {
}

Property::~Property() {
}

std::string_view Property::name() const {
    return name_;
}

};  // namespace v3d::ui::style
