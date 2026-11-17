/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::volume {

/**
 * A direction drawn from a medium's phase function, the value of the phase function along it at
 * each wavelength, and the density it was drawn with. A density of zero draws nothing.
 **/
struct PhaseSample {
    glm::vec3 incoming { 0.0F };
    spectrum::SampledSpectrum value;
    float density { 0.0F };

    MOYA_KERNEL bool found() const {
        return density > 0.0F;
    }
};

};  // namespace v3d::moya::kernel::volume
