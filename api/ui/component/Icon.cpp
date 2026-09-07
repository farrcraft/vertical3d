/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Icon.h"

#include <string>

namespace v3d::ui::component {

Icon::Icon(const std::string& source) : Component(component::Type::Icon), source_(source) {
}

Icon::~Icon() {
}

std::string_view Icon::source() const {
    return source_;
}

v3d::render::realtime::TextureHandle Icon::texture() const noexcept {
    return texture_;
}

void Icon::texture(const v3d::render::realtime::TextureHandle& tex) noexcept {
    texture_ = tex;
}

};  // end namespace v3d::ui::component
