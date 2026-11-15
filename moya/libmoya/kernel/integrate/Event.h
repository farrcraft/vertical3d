/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/bsdf/Lobe.h>
#include <moya/libmoya/kernel/bsdf/Sample.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

namespace v3d::moya::kernel::integrate {

/**
 * What happens at one vertex of a path, as OSL's light path expressions name it. A path starts at
 * the camera (C), and scatters by a reflection (R) or a transmission (T). It gathers the light of
 * a light (L) or of an emitting object (O).
 **/
enum class EventType : std::uint8_t {
    Camera,
    Reflection,
    Transmission,
    Light,
    Object
};

/**
 * How a reflection or a transmission scatters, as OSL's light path expressions name it: diffuse
 * (D), glossy (G) or singular (S). An event that does not scatter has none.
 **/
enum class Scattering : std::uint8_t {
    None,
    Diffuse,
    Glossy,
    Singular
};

/**
 * One event of a path: its type, how it scatters, and its label. A light's label is the place of
 * its light group's name among the render's labels, and every other event's is kNoLabel.
 **/
struct Event {
    EventType type { EventType::Camera };
    Scattering scattering { Scattering::None };
    std::uint32_t label { light::kNoLabel };
};

/** The event a bounce makes along a direction a BSDF sampled. **/
MOYA_KERNEL inline Event scatteredBy(const bsdf::Sample & drawn) {
    Event out;
    out.type = drawn.lobe == bsdf::Lobe::Transmission ? EventType::Transmission : EventType::Reflection;
    if (drawn.specular) {
        out.scattering = Scattering::Singular;
    } else {
        out.scattering = drawn.diffuse ? Scattering::Diffuse : Scattering::Glossy;
    }
    return out;
}

/**
 * The event of a light whose light a path gathers, with its group's label. A light an emitting
 * material made is an object.
 **/
MOYA_KERNEL inline Event litBy(const light::Light & light) {
    Event out;
    out.type = light.object ? EventType::Object : EventType::Light;
    out.label = light.label;
    return out;
}

};  // namespace v3d::moya::kernel::integrate
