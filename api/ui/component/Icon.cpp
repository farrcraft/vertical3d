/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Icon.h"

namespace v3d::ui::component {

    Icon::Icon(const v3d::render::realtime::TextureHandle& texture) : Component(component::Type::ICON), texture_(texture) {
    }

    Icon::~Icon() {
    }

};  // end namespace v3d::ui::component
