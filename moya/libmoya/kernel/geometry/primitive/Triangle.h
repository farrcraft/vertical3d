/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/primitive/Shear.h>
#include <moya/libmoya/kernel/geometry/primitive/TriangleHit.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::geometry::primitive {

/**
 * Whether a ray meets a triangle at a distance in [start, end), and where. Either side of the
 * triangle is hit.
 *
 * The test is Woop, Benthin and Wald's, and is watertight: a ray through an edge or a vertex that
 * triangles share meets at least one of them. Each edge's function is computed again in double
 * precision when it rounds to zero in single, so a ray exactly on an edge is decided the same way
 * by both triangles that share it.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - a triangle's three vertices, in order
MOYA_KERNEL inline bool intersectTriangle(const Shear & ray, const glm::vec3 & p0, const glm::vec3 & p1, const glm::vec3 & p2,
    float start, float end, TriangleHit & hit) {
    // the vertices from the ray's origin
    const glm::vec3 q0 = p0 - ray.origin;
    const glm::vec3 q1 = p1 - ray.origin;
    const glm::vec3 q2 = p2 - ray.origin;

    const float ax = q0[ray.kx] - (ray.sx * q0[ray.kz]);
    const float ay = q0[ray.ky] - (ray.sy * q0[ray.kz]);
    const float bx = q1[ray.kx] - (ray.sx * q1[ray.kz]);
    const float by = q1[ray.ky] - (ray.sy * q1[ray.kz]);
    const float cx = q2[ray.kx] - (ray.sx * q2[ray.kz]);
    const float cy = q2[ray.ky] - (ray.sy * q2[ray.kz]);

    // each edge's function, which is the weight of the vertex opposite it
    float u = (cx * by) - (cy * bx);
    float v = (ax * cy) - (ay * cx);
    float w = (bx * ay) - (by * ax);
    if (u == 0.0F || v == 0.0F || w == 0.0F) {
        u = static_cast<float>((static_cast<double>(cx) * by) - (static_cast<double>(cy) * bx));
        v = static_cast<float>((static_cast<double>(ax) * cy) - (static_cast<double>(ay) * cx));
        w = static_cast<float>((static_cast<double>(bx) * ay) - (static_cast<double>(by) * ax));
    }
    if ((u < 0.0F || v < 0.0F || w < 0.0F) && (u > 0.0F || v > 0.0F || w > 0.0F)) {
        return false;
    }
    const float det = u + v + w;
    if (det == 0.0F) {
        return false;
    }

    const float az = ray.sz * q0[ray.kz];
    const float bz = ray.sz * q1[ray.kz];
    const float cz = ray.sz * q2[ray.kz];
    const float scaled = (u * az) + (v * bz) + (w * cz);
    const float inverse = 1.0F / det;
    const float distance = scaled * inverse;
    if (std::isnan(distance) || distance < start || distance >= end) {
        return false;
    }
    hit.distance = distance;
    hit.uv = glm::vec2 { v * inverse, w * inverse };
    return true;
}

};  // namespace v3d::moya::kernel::geometry::primitive
