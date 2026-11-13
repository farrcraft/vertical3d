/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Placed.h>
#include <moya/libmoya/kernel/geometry/PlacedShapes.h>
#include <moya/libmoya/kernel/geometry/Placement.h>
#include <moya/libmoya/kernel/geometry/primitive/ShapeHit.h>
#include <moya/libmoya/kernel/geometry/primitive/Shear.h>
#include <moya/libmoya/kernel/geometry/primitive/Triangle.h>
#include <moya/libmoya/kernel/geometry/primitive/TriangleHit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

#include <glm/vec2.hpp>

namespace v3d::moya::kernel::geometry {

/**
 * The traversal that tests every placed primitive for every ray. It is slow, and it is the
 * reference every other traversal is compared with. A run hidden from the ray's kind is passed
 * over. It asks the filter about every hit, and each primitive is tested once.
 **/
struct BruteForce {
    Placed placed;

    template <typename F>
    MOYA_KERNEL Hit nearest(const Ray & ray, F & filter) const {
        Hit out;
        out.time = ray.time;
        float end = ray.end;
        visit(ray, [&](const Hit & hit) {
            if (hit.distance < end && filter.counts(hit)) {
                end = hit.distance;
                out = hit;
            }
            return false;
        });
        return out;
    }

    template <typename F>
    MOYA_KERNEL bool any(const Ray & ray, F & filter) const {
        return visit(ray, [&](const Hit & hit) {
            return filter.counts(hit);
        });
    }

 private:
    /** A hit on a primitive of a run, at a distance and a position on the primitive. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the primitive, then the distance
    MOYA_KERNEL static Hit hitOn(const Placement & placement, std::uint32_t primitive, float distance, const glm::vec2 & uv, float time) {
        Hit out;
        out.distance = distance;
        out.primitive = primitive;
        out.uv = uv;
        out.time = time;
        out.depth = placement.depth;
        for (std::uint32_t level = 0; level < kInstanceDepth; ++level) {
            out.instances[level] = placement.instances[level];
        }
        return out;
    }

    /**
     * Offers each hit within the ray's whole interval to a visitor, in the order the runs hold
     * them, until the visitor says to stop. Whether it stopped.
     **/
    template <typename V>
    MOYA_KERNEL bool visit(const Ray & ray, V && visitor) const {
        const primitive::Shear shear = primitive::Shear::of(ray);
        for (std::uint32_t run = 0; run < placed.placements.size; ++run) {
            const Placement & placement = placed.placements[run];
            if ((placement.visibility & ray.kind) == 0U) {
                continue;
            }
            for (std::uint32_t i = 0; i < placement.count; ++i) {
                const std::uint32_t at = placement.first + i;
                primitive::TriangleHit hit;
                if (primitive::intersectTriangle(shear, placed.points[at * 3U], placed.points[(at * 3U) + 1U], placed.points[(at * 3U) + 2U],
                        ray.start, ray.end, hit) &&
                    visitor(hitOn(placement, i, hit.distance, hit.uv, ray.time))) {
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
                primitive::ShapeHit hit;
                if (placed.meetsShape(shapes, i, local, ray.start, ray.end, hit) && visitor(hitOn(shapes.placement, i, hit.distance, hit.uv, ray.time))) {
                    return true;
                }
            }
        }
        return false;
    }
};

};  // namespace v3d::moya::kernel::geometry
