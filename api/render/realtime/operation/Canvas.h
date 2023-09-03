/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../Operation.h"
#include "../../../gl/Canvas.h"
#include "../../../gl/Program.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::render::realtime::operation {
    /**
     * Draw a GL texture at position
     **/
    class Canvas : public Operation {
     public:
        /**
         *
         **/
        Canvas(boost::shared_ptr<v3d::gl::Canvas> canvas, boost::shared_ptr<v3d::gl::Program> program);

        /**
         *
         **/
        bool run(boost::shared_ptr<Context> context);

     private:
        boost::shared_ptr<v3d::gl::Canvas> canvas_;
        boost::shared_ptr<v3d::gl::Program> program_;
    };
};  // namespace v3d::render::realtime::operation
