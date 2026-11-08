/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Shapes.h>
#include <moya/libmoya/kernel/geometry/Triangle.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::kernel::geometry {

/**
 * One chain of instances that places a geometry in the world, and the run of placed primitives it
 * owns. Its visibility holds the kinds of ray every instance of the chain is visible to.
 **/
struct Placement {
    std::uint32_t instances[kInstanceDepth] {};
    std::uint32_t depth { 0 };

    /** The first of its primitives, and how many there are. **/
    std::uint32_t first { 0 };
    std::uint32_t count { 0 };

    std::uint32_t visibility { kEveryRay };

    /** Whether a hit names this chain. **/
    MOYA_KERNEL bool places(const Hit & hit) const {
        if (depth != hit.depth || hit.primitive >= count) {
            return false;
        }
        bool same = true;
        for (std::uint32_t level = 0; level < hit.depth; ++level) {
            same = same && instances[level] == hit.instances[level];
        }
        return same;
    }
};

/**
 * Points or a quadric placed by one chain of instances. They are held in their own space as they
 * stood when the scene was placed, with the transformation into it from the world. A sphere
 * moved by a transformation that stretches it is no longer a sphere.
 **/
struct PlacedShapes {
    /** The run's chain, and its first sphere or quadric and how many. **/
    Placement placement;

    glm::mat4 toShape { 1.0F };
    Form form { Form::RoundPoints };

    /** A ray moved from the world into the run's own space. Its distances are unchanged. **/
    MOYA_KERNEL Ray into(const Ray & ray) const {
        Ray out = ray;
        out.origin = glm::vec3(toShape * glm::vec4(ray.origin, 1.0F));
        out.direction = glm::vec3(toShape * glm::vec4(ray.direction, 0.0F));
        return out;
    }
};

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

    Array<Quadric> quadrics;

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
    MOYA_KERNEL bool meetsShape(const PlacedShapes & run, std::uint32_t index, const Ray & ray, float start, float end, ShapeHit & hit) const {
        const std::uint32_t at = run.placement.first + index;
        switch (run.form) {
            case Form::RoundPoints:
                return intersectSphere(glm::vec3(spheres[at]), spheres[at].w, ray, start, end, hit);
            case Form::FlatPoints:
                return intersectFacing(glm::vec3(spheres[at]), spheres[at].w, ray, start, end, hit);
            case Form::Quadric:
                return intersectQuadric(quadrics[at], ray, start, end, hit);
            default:
                return false;
        }
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
            case Form::RoundPoints:
                own = point - glm::vec3(spheres[index]);
                break;
            case Form::FlatPoints:
                own = -local.direction;
                break;
            case Form::Quadric:
                own = quadricNormal(quadrics[index], point);
                break;
            default:
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

/**
 * The traversal that tests every placed primitive for every ray. It is slow, and it is the
 * reference every other traversal is compared with. A run hidden from the ray's kind is passed
 * over.
 **/
struct BruteForce {
    Placed placed;

    MOYA_KERNEL Hit nearest(const Ray & ray) const {
        Hit out;
        out.time = ray.time;
        const Shear shear = Shear::of(ray);
        float end = ray.end;
        // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the primitive, then the distance
        const auto keep = [&](const Placement & placement, std::uint32_t primitive, float distance, const glm::vec2 & uv) {
            end = distance;
            out.distance = distance;
            out.primitive = primitive;
            out.uv = uv;
            out.depth = placement.depth;
            for (std::uint32_t level = 0; level < kInstanceDepth; ++level) {
                out.instances[level] = placement.instances[level];
            }
        };
        for (std::uint32_t run = 0; run < placed.placements.size; ++run) {
            const Placement & placement = placed.placements[run];
            if ((placement.visibility & ray.kind) == 0U) {
                continue;
            }
            for (std::uint32_t i = 0; i < placement.count; ++i) {
                const std::uint32_t at = placement.first + i;
                TriangleHit hit;
                if (intersectTriangle(shear, placed.points[at * 3U], placed.points[(at * 3U) + 1U], placed.points[(at * 3U) + 2U],
                        ray.start, end, hit)) {
                    keep(placement, i, hit.distance, hit.uv);
                }
            }
        }
        for (std::uint32_t run = 0; run < placed.shapes.size; ++run) {
            const PlacedShapes & shapes = placed.shapes[run];
            if ((shapes.placement.visibility & ray.kind) == 0U) {
                continue;
            }
            const Ray local = shapes.into(ray);
            for (std::uint32_t i = 0; i < shapes.placement.count; ++i) {
                ShapeHit hit;
                if (placed.meetsShape(shapes, i, local, ray.start, end, hit)) {
                    keep(shapes.placement, i, hit.distance, hit.uv);
                }
            }
        }
        return out;
    }

    MOYA_KERNEL bool any(const Ray & ray) const {
        const Shear shear = Shear::of(ray);
        for (std::uint32_t run = 0; run < placed.placements.size; ++run) {
            const Placement & placement = placed.placements[run];
            if ((placement.visibility & ray.kind) == 0U) {
                continue;
            }
            for (std::uint32_t at = placement.first; at < placement.first + placement.count; ++at) {
                TriangleHit hit;
                if (intersectTriangle(shear, placed.points[at * 3U], placed.points[(at * 3U) + 1U], placed.points[(at * 3U) + 2U],
                        ray.start, ray.end, hit)) {
                    return true;
                }
            }
        }
        for (std::uint32_t run = 0; run < placed.shapes.size; ++run) {
            const PlacedShapes & shapes = placed.shapes[run];
            if ((shapes.placement.visibility & ray.kind) == 0U) {
                continue;
            }
            const Ray local = shapes.into(ray);
            for (std::uint32_t i = 0; i < shapes.placement.count; ++i) {
                ShapeHit hit;
                if (placed.meetsShape(shapes, i, local, ray.start, ray.end, hit)) {
                    return true;
                }
            }
        }
        return false;
    }
};

};  // namespace v3d::moya::kernel::geometry
