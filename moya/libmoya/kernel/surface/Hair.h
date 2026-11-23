/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::surface {

/**
 * The parameters of MaterialX's `chiang_hair_bsdf`, by its names and with its defaults. Each
 * tint is a colour in ACEScg that scales one lobe. R's is the reflection off the cuticle, and TT's
 * the light that passes through the fibre. TRT's is the light reflected once inside it, and every
 * longer path. The index of refraction is the fibre's. Each roughness is a lobe's longitudinal
 * variance and its azimuthal scale, as `chiang_hair_roughness` gives them. The cuticle angle runs
 * from zero to one, and a half tilts the cuticle's scales by nothing. The absorption is a
 * coefficient in each channel, over the fibre's radius.
 **/
struct Hair {
    glm::vec3 tintR { 1.0F };
    glm::vec3 tintTT { 1.0F };
    glm::vec3 tintTRT { 1.0F };
    float ior { 1.55F };  // NOLINT(readability-magic-numbers) - MaterialX's default
    glm::vec2 roughnessR { 0.1F, 0.1F };  // NOLINT(readability-magic-numbers) - MaterialX's default
    glm::vec2 roughnessTT { 0.05F, 0.05F };  // NOLINT(readability-magic-numbers) - MaterialX's default
    glm::vec2 roughnessTRT { 0.2F, 0.2F };  // NOLINT(readability-magic-numbers) - MaterialX's default
    float cuticleAngle { 0.5F };  // NOLINT(readability-magic-numbers) - MaterialX's default
    glm::vec3 absorption { 0.0F };
};

};  // namespace v3d::moya::kernel::surface
