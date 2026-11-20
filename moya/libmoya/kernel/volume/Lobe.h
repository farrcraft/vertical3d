/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::volume {

/**
 * One lobe of a medium's scattering: its coefficient in each ACEScg channel, and the anisotropy
 * of Henyey and Greenstein's phase function it scatters by, from -1 back towards the light to 1
 * onwards along it.
 **/
struct Lobe {
    glm::vec3 scattering { 0.0F };
    float anisotropy { 0.0F };
};

};  // namespace v3d::moya::kernel::volume
