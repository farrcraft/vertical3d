/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Primitive.h"

namespace v3d::render::offline::trace {

/**
 * RI's sphere: centred on its own origin, cut to the slab between two heights on its z axis
 * and swept through an angle about it, and placed in world space by the transformation that
 * was current.
 *
 * It is intersected where it is defined rather than tessellated, so its silhouette is exact
 * at any size. Its normal points out, which is RI's orientation; an inside-out sphere needs
 * `Orientation`, which this renderer does not read.
 **/
class Sphere final : public Primitive {
 public:
    /**
     * @param zmin, zmax the slab it is cut to, clamped to the radius
     * @param thetamax the sweep about z, in degrees
     * @param placement object to world, at the open end of its motion
     **/
    Sphere(float radius, float zmin, float zmax, float thetamax, const glm::mat4x4 & placement,
        const glm::vec3 & colour);

    /**
     * Where a line meets the sphere, as a parameter along it, nearest past `from`.
     *
     * The line is in world space and need not be unit length: the parameter is in its
     * units, which an affine placement keeps.
     *
     * @param point the hit in the sphere's own space, where the normal and the surface
     *        parameters are read off
     **/
    bool intersects(const glm::vec3 & origin, const glm::vec3 & direction, float from,
        float* along, glm::vec3* point) const;

    /** The outward normal at a point on the sphere in its own space, in world space. **/
    glm::vec3 normal(const glm::vec3 & point) const;
    /**
     * RI's u and v at a point in its own space: the fraction of the sweep, and the fraction of
     * the way from the bottom of the slab to the top in latitude.
     **/
    glm::vec2 parameters(const glm::vec3 & point) const;

    bool intersect(const v3d::type::geometry::Ray & ray, float from, const Pose & pose,
        Intersection* found) const override;
    void describe(const Intersection & found, Hit* hit) const override;

 private:
    float radius_;
    float zmin_;
    float zmax_;
    float thetamax_;
    glm::mat4x4 toObject_;
};

};  // namespace v3d::render::offline::trace
