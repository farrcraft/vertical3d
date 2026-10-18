/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Image.h"

#include <cassert>
#include <cstdint>
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

Image::Image(const glm::uvec2& dimensions, uint8_t bitsPerPixel) : width_(dimensions.x), height_(dimensions.y), bpp_(bitsPerPixel), format_(formatOf(bitsPerPixel)) {
    const unsigned int bytesPerPixel = bpp_ / 8;
    const uint64_t size = static_cast<uint64_t>(width_) * height_ * bytesPerPixel;
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

unsigned char& Image::operator[] (unsigned int index) {
    assert(index < (width_* height_* (bpp_ / 8)));
    assert(data_ != 0);
    return data_[index];
}

unsigned char Image::operator[] (unsigned int index) const {
    assert(index < (width_* height_* (bpp_ / 8)));
    assert(data_ != 0);
    return data_[index];
}

};  // namespace v3d::image
