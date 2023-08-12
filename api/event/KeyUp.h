/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Key.h"

#include <string>

namespace v3d::event {

    /**
     **/
    class KeyUp final : public Key {
     public:
        /**
         **/
        KeyUp(const std::string& name);
    };
};  // namespace v3d::event
