/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/ui/immediate/Id.h>

#include <optional>
#include <string>

#include <glm/vec2.hpp>

namespace v3d::ui::immediate {

/**
 * The tooltip of the widget under the cursor, drawn over everything at end().
 **/
struct Tip final {
    Id id = 0;               /**< the widget the cursor rests on, or nothing **/
    bool asked = false;      /**< whether that widget was drawn under the cursor this frame **/
    float since = 0.0F;      /**< when the cursor came to rest on it, on the layer's clock **/
    std::optional<float> shown;  /**< when a tooltip was last drawn, on the layer's clock, or never **/
    bool hidden = false;     /**< whether a press put it away **/
    glm::vec2 pressAt{0.0F}; /**< where that press went down, which the cursor has to leave **/
    std::string text;
    std::string keys;
    glm::vec2 boxMin{0.0F};  /**< the widget's box, which the tooltip hangs under **/
    glm::vec2 boxMax{0.0F};
};

};  // namespace v3d::ui::immediate
