/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Triangle.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::geometry {

/**
 * One chain of instances that places a geometry in the world, and the run of placed triangles it
 * owns.
 **/
struct Placement {
    std::uint32_t instances[kInstanceDepth] {};
    std::uint32_t depth { 0 };

    /** The first of its triangles, and how many there are. **/
    std::uint32_t first { 0 };
    std::uint32_t count { 0 };
};

/**
 * Every triangle of a scene placed in world space, in runs, one run per chain of instances.
 *
 * A triangle's primitive is its index within its run, which is its index within its geometry, so
 * a hit names the same primitive whichever traversal found it. Each triangle also keeps the face
 * of its mesh that it was cut from.
 **/
struct Placed {
    /** Three points per triangle, in world space. **/
    Array<glm::vec3> points;

    /** One face per triangle. **/
    Array<std::uint32_t> faces;

    Array<Placement> placements;

    /** The index among every placed triangle of a hit's triangle, or kNoPrimitive. **/
    MOYA_KERNEL std::uint32_t triangle(const Hit & hit) const {
        for (std::uint32_t run = 0; run < placements.size; ++run) {
            const Placement & placement = placements[run];
            if (placement.depth != hit.depth || hit.primitive >= placement.count) {
                continue;
            }
            bool same = true;
            for (std::uint32_t level = 0; level < hit.depth; ++level) {
                same = same && placement.instances[level] == hit.instances[level];
            }
            if (same) {
                return placement.first + hit.primitive;
            }
        }
        return kNoPrimitive;
    }

    /**
     * A hit's geometric normal in world space, of unit length, by the winding of its triangle's
     * points. Zero for a hit this scene does not hold.
     **/
    MOYA_KERNEL glm::vec3 normal(const Hit & hit) const {
        const std::uint32_t at = triangle(hit);
        if (at == kNoPrimitive) {
            return glm::vec3 { 0.0F };
        }
        const glm::vec3 & p0 = points[(at * 3U) + 0U];
        const glm::vec3 & p1 = points[(at * 3U) + 1U];
        const glm::vec3 & p2 = points[(at * 3U) + 2U];
        return glm::normalize(glm::cross(p1 - p0, p2 - p0));
    }

    /** The face of its mesh a hit's triangle was cut from, or kNoPrimitive. **/
    MOYA_KERNEL std::uint32_t face(const Hit & hit) const {
        const std::uint32_t at = triangle(hit);
        return at == kNoPrimitive ? kNoPrimitive : faces[at];
    }
};

/**
 * The traversal that tests every placed triangle for every ray. It is slow, and it is the
 * reference every other traversal is compared with.
 **/
struct BruteForce {
    Placed placed;

    MOYA_KERNEL Hit nearest(const Ray & ray) const {
        Hit out;
        out.time = ray.time;
        const Shear shear = Shear::of(ray);
        float end = ray.end;
        for (std::uint32_t run = 0; run < placed.placements.size; ++run) {
            const Placement & placement = placed.placements[run];
            for (std::uint32_t i = 0; i < placement.count; ++i) {
                const std::uint32_t at = placement.first + i;
                TriangleHit hit;
                if (intersectTriangle(shear, placed.points[at * 3U], placed.points[(at * 3U) + 1U], placed.points[(at * 3U) + 2U],
                        ray.start, end, hit)) {
                    end = hit.distance;
                    out.distance = hit.distance;
                    out.primitive = i;
                    out.uv = hit.uv;
                    out.depth = placement.depth;
                    for (std::uint32_t level = 0; level < kInstanceDepth; ++level) {
                        out.instances[level] = placement.instances[level];
                    }
                }
            }
        }
        return out;
    }

    MOYA_KERNEL bool any(const Ray & ray) const {
        const Shear shear = Shear::of(ray);
        for (std::uint32_t at = 0; at * 3U < placed.points.size; ++at) {
            TriangleHit hit;
            if (intersectTriangle(shear, placed.points[at * 3U], placed.points[(at * 3U) + 1U], placed.points[(at * 3U) + 2U],
                    ray.start, ray.end, hit)) {
                return true;
            }
        }
        return false;
    }
};

};  // namespace v3d::moya::kernel::geometry
