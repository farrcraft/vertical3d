/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Operation2D.h"

namespace v3d::render::realtime {

    bool Operation2D::run(boost::shared_ptr<Context2D> context) {
        if (!Operation::run(context)) {
            return false;
        }
        return true;
    }

};  // namespace v3d::render::realtime
