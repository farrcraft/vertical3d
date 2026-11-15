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

/**
 * The symbols a light path expression's automaton reads, one for each event. The camera comes
 * first, then a reflection and a transmission of each scattering, diffuse, glossy and singular,
 * then an object, then a light in no group. A light in a group follows those, at kLabelled plus
 * its label.
 **/
inline constexpr std::uint32_t kCameraSymbol = 0;
inline constexpr std::uint32_t kReflectionSymbol = 1;
inline constexpr std::uint32_t kTransmissionSymbol = 4;
inline constexpr std::uint32_t kObjectSymbol = 7;
inline constexpr std::uint32_t kLightSymbol = 8;
inline constexpr std::uint32_t kLabelled = 9;

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

/** The count of symbols an automaton reads under a count of labels. **/
MOYA_KERNEL inline std::uint32_t symbolsOf(std::uint32_t labels) {
    return kLabelled + labels;
}

/**
 * The symbol of an event. A bounce of no scattering is read as a glossy one, and a light's label
 * is read only on a light.
 **/
MOYA_KERNEL inline std::uint32_t symbolOf(const Event & event) {
    const std::uint32_t scattered = event.scattering == Scattering::None ? 1U : static_cast<std::uint32_t>(event.scattering) - 1U;
    switch (event.type) {
        case EventType::Camera:
            return kCameraSymbol;
        case EventType::Reflection:
            return kReflectionSymbol + scattered;
        case EventType::Transmission:
            return kTransmissionSymbol + scattered;
        case EventType::Object:
            return kObjectSymbol;
        case EventType::Light:
            break;
    }
    return event.label == light::kNoLabel ? kLightSymbol : kLabelled + event.label;
}

};  // namespace v3d::moya::kernel::integrate
