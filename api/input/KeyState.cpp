/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "KeyState.h"

#include <algorithm>
#include <string>
#include <vector>

namespace v3d::input {

/**
 **/
bool KeyState::held(std::string_view c) const {
    std::vector<std::string>::const_iterator iter = std::find(keys_.begin(), keys_.end(), c);
    return iter != keys_.end();
}

/**
 **/
bool KeyState::pressed(std::string_view c) const {
    return std::find(pressed_.begin(), pressed_.end(), c) != pressed_.end();
}

/**
 **/
bool KeyState::released(std::string_view c) const {
    return std::find(released_.begin(), released_.end(), c) != released_.end();
}

/**
 **/
void KeyState::flush() {
    pressed_.clear();
    released_.clear();
}

/**
 **/
bool KeyState::operator() (const std::string& c) {
    std::vector<std::string>::const_iterator iter = std::find(keys_.begin(), keys_.end(), c);
    bool held = true;
    if (iter != keys_.end()) {
        keys_.erase(iter);
        held = false;
    } else {
        keys_.push_back(c);
    }
    // both edges can be true of one key in one frame, so this records rather than replaces:
    // a key pressed and released between two flushes answers both, and held neither
    if (held) {
        pressed_.push_back(c);
    } else {
        released_.push_back(c);
    }
    return held;
}

};  // namespace v3d::input
