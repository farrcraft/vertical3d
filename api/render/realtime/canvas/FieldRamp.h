/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace v3d::render::realtime::canvas {

/**
 * Where an effect drawn from a distance field is whole and where it is gone, packed into a
 * vertex's glyph field for the shaders. A field's value is a distance from its glyph's edge,
 * from 0 to 1, with the edge at 0.5 and the inside above it. Coverage fades from the first
 * value to the second, and across one pixel centred between them when they are less than a
 * pixel apart.
 *
 * A ramp of 0 draws the glyph itself, across one pixel at its edge, so an effect's ramp is
 * never 0.
 *
 * @param whole the value at which the effect covers the whole pixel
 * @param gone the value at which it covers none of it, at most whole
 * @return whole in the low 16 bits and gone in the high 16, each as a fraction of 65535
 **/
inline uint32_t fieldRamp(float whole, float gone) {
    constexpr float MOST = 65535.0F;
    const auto pack = [](float value) {
        return static_cast<uint32_t>(std::lround(std::clamp(std::isnan(value) ? 0.5F : value, 0.0F, 1.0F) * MOST));  // checked: from 0 to 65535
    };
    const uint32_t low = std::max<uint32_t>(pack(whole), 1U);
    const uint32_t high = std::min(pack(gone), low);
    return low | (high << 16U);
}

};  // namespace v3d::render::realtime::canvas
