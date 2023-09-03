/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Context2D.h"
#include "../Frame.h"


#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
    /**
     **/
    class Scene2D {
     public:
        /**
         **/
        Scene2D(boost::shared_ptr<Context2D> context);

        /**
         **/
        boost::shared_ptr<Frame> collect();

     protected:
        boost::shared_ptr<v3d::render::realtime::Context2D> context_;
    };
};  // namespace v3d::render::realtime
