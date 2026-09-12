/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace v3d::input {

/**
 * KeyState keeps track of the current state of all keys based on all of the key up/down
 * events that we've seen.
 **/
class KeyState final {
 public:
    /**
     * Is a key currently held?
     *
     * @return bool
     **/
    bool held(std::string_view c) const;

    /**
     * Toggle the state of the key
     *
     * @param c The key being toggled
     *
     * @return bool true if the resulting state is a held key
     **/
    bool operator() (const std::string& c);

 private:
    std::vector<std::string> keys_;
};

};  // namespace v3d::input
