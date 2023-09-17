/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Icon.h"

namespace v3d::ui::component {

    Icon::Icon(boost::shared_ptr<v3d::gl::GLTexture> texture) : texture_(texture), Component(component::Type::ICON) {
    }

    Icon::~Icon() {
    }

};  // end namespace v3d::ui::component
