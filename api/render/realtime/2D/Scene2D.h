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
        explicit Scene2D(boost::shared_ptr<Context2D> context);

        /**
         **/
        virtual ~Scene2D() = default;

        /**
         * Build the frame for this tick. An app overrides this to add its own operations;
         * the base collects nothing.
         **/
        virtual boost::shared_ptr<Frame> collect();

     protected:
        boost::shared_ptr<v3d::render::realtime::Context2D> context_;
    };
};  // namespace v3d::render::realtime
