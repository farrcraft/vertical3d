/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/ui/Transition.h>

#include <cstdint>

#include <glm/vec2.hpp>

namespace v3d::ui::immediate {

/**
 * The few bits a widget cannot work out again from the state it reads.
 **/
struct Retained final {
    Retained() noexcept;

    std::uint64_t frame;  /**< the last frame that asked for it, which it is aged out from **/
    unsigned int tab;  /**< which tab of a strip is selected **/
    unsigned int tabs; /**< how many tabs the strip held last frame, which share its width out **/
    float scroll;      /**< how far the content is scrolled up, in pixels **/
    float content;     /**< how tall what it held came to last frame **/
    glm::vec2 offset;  /**< how far a window has been dragged from where the caller put it **/
    bool collapsed;    /**< whether a window is folded to its title bar **/
    bool dragging;     /**< whether the press on its bar has travelled far enough to move it **/
    int chosen;        /**< the item a combo's list was clicked on, taken next frame, or -1 **/
    Transition face;   /**< the fill of its box, easing from one colour to the next **/
};

};  // namespace v3d::ui::immediate
