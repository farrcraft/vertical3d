/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

/** The scene's lights as the integrator reads them, and the light each emits. **/
namespace v3d::moya::kernel::light {

/** A quarter turn, in radians: a cone this wide or wider, with no softness, shapes nothing. **/
inline constexpr float kQuarterTurn = 1.57079632679489661923F;

/**
 * The shape a light emits from. A point and a distant light have none that a ray can meet: a
 * point is a sphere light of no size, and a distant light lies beyond the scene. A mesh light
 * emits from the triangles of a geometry where a chain of instances places it. A dome light
 * surrounds the scene, and is met by every ray that leaves it.
 **/
enum class Shape : std::uint8_t {
    Rect,
    Disk,
    Sphere,
    Cylinder,
    Point,
    Distant,
    Mesh,
    Dome
};

/**
 * A light of the scene. With no temperature, its spectrum is its ACEScg colour as an illuminant.
 * With one, it is its colour as an unbounded spectrum times a Planckian radiator of a luminance of
 * one.
 *
 * - id is the index in the scene of the instance that places its shape, which a hit names, or
 *   kNoPrimitive for a light with no shape.
 * - transform takes its prototype's space into the world, and holds at every time while keyCount
 *   is below two. Otherwise its transformation at each of keyCount samples is held from keys
 *   among the lights' keys. A shaped light's is its instance's.
 * - scale is what its spectrum is multiplied by. A light with a shape emits a radiance: its
 *   intensity times two to its exposure, over its area when it is normalised. A point emits a
 *   radiant intensity, and a distant light of no angle an irradiance. A distant light with an
 *   angle emits a radiance.
 * - temperature is in kelvin, or zero when it has none. planckScale is what a Planckian
 *   radiator at that temperature, one at 560 nm, is multiplied by to give a luminance of one, and
 *   planckColour is that radiator's ACEScg colour, which an RGB build uses.
 * - shape and size are its shape in its prototype's space: a rect's width and height, a disk's
 *   or a sphere's radius, or a cylinder's radius and length along z. A distant light's size is
 *   the half angle of its cone, in radians. A light with a shape emits from one face: a rect's or
 *   a disk's -z face, or a sphere's or a cylinder's outer face.
 * - axis is the direction it emits along, in its prototype's space, which its cone is measured
 *   from. A distant light shines along it.
 * - coneAngle and coneSoftness shape what it emits, as UsdLux's shaping does: nothing past the
 *   angle off its axis, and a smooth fall to it from the angle times one less the softness. The
 *   angle is in radians.
 * - A mesh light is one place of its geometry: chain holds the instances that place it, by their
 *   indices in the scene, outermost first, and depth how many. geometry is its place among the
 *   surfaces' geometries. Its triangles' chances of being drawn are held from triangles among the
 *   lights' triangle chances, one for each of its count triangles. The light tree draws them
 *   through its geometry's subtree, whose root is tree among the lights' nodes. The leaf of each
 *   triangle is held from leaves among the lights' triangle leaves. camera is whether camera rays
 *   see what it emits. Its transform, axis and cone are not read.
 * - image is the place among the lights' images of the image a rect light's texture or a dome
 *   light reads, or kNoPrimitive for none. A dome light's scale is its intensity times two to its
 *   exposure, a radiance, and its transform turns its image into the world.
 * - profile is the place among the lights' profiles of the IES profile that shapes what it emits,
 *   or kNoPrimitive for none.
 * - shadowLinked is whether only some instances cast its shadows: those held among the links'
 *   casters, shadowCount of them from shadowFirst. A light that every instance shadows casts
 *   from all.
 **/
struct Light {
    std::uint32_t id { 0 };
    Shape shape { Shape::Rect };
    glm::vec2 size { 1.0F };
    glm::mat4 transform { 1.0F };
    std::uint32_t keys { 0 };
    std::uint32_t keyCount { 0 };
    glm::vec3 colour { 1.0F };
    float scale { 1.0F };
    float temperature { 0.0F };
    float planckScale { 1.0F };
    glm::vec3 planckColour { 1.0F };
    glm::vec3 axis { 0.0F, 0.0F, -1.0F };
    float coneAngle { kQuarterTurn };
    float coneSoftness { 0.0F };
    std::uint32_t chain[geometry::kInstanceDepth] {};
    std::uint32_t depth { 0 };
    std::uint32_t geometry { 0 };
    std::uint32_t triangles { 0 };
    std::uint32_t count { 0 };
    std::uint32_t tree { geometry::kNoPrimitive };
    std::uint32_t leaves { 0 };
    bool camera { false };
    std::uint32_t image { geometry::kNoPrimitive };
    std::uint32_t profile { geometry::kNoPrimitive };
    bool shadowLinked { false };
    std::uint32_t shadowFirst { 0 };
    std::uint32_t shadowCount { 0 };
};

/** Whether a light has a shape of its own: a rect, disk, sphere or cylinder. **/
MOYA_KERNEL inline bool shaped(const Light & light) {
    return light.shape == Shape::Rect || light.shape == Shape::Disk || light.shape == Shape::Sphere || light.shape == Shape::Cylinder;
}

/**
 * Whether a light lies at a single point or along a single direction: a point, or a distant
 * light of no angle. Only a light sample finds it, since no direction a BSDF draws meets it.
 **/
MOYA_KERNEL inline bool delta(const Light & light) {
    return light.shape == Shape::Point || (light.shape == Shape::Distant && !(light.size.x > 0.0F));
}

/**
 * Whether only a light sample finds a light: one that lies at a point or along a direction, or
 * one whose shadow link leaves an instance out. A BSDF's direction that meets such a light through
 * an instance it casts no shadow from would be stopped there, so it takes the whole weight.
 **/
MOYA_KERNEL inline bool sampledAlone(const Light & light) {
    return delta(light) || light.shadowLinked;
}

};  // namespace v3d::moya::kernel::light
