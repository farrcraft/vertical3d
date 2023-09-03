/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "TextBuffer.h"

#include <glm/glm.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::font {

    class BitmapFont;

    class BitmapTextBuffer : public TextBuffer {
     public:
        /**
            * Construct a new text buffer
            *
            * @param font the bitmap font to be rendered with this buffer
            * @param program the text GL shader program used for bitmap font rendering
            */
        explicit BitmapTextBuffer(boost::shared_ptr<BitmapFont> font);

        /**
            * Allocate the text buffer with a string of text
            *
            * @param vec2 pen window position to draw text
            * @param string txt string of text
            * @param vec4 color the text color
            */
        void text(glm::vec2 * pen, const std::string & txt, const glm::vec4 & color);

        /**
            * Calculate the width of a string of text
            *
            * @param string txt string of text
            * 
            * @return size_t pixel width of the string when rendered
            */
        size_t width(const std::string & txt);

     private:
        boost::shared_ptr<BitmapFont> font_;
    };
}  // namespace v3d::font
