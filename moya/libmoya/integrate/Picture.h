/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::moya::integrate {

/**
 * The film's beauty as a display shows it, made while a session samples. It has the film's
 * resolution, which is the crop window's, at 8 bits a channel.
 **/
struct Picture {
    /** The scene's version it shows. **/
    std::uint64_t version { 0 };

    /**
     * The samples each pixel that still samples holds. A pixel that stopped holds fewer. A coarse
     * picture holds one sample of each block.
     **/
    std::uint32_t samples { 0 };

    /** The samples each pixel holds once the version has every sample it takes. **/
    std::uint32_t total { 0 };

    /**
     * The side of the square of pixels each sample stands for. It is one once the picture has
     * the film's own resolution, and a power of two above one for a coarse picture.
     **/
    std::uint32_t block { 1 };

    /** The pixels it holds, and the image they are a window of, as a film::Snapshot names them. **/
    glm::uvec2 resolution { 0U };
    glm::uvec2 image { 0U };
    glm::uvec2 origin { 0U };

    /** Red, green and blue for each pixel, row by row from the top left, as output::displayed(). **/
    std::vector<std::uint8_t> rgb;

    /** The beauty denoised, as rgb shows the beauty, or empty for a picture not denoised. **/
    std::vector<std::uint8_t> denoised;

    /**
     * The albedo that guides the denoiser, as rgb shows the beauty, and the normal, each
     * component from -1 to 1 as 0 to 255 with no display transform. Each is empty for a picture
     * that does not show the guides.
     **/
    std::vector<std::uint8_t> albedo;
    std::vector<std::uint8_t> normal;
};

};  // namespace v3d::moya::integrate
