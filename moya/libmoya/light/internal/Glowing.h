/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/accel/Box.h>
#include <moya/libmoya/kernel/light/Light.h>
#include <moya/libmoya/light/internal/Gathered.h>
#include <moya/libmoya/light/Shading.h>

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::geometry {
struct Prototypes;
};  // namespace v3d::moya::geometry

namespace v3d::moya::light::internal {

/**
 * The part of a volume light that lies in its own space: the light, its bricks' box, and what it
 * emits there, the sum over its own space of its emission's luminance. Its chain is not yet set.
 **/
struct Glowing {
    kernel::light::Light light;
    accel::Box box;
    float emitted { 0.0F };
};

/**
 * The light of the volume at a place among the prototypes' volumes, with its bricks, their cells'
 * chances, its bricks' chances and its subtree appended to what is gathered. False, appending
 * nothing, for a volume that emits nothing.
 *
 * A brick is a leaf node of its walked field's grid, an active tile of a lower node, or a part of
 * an active tile of an upper node the size of a leaf. A root's tiles make none. Each of its cells
 * is weighed by the luminance of what the volume's program emits at the cell's centre, with each
 * field looked up there, as the volume stands when the shutter opens. A brick whose cells emit
 * nothing is left out, and a volume with no volume program emits nothing. The bricks are sorted by
 * their lower corners, z first, then y, then x. A brick's power in the subtree is the sum of its
 * cells'.
 **/
bool gridGlow(const geometry::Prototypes & prototypes, const Shading & shading, std::uint32_t volume, Gathered & out, Glowing & made);

/**
 * The light of a closed surface of a geometry whose medium emits an emission in ACEScg, with its
 * one brick and its chance appended to what is gathered. Its brick is the geometry's box in its
 * own space over the shutter. False, appending nothing, for a geometry that is not of triangles
 * or a quadric, or one of no box.
 **/
bool enclosedGlow(const geometry::Prototypes & prototypes, std::uint32_t geometry, const glm::vec3 & emission, Gathered & out, Glowing & made);

};  // namespace v3d::moya::light::internal
