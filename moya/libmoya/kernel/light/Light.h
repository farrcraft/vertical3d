/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

#include <glm/vec3.hpp>

/** The scene's lights as the integrator reads them, and the light each emits. **/
namespace v3d::moya::kernel::light {

/** A quarter turn, in radians: a cone this wide or wider, with no softness, shapes nothing. **/
inline constexpr float kQuarterTurn = 1.57079632679489661923F;

/**
 * A light with a shape, which an instance in the world places. Its radiance is scale times its
 * spectrum. With no temperature, the spectrum is its ACEScg colour as an illuminant. With one, it
 * is its colour as an unbounded spectrum times a Planckian radiator of a luminance of one.
 *
 * - id is its instance's index in the scene, which a hit names.
 * - scale is its intensity times two to its exposure, over its area when it is normalised.
 * - temperature is in kelvin, or zero when it has none. planckScale is what a Planckian
 *   radiator at that temperature, one at 560 nm, is multiplied by to give a luminance of one, and
 *   planckColour is that radiator's ACEScg colour, which an RGB build uses.
 * - axis is the direction it emits along, in its prototype's space. A one-sided light emits only
 *   to the side its axis points to.
 * - coneAngle and coneSoftness shape what it emits, as UsdLux's shaping does: nothing past the
 *   angle off its axis, and a smooth fall to it from the angle times one less the softness. The
 *   angle is in radians.
 **/
struct Light {
    std::uint32_t id { 0 };
    glm::vec3 colour { 1.0F };
    float scale { 1.0F };
    float temperature { 0.0F };
    float planckScale { 1.0F };
    glm::vec3 planckColour { 1.0F };
    glm::vec3 axis { 0.0F, 0.0F, -1.0F };
    float coneAngle { kQuarterTurn };
    float coneSoftness { 0.0F };
    std::uint32_t oneSided { 0 };
};

};  // namespace v3d::moya::kernel::light
