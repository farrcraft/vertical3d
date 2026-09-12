/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <glm/glm.hpp>

namespace v3d::input {

/**
 * MouseState keeps track of the cursor position and of which buttons are currently held,
 * based on all of the motion and button events we've seen. It is the mouse counterpart of
 * KeyState.
 *
 * A button is named rather than numbered, the way a key is. The names are the ones a
 * binding config uses - "left", "middle", "right", "x1", "x2" - so a consumer asking
 * whether the left button is down needs no SDL header to say which one that is.
 **/
class MouseState final {
 public:
    MouseState();

    /**
     * Is a button currently held?
     *
     * @param button the button's name
     *
     * @return bool
     **/
    bool held(std::string_view button) const;

    /**
     * Toggle the state of a button
     *
     * @param button the name of the button being toggled
     *
     * @return bool true if the resulting state is a held button
     **/
    bool operator() (const std::string& button);

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
    std::vector<std::string> buttons_;
    glm::vec2 position_;
};

};  // namespace v3d::input
