/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/PlacedShapes.h>
#include <moya/libmoya/kernel/geometry/Placement.h>
#include <moya/libmoya/kernel/geometry/primitive/Quadric.h>
#include <moya/libmoya/kernel/geometry/primitive/ShapeHit.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::kernel::geometry {

/**
 * Every primitive of a scene placed, in runs, one run per chain of instances. Triangles are
 * placed in world space. Points and quadrics are held in their own space, each run with the
 * transformation into it.
 *
 * A primitive is its index within its run, which is its index within its geometry, so a hit names
 * the same primitive whichever traversal found it. Each triangle also keeps the face of its mesh
 * that it was cut from.
 **/
struct Placed {
    /** Three points per triangle, in world space. **/
    Array<glm::vec3> points;

    /** One face per triangle. **/
    Array<std::uint32_t> faces;

    Array<Placement> placements;

    Array<PlacedShapes> shapes;

    /** Each point's centre and radius, in its run's own space. **/
    Array<glm::vec4> spheres;

    Array<primitive::Quadric> quadrics;

    /** The index among every placed triangle of a hit's triangle, or kNoPrimitive. **/
    MOYA_KERNEL std::uint32_t triangle(const Hit & hit) const {
        for (std::uint32_t run = 0; run < placements.size; ++run) {
            if (placements[run].places(hit)) {
                return placements[run].first + hit.primitive;
            }
        }
        return kNoPrimitive;
    }

    /** The index among the runs of shapes of the run a hit names, or kNoPrimitive. **/
    MOYA_KERNEL std::uint32_t shapeRun(const Hit & hit) const {
        for (std::uint32_t run = 0; run < shapes.size; ++run) {
            if (shapes[run].placement.places(hit)) {
                return run;
            }
        }
        return kNoPrimitive;
    }

    /** Whether a ray in a run's own space meets one of its shapes in [start, end), and where. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the shape, then the interval
    MOYA_KERNEL bool meetsShape(const PlacedShapes & run, std::uint32_t index, const Ray & ray, float start, float end, primitive::ShapeHit & hit) const {
        const std::uint32_t at = run.placement.first + index;
        switch (run.form) {
            case primitive::Form::RoundPoints:
                return primitive::intersectSphere(glm::vec3(spheres[at]), spheres[at].w, ray, start, end, hit);
            case primitive::Form::FlatPoints:
                return primitive::intersectFacing(glm::vec3(spheres[at]), spheres[at].w, ray, start, end, hit);
            case primitive::Form::Quadric:
                return primitive::intersectQuadric(quadrics[at], ray, start, end, hit);
            // a run of shapes holds no triangles
            case primitive::Form::Triangles:
                break;
        }
        return false;
    }

    /**
     * A hit's geometric normal in world space, of unit length. A triangle's is by the winding of
     * its points, a round point's from its centre, and a quadric's from its surface. A flat
     * point's faces back along the ray. Zero for a hit this scene does not hold.
     **/
    MOYA_KERNEL glm::vec3 normal(const Hit & hit, const Ray & ray) const {
        const std::uint32_t at = triangle(hit);
        if (at != kNoPrimitive) {
            const glm::vec3 & p0 = points[(at * 3U) + 0U];
            const glm::vec3 & p1 = points[(at * 3U) + 1U];
            const glm::vec3 & p2 = points[(at * 3U) + 2U];
            return glm::normalize(glm::cross(p1 - p0, p2 - p0));
        }
        const std::uint32_t run = shapeRun(hit);
        if (run == kNoPrimitive) {
            return glm::vec3 { 0.0F };
        }
        const PlacedShapes & placed = shapes[run];
        const Ray local = placed.into(ray);
        const glm::vec3 point = local.at(hit.distance);
        const std::uint32_t index = placed.placement.first + hit.primitive;
        glm::vec3 own { 0.0F, 0.0F, 1.0F };
        switch (placed.form) {
            case primitive::Form::RoundPoints:
                own = point - glm::vec3(spheres[index]);
                break;
            case primitive::Form::FlatPoints:
                own = -local.direction;
                break;
            case primitive::Form::Quadric:
                own = primitive::quadricNormal(quadrics[index], point);
                break;
            // a run of shapes holds no triangles
            case primitive::Form::Triangles:
                break;
        }
        // a normal moves by the inverse transpose of the transformation to the world
        return glm::normalize(glm::transpose(glm::mat3(placed.toShape)) * own);
    }

    /** The face a hit's triangle was cut from, the point a hit met, or kNoPrimitive. **/
    MOYA_KERNEL std::uint32_t face(const Hit & hit) const {
        const std::uint32_t at = triangle(hit);
        if (at != kNoPrimitive) {
            return faces[at];
        }
        return shapeRun(hit) == kNoPrimitive ? kNoPrimitive : hit.primitive;
    }
};

};  // namespace v3d::moya::kernel::geometry
