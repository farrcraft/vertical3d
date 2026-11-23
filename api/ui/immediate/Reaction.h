/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::ui::immediate {

/**
 * What a widget's box did with the cursor this frame.
 **/
struct Reaction final {
    Reaction() noexcept;

    bool hovered;  /**< the cursor is on it, and was on nothing over it last frame **/
    bool held;     /**< a press went down on it and has not come up **/
    bool clicked;  /**< a press and a release both landed on it **/
};

};  // namespace v3d::ui::immediate
