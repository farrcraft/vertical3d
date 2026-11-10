/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Diffuse.h>
#include <moya/libmoya/kernel/bsdf/Lobe.h>
#include <moya/libmoya/kernel/bsdf/Sample.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/surface/Frame.h>

#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::bsdf {

/** The model a BSDF holds. Kernel code has no virtual call, so a BSDF switches over it. **/
enum class Model : std::uint8_t {
    Diffuse
};

/**
 * How a shading point scatters light. Directions are in world space and point away from the
 * surface: outgoing towards where the light goes, incoming towards where it comes from. The model
 * works in the frame around the shading normal.
 *
 * A reflection that would cross the geometric surface is refused: a direction on the other side
 * of it from outgoing has no value, no density and no sample, whatever the shading normal says.
 **/
struct Bsdf {
    Model model { Model::Diffuse };
    Diffuse diffuse;
    surface::Frame frame;
    glm::vec3 geometric { 0.0F, 0.0F, 1.0F };

    /** The value for light coming in along incoming and leaving along outgoing. **/
    MOYA_KERNEL spectrum::SampledSpectrum evaluate(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        if (!sameSide(outgoing, incoming)) {
            return spectrum::SampledSpectrum(0.0F);
        }
        switch (model) {
            case Model::Diffuse:
                return diffuse.evaluate(frame.toLocal(outgoing), frame.toLocal(incoming));
        }
        return spectrum::SampledSpectrum(0.0F);
    }

    /** The density sample() draws incoming with, for outgoing. **/
    MOYA_KERNEL float density(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        if (!sameSide(outgoing, incoming)) {
            return 0.0F;
        }
        switch (model) {
            case Model::Diffuse:
                return Diffuse::density(frame.toLocal(outgoing), frame.toLocal(incoming));
        }
        return 0.0F;
    }

    /** A direction light comes in from for outgoing, drawn from two random numbers. **/
    MOYA_KERNEL Sample sample(const glm::vec3 & outgoing, const glm::vec2 & u) const {
        Sample out;
        const glm::vec3 local = frame.toLocal(outgoing);
        switch (model) {
            case Model::Diffuse: {
                const glm::vec3 drawn = Diffuse::sample(local, u);
                out.incoming = frame.toWorld(drawn);
                out.lobe = Lobe::Diffuse;
                if (sameSide(outgoing, out.incoming)) {
                    out.value = diffuse.evaluate(local, drawn);
                    out.density = Diffuse::density(local, drawn);
                }
                break;
            }
        }
        return out;
    }

 private:
    /** Whether two directions lie on one side of the geometric surface. **/
    MOYA_KERNEL bool sameSide(const glm::vec3 & outgoing, const glm::vec3 & incoming) const {
        return glm::dot(outgoing, geometric) * glm::dot(incoming, geometric) > 0.0F;
    }
};

};  // namespace v3d::moya::kernel::bsdf
