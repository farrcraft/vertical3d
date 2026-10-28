/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "TextBuffer.h"

#include <glm/glm.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::font {
class TextureFont;
/**
 * A text buffer for texture fonts
 */
class TextureTextBuffer : public TextBuffer {
 public:
    /**
     * How a run of text is drawn: the size it is laid out at, its colours, and the lines drawn
     * with it. What is here is what addText() honours.
     **/
    struct Markup {
        /**
         * The size the text is laid out at; the font's metrics are scaled by its ratio to the
         * font's own size. Zero or less means the font's own size.
         **/
        float size_ = 0.0F;
        float gamma_ = 1.0F;
        glm::vec4 foregroundColor_{1.0F};
        /** Transparent unless a background quad is wanted behind each glyph. **/
        glm::vec4 backgroundColor_{0.0F};
        bool underline_ = false;
        glm::vec4 underlineColor_{1.0F};
        bool overline_ = false;
        glm::vec4 overlineColor_{1.0F};
        bool strikethrough_ = false;
        glm::vec4 strikethroughColor_{1.0F};
        boost::shared_ptr<TextureFont> font_;
    };

    TextureTextBuffer();

    /**
     * Lay a UTF-8 string out from the pen, which is left after the last character.
     *
     * Only glyphs the font has already packed are drawn, so a layout never adds to an atlas
     * that may already be on the device. A code point with no packed glyph draws nothing and
     * moves the pen by nothing. The lines and the background need the font's SOLID glyph
     * packed too, and are left out without it. A newline moves the pen to the next line.
     **/
    void addText(glm::vec2 * pen, const Markup & markup, std::string_view utf8);
    void clear() override;

    std::vector<float> & shift();
    std::vector<float> & gamma();

 protected:
    void addCharacter(glm::vec2 * pen, const Markup & markup, char32_t current);
    void addVertex(const glm::vec3 & position, const glm::vec2 & texture, const glm::vec4 & color, float shift, float gamma);
    void addQuad(const glm::vec2 & xy0, const glm::vec2 & xy1, const glm::vec2 & uv0, const glm::vec2 & uv1, const glm::vec4 & color, float gamma);

 private:
    size_t lineStart_;
    glm::vec2 origin_;

    // vertex data
    std::vector<float> shift_;
    std::vector<float> gamma_;
    std::vector<glm::ivec4> items_;
};
};  // namespace v3d::font
