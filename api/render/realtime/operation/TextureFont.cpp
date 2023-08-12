/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "TextureFont.h"

#include <GL/glew.h>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace v3d::render::realtime::operation {

    /**
         * @param text the text to be printed
         * @param position the top left position to begin printing
     **/
    TextureFont::TextureFont(boost::shared_ptr<v3d::font::TextureTextBuffer> buffer, boost::shared_ptr<v3d::gl::Program> program,
        boost::shared_ptr<v3d::image::TextureAtlas> atlas, const boost::shared_ptr<v3d::log::Logger>& logger) :
        program_(program),
        buffer_(buffer),
        atlas_(atlas),
        vertexBuffer_(v3d::gl::VertexBuffer::BUFFER_TYPE_DYNAMIC),
        texture_(logger),
        logger_(logger) {
        texture_.create(atlas_->image());
    }

    boost::shared_ptr<v3d::font::TextureTextBuffer> TextureFont::buffer() {
        return buffer_;
    }


    void TextureFont::resize(float width, float height) {
        // update orthographic view matrix used for text rendering
        program_->enable();
        unsigned int MVPMatrix = program_->uniform("MVPMatrix");
        glm::mat4 mvp = glm::ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
        glUniformMatrix4fv(MVPMatrix, 1, GL_FALSE, glm::value_ptr(mvp));
        program_->disable();
    }

    void TextureFont::upload() {
        vertexBuffer_.attribute(0, 3, v3d::gl::VertexBuffer::ATTRIBUTE_TYPE_VERTEX, buffer_->vertices().size());
        vertexBuffer_.attribute(1, 2, v3d::gl::VertexBuffer::ATTRIBUTE_TYPE_NORMAL, buffer_->uvs().size());
        vertexBuffer_.attribute(2, 4, v3d::gl::VertexBuffer::ATTRIBUTE_TYPE_COLOR, buffer_->colors().size());
        vertexBuffer_.attribute(3, 1, v3d::gl::VertexBuffer::ATTRIBUTE_TYPE_GENERIC, buffer_->shift().size());
        vertexBuffer_.attribute(4, 1, v3d::gl::VertexBuffer::ATTRIBUTE_TYPE_GENERIC, buffer_->gamma().size());

        vertexBuffer_.allocate();

        vertexBuffer_.data3f(0, buffer_->vertices());
        vertexBuffer_.data2f(1, buffer_->uvs());
        vertexBuffer_.data4f(2, buffer_->colors());
        vertexBuffer_.data1f(3, buffer_->shift());
        vertexBuffer_.data1f(4, buffer_->gamma());

        vertexBuffer_.indices(buffer_->indices());
    }

    /**
     **/
    bool TextureFont::run(boost::shared_ptr<Context> context) {
        upload();

        glEnable(GL_BLEND);
        program_->enable();
        if (atlas_->depth() == 1) {
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            vertexBuffer_.render();
            program_->disable();
            return;
        }
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glBlendColor(1.0f, 1.0f, 1.0f, 1.0f);
        unsigned int texture = program_->uniform("texture");
        glUniform1i(texture, 0);
        unsigned int pixel = program_->uniform("pixel");
        float width = 1.0f / static_cast<float>(atlas_->width());
        float height = 1.0f / static_cast<float>(atlas_->height());
        float depth = static_cast<float>(atlas_->depth());
        glUniform3f(pixel, width, height, depth);
        vertexBuffer_.render();
        program_->disable();

        return true;
    }

};  // namespace v3d::render::realtime::operation
