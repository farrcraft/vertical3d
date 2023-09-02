/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Property.h"

namespace v3d::ui::style {

Property::Property(const std::string& str) : align_(NULL_ALIGNMENT), name_(str) {
}

Property::~Property() {
}

Property::Alignment Property::align() const {
    return align_;
}

void Property::align(Alignment a) {
    align_ = a;
}

std::string_view Property::name() const {
    return name_;
}

};  // namespace v3d::ui::style
