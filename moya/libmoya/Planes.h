/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/FrameBuffer.h>

#include <cstdint>

#include <boost/shared_ptr.hpp>

namespace v3d::moya {

/**
 * The image planes a render writes, which either hider fills and the film resolves samples
 * into.
 **/
class Planes final {
 public:
    /**
     * The planes in order. The colour planes come first, because image() takes its channels
     * from the leading planes. DEPTH follows them and is not an alpha channel, so the picture
     * is written with CHANNELS planes and never four.
     **/
    enum Plane : std::uint8_t {
        RED = 0,
        GREEN = 1,
        BLUE = 2,
        DEPTH = 3,
        /**
         * How much of the pixel was drawn into: the filtered fraction of its samples that hit.
         * An imager reads it as SL's alpha, to tell a pixel nothing was drawn into from a
         * black one.
         **/
        COVERAGE = 4
    };
    /** How many of the planes are the picture. **/
    static constexpr unsigned int CHANNELS = 3;

    /**
     * An empty picture of a size. Its depth plane holds the farthest depth, because the film
     * writes a depth only into a pixel some sample hit.
     **/
    static boost::shared_ptr<v3d::render::offline::FrameBuffer> make(unsigned int width, unsigned int height);
};

};  // namespace v3d::moya
