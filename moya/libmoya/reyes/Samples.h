/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampling.h>

#include <limits>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::reyes {

/**
 * Every sample of the frame, and the nearest micropolygon hidden at each so far.
 *
 * Visibility is resolved per sample. A sample is finished only when every grid that could
 * reach it has been hidden. A split can put a piece in a bucket already rendered, and the
 * buckets are then all rendered again, so no bucket is finished before the last round. The
 * samples therefore cover the whole image, and go through the film once, after that round.
 **/
class Samples final {
 public:
    /** One surface a sample sees: its Ci, which is premultiplied, its Oi and its depth. **/
    class Layer {
     public:
        glm::vec3 colour { 0.0F, 0.0F, 0.0F };
        glm::vec3 opacity { 1.0F, 1.0F, 1.0F };
        float depth { 0.0F };
        /** Which grid it came from, as surface() numbered it. **/
        unsigned int surface { 0 };
    };

    /** The surfaces one sample sees. **/
    class Layers {
     public:
        /**
         * Keeps a surface the sample sees, unless an opaque one is already as near or nearer.
         * An opaque surface drops every layer at its depth or behind it. A sample keeps one
         * layer from each grid, the nearest, because two micropolygons of one grid that share
         * an edge both cover a sample on that edge.
         **/
        void add(const Layer & layer);
        bool hit() const;
        /** Every surface it keeps, in the order they arrived. **/
        const std::vector<Layer> & all() const;
        /** The depth of the nearest opaque surface, or the largest float when there is none. **/
        float opaque() const;

     private:
        std::vector<Layer> layers_;
        float opaque_ { std::numeric_limits<float>::max() };
    };

    class Sample {
     public:
        /** In raster space, in pixels from the image's upper left. **/
        glm::vec2 raster { 0.0F, 0.0F };
        /** When, between the shutter opening and closing. **/
        float time { 0.0F };
        /** A point on the unit disc, which the lens scales by its radius. **/
        glm::vec2 lens { 0.0F, 0.0F };
        Layers layers;
    };

    /**
     * Places every pixel's samples where offline::Sampler puts them, with nothing hidden yet.
     **/
    Samples(const glm::uvec2 & size, const v3d::render::offline::Sampling & sampling);

    unsigned int width() const;
    unsigned int height() const;
    /** How many samples each pixel has. **/
    unsigned int perPixel() const;

    /**
     * @param index which of the pixel's samples, below perPixel()
     **/
    Sample & at(unsigned int column, unsigned int row, unsigned int index);

    /**
     * A number for the next grid to be hidden, so a sample can tell its micropolygons apart
     * from another grid's.
     **/
    unsigned int surface();

    /**
     * Filters every sample into the planes: colour into the three from red, coverage and
     * the nearest depth into theirs. A pixel no sample hit keeps the depth its plane held.
     *
     * A sample's layers are composited front to back first, as a ray composites what it
     * passes through: each lets through what the nearer ones did not stop, scaled by
     * `1 - Oi`. Its depth is its nearest surface's.
     **/
    void resolve(v3d::render::offline::FrameBuffer * planes, unsigned int red, unsigned int coverage,
        unsigned int depth) const;

 private:
    unsigned int width_;
    unsigned int height_;
    unsigned int perPixel_;
    v3d::render::offline::Sampling sampling_;
    std::vector<Sample> samples_;
    unsigned int surfaces_ = 0;
};

};  // namespace v3d::moya::reyes
