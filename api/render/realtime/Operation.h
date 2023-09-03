/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Context.h"

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
    /**
     **/
    class Operation {
     public:
        /**
         **/
        virtual bool run(boost::shared_ptr<Context> context) = 0;
    };
};  // namespace v3d::render::realtime
