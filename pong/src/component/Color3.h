/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/vec3.hpp>

/**
 * An entity's colour, as red, green and blue. A plain aggregate, written directly.
 **/
struct Color3 final {
    glm::vec3 value{1.0F};
};

