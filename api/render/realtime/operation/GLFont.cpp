/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "GLFont.h"

#include <GL/glew.h>

namespace v3d::render::realtime::operation {

    /**
         * @param text the text to be printed
         * @param position the top left position to begin printing
     **/
    GLFont::GLFont(const v3d::font::Font2D& font, const std::string& text, const glm::vec2& position, const boost::shared_ptr<v3d::log::Logger>& logger) :
        font_(font),
        position_(position),
        text_(text),
        texture_(logger) {
        texture_.create(font_.texture()->image());
    }

    /**
        **/
    bool GLFont::run(boost::shared_ptr<Context> context) {
        // set texture & blending states for font drawing
        glPushAttrib(GL_TEXTURE_BIT | GL_COLOR_BUFFER_BIT);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        if (!texture_.bind()) {
            return;
        }

        glBegin(GL_QUADS);
        float x = position_.x;
        for (size_t i = 0; i < text_.size(); i++) {
            const v3d::font::Font2D::Glyph* glyph = font_.glyph(text_[i]);

            glTexCoord2f(glyph->x1_, glyph->y1_);
            glVertex2f(x, position_.y);

            glTexCoord2f(glyph->x1_, glyph->y1_ + font_.textureHeight());
            glVertex2f(x, position_.y + font_.height());

            glTexCoord2f(glyph->x2_, glyph->y1_ + font_.textureHeight());
            glVertex2f(x + glyph->advance_, position_.y + font_.height());

            glTexCoord2f(glyph->x2_, glyph->y1_);
            glVertex2f(x + glyph->advance_, position_.y);

            x += glyph->advance_;
        }
        glEnd();

        // reset texture & blend states to their initial settings
        glPopAttrib();

        return true;
    }

};  // namespace v3d::render::realtime::operation
