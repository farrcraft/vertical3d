/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Sampling.h>

#include <vector>

#include <glm/vec2.hpp>

namespace v3d::render::offline {

/**
 * Where and when a pixel is sampled, per ADR-0076.
 *
 * A pixel's samples are drawn from a generator seeded by its column and row, so they are the
 * same on every run and whatever order pixels or buckets are asked for in.
 **/
class Sampler final {
 public:
    /**
     * One sample of a pixel.
     **/
    struct Sample {
        /** The position in raster space, in pixels from the image's upper left. **/
        glm::vec2 raster { 0.0f, 0.0f };
        /** When, between the shutter opening and closing. **/
        float time { 0.0f };
        /** A point on the unit disc, which the lens scales by its radius. **/
        glm::vec2 lens { 0.0f, 0.0f };
    };

    explicit Sampler(const Sampling & sampling);

    /**
     * The samples of one pixel, stratified over a grid the size PixelSamples asked for and
     * jittered within each cell.
     *
     * An axis with one stratum is sampled at the pixel centre rather than jittered, so one
     * sample a pixel is exactly the pixel centre.
     *
     * @param pass which set of samples; a pixel asked for more than once is given a different
     *        set each time, and the same set for the same pass
     **/
    std::vector<Sample> pixel(unsigned int column, unsigned int row, unsigned int pass = 0) const;

 private:
    Sampling sampling_;
};

};  // namespace v3d::render::offline
