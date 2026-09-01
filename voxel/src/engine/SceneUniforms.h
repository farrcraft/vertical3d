/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>

#include <glm/vec4.hpp>

/**
 * The block palette and the one light, as set 1 of the voxel pipeline.
 *
 * This is the mirror of the Scene block in shaders/voxel.vert and has to stay laid out the
 * way std140 lays that block out: every member is a vec4 because std140 rounds a vec3 up to
 * one anyway, and writing them as vec4 here is what makes the C++ struct copyable straight
 * into the uniform buffer rather than needing padding of its own.
 **/
struct MaterialInfo {
    glm::vec4 ambient;   /**< reflectivity in rgb **/
    glm::vec4 diffuse;
    glm::vec4 specular;  /**< reflectivity in rgb, the shininess exponent in w **/
};

/**
 * How many block types the palette holds, which is every solid type Voxel declares. The
 * shader declares the same count, so the two have to be changed together.
 **/
const unsigned int materialCount = 16;

/**
 **/
struct SceneUniforms {
    glm::vec4 lightPosition;  /**< in world space **/
    glm::vec4 ambient;        /**< light intensities **/
    glm::vec4 diffuse;
    glm::vec4 specular;
    std::array<MaterialInfo, materialCount> materials;
};
