/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/spectrum/Spectrum.h>

#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::integrate::media {

/**
 * A volume a stretch of a ray crosses. origin and direction are the ray in the volume's own
 * space, over the same parameters as the ray, and toWorld turns that space into the world.
 * enter and exit are the parameters at which the ray enters and leaves the volume's bounds,
 * within the stretch. cellEnd is where the ray leaves the cell of the walked field it is in, and
 * majorant bounds the medium's extinction in that cell, at each wavelength.
 **/
struct Crossing {
    std::uint32_t volume { 0 };
    glm::vec3 origin { 0.0F };
    glm::vec3 direction { 0.0F };
    glm::mat4 toWorld { 1.0F };
    float enter { 0.0F };
    float exit { 0.0F };
    float cellEnd { 0.0F };
    spectrum::SampledSpectrum majorant { 0.0F };
};

};  // namespace v3d::moya::kernel::integrate::media
