/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "BitmapFont.h"

#include <GL/glew.h>

namespace v3d::render::realtime::operation {

    /**
         * @param text the text to be printed
         * @param position the top left position to begin printing
     **/
    BitmapFont::BitmapFont(boost::shared_ptr<v3d::font::BitmapTextBuffer> buffer, boost::shared_ptr<v3d::gl::Program> program,
        const boost::shared_ptr<v3d::log::Logger>& logger) :
        program_(program),
        buffer_(buffer),
        vertexBuffer_(v3d::gl::VertexBuffer::BUFFER_TYPE_DYNAMIC),
        texture_(logger) {
    }


    boost::shared_ptr<v3d::font::BitmapTextBuffer> BitmapFont::buffer() {
        return buffer_;
    }

    void BitmapFont::clear() {
        // allocating effectively clears the buffer
        vertexBuffer_.allocate();
    }

    void BitmapFont::upload() {
        vertexBuffer_.attribute(0, 3, v3d::gl::VertexBuffer::ATTRIBUTE_TYPE_VERTEX, buffer_->vertices().size());
        vertexBuffer_.attribute(1, 2, v3d::gl::VertexBuffer::ATTRIBUTE_TYPE_NORMAL, buffer_->uvs().size());
        vertexBuffer_.attribute(2, 4, v3d::gl::VertexBuffer::ATTRIBUTE_TYPE_COLOR, buffer_->colors().size());

        vertexBuffer_.allocate();

        vertexBuffer_.data3f(0, buffer_->vertices());
        vertexBuffer_.data2f(1, buffer_->uvs());
        vertexBuffer_.data4f(2, buffer_->colors());

        vertexBuffer_.indices(buffer_->indices());
    }

    /**
     **/
    bool BitmapFont::run(boost::shared_ptr<Context> context) {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glBlendColor(1.0f, 1.0f, 1.0f, 1.0f);
        program_->enable();
        unsigned int texture = program_->uniform("texture");
        glUniform1i(texture, 0);
        vertexBuffer_.render();
        program_->disable();

        return true;
    }

};  // namespace v3d::render::realtime::operation
