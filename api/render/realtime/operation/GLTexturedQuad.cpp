/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "GLTexturedQuad.h"

namespace v3d::render::realtime::operation {

	/**
		**/
	GLTexturedQuad::GLTexturedQuad(boost::shared_ptr<v3d::gl::GLTexture> texture, const glm::vec2& position, const glm::vec2& dimensions) :
		dimensions_(dimensions),
		position_(position),
		texture_(texture) {

	}

	/**
		**/
	bool GLTexturedQuad::run(boost::shared_ptr<Context> context) {
        // get texturing state
        GLboolean texture_enabled;
        texture_enabled = glIsEnabled(GL_TEXTURE_2D);
        // enable states if they weren't already
        if (texture_enabled == GL_FALSE)
            glEnable(GL_TEXTURE_2D);

        if (!texture_) {
            return false;
        }
        texture_->bind();

        glPushMatrix();
        glTranslatef(position_.x, position_.y, 0.0f);

        glScalef(dimensions_.x, dimensions_.y, 1.0f);

        // draw quad
        glBegin(GL_QUADS);

        glTexCoord2f(0.0f, 0.0f);
        glVertex3f(0.0f, 1.0f, 0.0f);

        glTexCoord2f(1.0f, 0.0f);
        glVertex3f(1.0f, 1.0f, 0.0f);

        glTexCoord2f(1.0f, 1.0f);
        glVertex3f(1.0f, 0.0f, 0.0f);

        glTexCoord2f(0.0f, 1.0f);
        glVertex3f(0.0f, 0.0f, 0.0f);

        glEnd();

        glPopMatrix();

        // reset texture state to initial setting
        if (texture_enabled == GL_FALSE)
            glDisable(GL_TEXTURE_2D);

		return true;
	}

};  // namespace v3d::render::realtime::operation
