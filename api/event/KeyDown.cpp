/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "KeyDown.h"

namespace v3d::event {

    /**
     **/
    KeyDown::KeyDown(const std::string_view& name) :
        Key(name, true) {
    }

};  // namespace v3d::event
