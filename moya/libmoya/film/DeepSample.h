/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::film {

/**
 * One sample of a pixel's deep list, as a deep file holds it: a span along the camera ray from
 * its depth to its back, and its colour in ACEScg premultiplied by its alpha. A surface's sample
 * is a point, whose back is its depth. It names its object's and material's IDs, or zero for
 * none. Composited front to back over each other, a pixel's samples give its flat beauty and
 * alpha.
 **/
struct DeepSample {
    float depth { 0.0F };
    float red { 0.0F };
    float green { 0.0F };
    float blue { 0.0F };
    float alpha { 0.0F };
    std::uint32_t object { 0 };
    std::uint32_t material { 0 };
    float back { 0.0F };
};

};  // namespace v3d::moya::film
