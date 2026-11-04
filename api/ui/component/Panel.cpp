/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Panel.h"

#include <api/ui/Component.h>

#include "Type.h"

namespace v3d::ui::component {

Panel::Panel() :
    Component(Type::Panel),
    recess_(false) {
}

void Panel::recess(bool on) noexcept {
    recess_ = on;
}

bool Panel::recess() const noexcept {
    return recess_;
}

};  // namespace v3d::ui::component
