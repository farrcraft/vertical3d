/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Samples.h"

#include <api/render/offline/Film.h>
#include <api/render/offline/Sampler.h>

#include <cassert>
#include <vector>

namespace v3d::moya {

Samples::Samples(unsigned int width, unsigned int height, const v3d::render::offline::Sampling & sampling) :
    width_(width), height_(height), perPixel_(0), sampling_(sampling) {
    const v3d::render::offline::Sampler sampler(sampling);
    for (unsigned int row = 0; row < height; row++) {
        for (unsigned int column = 0; column < width; column++) {
            const std::vector<v3d::render::offline::Sampler::Sample> placed = sampler.pixel(column, row);
            perPixel_ = static_cast<unsigned int>(placed.size());
            for (const v3d::render::offline::Sampler::Sample & at : placed) {
                Sample sample;
                sample.raster = at.raster;
                sample.time = at.time;
                sample.lens = at.lens;
                samples_.push_back(sample);
            }
        }
    }
}

unsigned int Samples::width() const {
    return width_;
}

unsigned int Samples::height() const {
    return height_;
}

unsigned int Samples::perPixel() const {
    return perPixel_;
}

Samples::Sample & Samples::at(unsigned int column, unsigned int row, unsigned int index) {
    assert(column < width_ && row < height_ && index < perPixel_);
    return samples_[(static_cast<std::size_t>(row) * width_ + column) * perPixel_ + index];
}

void Samples::resolve(v3d::render::offline::FrameBuffer * planes, unsigned int red, unsigned int coverage,
    unsigned int depth) const {
    v3d::render::offline::Film film(width_, height_, sampling_);
    for (const Sample & hidden : samples_) {
        v3d::render::offline::Film::Sample sample;
        sample.raster = hidden.raster;
        sample.hit = hidden.hit;
        if (hidden.hit) {
            sample.colour = hidden.colour;
            sample.opacity = hidden.opacity;
            sample.depth = hidden.depth;
        }
        film.add(sample);
    }
    film.resolve(planes, red, coverage, static_cast<int>(depth));
}

};  // namespace v3d::moya
