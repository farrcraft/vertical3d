/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampling.h>

#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya {

/**
 * Every sample of the frame, and the nearest micropolygon hidden at each so far.
 *
 * A reyes hider resolves visibility per sample, and a sample is not finished until every
 * grid that could reach it has been hidden. moya's sweep comes round again when a split
 * lands behind it, so no bucket is finished until the sweep is: the store is the whole
 * image's, and it goes through the film once, after the last bucket, per ADR-0076.
 **/
class Samples final {
 public:
    class Sample {
     public:
        /** In raster space, in pixels from the image's upper left. **/
        glm::vec2 raster { 0.0f, 0.0f };
        /** When, between the shutter opening and closing. **/
        float time { 0.0f };
        /** A point on the unit disc, which the lens scales by its radius. **/
        glm::vec2 lens { 0.0f, 0.0f };
        glm::vec3 colour { 0.0f, 0.0f, 0.0f };
        glm::vec3 opacity { 0.0f, 0.0f, 0.0f };
        float depth { 0.0f };
        bool hit { false };
    };

    /**
     * Places every pixel's samples where offline::Sampler puts them, with nothing hidden.
     **/
    Samples(unsigned int width, unsigned int height, const v3d::render::offline::Sampling & sampling);

    unsigned int width() const;
    unsigned int height() const;
    /** How many samples each pixel has. **/
    unsigned int perPixel() const;

    /**
     * @param index which of the pixel's samples, below perPixel()
     **/
    Sample & at(unsigned int column, unsigned int row, unsigned int index);

    /**
     * Filters every sample into the planes: colour into the three from red, coverage and
     * the nearest depth into theirs. A pixel no sample hit keeps the depth its plane held.
     **/
    void resolve(v3d::render::offline::FrameBuffer * planes, unsigned int red, unsigned int coverage,
        unsigned int depth) const;

 private:
    unsigned int width_;
    unsigned int height_;
    unsigned int perPixel_;
    v3d::render::offline::Sampling sampling_;
    std::vector<Sample> samples_;
};

};  // namespace v3d::moya
