/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Image.h"

#include <cassert>
#include <algorithm>
#include <iostream>
#include <cstring>

namespace v3d::image {

namespace {

/**
 * @return the format of that many bits per pixel, or RGB for a depth no format describes.
 *         RGB is then a defined value but not necessarily a correct one, so a writer checks
 *         the format it receives rather than assuming one
 **/
Image::Format formatOf(uint8_t bpp) {
    switch (bpp / 8) {
    case 1:
        return Image::Format::Grey;
    case 4:
        return Image::Format::RGBA;
    default:
        return Image::Format::RGB;
    }
}

};  // namespace

Image::Image() : data_(0), width_(0), height_(0), bpp_(24), format_(Format::RGB) {
}

Image::Image(uint32_t w, uint32_t h, uint8_t b) : width_(w), height_(h), bpp_(b), format_(formatOf(b)) {
    unsigned int bytesPerPixel = bpp_ / 8;
    uint64_t size = static_cast<uint64_t>(width_) * height_ * bytesPerPixel;
    data_ = new unsigned char[size];
    memset(data_, 0, size);
}

Image::~Image() {
    delete[] data_;
}

unsigned char* Image::data() {
    return data_;
}

uint8_t Image::bpp() const {
    return bpp_;
}

uint32_t Image::width() const {
    return width_;
}

uint32_t Image::height() const {
    return height_;
}

Image::Format Image::format() const {
    return format_;
}

unsigned char& Image::operator[] (unsigned int i) {
    assert(i < (width_* height_* (bpp_ / 8)));
    assert(data_ != 0);
    return data_[i];
}

unsigned char Image::operator[] (unsigned int i) const {
    assert(i < (width_* height_* (bpp_ / 8)));
    assert(data_ != 0);
    return data_[i];
}

};  // namespace v3d::image
