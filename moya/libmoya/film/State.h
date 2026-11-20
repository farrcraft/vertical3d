/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Covering.h>
#include <moya/libmoya/film/DeepPoint.h>

#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::moya::film {

/**
 * A film's pixels exactly as its device holds them, unresolved, and which pixels have stopped. A
 * film restored from it continues as the film it was taken from would have.
 **/
struct State {
    glm::uvec2 resolution { 0 };

    /** The passes it holds the samples of. **/
    std::uint32_t passes { 0 };

    /** The film's channels, in order. **/
    std::vector<Channel> channels;

    /**
     * The floats each pixel holds: the beauty's three and its alpha, each channel's, then the
     * floats that kernel::film::Pixels lays after the channels.
     **/
    std::uint32_t stored { 0 };

    /** Every pixel's floats, row by row from the top left. **/
    std::vector<float> sums;

    /** Every pixel's flag, row by row from the top left, which is not zero once it has stopped. **/
    std::vector<std::uint32_t> stopped;

    /**
     * Every pixel's Cryptomatte coverage, row by row from the top left, or none for a render of no
     * Cryptomatte layer.
     **/
    std::vector<std::vector<Covering>> coverage;

    /** Every pixel's deep list, row by row from the top left, or none without a deep file. **/
    std::vector<std::vector<DeepPoint>> deep;
};

};  // namespace v3d::moya::film
