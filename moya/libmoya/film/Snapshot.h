/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/film/Channel.h>

#include <cstdint>
#include <string>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::moya::film {

/**
 * A film's pixels as they stood after some number of passes, with each sum divided by its pixel's
 * weight. A pixel no sample reached is zero.
 **/
struct Snapshot {
    glm::uvec2 resolution { 0 };

    /** The passes it holds the samples of. **/
    std::uint32_t passes { 0 };

    /** The floats each pixel holds: the beauty's three, then each channel's, in order. **/
    std::uint32_t stride { 0 };

    /** The film's channels, in order, and where each one's first float sits in a pixel. **/
    std::vector<Channel> channels;
    std::vector<std::uint32_t> offsets;

    /** Every pixel's floats, row by row from the top left. **/
    std::vector<float> values;

    /**
     * Every pixel's standard error of its beauty's luminance, row by row from the top left. It
     * treats the samples as independent, so it overstates the error of stratified ones. It is
     * infinite for a pixel whose samples weigh less than two of equal weight.
     **/
    std::vector<float> errors;

    /** Every pixel's count of samples, row by row from the top left. **/
    std::vector<std::uint32_t> samples;

    /** A pixel's standard error. **/
    float error(std::uint32_t x, std::uint32_t y) const;

    /** A pixel's count of samples. **/
    std::uint32_t count(std::uint32_t x, std::uint32_t y) const;

    /** A pixel's float at an offset: 0 to 2 for the beauty, or offset() for a channel. **/
    float at(std::uint32_t x, std::uint32_t y, std::uint32_t offset) const;

    /**
     * Where a channel's first float sits in a pixel, as Film::offset() gives it.
     *
     * @throws std::out_of_range when the snapshot has no channel of that name
     **/
    std::uint32_t offset(const std::string & name) const;
};

};  // namespace v3d::moya::film
