/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/ui/immediate/Id.h>

#include <string>
#include <vector>

#include <glm/vec2.hpp>

namespace v3d::ui::immediate {

/**
 * The list an open combo shows, drawn over everything at end().
 **/
struct Popup final {
    Popup() noexcept;

    Id id;             /**< the combo that is open, or nothing **/
    bool asked;        /**< whether the combo was drawn this frame **/
    glm::vec2 boxMin;  /**< the combo's own box, where a press does not close the list **/
    glm::vec2 boxMax;
    std::vector<std::string> items;
    int selected;
};

};  // namespace v3d::ui::immediate
