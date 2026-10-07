/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Footprint.h"

#include <algorithm>
#include <cmath>

namespace v3d::moya {

std::array<int, 4> footprint(const glm::vec3 & min, const glm::vec3 & max, int columns, int rows) {
    const std::array<int, 4> none = { 0, 0, -1, -1 };
    if (std::isnan(min.x) || std::isnan(min.y) || std::isnan(max.x) || std::isnan(max.y)) {
        return none;
    }
    if (columns < 1 || rows < 1) {
        return none;
    }
    // the pixel a coordinate falls in, held within one pixel of the frame before it is made an
    // integer
    const auto pixel = [](float coordinate, int count) {
        const float held = std::clamp(std::floor(coordinate), -1.0f, static_cast<float>(count));
        return static_cast<int>(held);  // checked: not NaN above, and held within [-1, count]
    };
    // a sample may be anywhere in its pixel, so every pixel the bound touches
    return {
        std::max(0, pixel(min.x, columns)),
        std::max(0, pixel(min.y, rows)),
        std::min(columns - 1, pixel(max.x, columns)),
        std::min(rows - 1, pixel(max.y, rows))
    };
}

unsigned int shutterSlice(float time, const glm::vec2 & shutter, unsigned int slices) {
    if (slices == 0) {
        return 0;
    }
    // in doubles, which hold every count of slices exactly
    const double span = static_cast<double>(shutter.y) - static_cast<double>(shutter.x);
    const double along = span > 0.0 ? (static_cast<double>(time) - shutter.x) / span * slices : 0.0;
    // held within the slices before it is made an integer. A NaN fails the test and takes the first
    const double held = along >= 0.0 ? std::min(along, static_cast<double>(slices - 1)) : 0.0;
    return static_cast<unsigned int>(held);  // checked: held is in [0, slices - 1]
}

};  // namespace v3d::moya
