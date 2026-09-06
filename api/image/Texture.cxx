/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Texture.h"

#include <cassert>
#include <iostream>

namespace v3d::image {
Texture::Texture() : type_(UNKNOWN), texID_(0), wrap_(false), width_(0), height_(0) {
}

// the image and the dimensions come across too - a copy that kept the id but reported
// itself as an empty 0x0 texture was no use to anything holding it
Texture::Texture(const Texture& t) :
    type_(t.type_), texID_(t.texID_), wrap_(t.wrap_), width_(t.width_), height_(t.height_), image_(t.image_) {
}

Texture::Texture(boost::shared_ptr<Image> image) : wrap_(false) {
    bool ok;
    ok = create(image);
    assert(ok);
}

boost::shared_ptr<Image> Texture::image(void) const {
    return image_;
}

void Texture::release(void) {
    boost::shared_ptr<Image> empty_ptr;
    image_ = empty_ptr;
    // isnull() reads the type rather than the image, so releasing has to clear it or the
    // texture goes on claiming it has something to draw
    type_ = UNKNOWN;
    width_ = 0;
    height_ = 0;
}

Texture& Texture::operator = (const Texture& t) {
    type_ = t.type_;
    texID_ = t.texID_;
    wrap_ = t.wrap_;
    width_ = t.width_;
    height_ = t.height_;
    image_ = t.image_;

    return *this;
}

bool Texture::operator == (const Texture& t) const {
    return texID_ == t.texID_;
}

Texture::~Texture() {
}

unsigned int Texture::width(void) const {
    return width_;
}

unsigned int Texture::height(void) const {
    return height_;
}

bool Texture::isnull(void) {
    return (type_ == UNKNOWN);
}

unsigned int Texture::id(void) const {
    return texID_;
}

void Texture::id(unsigned int id) {
    texID_ = id;
}

bool Texture::wrap(void) const {
    return wrap_;
}

void Texture::wrap(bool repeat) {
    wrap_ = repeat;
}

bool Texture::create(boost::shared_ptr<Image> image) {
    type_ = LINEAR;

    image_ = image;
    width_ = image->width();
    height_ = image->height();

    return true;
}

};  // namespace v3d::image
