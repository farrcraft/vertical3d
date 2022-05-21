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
#include "Bound2D.h"

using namespace v3D;

Bound2D::Bound2D()
{
}
Bound2D::Bound2D(real_t x, real_t y, real_t width, real_t height) : _size(width, height), _position(x, y)
{
}

Bound2D::Bound2D(const Vector2 & position, const Vector2 & size) : _size(size), _position(position)
{
}

Bound2D::~Bound2D()
{
}

real_t Bound2D::width(void) const
{
	return _size[0];
}

real_t Bound2D::height(void) const
{
	return _size[1];
}

real_t Bound2D::left(void) const
{
	return _position[0];
}

real_t Bound2D::right(void) const
{
	return _position[0] + _size[0];
}

real_t Bound2D::top(void) const
{
	return _position[1];
}

real_t Bound2D::bottom(void) const
{
	return _position[1] + _size[1];
}

void Bound2D::width(real_t width)
{
	_size[0] = width;
}

void Bound2D::height(real_t height)
{
	_size[1] = height;
}

void Bound2D::left(real_t left)
{
	_position[0] = left;
}

void Bound2D::right(real_t right)
{
	// left = right - width
	_position[0] = right - _size[0];
}

void Bound2D::top(real_t top)
{
	_position[1] = top;
}

void Bound2D::bottom(real_t bottom)
{
	// top = bottom - height
	_position[1] = bottom - _size[1];
}

void Bound2D::shrink(real_t size)
{
	// shrink the bounds 
	_size -= size;
	// adjust the origin so the midpoint doesn't move
	_position += size;
}

Bound2D & Bound2D::operator += (const Bound2D & bound)
{
	_size += bound.size();
	return *this;
}


Vector2	Bound2D::size(void) const
{
	return _size;
}

Vector2	Bound2D::position(void) const
{
	return _position;
}

void Bound2D::size(const Vector2 & size)
{
	_size = size;
}

void Bound2D::position(const Vector2 & pos)
{
	_position = pos;
}
