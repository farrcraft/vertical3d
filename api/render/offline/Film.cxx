/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Film.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace v3d::render::offline {

Film::Film(unsigned int width, unsigned int height, const Sampling & sampling) :
    width_(width), height_(height), filter_(sampling.filter), filterWidth_(sampling.width),
    pixels_(static_cast<std::size_t>(width) * height) {
}

unsigned int Film::width() const {
    return width_;
}

unsigned int Film::height() const {
    return height_;
}

const Film::Pixel & Film::at(unsigned int column, unsigned int row) const {
    if (column >= width_ || row >= height_) {
        throw std::out_of_range("a pixel outside the film");
    }
    return pixels_[static_cast<std::size_t>(row) * width_ + column];
}

void Film::add(const Sample & sample) {
    if (width_ == 0 || height_ == 0) {
        return;
    }
    // the pixels whose centres are within half the filter's width of the sample
    const glm::vec2 half = filterWidth_ * 0.5f;
    const float left = std::ceil(sample.raster.x - half.x - 0.5f);
    const float right = std::floor(sample.raster.x + half.x - 0.5f);
    const float top = std::ceil(sample.raster.y - half.y - 0.5f);
    const float bottom = std::floor(sample.raster.y + half.y - 0.5f);
    const int x0 = static_cast<int>(std::max(left, 0.0f));
    const int x1 = static_cast<int>(std::min(right, static_cast<float>(width_) - 1.0f));
    const int y0 = static_cast<int>(std::max(top, 0.0f));
    const int y1 = static_cast<int>(std::min(bottom, static_cast<float>(height_) - 1.0f));

    for (int row = y0; row <= y1; row++) {
        for (int column = x0; column <= x1; column++) {
            const glm::vec2 offset = sample.raster -
                glm::vec2(static_cast<float>(column) + 0.5f, static_cast<float>(row) + 0.5f);
            const double weight = filter(filter_, offset, filterWidth_);
            Pixel & pixel = pixels_[static_cast<std::size_t>(row) * width_ + static_cast<std::size_t>(column)];
            pixel.weight += weight;
            pixel.colour += glm::dvec3(sample.colour) * weight;
            pixel.opacity += glm::dvec3(sample.opacity) * weight;
            if (sample.hit) {
                pixel.coverage += weight;
            }
        }
    }

    // a depth belongs to the pixel the sample is in, and to no other
    if (!sample.hit || sample.raster.x < 0.0f || sample.raster.y < 0.0f) {
        return;
    }
    const unsigned int column = static_cast<unsigned int>(sample.raster.x);
    const unsigned int row = static_cast<unsigned int>(sample.raster.y);
    if (column >= width_ || row >= height_) {
        return;
    }
    Pixel & pixel = pixels_[static_cast<std::size_t>(row) * width_ + column];
    if (!pixel.hit || sample.depth < pixel.depth) {
        pixel.depth = sample.depth;
        pixel.hit = true;
    }
}

glm::vec3 Film::colour(unsigned int column, unsigned int row) const {
    const Pixel & pixel = at(column, row);
    // a filter with negative lobes can bring a weight to zero without a sample being absent
    if (std::fabs(pixel.weight) < 1.0e-12) {
        return glm::vec3(0.0f);
    }
    return glm::vec3(pixel.colour / pixel.weight);
}

glm::vec3 Film::opacity(unsigned int column, unsigned int row) const {
    const Pixel & pixel = at(column, row);
    if (std::fabs(pixel.weight) < 1.0e-12) {
        return glm::vec3(0.0f);
    }
    return glm::vec3(pixel.opacity / pixel.weight);
}

float Film::coverage(unsigned int column, unsigned int row) const {
    const Pixel & pixel = at(column, row);
    if (std::fabs(pixel.weight) < 1.0e-12) {
        return 0.0f;
    }
    return static_cast<float>(pixel.coverage / pixel.weight);
}

bool Film::depth(unsigned int column, unsigned int row, float * depth) const {
    const Pixel & pixel = at(column, row);
    if (!pixel.hit) {
        return false;
    }
    *depth = pixel.depth;
    return true;
}

void Film::resolve(FrameBuffer * frame, unsigned int colour, unsigned int coverage, int depth) const {
    for (unsigned int row = 0; row < height_ && row < frame->height(); row++) {
        for (unsigned int column = 0; column < width_ && column < frame->width(); column++) {
            const glm::vec3 c = this->colour(column, row);
            frame->value(colour, column, row, c.r);
            frame->value(colour + 1, column, row, c.g);
            frame->value(colour + 2, column, row, c.b);
            frame->value(coverage, column, row, this->coverage(column, row));
            float nearest = 0.0f;
            if (depth >= 0 && this->depth(column, row, &nearest)) {
                frame->value(static_cast<unsigned int>(depth), column, row, nearest);
            }
        }
    }
}

};  // namespace v3d::render::offline
