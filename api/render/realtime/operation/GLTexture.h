/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../Operation.h"
#include "../../../gl/GLTexture.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::render::realtime::operation {
    /**
     * Draw a GL texture at position
     **/
    class GLTexture : public Operation {
     public:
        /**
         *
         **/
        GLTexture(boost::shared_ptr<v3d::gl::GLTexture> texture, const glm::vec2& position);

        /**
         *
         **/
        bool run(boost::shared_ptr<Context> context);

     private:
        glm::vec2 position_;
        boost::shared_ptr<v3d::gl::GLTexture> texture_;
    };
};  // namespace v3d::render::realtime::operation
