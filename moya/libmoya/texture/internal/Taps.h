/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <vector>

namespace v3d::moya::texture::internal {

/**
 * What one texel of the level being made reads along one axis of the level above. The first
 * texel's place is given, and then each texel's offset from the made texel's centre, in texels
 * of the level being made.
 **/
struct Taps {
    std::int64_t first { 0 };
    std::vector<float> offsets;

    /** The texel of the level above the centre lies in, for a sum whose weights total zero. **/
    std::int64_t nearest { 0 };
};

};  // namespace v3d::moya::texture::internal
