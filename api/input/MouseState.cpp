/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MouseState.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace v3d::input {

/**
 **/
MouseState::MouseState() : position_(0.0f, 0.0f) {
}

/**
 **/
bool MouseState::held(std::string_view button) const {
    std::vector<std::string>::const_iterator iter = std::find(buttons_.begin(), buttons_.end(), button);
    return iter != buttons_.end();
}

/**
 **/
bool MouseState::pressed(std::string_view button) const {
    return std::find(pressed_.begin(), pressed_.end(), button) != pressed_.end();
}

/**
 **/
bool MouseState::released(std::string_view button) const {
    return std::find(released_.begin(), released_.end(), button) != released_.end();
}

/**
 **/
void MouseState::flush() {
    pressed_.clear();
    released_.clear();
}

/**
 **/
bool MouseState::operator() (const std::string& button) {
    std::vector<std::string>::const_iterator iter = std::find(buttons_.begin(), buttons_.end(), button);
    bool held = true;
    if (iter != buttons_.end()) {
        buttons_.erase(iter);
        held = false;
    } else {
        buttons_.push_back(button);
    }
    // a click and its release inside one frame is both edges of one button, so this records
    // rather than replaces - the same rule KeyState keeps
    if (held) {
        pressed_.push_back(button);
    } else {
        released_.push_back(button);
    }
    return held;
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
