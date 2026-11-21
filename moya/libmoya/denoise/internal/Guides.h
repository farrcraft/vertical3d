/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <vector>

namespace v3d::moya::denoise::internal {

/** A snapshot's albedo and normal, three floats a pixel, each empty when its film holds none. **/
struct Guides {
    std::vector<float> albedo;
    std::vector<float> normal;

    /** Whether the guides are given to a filter, which they are not once cleaning them failed. **/
    bool usable { true };

    /** Whether they were cleaned before the filters that read them. **/
    bool clean { false };
};

};  // namespace v3d::moya::denoise::internal
