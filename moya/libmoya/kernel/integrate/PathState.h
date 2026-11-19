/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Cone.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/integrate/media/Media.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>

#include <cstdint>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/**
 * The fields of a path's state, declared once as a type and a name each. Every device lays the
 * state out from this list: the CPU as one structure per path, and a GPU as one array per field.
 * Stage code reaches a field through MOYA_PATH, which each layout defines, so it reads either.
 *
 * - pixel and sample name the camera sample the path is for.
 * - ray is the ray the path traces next, and cone how wide the area its camera sample covers is
 *   along that ray.
 * - throughput is what the path carries back to the camera from where it is.
 * - radiance is the light it has gathered so far.
 * - wavelengths are the four it carries, and weight is its pixel filter's weight.
 * - bounces counts its bounces, and each kind's count is kept apart for its depth limit. A
 *   scattering event in a medium is a volume bounce. subsurfaceBounces counts the events of walks
 *   below surfaces, which bounces counts too, so each event draws numbers of its own. A depth
 *   limit other than the walk's reads bounces less subsurfaceBounces.
 * - chances are the densities with which the path so far would have been tracked through media at
 *   each of its wavelengths, over a common scale, with an average of one. A path that crosses no
 *   medium keeps them at one. lightChances are the densities with which a light sample's shadow
 *   ray would have tracked the path through volumes since its last scattering, at each
 *   wavelength, starting from the chances there, with an average of one.
 * - density is the last BSDF or phase function sample's, which multiple importance sampling
 *   weighs a light by, times the chance the path then passed the media along its way. That chance
 *   is over the chance a shadow ray would have passed the volumes among them the same way. from and
 *   normal are the point that sample was drawn at and its shading normal, which is zero in a
 *   medium.
 *   linked is that shading point's set of linked lights. specular is one when that sample was
 *   specular, so no light sample could have drawn its direction.
 * - coverage is one once its camera ray meets anything, which the beauty's alpha adds.
 * - featured is one once the path has given a denoiser its albedo and normal, or can no longer.
 * - expression is the state of the render's light path expressions' automaton after the path's
 *   events so far.
 * - media are the closed surfaces the path is inside.
 **/
#define MOYA_PATH_FIELDS(FIELD)                                \
    FIELD(glm::uvec2, pixel)                                   \
    FIELD(std::uint32_t, sample)                               \
    FIELD(::v3d::moya::kernel::geometry::Ray, ray)             \
    FIELD(::v3d::moya::kernel::geometry::Cone, cone)           \
    FIELD(::v3d::moya::kernel::spectrum::SampledSpectrum, throughput) \
    FIELD(::v3d::moya::kernel::spectrum::SampledSpectrum, radiance)   \
    FIELD(::v3d::moya::kernel::spectrum::SampledWavelengths, wavelengths) \
    FIELD(float, weight)                                       \
    FIELD(std::uint32_t, bounces)                              \
    FIELD(std::uint32_t, diffuseBounces)                       \
    FIELD(std::uint32_t, glossyBounces)                        \
    FIELD(std::uint32_t, specularBounces)                      \
    FIELD(std::uint32_t, transmissionBounces)                  \
    FIELD(std::uint32_t, volumeBounces)                            FIELD(std::uint32_t, subsurfaceBounces)                    \
    FIELD(::v3d::moya::kernel::spectrum::SampledSpectrum, chances)    \
    FIELD(::v3d::moya::kernel::spectrum::SampledSpectrum, lightChances) \
    FIELD(float, density)                                      \
    FIELD(glm::vec3, from)                                     \
    FIELD(glm::vec3, normal)                                   \
    FIELD(std::uint32_t, linked)                               \
    FIELD(std::uint32_t, specular)                             \
    FIELD(float, coverage)                                     \
    FIELD(std::uint32_t, featured)                             \
    FIELD(std::uint32_t, expression)                           \
    FIELD(::v3d::moya::kernel::integrate::media::Media, media)

namespace v3d::moya::kernel::integrate {

/** A path's state as the CPU lays it out: one structure, on the thread that runs the path. **/
struct PathState {
#define MOYA_PATH_MEMBER(type, name) type name {};
    MOYA_PATH_FIELDS(MOYA_PATH_MEMBER)
#undef MOYA_PATH_MEMBER
};

};  // namespace v3d::moya::kernel::integrate

/** A field of a path's state, in the CPU's layout. **/
#define MOYA_PATH(state, field) ((state).field)

namespace v3d::moya::kernel::integrate {

/** The bounces a path's depth limits count: all of them but its walks' events. **/
MOYA_KERNEL inline std::uint32_t depthOf(const PathState & state) {
    return MOYA_PATH(state, bounces) - MOYA_PATH(state, subsurfaceBounces);
}

};  // namespace v3d::moya::kernel::integrate
