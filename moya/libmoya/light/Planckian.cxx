/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/light/Planckian.h>

#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/spectrum/CieObserver.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>

#include <glm/vec3.hpp>

namespace v3d::moya::light {

Planckian planckian(float temperature) {
    if (!(temperature > 0.0F)) {
        return {};
    }
    glm::vec3 xyz { 0.0F };
    for (int row = 0; row < kernel::spectrum::kObserverRows; ++row) {
        const float lambda = static_cast<float>(kernel::spectrum::kObserverFirst + row);
        xyz += kernel::spectrum::observer(lambda) * kernel::spectrum::planck(lambda, temperature);
    }
    xyz /= kernel::spectrum::kYIntegral;
    if (!(xyz.y > 0.0F)) {
        return {};
    }
    return Planckian { .scale = 1.0F / xyz.y, .colour = kernel::spectrum::acescgFromXyz(xyz / xyz.y) };
}

};  // namespace v3d::moya::light
