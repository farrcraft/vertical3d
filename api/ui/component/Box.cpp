/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Box.h"

namespace v3d::ui::component {

Box::Box(Type type) :
    Component(type),
    spacing_(0.0f),
    stretch_(false) {
}

void Box::spacing(float gap) {
    spacing_ = gap;
}

float Box::spacing() const noexcept {
    return spacing_;
}

void Box::stretch(bool fill) {
    stretch_ = fill;
}

bool Box::stretch() const noexcept {
    return stretch_;
}

};  // namespace v3d::ui::component
