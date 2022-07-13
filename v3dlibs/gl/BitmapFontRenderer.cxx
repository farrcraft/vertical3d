/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "BitmapFontRenderer.h"
#include <GL/glew.h>

namespace v3d::gl {

    BitmapFontRenderer::BitmapFontRenderer(const boost::shared_ptr<v3d::core::Logger>& logger) :
        vertexBuffer_(VertexBuffer::BUFFER_TYPE_DYNAMIC), texture_(logger) {
    }

    BitmapFontRenderer::BitmapFontRenderer(boost::shared_ptr<v3d::font::BitmapTextBuffer> buffer, boost::shared_ptr<Program> program,
        const boost::shared_ptr<v3d::core::Logger>& logger) :
        program_(program),
        buffer_(buffer),
        vertexBuffer_(VertexBuffer::BUFFER_TYPE_DYNAMIC),
        texture_(logger) {
    }

    boost::shared_ptr<v3d::font::BitmapTextBuffer> BitmapFontRenderer::buffer() {
        return buffer_;
    }

    void BitmapFontRenderer::upload() {
        vertexBuffer_.attribute(0, 3, VertexBuffer::ATTRIBUTE_TYPE_VERTEX, buffer_->vertices().size());
        vertexBuffer_.attribute(1, 2, VertexBuffer::ATTRIBUTE_TYPE_NORMAL, buffer_->uvs().size());
        vertexBuffer_.attribute(2, 4, VertexBuffer::ATTRIBUTE_TYPE_COLOR, buffer_->colors().size());

        vertexBuffer_.allocate();

        vertexBuffer_.data3f(0, buffer_->vertices());
        vertexBuffer_.data2f(1, buffer_->uvs());
        vertexBuffer_.data4f(2, buffer_->colors());

        vertexBuffer_.indices(buffer_->indices());
    }

    void BitmapFontRenderer::render() {
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        glBlendColor(1.0f, 1.0f, 1.0f, 1.0f);
        program_->enable();
        unsigned int texture = program_->uniform("texture");
        glUniform1i(texture, 0);
        vertexBuffer_.render();
        program_->disable();
    }

    void BitmapFontRenderer::clear() {
        // allocating effectively clears the buffer
        vertexBuffer_.allocate();
    }

};  // namespace v3d::gl
