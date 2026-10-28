/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>

#include "MaterialInfo.h"

#include <glm/vec4.hpp>

/**
 * How many block types the palette holds, which is every solid type Voxel declares. The
 * shader declares the same count, so the two have to be changed together.
 **/
const unsigned int materialCount = 16;

/**
 * The block palette and the one light, as set 1 of the voxel pipeline.
 *
 * This mirrors the Scene block in shaders/voxel.vert, and has to match the std140 layout of
 * that block. Every member is a vec4, because std140 pads a vec3 to the size of a vec4
 * anyway. With vec4 members the struct can be copied straight into the uniform buffer, with
 * no padding of its own.
 **/
struct SceneUniforms {
    glm::vec4 lightPosition;  /**< in world space **/
    glm::vec4 ambient;        /**< light intensities **/
    glm::vec4 diffuse;
    glm::vec4 specular;
    std::array<MaterialInfo, materialCount> materials;
};
