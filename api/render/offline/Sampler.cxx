/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Sampler.h"

#include <api/type/Random.h>

#include <cmath>
#include <cstdint>
#include <utility>
#include <vector>

namespace v3d::render::offline {

namespace {

/**
 * Shirley's concentric map from the unit square onto the unit disc, which keeps a stratified
 * square stratified on the disc.
 **/
glm::vec2 disc(const glm::vec2 & square) {
    const float pi = 3.14159265358979f;
    const glm::vec2 centred = square * 2.0f - glm::vec2(1.0f);
    if (centred.x == 0.0f && centred.y == 0.0f) {
        return glm::vec2(0.0f);
    }
    float radius = 0.0f;
    float angle = 0.0f;
    if (std::fabs(centred.x) > std::fabs(centred.y)) {
        radius = centred.x;
        angle = (pi / 4.0f) * (centred.y / centred.x);
    } else {
        radius = centred.y;
        angle = (pi / 2.0f) - (pi / 4.0f) * (centred.x / centred.y);
    }
    return glm::vec2(radius * std::cos(angle), radius * std::sin(angle));
}

/**
 * Where in a stratum a sample falls: its centre when the axis has one stratum, and anywhere
 * in it otherwise.
 **/
float jitter(unsigned int strata, v3d::type::Random * random) {
    return strata == 1 ? 0.5f : random->unit();
}

};  // namespace

Sampler::Sampler(const Sampling & sampling) : sampling_(sampling) {
}

std::vector<Sampler::Sample> Sampler::pixel(unsigned int column, unsigned int row, unsigned int pass) const {
    const unsigned int nx = sampling_.samples.x == 0 ? 1 : sampling_.samples.x;
    const unsigned int ny = sampling_.samples.y == 0 ? 1 : sampling_.samples.y;
    const unsigned int count = nx * ny;

    // splitmix64 mixes its seed, so the position alone is a good one; a pass is folded in by
    // the golden ratio constant so that each pass is a sequence of its own
    const uint64_t seed = ((static_cast<uint64_t>(row) << 32) | column) ^
        (static_cast<uint64_t>(pass) * 0x9E3779B97F4A7C15ull);
    v3d::type::Random random(seed);

    std::vector<Sample> samples(count);
    for (unsigned int j = 0; j < ny; j++) {
        for (unsigned int i = 0; i < nx; i++) {
            Sample & sample = samples[j * nx + i];
            sample.raster = glm::vec2(
                static_cast<float>(column) + (static_cast<float>(i) + jitter(nx, &random)) / static_cast<float>(nx),
                static_cast<float>(row) + (static_cast<float>(j) + jitter(ny, &random)) / static_cast<float>(ny));
        }
    }

    // time and the lens are stratified too, each over the samples in an order of its own, so
    // that a sample's position does not decide when it looks or where on the lens it looks from
    std::vector<unsigned int> times(count);
    std::vector<unsigned int> lenses(count);
    for (unsigned int k = 0; k < count; k++) {
        times[k] = k;
        lenses[k] = k;
    }
    for (unsigned int k = count; k > 1; k--) {
        std::swap(times[k - 1], times[random.below(k)]);
        std::swap(lenses[k - 1], lenses[random.below(k)]);
    }
    const float open = sampling_.shutter.x;
    const float close = sampling_.shutter.y;
    for (unsigned int k = 0; k < count; k++) {
        const float t = (static_cast<float>(times[k]) + jitter(count, &random)) / static_cast<float>(count);
        samples[k].time = open + (close - open) * t;
        const unsigned int across = lenses[k] % nx;
        const unsigned int down = lenses[k] / nx;
        const glm::vec2 square(
            (static_cast<float>(across) + jitter(nx, &random)) / static_cast<float>(nx),
            (static_cast<float>(down) + jitter(ny, &random)) / static_cast<float>(ny));
        samples[k].lens = disc(square);
    }
    return samples;
}

};  // namespace v3d::render::offline
