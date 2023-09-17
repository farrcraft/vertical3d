/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "KeyUp.h"

namespace v3d::event {

    /**
     **/
    KeyUp::KeyUp(const std::string& name, const boost::shared_ptr<Context>& context) :
        Key(name, context, false) {
    }

};  // namespace v3d::event
