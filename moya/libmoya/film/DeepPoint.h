/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

namespace v3d::moya::film {

/**
 * One entry of a pixel's deep list before it is resolved, as a checkpoint keeps it. It holds the
 * depth of its first sample, and its object's and material's IDs. It holds the filter weights of
 * the samples joined into it, and their weighted radiance in ACEScg. An entry whose object is
 * kWhole holds the weight of every sample the pixel took, and nothing else.
 **/
struct DeepPoint {
    /** The object of the entry that holds a pixel's whole weight. **/
    static constexpr std::uint32_t kWhole = 0xFFFFFFFFU;

    float depth { 0.0F };
    std::uint32_t object { 0 };
    std::uint32_t material { 0 };
    double weight { 0.0 };
    double red { 0.0 };
    double green { 0.0 };
    double blue { 0.0 };
};

};  // namespace v3d::moya::film
