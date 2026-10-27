/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include <glm/vec3.hpp>

namespace diorama {

/**
 * One model standing in the diorama: the file it is drawn from, where its base stands, which way
 * it is turned about the vertical, and whether it casts a shadow.
 **/
struct Placement final {
    std::string model;      /**< a glTF file under data/ **/
    glm::vec3 position{0.0F};
    float yaw = 0.0F;       /**< radians, counterclockwise seen from above **/
    bool castsShadow = true;
};

/**
 * Where the well stands. Its voice is placed there too.
 **/
inline constexpr glm::vec3 WELL(1.5F, 0.0F, 1.0F);

/**
 * How long the sun takes to go once around the sky, in seconds.
 **/
inline constexpr float DAY = 120.0F;

/**
 * @return the village: a ground that casts nothing, two houses, a ring of trees and the well
 **/
std::vector<Placement> layout();

/**
 * The direction towards the sun, a time into the day. The sun goes around the sky once a DAY at
 * a fixed height, so it never sets and the scene is always lit.
 *
 * @param time seconds since the day began
 * @return a unit vector, above the horizon
 **/
glm::vec3 sunlight(float time);

};  // namespace diorama
