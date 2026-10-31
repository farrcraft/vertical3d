/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Motions.h"

#include <algorithm>
#include <array>
#include <cstddef>

#include <glm/mat4x4.hpp>

#include "Placement.h"

namespace v3d::moya::reyes {

Motions::Motions(const Placement & placement, unsigned int perPixel, const std::array<int, 4> & region) :
    placement_(placement), perPixel_(perPixel), left_(region[0]), top_(region[1]),
    width_(std::max(0, region[2] - region[0] + 1)), height_(std::max(0, region[3] - region[1] + 1)),
    values_(static_cast<std::size_t>(width_) * static_cast<std::size_t>(height_) * perPixel),
    known_(values_.size(), false) {
}

const glm::mat4x4 & Motions::at(const Key & key) {
    const int column = key.column;
    const int row = key.row;
    const unsigned int index = key.index;
    const float time = key.time;
    const std::size_t slot = (((static_cast<std::size_t>(row - top_) * static_cast<std::size_t>(width_)) +
        static_cast<std::size_t>(column - left_)) * perPixel_) + index;
    if (!known_[slot]) {
        values_[slot] = placement_.delta(time);
        known_[slot] = true;
    }
    return values_[slot];
}

};  // namespace v3d::moya::reyes
