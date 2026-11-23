/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/Qualifiers.h>
#include <moya/libmoya/kernel/volume/Block.h>
#include <moya/libmoya/kernel/volume/Cell.h>
#include <moya/libmoya/kernel/volume/GridPlace.h>
#include <moya/libmoya/kernel/volume/Nano.h>

#include <cmath>
#include <cstdint>
#include <limits>

#include <glm/common.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::kernel::volume {

/** The grid of floats a place among the blocks holds. **/
MOYA_KERNEL inline const nanovdb::FloatGrid & gridIn(const Array<Block> & blocks, const GridPlace & place) {
    return *reinterpret_cast<const nanovdb::FloatGrid *>(&blocks[place.first]);
}

/** The greatest value a grid holds, which its root keeps. **/
MOYA_KERNEL inline float mostOf(const nanovdb::FloatGrid & grid) {
    return grid.tree().root().maximum();
}

/**
 * A grid's value at a point of its index space, interpolated between the eight voxels around it.
 * A voxel's value lies at its integer coordinates, so the point reads the voxels from its floor
 * to one past it on each axis.
 **/
MOYA_KERNEL inline float valueAt(const nanovdb::FloatGrid & grid, const glm::vec3 & index) {
    const glm::vec3 below = glm::floor(index);
    const glm::vec3 along = index - below;
    const auto x = static_cast<std::int32_t>(below.x);
    const auto y = static_cast<std::int32_t>(below.y);
    const auto z = static_cast<std::int32_t>(below.z);
    const nanovdb::FloatGrid::AccessorType access = grid.getAccessor();
    float corners[8];
    for (std::int32_t corner = 0; corner < 8; ++corner) {
        corners[corner] = access.getValue(nanovdb::Coord(x + (corner & 1), y + ((corner >> 1) & 1), z + ((corner >> 2) & 1)));
    }
    float alongX[4];
    for (std::int32_t row = 0; row < 4; ++row) {
        const float low = corners[row + row];
        const float high = corners[row + row + 1];
        alongX[row] = low + (along.x * (high - low));
    }
    const float near = alongX[0] + (along.y * (alongX[1] - alongX[0]));
    const float far = alongX[2] + (along.y * (alongX[3] - alongX[2]));
    return near + (along.z * (far - near));
}

/**
 * The most of the node or tile of a side that holds a voxel: a node's most, a tile's value, or
 * the background beyond every node. A coarser tile holds the voxel at a side less than its own,
 * and gives its value.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the voxel, then the side
MOYA_KERNEL inline float mostAt(const nanovdb::FloatGrid & grid, const nanovdb::Coord & voxel, std::int32_t side) {
    const nanovdb::NanoRoot<float> & root = grid.tree().root();
    const nanovdb::NanoRoot<float>::Tile * tile = root.probeTile(voxel);
    if (tile == nullptr) {
        return root.background();
    }
    if (!tile->isChild()) {
        return tile->value;
    }
    const nanovdb::NanoUpper<float> * upper = root.getChild(tile);
    if (side >= kUpperSide) {
        return upper->maximum();
    }
    const nanovdb::NanoLower<float> * lower = upper->probeChild(voxel);
    if (lower == nullptr) {
        return upper->getValue(voxel);
    }
    if (side >= kLowerSide) {
        return lower->maximum();
    }
    const nanovdb::NanoLeaf<float> * leaf = lower->probeChild(voxel);
    return leaf == nullptr ? lower->getValue(voxel) : leaf->maximum();
}

/**
 * The cell that holds a voxel: the smallest node or tile of the grid's tree around it, no smaller
 * than a leaf. A point whose floor lies in the cell reads voxels up to one past its far side.
 * The most is taken over the cell and the seven cells of its side beyond it on each axis.
 *
 * A node's most is that of the values it holds that are active, so an inactive voxel is taken to
 * hold no more than the active ones of its node.
 **/
MOYA_KERNEL inline Cell cellAt(const nanovdb::FloatGrid & grid, const glm::ivec3 & voxel) {
    const nanovdb::Coord held(voxel.x, voxel.y, voxel.z);
    const nanovdb::NanoRoot<float> & root = grid.tree().root();
    Cell out;
    const nanovdb::NanoRoot<float>::Tile * tile = root.probeTile(held);
    if (tile != nullptr && tile->isChild()) {
        out.side = root.getChild(tile)->probeChild(held) == nullptr ? kLowerSide : kLeafSide;
    }
    const std::int32_t mask = ~(out.side - 1);
    out.origin = glm::ivec3 { voxel.x & mask, voxel.y & mask, voxel.z & mask };
    out.most = -std::numeric_limits<float>::infinity();
    for (std::int32_t corner = 0; corner < 8; ++corner) {
        const nanovdb::Coord beside(out.origin.x + ((corner & 1) * out.side), out.origin.y + (((corner >> 1) & 1) * out.side),
            out.origin.z + (((corner >> 2) & 1) * out.side));
        out.most = std::fmax(out.most, mostAt(grid, beside, out.side));
    }
    return out;
}

/**
 * The parameter at which a ray of index space leaves a cell: where it first crosses the far face
 * of the cell's slab on an axis it moves along. A ray that moves along no axis never leaves.
 **/
MOYA_KERNEL inline float leaving(const Cell & cell, const glm::vec3 & origin, const glm::vec3 & direction) {
    float out = std::numeric_limits<float>::infinity();
    for (glm::length_t axis = 0; axis < 3; ++axis) {
        const auto lower = static_cast<float>(cell.origin[axis]);
        if (direction[axis] > 0.0F) {
            out = std::fmin(out, (lower + static_cast<float>(cell.side) - origin[axis]) / direction[axis]);
        } else if (direction[axis] < 0.0F) {
            out = std::fmin(out, (lower - origin[axis]) / direction[axis]);
        }
    }
    return out;
}

};  // namespace v3d::moya::kernel::volume
