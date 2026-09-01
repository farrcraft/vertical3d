/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Image.h"

namespace v3d::ui::style::prop {

Image::Image(const std::string& name, const std::string& src) : Property(name), source_(src) {
}

Image::~Image() {
}

v3d::render::realtime::TextureHandle Image::texture() const noexcept {
    return texture_;
}

void Image::texture(const v3d::render::realtime::TextureHandle& tex) noexcept {
    texture_ = tex;
}

std::string_view Image::source() const {
    return source_;
}

};  // namespace v3d::ui::style::prop
