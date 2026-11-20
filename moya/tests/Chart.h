/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <array>
#include <vector>

#include <glm/vec3.hpp>

namespace v3d::moya::chart {

/**
 * A chart of 24 moderate colours in ACEScg: each of six hues, at two saturations and two
 * brightnesses, mixed with grey so that a reflectance reaches each. The hues run red, yellow,
 * green, cyan, blue and magenta, and each hue's four colours are in the order dim and pale, bright
 * and pale, dim and saturated, bright and saturated.
 **/
inline std::vector<glm::vec3> colours() {
    const std::array<glm::vec3, 6> hues { glm::vec3(1, 0, 0), glm::vec3(1, 1, 0), glm::vec3(0, 1, 0), glm::vec3(0, 1, 1), glm::vec3(0, 0, 1),
        glm::vec3(1, 0, 1) };
    std::vector<glm::vec3> out;
    for (const glm::vec3 & hue : hues) {
        for (const float saturation : { 0.25F, 0.5F }) {
            for (const float brightness : { 0.2F, 0.8F }) {
                out.push_back(brightness * ((glm::vec3(1.0F - saturation)) + (saturation * hue)));
            }
        }
    }
    return out;
}

};  // namespace v3d::moya::chart
