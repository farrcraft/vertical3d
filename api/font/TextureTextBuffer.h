/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
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
        /** The size the text is laid out at; the font's metrics are scaled by its ratio to the font's own size. **/
        float size_ = 0.0f;
        float gamma_ = 1.0f;
        glm::vec4 foregroundColor_{1.0f};
        /** Transparent unless a background quad is wanted behind each glyph. **/
        glm::vec4 backgroundColor_{0.0f};
        bool underline_ = false;
        glm::vec4 underlineColor_{1.0f};
        bool overline_ = false;
        glm::vec4 overlineColor_{1.0f};
        bool strikethrough_ = false;
        glm::vec4 strikethroughColor_{1.0f};
        boost::shared_ptr<TextureFont> font_;
    };

    TextureTextBuffer();

    void addText(glm::vec2 * pen, const Markup & markup, const std::wstring & text);
    void clear() override;

    std::vector<float> & shift();
    std::vector<float> & gamma();

 protected:
    void addCharacter(glm::vec2 * pen, const Markup & markup, wchar_t current);
    void addVertex(const glm::vec3 & position, const glm::vec2 & texture, const glm::vec4 & color, float shift, float gamma);
    void addQuad(const glm::vec2 & xy0, const glm::vec2 & xy1, const glm::vec2 & uv0, const glm::vec2 & uv1, const glm::vec4 & color, float gamma);

 private:
    float ascender_;
    float descender_;
    size_t lineStart_;
    glm::vec2 origin_;

    // vertex data
    std::vector<float> shift_;
    std::vector<float> gamma_;
    std::vector<glm::ivec4> items_;
};
};  // namespace v3d::font
