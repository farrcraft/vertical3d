/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "AABBox.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::type {

    /**
     * A half line: an origin and a direction, and the things it can be intersected with.
     *
     * A distance along a ray is in units of its direction vector. The constructor
     * normalises, so that is world units; transformed() does not, so a distance found in
     * one space is comparable with one found in another.
     **/
    class Ray final {
     public:
        /**
         * A degenerate ray at the origin, pointing along +z.
         **/
        Ray();

        /**
         * @param origin where the ray starts
         * @param direction which way it goes - normalised, unless it is zero length
         **/
        Ray(const glm::vec3& origin, const glm::vec3& direction);

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
         * The direction is transformed as a vector and the origin as a point, and neither
         * is renormalised: under a scale, renormalising would change what a distance means
         * and a hit found in model space could no longer be compared with one found in
         * world space. To move a world ray into a mesh's own space, pass the inverse of
         * that mesh's matrix.
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
        bool intersects(const glm::vec3& a, const glm::vec3& b, const glm::vec3& c, float* distance) const;

     private:
        glm::vec3 origin_;
        glm::vec3 direction_;
    };

};  // namespace v3d::type
