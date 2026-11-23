/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/volume/Nano.h>

#include <cstdint>

#include <glm/vec3.hpp>

namespace v3d::moya::kernel::volume {

/** The side in voxels of a leaf, of a lower node and of an upper node of a grid's tree. **/
inline constexpr std::int32_t kLeafSide = static_cast<std::int32_t>(nanovdb::NanoLeaf<float>::DIM);
inline constexpr std::int32_t kLowerSide = static_cast<std::int32_t>(nanovdb::NanoLower<float>::DIM);
inline constexpr std::int32_t kUpperSide = static_cast<std::int32_t>(nanovdb::NanoUpper<float>::DIM);

/**
 * A cell of a grid's tree that a ray walks through: the node or tile that holds a voxel. It holds
 * its first voxel, its side in voxels, and the most a lookup inside it can give.
 **/
struct Cell {
    glm::ivec3 origin { 0 };
    std::int32_t side { kUpperSide };
    float most { 0.0F };
};

};  // namespace v3d::moya::kernel::volume
