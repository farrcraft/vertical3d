/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>
#include <vector>

#include <glm/mat4x4.hpp>

#include "Placement.h"

namespace v3d::moya::reyes {

/**
 * The motion from a moving primitive's reference end to each sample's time, worked out once per
 * sample over a region of the frame. Every micropolygon of the grid whose swept bound reaches a
 * sample reads the same motion from here.
 */
class Motions final {
 public:
    /**
     * @param region the pixels it covers, as left, top, right and bottom, each inclusive
     **/
    Motions(const Placement & placement, unsigned int perPixel, const std::array<int, 4> & region);

    /** One sample of one pixel, and when it was taken. **/
    struct Key {
        int column = 0;
        int row = 0;
        unsigned int index = 0;
        float time = 0.0F;
    };

    const glm::mat4x4 & at(const Key & key);

 private:
    const Placement & placement_;
    unsigned int perPixel_;
    int left_;
    int top_;
    int width_;
    int height_;
    std::vector<glm::mat4x4> values_;
    std::vector<bool> known_;
};

};  // namespace v3d::moya::reyes
