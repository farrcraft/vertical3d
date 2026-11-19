/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/volume/Block.h>

#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::moya::volume {

/**
 * A grid of floats read from an OpenVDB file, held as kernel code reads it: the NanoVDB buffer its
 * conversion gives, with each node's least and most. lower and upper bound, in the volume's own
 * space, every point at which a lookup can read an active voxel, which reaches one voxel past
 * them. A grid of no active voxel bounds nothing, and lower lies above upper.
 **/
struct Grid {
    std::vector<kernel::volume::Block> blocks;
    glm::vec3 lower { 0.0F };
    glm::vec3 upper { -1.0F };
};

/**
 * The grid of a name in an OpenVDB file, converted to NanoVDB by NanoVDB's own tools.
 *
 * @throws std::runtime_error naming why, when the file cannot be read, holds no grid of the name,
 *         or holds one that is not of floats
 **/
Grid read(const std::string & path, const std::string & name);

};  // namespace v3d::moya::volume
