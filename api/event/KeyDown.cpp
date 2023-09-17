/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "KeyDown.h"

namespace v3d::event {

    /**
     **/
    KeyDown::KeyDown(const std::string& name, const boost::shared_ptr<Context>& context) :
        Key(name, context, true) {
    }

};  // namespace v3d::event
