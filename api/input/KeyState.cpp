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
bool KeyState::operator() (const std::string& c) {
    std::vector<std::string>::const_iterator iter = std::find(keys_.begin(), keys_.end(), c);
    bool held = true;
    if (iter != keys_.end()) {
        keys_.erase(iter);
        held = false;
    } else {
        keys_.push_back(c);
    }
    return held;
}

};  // namespace v3d::input
