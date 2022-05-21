#include "Image.h"

#include <cassert>
#include <algorithm>
#include <iostream>
#include <cstring>

using namespace v3D;

Image::Image() : data_(0), width_(0), height_(0), bpp_(24), format_(FORMAT_RGB)
{
}

Image::Image(unsigned int w, unsigned int h, unsigned int b) : width_(w), height_(h), bpp_(b)
{
	unsigned int bytesPerPixel	= bpp_ / 8;
	unsigned int size = width_ * height_ * bytesPerPixel;
	if (bytesPerPixel == 3)
	{
		format_ = FORMAT_RGB;
	}
	else if (bytesPerPixel == 4)
	{
		format_ = FORMAT_RGBA;
	}
	data_ = new unsigned char [size];
	memset(data_, 0, size);
}

Image::Image(unsigned long len) : width_(0), height_(0), bpp_(0)
{
	data_ = new unsigned char [len];
	memset(data_, 0, len);
}


Image::~Image()
{
	delete [] data_;
}

unsigned char * Image::data()
{
	return data_;
}

unsigned int Image::bpp() const
{
	return bpp_;
}

unsigned int Image::width() const
{
	return width_;
}

unsigned int Image::height() const
{
	return height_;
}

Image::ImageFormat Image::format() const
{
	return format_;
}

void Image::bpp(unsigned int bits)
{
	bpp_ = bits;
}

void Image::width(unsigned int w)
{
	width_ = w;
}

void Image::height(unsigned int h)
{
	height_ = h;
}

unsigned char & Image::operator[] (unsigned int i)
{
	assert(i < (width_ * height_ * (bpp_ / 8)));
	assert(data_ != 0);
	return data_[i];
}

unsigned char Image::operator[] (unsigned int i) const
{
	assert(i < (width_ * height_ * (bpp_ / 8)));
	assert(data_ != 0);
	return data_[i];
}

