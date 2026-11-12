/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Model.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>

namespace v3d::moya::kernel::bsdf {

/**
 * One lobe of a BSDF: its model, its weight at the path's wavelengths, and the estimate of its
 * albedo that sampling picks it by.
 **/
struct Closure {
    Model model { Model::Diffuse };
    spectrum::SampledSpectrum weight { 0.0F };
    float albedo { 0.0F };
};

};  // namespace v3d::moya::kernel::bsdf
