/**
 * (c) Joshua Farr <j.wgasa@gmail.com>
 *
 */

#include "TextBuffer.h"

using namespace v3D;

void TextBuffer::addVertex(const glm::vec3 & vertex)
{
	vertices_.push_back(vertex);
}

void TextBuffer::addIndex(unsigned int index)
{
	indices_.push_back(index);
}

void TextBuffer::addColor(const glm::vec4 & color)
{
	colors_.push_back(color);
}

void TextBuffer::addTextureCoordinate(const glm::vec2 & uv)
{
	uvs_.push_back(uv);
}

void TextBuffer::invalidate()
{
	dirty_ = true;
}

void TextBuffer::dirty(bool state)
{
	dirty_ = state;
}

bool TextBuffer::dirty() const
{
	return dirty_;
}

std::vector<glm::vec3> & TextBuffer::vertices()
{
	return vertices_;
}

std::vector<glm::vec2> & TextBuffer::uvs()
{
	return uvs_;
}

std::vector<glm::vec4> & TextBuffer::colors()
{
	return colors_;
}

std::vector<unsigned int> & TextBuffer::indices()
{
	return indices_;
}

void TextBuffer::clear()
{
	// allocating effectively clears the buffer
	//buffer_.allocate();
	indices_.clear();
	vertices_.clear();
	uvs_.clear();
	colors_.clear();
}

void TextBuffer::resize(unsigned int size)
{
	vertices_.resize(size);
	uvs_.resize(size);
	colors_.resize(size);
	indices_.resize(size);
}

