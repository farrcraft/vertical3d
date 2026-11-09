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
 * over.
 **/
struct BruteForce {
    Placed placed;

    MOYA_KERNEL Hit nearest(const Ray & ray) const {
        Hit out;
        out.time = ray.time;
        const primitive::Shear shear = primitive::Shear::of(ray);
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
                primitive::TriangleHit hit;
                if (primitive::intersectTriangle(shear, placed.points[at * 3U], placed.points[(at * 3U) + 1U], placed.points[(at * 3U) + 2U],
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
                primitive::ShapeHit hit;
                if (placed.meetsShape(shapes, i, local, ray.start, end, hit)) {
                    keep(shapes.placement, i, hit.distance, hit.uv);
                }
            }
        }
        return out;
    }

    MOYA_KERNEL bool any(const Ray & ray) const {
        const primitive::Shear shear = primitive::Shear::of(ray);
        for (std::uint32_t run = 0; run < placed.placements.size; ++run) {
            const Placement & placement = placed.placements[run];
            if ((placement.visibility & ray.kind) == 0U) {
                continue;
            }
            for (std::uint32_t at = placement.first; at < placement.first + placement.count; ++at) {
                primitive::TriangleHit hit;
                if (primitive::intersectTriangle(shear, placed.points[at * 3U], placed.points[(at * 3U) + 1U], placed.points[(at * 3U) + 2U],
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
                primitive::ShapeHit hit;
                if (placed.meetsShape(shapes, i, local, ray.start, ray.end, hit)) {
                    return true;
                }
            }
        }
        return false;
    }
};

};  // namespace v3d::moya::kernel::geometry
