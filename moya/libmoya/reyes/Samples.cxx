/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Samples.h"

#include <api/render/offline/Film.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampler.h>
#include <api/render/offline/Sampling.h>

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <vector>

namespace v3d::moya::reyes {

Samples::Samples(const glm::uvec2 & size, const v3d::render::offline::Sampling & sampling) :
    width_(size.x), height_(size.y), perPixel_(0), sampling_(sampling) {
    const unsigned int width = size.x;
    const unsigned int height = size.y;
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

namespace {

bool solid(const glm::vec3 & opacity) {
    return opacity.r >= 1.0F && opacity.g >= 1.0F && opacity.b >= 1.0F;
}

};  // namespace

void Samples::Layers::add(const Layer & layer) {
    if (layer.depth >= opaque_) {
        return;
    }
    for (Layer & kept : layers_) {
        if (kept.surface != layer.surface) {
            continue;
        }
        if (layer.depth >= kept.depth) {
            return;
        }
        // the same grid nearer: this replaces the one it kept, wherever that was
        kept = layers_.back();
        layers_.pop_back();
        break;
    }
    if (solid(layer.opacity)) {
        opaque_ = layer.depth;
        std::erase_if(layers_, [&layer](const Layer & kept) { return kept.depth >= layer.depth; });
    }
    layers_.push_back(layer);
}

bool Samples::Layers::hit() const {
    return !layers_.empty();
}

const std::vector<Samples::Layer> & Samples::Layers::all() const {
    return layers_;
}

float Samples::Layers::opaque() const {
    return opaque_;
}

unsigned int Samples::surface() {
    return surfaces_++;
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
    return samples_[(((static_cast<std::size_t>(row) * width_) + column) * perPixel_) + index];
}

void Samples::resolve(v3d::render::offline::FrameBuffer * planes, unsigned int red, unsigned int coverage,
    unsigned int depth) const {
    v3d::render::offline::Film film(width_, height_, sampling_);
    std::vector<Layer> sorted;
    for (const Sample & hidden : samples_) {
        v3d::render::offline::Film::Sample sample;
        sample.raster = hidden.raster;
        sample.hit = hidden.layers.hit();
        if (sample.hit) {
            sorted = hidden.layers.all();
            std::stable_sort(sorted.begin(), sorted.end(), [](const Layer & front, const Layer & back) {
                return front.depth < back.depth;
            });
            glm::vec3 colour(0.0F);
            glm::vec3 opacity(0.0F);
            for (const Layer & layer : sorted) {
                colour += (glm::vec3(1.0F) - opacity) * layer.colour;
                opacity += (glm::vec3(1.0F) - opacity) * layer.opacity;
            }
            sample.colour = colour;
            sample.opacity = opacity;
            sample.depth = sorted.front().depth;
        }
        film.add(sample);
    }
    film.resolve(planes, red, coverage, static_cast<int>(depth));
}

};  // namespace v3d::moya::reyes
