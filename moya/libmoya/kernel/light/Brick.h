/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/geometry/Hit.h>

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::light {

/** The cells along each side of a brick of a grid, as a NanoVDB leaf node holds its voxels. **/
inline constexpr std::uint32_t kBrickSide = 8;

/** The cells a brick of a grid holds. **/
inline constexpr std::uint32_t kBrickCells = kBrickSide * kBrickSide * kBrickSide;

/**
 * A box a volume light emits from, in the light's own space.
 *
 * A brick of a grid is a NanoVDB leaf node's: lower is the node's origin in the grid's index
 * space and upper lies kBrickSide past it. Each of its cells is a unit cube from a voxel to one
 * past it on each axis, numbered x first, then y, then z. cells is the first of the kBrickCells
 * sums of the cells' chances among the lights' cells, as Lights::drawnFrom() reads them. The
 * brick of a closed surface is its box, which is drawn uniformly, and its cells are kNoPrimitive.
 *
 * leaf is the brick's node among the lights' nodes, or kNoPrimitive when its light has no subtree.
 **/
struct Brick {
    glm::vec3 lower { 0.0F };
    glm::vec3 upper { 0.0F };
    std::uint32_t cells { geometry::kNoPrimitive };
    std::uint32_t leaf { geometry::kNoPrimitive };
};

};  // namespace v3d::moya::kernel::light
