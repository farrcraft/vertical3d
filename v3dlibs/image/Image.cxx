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

    Image::Image() : data_(0), width_(0), height_(0), bpp_(24), format_(Format::RGB) {
    }

    Image::Image(uint32_t w, uint32_t h, uint8_t b) : width_(w), height_(h), bpp_(b) {
        unsigned int bytesPerPixel = bpp_ / 8;
        uint64_t size = width_ * height_ * bytesPerPixel;
        if (bytesPerPixel == 3) {
            format_ = Format::RGB;
        } else if (bytesPerPixel == 4) {
            format_ = Format::RGBA;
        }
        data_ = new unsigned char[size];
        memset(data_, 0, size);
    }

    Image::Image(uint64_t len) : width_(0), height_(0), bpp_(0) {
        data_ = new unsigned char[len];
        memset(data_, 0, len);
    }


    Image::~Image() {
        delete[] data_;
    }

    unsigned char* Image::data() {
        return data_;
    }

    unsigned int Image::bpp() const {
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

    void Image::bpp(unsigned int bits) {
        bpp_ = bits;
    }

    void Image::width(unsigned int w) {
        width_ = w;
    }

    void Image::height(unsigned int h) {
        height_ = h;
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
