/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec4.hpp>

/**
 * How one block type reflects the light, as an element of the palette in SceneUniforms. Its
 * members are vec4 for the reason that struct gives.
 **/
struct MaterialInfo {
    glm::vec4 ambient;   /**< reflectivity in rgb **/
    glm::vec4 diffuse;
    glm::vec4 specular;  /**< reflectivity in rgb, the shininess exponent in w **/
};
