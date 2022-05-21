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
#include "AABBox.h"

using namespace v3D;

AABBox::AABBox()
{
}

AABBox::~AABBox()
{
}

Vector3 AABBox::min(void) const
{
	return _min;
}

Vector3 AABBox::max(void) const
{
	return _max;
}


Vector3 AABBox::origin(void) const
{
	return (_max - _min);
}

void AABBox::vertices(Vector3 * v) const
{
}

// set min & max extents
void AABBox::extents(const Vector3 & min, const Vector3 & max)
{
	_min = min;
	_max = max;
}

void AABBox::extend(const Vector3 & point)
{
	if (point[0] < _min[0])
		_min[0] = point[0];
	if (point[1] < _min[1])
		_min[1] = point[1];
	if (point[2] < _min[2])
		_min[2] = point[2];

	if (point[0] > _max[0])
		_max[0] = point[0];
	if (point[1] > _max[1])
		_max[1] = point[1];
	if (point[2] > _max[2])
		_max[2] = point[2];
}

