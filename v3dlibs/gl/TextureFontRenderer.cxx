/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#include "TextureFontRenderer.h"
#include "../image/TextureAtlas.h"
#include "../font/TextureFontCache.h"

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GL/glew.h>

using namespace v3D;

TextureFontRenderer::TextureFontRenderer() :
	vertexBuffer_(v3D::VertexBuffer::BUFFER_TYPE_DYNAMIC)
{

}

TextureFontRenderer::TextureFontRenderer(boost::shared_ptr<TextureTextBuffer> buffer, boost::shared_ptr<Program> program, boost::shared_ptr<TextureAtlas> atlas) :
	program_(program),
	buffer_(buffer),
	atlas_(atlas),
	vertexBuffer_(v3D::VertexBuffer::BUFFER_TYPE_DYNAMIC)
{
	texture_.create(atlas_->image());
}

TextureFontRenderer::~TextureFontRenderer()
{
}

boost::shared_ptr<TextureTextBuffer> TextureFontRenderer::buffer()
{
	return buffer_;
}


void TextureFontRenderer::resize(float width, float height)
{
	// update orthographic view matrix used for text rendering
	program_->enable();
	unsigned int MVPMatrix = program_->uniform("MVPMatrix");
	glm::mat4 mvp = glm::ortho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
	glUniformMatrix4fv(MVPMatrix, 1, GL_FALSE, glm::value_ptr(mvp));
	program_->disable();
}

void TextureFontRenderer::upload()
{
	vertexBuffer_.attribute(0, 3, v3D::VertexBuffer::ATTRIBUTE_TYPE_VERTEX, buffer_->vertices().size());
	vertexBuffer_.attribute(1, 2, v3D::VertexBuffer::ATTRIBUTE_TYPE_NORMAL, buffer_->uvs().size());
	vertexBuffer_.attribute(2, 4, v3D::VertexBuffer::ATTRIBUTE_TYPE_COLOR, buffer_->colors().size());
	vertexBuffer_.attribute(3, 1, v3D::VertexBuffer::ATTRIBUTE_TYPE_GENERIC, buffer_->shift().size());
	vertexBuffer_.attribute(4, 1, v3D::VertexBuffer::ATTRIBUTE_TYPE_GENERIC, buffer_->gamma().size());

	vertexBuffer_.allocate();

	vertexBuffer_.data3f(0, buffer_->vertices());
	vertexBuffer_.data2f(1, buffer_->uvs());
	vertexBuffer_.data4f(2, buffer_->colors());
	vertexBuffer_.data1f(3, buffer_->shift());
	vertexBuffer_.data1f(4, buffer_->gamma());

	vertexBuffer_.indices(buffer_->indices());
}

void TextureFontRenderer::render()
{
	glEnable(GL_BLEND);
	program_->enable();
	if (atlas_->depth() == 1)
	{
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
}
