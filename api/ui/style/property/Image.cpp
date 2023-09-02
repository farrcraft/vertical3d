/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Image.h"

namespace v3d::ui::style::prop {

Image::Image(const std::string& name, const std::string& src) : Property(name), source_(src)
{
}

Image::~Image()
{
}

boost::shared_ptr<v3d::gl::GLTexture> Image::texture(void) const
{
    return texture_;
}

void Image::texture(boost::shared_ptr<v3d::gl::GLTexture> tex)
{
    texture_ = tex;
}

std::string_view Image::source() const
{
    return source_;
}

};  // namespace v3d::ui::style::prop
