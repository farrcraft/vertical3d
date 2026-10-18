/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "AABBox.h"
#include "Line.h"
#include "Plane.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::type::geometry {

/**
 * A half line: an origin and a direction, and the things it can be intersected with.
 *
 * A distance along a ray is in units of its direction vector. The constructor normalises
 * the direction, so a distance is in world units. transformed() does not normalise, so a
 * distance found in one space can be compared with one found in another.
 *
 * A ray whose origin or direction has a component that is NaN or infinite meets nothing.
 * Every intersects() overload returns false for it and leaves its outputs untouched.
 **/
class Ray final {
 public:
    /**
     * A ray at the origin, pointing along +z.
     **/
    Ray();

    /**
     * @param line where the ray starts, and which way it goes. The direction is normalised,
     *        unless it is zero length.
     **/
    explicit Ray(const Line& line);

    /**
     **/
    const glm::vec3& origin() const noexcept;

    /**
     **/
    const glm::vec3& direction() const noexcept;

    /**
     * @return the point at that distance along the ray
     **/
    glm::vec3 point(float distance) const;

    /**
     * The same ray seen from another space.
     *
     * The direction is transformed as a vector and the origin as a point. Neither is
     * renormalised, so a distance means the same in both spaces, even under a scale. A hit
     * found in model space can then be compared with one found in world space. To move a
     * world ray into a mesh's own space, pass the inverse of that mesh's matrix.
     **/
    Ray transformed(const glm::mat4& transform) const;  // NOLINT(build/include_what_you_use) - the name, not std::transform

    /**
     * Slab test against an axis aligned box.
     *
     * A ray starting inside the box hits it at distance zero.
     *
     * @param box the box, in the ray's own space
     * @param distance where the hit is, if there is one - may be null
     * @return whether the ray meets the box at a non negative distance
     **/
    bool intersects(const AABBox& box, float* distance) const;

    /**
     * Moller-Trumbore against a triangle, hit from either side.
     *
     * @param distance where the hit is, if there is one - may be null
     * @return whether the ray meets the triangle at a non negative distance
     **/
    bool intersects(const glm::vec3& vertexA, const glm::vec3& vertexB, const glm::vec3& vertexC, float* distance) const;

    /**
     * The same test, also reporting where in the triangle the hit landed.
     *
     * Moller-Trumbore solves for the barycentric coordinates on its way to the distance.
     * Use this overload to interpolate a per vertex value, such as a normal or a texture
     * coordinate, without solving for them again.
     *
     * The weights are of vertexB and vertexC: the hit is
     * vertexA + u * (vertexB - vertexA) + v * (vertexC - vertexA), so vertexA carries 1 - u - v.
     *
     * @param distance where the hit is, if there is one - may be null
     * @param weights the weights of vertexB and then vertexC, if there is a hit - may be null
     * @return whether the ray meets the triangle at a non negative distance
     **/
    bool intersects(const glm::vec3& vertexA, const glm::vec3& vertexB, const glm::vec3& vertexC, float* distance,
        glm::vec2* weights) const;

    /**
     * Where this ray crosses a plane.
     *
     * A ray lying in the plane, or parallel to it, does not cross it. Neither does a ray
     * that would have to run backwards from its origin to reach it.
     *
     * @param plane the plane, whose normal need not be unit length
     * @param distance where the crossing is, if there is one - may be null
     * @return whether the ray crosses the plane at or ahead of its origin
     **/
    bool intersects(const Plane& plane, float* distance) const;

 private:
    glm::vec3 origin_;
    glm::vec3 direction_;
};

};  // namespace v3d::type::geometry
