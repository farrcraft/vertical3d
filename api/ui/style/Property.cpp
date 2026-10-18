/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Property.h"

#include <string>

namespace v3d::ui::style {

Property::Property(const std::string& str) : align_(NULL_ALIGNMENT), name_(str) {
}

Property::~Property() {
}

Property::Alignment Property::align() const {
    return align_;
}

void Property::align(Alignment alignment) {
    align_ = alignment;
}

std::string_view Property::name() const {
    return name_;
}

};  // namespace v3d::ui::style
