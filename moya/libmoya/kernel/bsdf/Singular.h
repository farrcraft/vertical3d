/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/spectrum/Spectrum.h>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf {

/**
 * A direction a singular closure scatters into, what it carries along it with no cosine, and the
 * chance its sample takes that direction rather than another of the closure's.
 **/
struct Singular {
    glm::vec3 direction { 0.0F, 0.0F, 1.0F };
    spectrum::SampledSpectrum value { 1.0F };
    float chance { 1.0F };
};

};  // namespace v3d::moya::kernel::bsdf
