/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/rib/Requests.h>
#include <api/type/geometry/Line.h>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Intersection.h"
#include "Pose.h"
#include "Primitive.h"

namespace v3d::render::offline::trace {

/**
 * RI's sphere. It is centred on its own origin, cut to the slab between two heights on its z
 * axis and swept through an angle about it. The transformation that was current places it in
 * world space.
 *
 * It is intersected exactly, not tessellated, so its silhouette is exact at any size. Its
 * normal points outward, which is RI's default orientation. An inside-out sphere needs
 * `Orientation`, which this renderer does not read.
 **/
class Sphere final : public Primitive {
 public:
    /**
     * @param shape its radius, the slab it is cut to, which is clamped to the radius, and its
     *        sweep about z in degrees
     * @param placement object to world, at the reference end of its motion
     **/
    Sphere(const v3d::render::offline::rib::Sphere & shape, const glm::mat4x4 & placement, const glm::vec3 & colour);

    /**
     * Where a line meets the sphere, as a parameter along it, nearest past `from`.
     *
     * The line is in world space and need not be unit length: the parameter is in its
     * units, which an affine placement keeps.
     *
     * @param point the hit in the sphere's own space, where the normal and the surface
     *        parameters are read off
     **/
    bool intersects(const v3d::type::geometry::Line & line, float from, float* along, glm::vec3* point) const;

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
    /** The box around its own slab, [-r, r] by [-r, r] by [zmin, zmax], placed in world space. **/
    bool bound(v3d::type::geometry::AABBox* box) const override;

 private:
    float radius_;
    float zmin_;
    float zmax_;
    float thetamax_;
    glm::mat4x4 toObject_;
};

};  // namespace v3d::render::offline::trace
