/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Image.h"

namespace v3d::ui {

Image::Image() noexcept : uv0(0.0F, 0.0F), uv1(1.0F, 1.0F) {
}

Image::Image(const v3d::render::realtime::TextureHandle& tex) noexcept :
    texture(tex), uv0(0.0F, 0.0F), uv1(1.0F, 1.0F) {
}

Image::Image(const v3d::render::realtime::TextureHandle& tex, const Corners& corners) noexcept :
    texture(tex), uv0(corners.min), uv1(corners.max) {
}

bool Image::valid() const noexcept {
    return texture.valid();
}

};  // namespace v3d::ui
