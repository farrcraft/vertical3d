/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Operation2D.h"

namespace v3d::render::realtime {

    bool Operation2D::run(boost::shared_ptr<Context> context) {
        boost::shared_ptr<Context2D> context2D = boost::dynamic_pointer_cast<Context2D>(context);
        if (!context2D) {
            return false;
        }
        return run2D(context2D);
    }

};  // namespace v3d::render::realtime
