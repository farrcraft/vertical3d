/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Image.h"

#include <string>

namespace v3d::ui::style::property {

Image::Image(const Named& named) : Property(named.name), source_(named.source) {
}

Image::~Image() {
}

const v3d::ui::Image& Image::image() const noexcept {
    return image_;
}

void Image::image(const v3d::ui::Image& resolved) noexcept {
    image_ = resolved;
}

std::string_view Image::source() const {
    return source_;
}

};  // namespace v3d::ui::style::property
