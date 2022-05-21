/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include "Color4.h"

#include <cassert>

using namespace v3d::type;

Color4::Color4()
{
	data_[0] = 0.0f;
	data_[1] = 0.0f;
	data_[2] = 0.0f;
	data_[3] = 0.0f;
}

Color4::Color4(float r, float g, float b, float a)
{
	data_[RED] = r;
	data_[GREEN] = g;
	data_[BLUE] = b;
	data_[ALPHA] = a;
}

Color4::Color4(float * color)
{
	data_[0] = *color++;
	data_[1] = *color++;
	data_[2] = *color++;
	data_[3] = *color;
}

float Color4::operator [] (unsigned int index) const
{
	assert(index < 4);
	return data_[index];
}

void Color4::scale(float s)
{
	data_[0] *= s;
	data_[1] *= s;
	data_[2] *= s;
	data_[3] *= s;
}
