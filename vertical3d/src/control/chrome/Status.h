/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace v3d::editor::control::chrome {

/**
 * What the status bar along the foot of the window says.
 *
 * On the left it says what each mouse button does in the active view, under the tools and the
 * modifiers held. On the right it says what is selected, the frame the timeline is at, and the
 * frame rate.
 **/
struct Status final {
    /**
     * A mouse button and what a drag or a click of it does.
     **/
    struct Button final {
        std::string name;       /**< the button's name, which shows when its icon does not **/
        std::string action;
        std::string icon;       /**< the icon of a mouse with the button lit **/
        std::string modifier;   /**< a key held with the button for this action, or empty **/
    };

    /**
     * How the frame rate stands against the monitor's.
     **/
    enum class Health : std::uint8_t { Good, Low, Poor };

    std::vector<Button> buttons;
    std::string selection;
    std::string frame;
    std::string rate;   /**< empty before a frame has been measured **/
    Health health = Health::Good;
};

};  // namespace v3d::editor::control::chrome
