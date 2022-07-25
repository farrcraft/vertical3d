/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Key.h"

#include <string_view>

namespace v3d::event {

    /**
     **/
    class KeyDown final : public Key {
     public:
        /**
         **/
        KeyDown(const std::string_view& name);
    };
};  // namespace v3d::event
