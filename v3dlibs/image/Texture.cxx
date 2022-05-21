#include "Texture.h"

#include <cassert>
#include <iostream>

using namespace v3D;

Texture::Texture() : type_(UNKNOWN), texID_(-1), wrap_(false), width_(0), height_(0) 
{ 
}

Texture::Texture(const Texture &t) : type_(t.type_), texID_(t.texID_), wrap_(t.wrap_), width_(0), height_(0)
{ 
}

Texture::Texture(boost::shared_ptr<Image> image) : wrap_(false)
{
	bool ok;
	ok = create(image);
	assert(ok);
}

boost::shared_ptr<Image> Texture::image(void) const
{
	return image_;
}

void Texture::release(void)
{
	boost::shared_ptr<Image> empty_ptr;
	image_ = empty_ptr;
}

Texture & Texture::operator = (const Texture & t)
{
	type_	= t.type_;
	texID_	= t.texID_;
	wrap_	= t.wrap_;
	width_ 	= t.width_;
	height_ = t.height_;

	return *this;
}

bool Texture::operator == (const Texture & t) const 
{
	if (texID_ == t.texID_)
		return true;

	return false;
}

Texture::~Texture()
{
}

unsigned int Texture::width(void) const
{
	return width_;
}

unsigned int Texture::height(void) const
{
	return height_;
}

bool Texture::isnull(void) 
{ 
	return (type_ == UNKNOWN);
}

unsigned int Texture::id(void) const
{
	return texID_;
}

void Texture::id(unsigned int id)
{
	texID_ = id;
}

bool Texture::wrap(void) const 
{ 
	return wrap_; 
}

void Texture::wrap(bool repeat)
{ 
	wrap_ = repeat; 
}

bool Texture::create(boost::shared_ptr<Image> image)
{
	type_ = LINEAR;

	image_ 	= image;
	width_ 	= image->width();
	height_ = image->height();

	return true;
}
