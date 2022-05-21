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
#include <iostream>
#include <cmath>

#include <GL/gl.h>

#include <libv3dtypes/Color3.h>

#include "ConstructionPlane.h"

using namespace v3D;

ConstructionPlane::ConstructionPlane() : _spacing(1.0), _majorIntervals(10)
{
}

ConstructionPlane::~ConstructionPlane()
{
}

void ConstructionPlane::size(float s)
{
	_size = s;
}

void ConstructionPlane::spacing(float s)
{
	_spacing = s;
}

void ConstructionPlane::intervals(int m)
{
	_majorIntervals = m;
}

void ConstructionPlane::infinite(bool i)
{
	_infinite = i;
}

void ConstructionPlane::autoscale(bool a)
{
	_autoscale = a;
}

float ConstructionPlane::size(void)
{
	return _size;
}

float ConstructionPlane::spacing(void)
{
	return _spacing;
}

int ConstructionPlane::intervals(void)
{
	return _majorIntervals;
}

bool ConstructionPlane::infinite(void)
{
	return _infinite;
}

bool ConstructionPlane::autoscale(void)
{
	return _autoscale;
}

void ConstructionPlane::draw(Camera * cam)
{
	Color3 minorColor(0.3, 0.3, 0.3);
	Color3 majorColor(0.1, 0.1, 0.3);
	Color3 originColor(0.0, 0.0, 0.0);

	Vector3 right(1.0, 0.0, 0.0);
	Vector3 up(0.0, 0.0, 1.0);
	Vector3 depth(0.0, 0.0, 0.0);
	if (cam->orthographic())
	{
		right = cam->right();
		up = cam->up();
		Vector3 dir = cam->direction();

		depth = cam->eye();
		depth[0] *= dir[0];
		depth[1] *= dir[1];
		depth[2] *= dir[2];
		dir *= fabs(cam->far()) - 1.0;
		dir.abs();
		depth.abs();
		dir -= depth;

		depth = dir;
		dir = cam->direction();
		depth[0] *= dir[0];
		depth[1] *= dir[1];
		depth[2] *= dir[2];
	}
	Vector3 line_pos;

	glBegin(GL_LINES);
	int i;
	int max = 60;
	float extents = max * _spacing / 2.0;
	float offset;
	// horizontal lines
	for (i = 0; i <= max; i++)
	{
		offset = -extents + (i * _spacing);

		if (offset == 0.0)
		{
			// set line width to thicker
			glLineWidth(2.0);
			glColor3f(originColor[0], originColor[1], originColor[2]);
		}
		else if (i % _majorIntervals == 0)
			glColor3f(majorColor[0], majorColor[1], majorColor[2]);
		else
			glColor3f(minorColor[0], minorColor[1], minorColor[2]);

		line_pos = (right * offset) + (up * -extents) + depth;
		glVertex3f(line_pos[0], line_pos[1], line_pos[2]);
		line_pos = (right * offset) + (up * extents) + depth;
		glVertex3f(line_pos[0], line_pos[1], line_pos[2]);
		glLineWidth(1.0);
	}
	// vertical lines
	for (i = 0; i <= max; i++)
	{
		offset = -extents + (i * _spacing);

		if (offset == 0.0)
		{
			// set line width to thicker
			glLineWidth(2.0);
			glColor3f(originColor[0], originColor[1], originColor[2]);
		}
		else if (i % _majorIntervals == 0)
			glColor3f(majorColor[0], majorColor[1], majorColor[2]);
		else
			glColor3f(minorColor[0], minorColor[1], minorColor[2]);

		line_pos = (right * -extents) + (up * offset) + depth;
		glVertex3f(line_pos[0], line_pos[1], line_pos[2]);
		line_pos = (right * extents) + (up * offset) + depth;
		glVertex3f(line_pos[0], line_pos[1], line_pos[2]);
		glLineWidth(1.0);
	}
	glEnd();
}
