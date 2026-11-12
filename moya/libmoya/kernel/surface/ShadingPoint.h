/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/surface/Frame.h>
#include <moya/libmoya/kernel/surface/Material.h>

#include <cstdint>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::surface {

/**
 * Where a ray met a surface, in world space, as the shade stage reads it. Each is computed once
 * from a hit, as the surface stood at the hit's time.
 **/
struct ShadingPoint {
    glm::vec3 position { 0.0F };

    /**
     * Where a ray leaving the surface on the side its normal faces starts from, before it is
     * offset. On a triangle with a normal at each corner it is moved out to the curved surface
     * the normals describe, so a shadow does not show the triangles' facets. Elsewhere it is the
     * position.
     **/
    glm::vec3 leave { 0.0F };

    /** The surface's own normal, of unit length: by a triangle's winding, or from its shape. **/
    glm::vec3 geometric { 0.0F, 0.0F, 1.0F };

    /**
     * The normal shading takes, of unit length and on the geometric normal's side: a mesh's
     * interpolated `N`, a subdivision surface's limit normal, or the geometric normal.
     **/
    glm::vec3 shading { 0.0F, 0.0F, 1.0F };

    /** The frame around the shading normal. **/
    Frame frame;

    /** The transformation from the geometry's own space into the world, at the hit's time. **/
    glm::mat4 toWorld { 1.0F };

    /** The hit's time, held to the shutter. **/
    float time { 0.0F };

    /** The hit's position on its primitive. **/
    glm::vec2 uv { 0.0F };

    /** The weight of each corner of a triangle at the position, in the order of its points. **/
    glm::vec3 weights { 1.0F, 0.0F, 0.0F };

    /** The geometry met, its primitive, and the face or curve the primitive was cut from. **/
    std::uint32_t geometry { geometry::kNoPrimitive };
    std::uint32_t primitive { geometry::kNoPrimitive };
    std::uint32_t face { geometry::kNoPrimitive };

    /**
     * The material of the innermost instance of the chain that names one, or kNoMaterial. An
     * instance in a group with no material takes the material of an instance that places the group.
     **/
    std::uint32_t material { kNoMaterial };

    /** Whether the ray met the side the geometric normal faces. **/
    bool front { true };

    /** Whether the hit named a surface the scene holds. **/
    MOYA_KERNEL bool found() const {
        return geometry != geometry::kNoPrimitive;
    }
};

};  // namespace v3d::moya::kernel::surface
