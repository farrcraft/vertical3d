/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "FrameBuffer.h"

#include <api/render/offline/Units.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::offline {

FrameBuffer::FrameBuffer(unsigned int width, unsigned int height, unsigned int planes) :
    planes_(planes, plane_t(height, std::vector<float>(width, 0.0f))),
    width_(width),
    height_(height) {
}

unsigned int FrameBuffer::width() const {
    return width_;
}

unsigned int FrameBuffer::height() const {
    return height_;
}

unsigned int FrameBuffer::planes() const {
    return static_cast<unsigned int>(planes_.size());
}

float FrameBuffer::value(unsigned int plane, unsigned int column, unsigned int row) const {
    assert(plane < planes() && column < width_ && row < height_);
    return planes_[plane][row][column];
}

void FrameBuffer::value(unsigned int plane, unsigned int column, unsigned int row, float v) {
    assert(plane < planes() && column < width_ && row < height_);
    planes_[plane][row][column] = v;
}

void FrameBuffer::clear(const Fill & fill) {
    assert(fill.plane < planes());
    for (auto & row : planes_[fill.plane]) {
        std::fill(row.begin(), row.end(), fill.value);
    }
}

boost::shared_ptr<v3d::image::Image> FrameBuffer::image(unsigned int channels) const {
    assert(channels <= planes());
    assert(channels == static_cast<unsigned int>(v3d::image::Image::Format::RGB) ||
           channels == static_cast<unsigned int>(v3d::image::Image::Format::RGBA));

    auto bpp = static_cast<uint8_t>(channels * 8);
    auto image = boost::make_shared<v3d::image::Image>(glm::uvec2(width_, height_), bpp);
    unsigned char* data = image->data();
    for (unsigned int row = 0; row < height_; row++) {
        for (unsigned int column = 0; column < width_; column++) {
            // an image pixel is `channels` bytes wide, so the plane coordinate has to be
            // scaled by that to reach the pixel rather than landing on its neighbours
            const unsigned int index = ((row * width_) + column) * channels;
            for (unsigned int i = 0; i < channels; i++) {
                // a value that is not a number is black, since converting it to a byte is
                // undefined; the rest is clamped and truncated
                const float stored = planes_[i][row][column];
                const float v = std::isnan(stored) ? 0.0f : std::clamp(stored, 0.0f, 1.0f);
                data[index + i] = static_cast<unsigned char>(v * BYTE_SCALE);
            }
        }
    }
    return image;
}

};  // namespace v3d::render::offline
