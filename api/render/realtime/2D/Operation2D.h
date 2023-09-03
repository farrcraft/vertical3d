/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../Operation.h"
#include "Context2D.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
    /**
     **/
    class Operation2D : public Operation {
     public:
        /**
         **/
        virtual bool run(boost::shared_ptr<Context2D> context);
    };
};
