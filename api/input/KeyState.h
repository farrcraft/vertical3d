/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string_view>
#include <vector>

namespace v3d::input {

    /**
     * KeyState keeps track of the current state of all keys based on all of the key up/down
     * events that we've seen.
     **/
    class KeyState {
     public:
        /**
         * Update the state to indicate that a key is currently being pressed
         * 
         * @return bool
         **/
        bool pressed(std::string_view c) const;

        /**
         * Toggle the state of the key
         * 
         * @param c The key being toggled
         * 
         * @return bool true if the resulting state is a pressed key
         **/
        bool operator() (std::string_view c);

     private:
        std::vector<std::string_view> keys_;
    };

};  // namespace v3d::input
