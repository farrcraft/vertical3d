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

namespace v3d::render::offline {

/**
 * Turns a renderer's samples into pixels. Both of moya's hiders write here, and the pixel filter
 * is applied here and nowhere else.
 *
 * A sample is filtered into every pixel whose centre is within the filter's width of it, as it
 * arrives, so the film holds a weighted sum per pixel rather than the samples. Its memory is a
 * frame buffer's, whatever the sample count.
 **/
class Film final {
 public:
    /**
     * What a renderer found at one sample.
     **/
    struct Sample {
        /** In raster space, in pixels from the image's upper left. **/
        glm::vec2 raster { 0.0f, 0.0f };
        /**
         * Premultiplied by its opacity, as Ci is. A miss's colour is filtered in too, and is
         * black unless a renderer has a background of its own.
         **/
        glm::vec3 colour { 0.0f, 0.0f, 0.0f };
        glm::vec3 opacity { 0.0f, 0.0f, 0.0f };
        float depth { 0.0f };
        /** Whether the sample hit a surface. Coverage counts these samples. **/
        bool hit { false };
    };

    /**
     * Every pixel starts with nothing in it.
     **/
    Film(unsigned int width, unsigned int height, const Sampling & sampling);

    unsigned int width() const;
    unsigned int height() const;

    void add(const Sample & sample);

    /**
     * The filtered colour, which is premultiplied: a pixel half covered by white over black
     * misses is half grey. Black where no sample reached.
     **/
    glm::vec3 colour(unsigned int column, unsigned int row) const;
    glm::vec3 opacity(unsigned int column, unsigned int row) const;
    /**
     * The filtered fraction of samples that hit. A coverage plane holds this value.
     **/
    float coverage(unsigned int column, unsigned int row) const;
    /**
     * The nearest hit among the samples inside the pixel itself. A depth is not filtered,
     * since a blend of two surfaces' depths is a depth neither of them is at.
     *
     * @return false, leaving depth untouched, when no sample inside the pixel hit
     **/
    bool depth(unsigned int column, unsigned int row, float * depth) const;

    /**
     * Writes the resolved pixels into a renderer's planes.
     *
     * @param colour the first of three planes the colour goes into
     * @param coverage the plane the coverage goes into
     * @param depth the plane the depth goes into, or negative for none; a pixel no sample
     *        hit keeps what the plane held
     **/
    void resolve(FrameBuffer * frame, unsigned int colour, unsigned int coverage, int depth = -1) const;

 private:
    class Pixel {
     public:
        glm::dvec3 colour { 0.0, 0.0, 0.0 };
        glm::dvec3 opacity { 0.0, 0.0, 0.0 };
        double coverage { 0.0 };
        double weight { 0.0 };
        float depth { 0.0f };
        bool hit { false };
    };

    const Pixel & at(unsigned int column, unsigned int row) const;

    unsigned int width_;
    unsigned int height_;
    Filter filter_;
    glm::vec2 filterWidth_;
    std::vector<Pixel> pixels_;
};

};  // namespace v3d::render::offline
