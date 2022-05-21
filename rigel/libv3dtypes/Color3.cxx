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
#include <cassert>

#include "Color3.h"

using namespace v3D;

Color3::Color3()
{
	_color[0] = 0.;
	_color[1] = 0.;
	_color[2] = 0.;
}

Color3::Color3(real_t r, real_t g, real_t b)
{
	_color[0] = r;
	_color[1] = g;
	_color[2] = b;
}

Color3::Color3(const Color3 & c)
{
	_color[0] = c._color[0];
	_color[1] = c._color[1];
	_color[2] = c._color[2];
}

Color3::Color3(real_t c[3])
{
	_color[0] = c[0];
	_color[1] = c[1];
	_color[2] = c[2];
}

Color3::Color3(real_t c)
{
	_color[0] = c;
	_color[1] = c;
	_color[2] = c;
}

Color3::~Color3()
{
}

real_t Color3::operator[](unsigned int i) const
{
	assert (i < 3);
	return _color[i];
}
