/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <vector>

namespace v3d::moya::texture {

/** One level of a texture: each texel's channels in turn, row after row from its top. **/
struct Level {
    std::uint32_t width { 0 };
    std::uint32_t height { 0 };
    std::vector<float> values;
};

};  // namespace v3d::moya::texture
