/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MouseState.h"

#include <algorithm>
#include <vector>

namespace v3d::input {

/**
 **/
MouseState::MouseState() : position_(0.0f, 0.0f) {
}

/**
 **/
bool MouseState::pressed(unsigned int button) const {
    std::vector<unsigned int>::const_iterator iter = std::find(buttons_.begin(), buttons_.end(), button);
    return iter != buttons_.end();
}

/**
 **/
bool MouseState::operator() (unsigned int button) {
    std::vector<unsigned int>::const_iterator iter = std::find(buttons_.begin(), buttons_.end(), button);
    bool pressed = true;
    if (iter != buttons_.end()) {
        buttons_.erase(iter);
        pressed = false;
    } else {
        buttons_.push_back(button);
    }
    return pressed;
}

/**
 **/
glm::vec2 MouseState::operator() (const glm::vec2& p) {
    glm::vec2 previous = position_;
    position_ = p;
    return previous;
}

/**
 **/
glm::vec2 MouseState::position() const {
    return position_;
}

};  // namespace v3d::input
