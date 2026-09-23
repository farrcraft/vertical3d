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

void Icon::source(const std::string& name) {
    if (name == source_) {
        return;
    }
    source_ = name;
    image_ = v3d::ui::Image();
}

const v3d::ui::Image& Icon::image() const noexcept {
    return image_;
}

void Icon::image(const v3d::ui::Image& resolved) noexcept {
    image_ = resolved;
}

};  // end namespace v3d::ui::component
