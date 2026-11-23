/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::geometry::primitive {

/**
 * A ray made ready for the watertight triangle test, once for every triangle it meets. The axis
 * the direction is longest along becomes z, and a shear turns the direction into +z, so each
 * triangle is tested in two dimensions at the ray's origin. The other two axes are swapped when
 * the direction runs down z, so a triangle keeps its winding.
 **/
struct Shear {
    glm::vec3 origin { 0.0F };
    int kx { 0 };
    int ky { 1 };
    int kz { 2 };
    float sx { 0.0F };
    float sy { 0.0F };
    float sz { 1.0F };

    MOYA_KERNEL static Shear of(const Ray & ray) {
        Shear out;
        out.origin = ray.origin;
        const glm::vec3 size { std::fabs(ray.direction.x), std::fabs(ray.direction.y), std::fabs(ray.direction.z) };
        if (size.x > size.y && size.x > size.z) {
            out.kz = 0;
        } else if (size.y > size.z) {
            out.kz = 1;
        }
        out.kx = (out.kz + 1) % 3;
        out.ky = (out.kx + 1) % 3;
        if (ray.direction[out.kz] < 0.0F) {
            const int swap = out.kx;
            out.kx = out.ky;
            out.ky = swap;
        }
        out.sx = ray.direction[out.kx] / ray.direction[out.kz];
        out.sy = ray.direction[out.ky] / ray.direction[out.kz];
        out.sz = 1.0F / ray.direction[out.kz];
        return out;
    }
};

};  // namespace v3d::moya::kernel::geometry::primitive
