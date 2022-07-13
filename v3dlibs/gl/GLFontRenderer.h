/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../font/Font2D.h"
#include "GLTexture.h"
#include "../core/Logger.h"

#include <boost/shared_ptr.hpp>

namespace v3d::gl {

    /**
     * A OpenGL Font renderer.
     */
    class GLFontRenderer {
     public:
        explicit GLFontRenderer(const boost::shared_ptr<v3d::core::Logger> & logger);
        GLFontRenderer(const v3d::font::Font2D &f, const boost::shared_ptr<v3d::core::Logger>& logger);
        virtual ~GLFontRenderer();

        /**
         * 
         * Render the the text to the current drawing surface
         * @param text the text to be printed
         * @param x the left position to print
         * @param y the top of the position to begin printing
         */
        void print(const std::string & text, float x, float y);


     private:
        v3d::font::Font2D font_;
        GLTexture texture_;
    };

};  // namespace v3d::gl
