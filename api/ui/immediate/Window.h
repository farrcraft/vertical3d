/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/ui/immediate/Id.h>

#include <glm/vec2.hpp>

namespace v3d::ui::immediate {

/**
 * The window being written, and the row state it interrupted.
 **/
struct Window final {
    Window() noexcept;

    bool open;           /**< whether a window is being written at all **/
    float margin;        /**< the row margin to go back to at endWindow() **/
    float right;         /**< and the right edge **/
    Id id;               /**< the id that holds the window's scroll and content **/
    Id scroll;           /**< the id its scrollbar is hit tested under **/
    glm::vec2 bodyMin;   /**< the part of it below the title bar, which is clipped to **/
    glm::vec2 bodyMax;
    float contentTop;    /**< where its content would start if it were not scrolled **/
    bool scrolls;
    bool clipped;
};

};  // namespace v3d::ui::immediate
