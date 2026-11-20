/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/integrate/Trail.h>

#include <string>
#include <vector>

namespace v3d::moya::integrate {

/**
 * The events of each pixel's sample zero, which a render given one records, for inspecting what
 * paths do. Each pixel keeps at most kernel::integrate::kTrailSteps of them, and a render that
 * resumes past its first pass records none.
 **/
struct Trails {
    /** Each pixel's events in the order its path met them, row by row. **/
    std::vector<std::vector<kernel::integrate::Step>> pixels;

    /** The names of the light groups, by the label a light's event carries. **/
    std::vector<std::string> labels;
};

};  // namespace v3d::moya::integrate
