/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Placement.h>
#include <moya/libmoya/kernel/geometry/primitive/Shapes.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/Qualifiers.h>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::kernel::geometry {

/**
 * Points or a quadric placed by one chain of instances. They are held in their own space as they
 * stood when the scene was placed, with the transformation into it from the world. A sphere
 * moved by a transformation that stretches it is no longer a sphere.
 **/
struct PlacedShapes {
    /** The run's chain, and its first sphere or quadric and how many. **/
    Placement placement;

    glm::mat4 toShape { 1.0F };
    primitive::Form form { primitive::Form::RoundPoints };

    /** A ray moved from the world into the run's own space. Its distances are unchanged. **/
    MOYA_KERNEL Ray into(const Ray & ray) const {
        Ray out = ray;
        out.origin = glm::vec3(toShape * glm::vec4(ray.origin, 1.0F));
        out.direction = glm::vec3(toShape * glm::vec4(ray.direction, 0.0F));
        return out;
    }
};

};  // namespace v3d::moya::kernel::geometry
