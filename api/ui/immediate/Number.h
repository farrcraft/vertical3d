/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/ui/immediate/Id.h>

#include <optional>
#include <string>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::immediate {

/**
 * A number field being drawn: its id, its label, the value it shows, and its box.
 **/
struct Number {
    Id id = 0;
    std::string label;
    float value = 0.0F;
    glm::vec2 min{0.0F};
    glm::vec2 max{0.0F};
    std::optional<glm::vec4> labelColour;  // in place of secondary text
    std::string unit;                      // written straight after the value
    std::optional<glm::vec4> edge;         // a rule down the left side
};

};  // namespace v3d::ui::immediate
