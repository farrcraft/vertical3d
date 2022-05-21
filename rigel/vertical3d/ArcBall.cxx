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
#include "ArcBall.h"

#include <cmath>

using namespace v3D;

ArcBall::ArcBall()
{
}

ArcBall::~ArcBall()
{
}

Vector3 ArcBall::map(const Vector2 & new_point)
{
	Vector3 mapped_point;
	Vector2 point;
	point = new_point;

	// scale point to range [-1, 1]
	point[0] = (point[0] * _width) - 1.0;
	point[1] = 1.0 - (point[1] * _height);

	float length;
	// squared length
	length = (point[0] * point[0]) + (point[1] * point[1]);

	// point mapped outside sphere (length > radius^2)
	if (length > 1.0)
	{
		float norm;
		norm = 1.0 / sqrtf(length);

		mapped_point[0] = point[0] * norm;
		mapped_point[1] = point[1] * norm;
		mapped_point[2] = 0.0;
	}
	else // point mapped inside sphere
	{
		mapped_point[0] = point[0];
		mapped_point[1] = point[1];
		mapped_point[2] = sqrtf(1.0 - length);
	}
	return mapped_point;
}

void ArcBall::click(const Vector2 & point)
{
	_start = map(point);
}

Quaternion ArcBall::drag(const Vector2 & point)
{
	_end = map(point);

	Quaternion rot;
	Vector3 perp;

	perp = _start.cross(_end);

	if (perp.length() > EPSILON)
	{
		rot[0] = perp[0];
		rot[1] = perp[1];
		rot[2] = perp[2];
		rot[3] = _start.dot(_end);
	}
	else
	{
		rot[0] = rot[1] = rot[2] = rot[3] = 0.0;
	}
	_start = _end;
	return rot;
}

void ArcBall::bounds(float width, float height)
{
	if (width < 1.0)
		width = 1.0;
	if (height < 1.0)
		height = 1.0;
	_width = 1.0 / ((width - 1.0) * 0.5);
	_height = 1.0 / ((height - 1.0) * 0.5);
}
