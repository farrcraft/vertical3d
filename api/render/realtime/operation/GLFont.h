/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../Operation.h"
#include "../../../font/Font2D.h"
#include "../../../gl/GLTexture.h"
#include "../../../log/Logger.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::render::realtime::operation {
    /**
     * Draw a GL font at position
     **/
    class GLFont : public Operation {
     public:
        /**
         *
         **/
        GLFont(const v3d::font::Font2D& font, const std::string& text, const glm::vec2& position, const boost::shared_ptr<v3d::log::Logger>& logger);

        /**
         *
         **/
        bool run(boost::shared_ptr<Context> context);

     private:
        v3d::font::Font2D font_;
        glm::vec2 position_;
        std::string text_;
        v3d::gl::GLTexture texture_;
    };
};  // namespace v3d::render::realtime::operation
