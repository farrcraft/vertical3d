/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/DeepSample.h>

#include <cstdint>
#include <map>
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

    /**
     * The image the pixels are a window of, and the place of the window's top left pixel in it.
     * An image of zero is the snapshot's own resolution. A crop window renders a window of the
     * image, and a file is written with the image as its whole and the window as what it holds.
     **/
    glm::uvec2 image { 0 };
    glm::uvec2 origin { 0 };

    /** The passes it holds the samples of. **/
    std::uint32_t passes { 0 };

    /**
     * The floats each pixel holds: the beauty's three and its alpha, then each channel's. A
     * channel that keeps its closest sample holds that sample's depth after its components.
     **/
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

    /** Each Cryptomatte layer's manifest, as JSON, by the layer's name. **/
    std::map<std::string, std::string> manifests;

    /**
     * Every pixel's deep samples, front to back, row by row from the top left, or none for a
     * render that writes no deep file.
     **/
    std::vector<std::vector<DeepSample>> deep;

    /** The name of each object's and each material's ID the deep samples hold. **/
    std::map<std::uint32_t, std::string> deepObjects;
    std::map<std::uint32_t, std::string> deepMaterials;

    /** A pixel's standard error. **/
    float error(std::uint32_t x, std::uint32_t y) const;

    /** A pixel's count of samples. **/
    std::uint32_t count(std::uint32_t x, std::uint32_t y) const;

    /** A pixel's float at an offset: 0 to 3 for the beauty and alpha, or a channel's offset(). **/
    float at(std::uint32_t x, std::uint32_t y, std::uint32_t offset) const;

    /**
     * Where a channel's first float sits in a pixel, as Film::offset() gives it.
     *
     * @throws std::out_of_range when the snapshot has no channel of that name
     **/
    std::uint32_t offset(const std::string & name) const;
};

};  // namespace v3d::moya::film
