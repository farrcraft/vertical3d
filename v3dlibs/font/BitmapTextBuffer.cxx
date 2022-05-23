/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "BitmapTextBuffer.h"
#include "BitmapFont.h"

#include <vector>

using namespace v3d::font;

BitmapTextBuffer::BitmapTextBuffer(boost::shared_ptr<BitmapFont> font) : 
	font_(font) 
{
	dirty(true);
}

size_t BitmapTextBuffer::width(const std::string & txt)
{
	size_t w = 0;
	for (unsigned int index = 0; index < txt.length(); ++index)
	{
		BitmapFont::CharDescriptor character = font_->character(txt[index]);
		w += character.xAdvance_;
	}
	return w;
}

void BitmapTextBuffer::text(glm::vec2 & pen, const std::string & txt, const glm::vec4 & color)
{
	float charsetWidth = font_->charsetWidth();
	float charsetHeight = font_->charsetHeight();

	resize(txt.length() * 6);

	unsigned int vcount = 0;
	glm::vec2 uv;
	glm::vec3 vertex;

	for (unsigned int index = 0; index < txt.length(); ++index)
	{
		BitmapFont::CharDescriptor character = font_->character(txt[index]);
		float charX = character.x_;
		float charY = character.y_;
		float width = character.width_;
		float height = character.height_;
		float offsetX = character.xOffset_;
		float offsetY = character.yOffset_;

		// upper left
		uv.s = charX / charsetWidth;
		uv.t = charY / charsetHeight;
		vertex.x = pen.x + offsetX;
		vertex.y = pen.y + offsetY;
		vertex.z = 0.0f;
		addTextureCoordinate(uv);
		addVertex(vertex);
		addColor(color);

		// upper right
		uv.s = (charX + width) / charsetWidth;
		uv.t = charY / charsetHeight;
		vertex.x = width + pen.x + offsetX;
		vertex.y = pen.y + offsetY;
		vertex.z = 0.0f;
		addTextureCoordinate(uv);
		addVertex(vertex);
		addColor(color);

		// lower right
		uv.s = (charX + width) / charsetWidth;
		uv.t = (charY + height) / charsetHeight;
		vertex.x = width + pen.x + offsetX;
		vertex.y = height + pen.y + offsetY;
		vertex.z = 0.0f;
		addTextureCoordinate(uv);
		addVertex(vertex);
		addColor(color);

		// lower left
		uv.s = charX / charsetWidth;
		uv.t = (charY + height) / charsetHeight;
		vertex.x = pen.x + offsetX;
		vertex.y = height + pen.y + offsetY;
		vertex.z = 0.0f;
		addTextureCoordinate(uv);
		addVertex(vertex);
		addColor(color);

		addIndex(vcount);
		addIndex(vcount + 1);
		addIndex(vcount + 3);

		addIndex(vcount + 3);
		addIndex(vcount + 1);
		addIndex(vcount + 2);

		vcount += 4;
		pen.x += character.xAdvance_;
	}

	dirty(false);
}


