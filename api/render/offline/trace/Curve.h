/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/type/geometry/AABBox.h>
#include <api/type/geometry/Ray.h>

#include <array>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Primitive.h"

namespace v3d::render::offline::trace {

/**
 * One cubic Bezier segment of a curve, drawn as a flat ribbon of a width that varies along it,
 * in world space.
 *
 * The ribbon faces whatever ray meets it, so it has its full width from every direction and
 * its normal points back along the ray. A camera ray therefore sees it facing the camera, as RI
 * defines a curve. It is found by halving the segment until each piece is nearly straight, in
 * a frame where the ray runs along +z from the origin, and testing how far each piece passes
 * from the ray.
 *
 * Its parameters are v along the segment, from 0 at its first point to 1 at its last, and u
 * across the ribbon, as RI gives a curve's. s and t are u and v.
 **/
class Curve final : public Primitive {
 public:
    /** The four control points, in order. **/
    using Controls = std::array<glm::vec3, 4>;

    /**
     * @param widths the ribbon's width at the segment's start and at its end
     **/
    Curve(const Controls & controls, const glm::vec2 & widths, const glm::vec3 & colour);

    const Controls & controls() const;
    const glm::vec2 & widths() const;

    /** The point at v along the segment's centre line. **/
    glm::vec3 point(float v) const;

    bool intersect(const v3d::type::geometry::Ray & ray, float from, const Pose & pose,
        Intersection* found) const override;
    void describe(const Intersection & found, Hit* hit) const override;
    bool bound(v3d::type::geometry::AABBox* box) const override;

 private:
    Controls controls_;
    glm::vec2 widths_;
};

};  // namespace v3d::render::offline::trace
