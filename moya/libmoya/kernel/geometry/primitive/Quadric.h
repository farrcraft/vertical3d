/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::geometry::primitive {

/** A whole turn, in radians. **/
inline constexpr float kWholeTurn = 6.28318530717958647692F;

/** RI's tube of a torus, which it gives as a quarter of the ring's radius of one. **/
inline constexpr float kMinorRadius = 0.25F;

/** RI's seven quadrics. **/
enum class Shape : std::uint8_t {
    Sphere,
    Cone,
    Cylinder,
    Hyperboloid,
    Paraboloid,
    Disk,
    Torus
};

/**
 * One of RI's quadrics in its own space, each a profile swept about z from the +x axis by
 * thetamax. Each shape reads its own of the dimensions, with RI's meanings:
 * - a sphere, its radius, cut at zmin and zmax.
 * - a cone, its radius at z = 0, narrowing to its apex at z = height.
 * - a cylinder, its radius, from zmin to zmax.
 * - a hyperboloid, the line from point1 to point2 swept about z.
 * - a paraboloid, its radius at zmax, rising from z = 0 at its apex, cut at zmin and zmax.
 * - a disk, its radius at z = height.
 * - a torus, its radius to the tube's centre, and the tube's minorRadius. The tube runs
 *   from phimin to phimax about its own centre, from the outside of the ring.
 * Angles are in radians.
 **/
struct Quadric {
    Shape shape { Shape::Sphere };
    float radius { 1.0F };
    float minorRadius { kMinorRadius };
    float zmin { -1.0F };
    float zmax { 1.0F };
    float height { 1.0F };
    float phimin { 0.0F };
    float phimax { kWholeTurn };
    glm::vec3 point1 { 1.0F, 0.0F, -1.0F };
    glm::vec3 point2 { 1.0F, 0.0F, 1.0F };
    float thetamax { kWholeTurn };
};

};  // namespace v3d::moya::kernel::geometry::primitive
