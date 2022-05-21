/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#include "BitmapFontRenderer.h"
#include <GL/glew.h>

using namespace v3D;

BitmapFontRenderer::BitmapFontRenderer() :
	vertexBuffer_(v3D::VertexBuffer::BUFFER_TYPE_DYNAMIC)
{

}

BitmapFontRenderer::BitmapFontRenderer(boost::shared_ptr<BitmapTextBuffer> buffer, boost::shared_ptr<Program> program) :
	program_(program),
	buffer_(buffer),
	vertexBuffer_(v3D::VertexBuffer::BUFFER_TYPE_DYNAMIC)
{
}

BitmapFontRenderer::~BitmapFontRenderer()
{
}

boost::shared_ptr<BitmapTextBuffer> BitmapFontRenderer::buffer()
{
	return buffer_;
}

void BitmapFontRenderer::upload()
{
	vertexBuffer_.attribute(0, 3, v3D::VertexBuffer::ATTRIBUTE_TYPE_VERTEX, buffer_->vertices().size());
	vertexBuffer_.attribute(1, 2, v3D::VertexBuffer::ATTRIBUTE_TYPE_NORMAL, buffer_->uvs().size());
	vertexBuffer_.attribute(2, 4, v3D::VertexBuffer::ATTRIBUTE_TYPE_COLOR, buffer_->colors().size());

	vertexBuffer_.allocate();

	vertexBuffer_.data3f(0, buffer_->vertices());
	vertexBuffer_.data2f(1, buffer_->uvs());
	vertexBuffer_.data4f(2, buffer_->colors());

	vertexBuffer_.indices(buffer_->indices());
}

void BitmapFontRenderer::render()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glBlendColor(1.0f, 1.0f, 1.0f, 1.0f);
	program_->enable();
	unsigned int texture = program_->uniform("texture");
	glUniform1i(texture, 0);
	vertexBuffer_.render();
	program_->disable();
}

void BitmapFontRenderer::clear()
{
	// allocating effectively clears the buffer
	vertexBuffer_.allocate();
}
