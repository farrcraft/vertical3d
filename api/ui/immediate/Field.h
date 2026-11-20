/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/ui/Image.h>
#include <api/ui/immediate/Id.h>

#include <string>

#include <glm/vec2.hpp>

namespace v3d::ui::immediate {

/**
 * A text field being drawn: its id, its label, the value it shows, its box, and what leads it.
 **/
struct Field {
    Id id = 0;
    std::string label;
    const std::string* value = nullptr;
    glm::vec2 min{0.0F};
    glm::vec2 max{0.0F};
    Image icon;                // drawn at the left, or nothing when it is not valid
    float iconSide = 0.0F;     // how big the icon is drawn
    float start = 0.0F;        // where the label starts, past the icon
    bool labelled = false;     // the label is drawn after the icon, and not as a placeholder
    glm::vec2 low{0.0F};       // where the text starts, past both
    glm::vec2 high{0.0F};      // where the text ends
};

};  // namespace v3d::ui::immediate
