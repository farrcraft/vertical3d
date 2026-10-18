/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec4.hpp>

/**
 * How one block type reflects the light, as an element of the palette in SceneUniforms. Its
 * members are vec4, because std140 pads a vec3 to the size of a vec4.
 **/
struct MaterialInfo {
    glm::vec4 ambient;   /**< reflectivity in rgb **/
    glm::vec4 diffuse;
    glm::vec4 specular;  /**< reflectivity in rgb, the shininess exponent in w **/
};
