/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include <glm/glm.hpp>

namespace v3d::input {

/**
 * MouseState keeps track of the cursor position and of which buttons are currently held,
 * based on all of the motion and button events we've seen. It is the mouse counterpart of
 * KeyState.
 **/
class MouseState final {
 public:
    MouseState();

    /**
     * Is a button currently held?
     *
     * @param button the SDL button index
     *
     * @return bool
     **/
    bool pressed(unsigned int button) const;

    /**
     * Toggle the state of a button
     *
     * @param button the button being toggled
     *
     * @return bool true if the resulting state is a pressed button
     **/
    bool operator() (unsigned int button);

    /**
     * Move the cursor
     *
     * @param p the new cursor position
     *
     * @return the previous position
     **/
    glm::vec2 operator() (const glm::vec2& p);

    /**
     * @return the current cursor position
     **/
    glm::vec2 position() const;

 private:
    std::vector<unsigned int> buttons_;
    glm::vec2 position_;
};

};  // namespace v3d::input
