/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Layout.h"

#include <cmath>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/vec3.hpp>

namespace diorama {

namespace {

/** How high the sun stands above the horizon, in radians. **/
constexpr float SUN_HEIGHT = 0.85F;

/** Where the two houses stand and which way each is turned, and the well's turn. **/
constexpr glm::vec3 WEST_HOUSE(-2.0F, 0.0F, -1.5F);
constexpr float WEST_HOUSE_YAW = 0.3F;
constexpr glm::vec3 EAST_HOUSE(2.5F, 0.0F, -2.5F);
constexpr float EAST_HOUSE_YAW = -0.6F;
constexpr float WELL_YAW = 0.4F;

/** How many trees stand in the ring around the village, and how far out. **/
constexpr int TREES = 9;
constexpr float RING = 6.0F;

};  // namespace

std::vector<Placement> layout() {
    std::vector<Placement> placed{
        {.model = "models/ground.glb", .castsShadow = false},
        {.model = "models/house.glb", .position = WEST_HOUSE, .yaw = WEST_HOUSE_YAW},
        {.model = "models/house.glb", .position = EAST_HOUSE, .yaw = EAST_HOUSE_YAW},
        {.model = "models/well.glb", .position = WELL, .yaw = WELL_YAW},
    };
    for (int tree = 0; tree < TREES; tree++) {
        const float around = (static_cast<float>(tree) + 0.5F) * glm::two_pi<float>() / static_cast<float>(TREES);
        // every other tree stands a little further in, so the ring does not read as a circle
        const float distance = RING - ((tree % 2 == 0) ? 0.0F : 0.8F);
        placed.push_back({.model = "models/tree.glb",
            .position = glm::vec3(std::cos(around) * distance, 0.0F, std::sin(around) * distance), .yaw = around});
    }
    return placed;
}

glm::vec3 sunlight(float time) {
    const float around = glm::two_pi<float>() * time / DAY;
    return glm::normalize(glm::vec3(
        std::cos(SUN_HEIGHT) * std::cos(around), std::sin(SUN_HEIGHT), std::cos(SUN_HEIGHT) * std::sin(around)));
}

};  // namespace diorama
