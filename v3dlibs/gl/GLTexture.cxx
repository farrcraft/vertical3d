/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "GLTexture.h"

#include <GL/glew.h>

#include <cassert>
#include <iostream>

namespace v3d::gl {
    GLTexture::GLTexture(const boost::shared_ptr<v3d::core::Logger> & logger) : logger_(logger) {
    }

    GLTexture::GLTexture(boost::shared_ptr<v3d::image::Image> image, const boost::shared_ptr<v3d::core::Logger> & logger) :
        logger_(logger) {
        create(image);
    }

    GLTexture::GLTexture(const v3d::image::Texture & t) : v3d::image::Texture(t) {
    }

    GLTexture::~GLTexture() {
        if (!isnull()) {
            unsigned int tex_id = id();
            glDeleteTextures(1, &tex_id);
        }
    }

    void GLTexture::wrap(bool repeat) {
        Texture::wrap(repeat);
        if (!bind()) {
            return;
        }
        if (!repeat) {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        } else {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
    }

    bool GLTexture::bind() {
        unsigned int tex_id = id();
        if (tex_id != -1) {
            glBindTexture(GL_TEXTURE_2D, tex_id);
        } else {
            LOG_ERROR(logger_) << "GLTexture::bind - bad texture id!";
            return false;
        }

        return true;
    }

    bool GLTexture::create(boost::shared_ptr<v3d::image::Image> image) {
        Texture::create(image);
        unsigned int tex_id = id();
        // Build A Texture From The Data
        glGenTextures(1, &tex_id);  // Generate OpenGL texture IDs
        id(tex_id);

        glBindTexture(GL_TEXTURE_2D, tex_id);  // Bind Our Texture

        int format = GL_RGB;
        int internalformat = GL_RGB;
        if (image->bpp() == 24) {  // is the image 24 bits?
            format = GL_RGB;
            internalformat = GL_RGB;
        } else if (image->bpp() == 8) {
            format = GL_ALPHA;
            internalformat = GL_ALPHA8;
        }

        // test to make sure texture will fit first
        glTexImage2D(GL_PROXY_TEXTURE_2D, 0, internalformat, image->width(), image->height(), 0, format, GL_UNSIGNED_BYTE, image->data());
        GLint params[1];
        glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, params);
        if (params[0] == 0) {
            return false;
        }
        // load the image data into texture memory
        glTexImage2D(GL_TEXTURE_2D, 0, internalformat, image->width(), image->height(), 0, format, GL_UNSIGNED_BYTE, image->data());

        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);  // Linear Filtered
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // Linear Filtered

        if (!Texture::wrap()) {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        } else {
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }

        return true;
    }

};  // namespace v3d::gl
