/*
	Copyright (C) 2001-2004 by Josh Farr
	merkaba_at_quantumfish_dot_com

	This file is part of Vertical|3D.

	Vertical|3D is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Vertical|3D is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Vertical|3D; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "Image.h"

using namespace v3D;

Image::Image() : _width(0), _height(0), _bpp(0), _image(0)
{
}

Image::Image(unsigned int width, unsigned int height, unsigned int bpp) : _width(width), _height(height), _bpp(bpp), _image(0)
{
}

Image::~Image()
{
	destroy();
}

void Image::format(FormatType format)
{
	_format = format;
}

Image::FormatType Image::format(void) const
{
	return _format;
}

unsigned char * Image::allocate(unsigned int width, unsigned int height, unsigned int bpp)
{
	_width = width;
	_height = height;
	_bpp = bpp;
	return allocate();
}

unsigned char * Image::allocate(void)
{
	unsigned int size = _width * _height * (_bpp / 8);
	if (_height <= 0)
		return 0;
	_image = new unsigned char [size];
	return _image;
}

void Image::destroy(void)
{
	if (_image)
		delete [] _image;
}

unsigned char * Image::image(void)
{
	return _image;
}

unsigned int Image::width(void) const
{
	return _width;
}

unsigned int Image::height(void) const
{
	return _height;
}

unsigned int Image::bpp(void) const
{
	return _bpp;
}
