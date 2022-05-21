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

#include <GL/gl.h>

#include <vector>
#include <cmath>

#include <libv3dtypes/Vector3.h>
#include <libv3dcore/Project.h>

#include "ScaleManipulator.h"
#include "../Window.h"

using namespace v3D;

ScaleManipulator::ScaleManipulator()
{
}

ScaleManipulator::~ScaleManipulator()
{
}

/*
the scale manipulator is represented by a colored
axis line with a cube on the end
*/
void ScaleManipulator::draw(void)
{
	glPolygonMode(GL_FRONT, GL_FILL);
	glPolygonMode(GL_BACK, GL_FILL);

	// x axis
	glLoadName(SELECT_NAME_X_MANIPULATOR);
	if (axis() == CONSTRAINT_X_AXIS)
		glColor3f(0.8, 0.8, 0.0);
	else
		glColor3f(1.0, 0.0, 0.0);
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(1.0, 0.0, 0.0);
	glEnd();

	float offset = 1.0;

	float length = 0.25;
	float height = 0.25;
	float width  = 0.25;
	length /= 2.0;
	height /= 2.0;
	width  /= 2.0;
	// start new polygon - front
	glBegin(GL_QUADS);
	// add 4 vertices
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(-width + offset, -height, -length);
	glVertex3f( width + offset, -height, -length);
	glVertex3f( width + offset,  height, -length);
	glVertex3f(-width + offset,  height, -length);
	// next polygon - right
	// add 4 vertices
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f( width + offset, -height, -length);
	glVertex3f( width + offset, -height,  length);
	glVertex3f( width + offset,  height,  length);
	glVertex3f( width + offset,  height, -length);
	// next polygon - top
	// add 4 vertices
	glNormal3f(0.0, 1.0, 0.0);
	glVertex3f(-width + offset,  height, -length);
	glVertex3f( width + offset,  height, -length);
	glVertex3f( width + offset,  height,  length);
	glVertex3f(-width + offset,  height,  length);
	// next polygon - left
	// add 4 vertices
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-width + offset, -height, -length);
	glVertex3f(-width + offset, -height,  length);
	glVertex3f(-width + offset,  height,  length);
	glVertex3f(-width + offset,  height, -length);
	// next polygon - back
	// add 4 vertices
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(-width + offset, -height,  length);
	glVertex3f( width + offset, -height,  length);
	glVertex3f( width + offset,  height,  length);
	glVertex3f(-width + offset,  height,  length);
	// next polygon - bottom
	// add 4 vertices
	glNormal3f(0.0, -1.0, 0.0);
	glVertex3f(-width + offset, -height, -length);
	glVertex3f( width + offset, -height, -length);
	glVertex3f( width + offset, -height,  length);
	glVertex3f(-width + offset, -height,  length);
	glEnd();

	// y axis
	glLoadName(SELECT_NAME_Y_MANIPULATOR);
	if (axis() == CONSTRAINT_Y_AXIS)
		glColor3f(0.8, 0.8, 0.0);
	else
		glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, 1.0, 0.0);
	glEnd();

	// start new polygon - front
	glBegin(GL_QUADS);
	// add 4 vertices
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(-width, -height + offset, -length);
	glVertex3f( width, -height + offset, -length);
	glVertex3f( width,  height + offset, -length);
	glVertex3f(-width,  height + offset, -length);
	// next polygon - right
	// add 4 vertices
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f( width, -height + offset, -length);
	glVertex3f( width, -height + offset,  length);
	glVertex3f( width,  height + offset,  length);
	glVertex3f( width,  height + offset, -length);
	// next polygon - top
	// add 4 vertices
	glNormal3f(0.0, 1.0, 0.0);
	glVertex3f(-width,  height + offset, -length);
	glVertex3f( width,  height + offset, -length);
	glVertex3f( width,  height + offset,  length);
	glVertex3f(-width,  height + offset,  length);
	// next polygon - left
	// add 4 vertices
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-width, -height + offset, -length);
	glVertex3f(-width, -height + offset,  length);
	glVertex3f(-width,  height + offset,  length);
	glVertex3f(-width,  height + offset, -length);
	// next polygon - back
	// add 4 vertices
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(-width, -height + offset,  length);
	glVertex3f( width, -height + offset,  length);
	glVertex3f( width,  height + offset,  length);
	glVertex3f(-width,  height + offset,  length);
	// next polygon - bottom
	// add 4 vertices
	glNormal3f(0.0, -1.0, 0.0);
	glVertex3f(-width, -height + offset, -length);
	glVertex3f( width, -height + offset, -length);
	glVertex3f( width, -height + offset,  length);
	glVertex3f(-width, -height + offset,  length);
	glEnd();

	// z axis
	glLoadName(SELECT_NAME_Z_MANIPULATOR);
	if (axis() == CONSTRAINT_Z_AXIS)
		glColor3f(0.8, 0.8, 0.0);
	else
		glColor3f(0.0, 0.0, 1.0);
	glBegin(GL_LINES);
	glVertex3f(0.0, 0.0, 0.0);
	glVertex3f(0.0, 0.0, 1.0);
	glEnd();

	// start new polygon - front
	glBegin(GL_QUADS);
	// add 4 vertices
	glNormal3f(0.0, 0.0, -1.0);
	glVertex3f(-width, -height, -length + offset);
	glVertex3f( width, -height, -length + offset);
	glVertex3f( width,  height, -length + offset);
	glVertex3f(-width,  height, -length + offset);
	// next polygon - right
	// add 4 vertices
	glNormal3f(1.0, 0.0, 0.0);
	glVertex3f( width, -height, -length + offset);
	glVertex3f( width, -height,  length + offset);
	glVertex3f( width,  height,  length + offset);
	glVertex3f( width,  height, -length + offset);
	// next polygon - top
	// add 4 vertices
	glNormal3f(0.0, 1.0, 0.0);
	glVertex3f(-width,  height, -length + offset);
	glVertex3f( width,  height, -length + offset);
	glVertex3f( width,  height,  length + offset);
	glVertex3f(-width,  height,  length + offset);
	// next polygon - left
	// add 4 vertices
	glNormal3f(-1.0, 0.0, 0.0);
	glVertex3f(-width, -height, -length + offset);
	glVertex3f(-width, -height,  length + offset);
	glVertex3f(-width,  height,  length + offset);
	glVertex3f(-width,  height, -length + offset);
	// next polygon - back
	// add 4 vertices
	glNormal3f(0.0, 0.0, 1.0);
	glVertex3f(-width, -height,  length + offset);
	glVertex3f( width, -height,  length + offset);
	glVertex3f( width,  height,  length + offset);
	glVertex3f(-width,  height,  length + offset);
	// next polygon - bottom
	// add 4 vertices
	glNormal3f(0.0, -1.0, 0.0);
	glVertex3f(-width, -height, -length + offset);
	glVertex3f( width, -height, -length + offset);
	glVertex3f( width, -height,  length + offset);
	glVertex3f(-width, -height,  length + offset);
	glEnd();
}

void ScaleManipulator::transform(const Vector2 & delta)
{
	Vector2 mouse_delta = delta;
	ViewPort * viewport = Window::instance()->activeView();
	Camera * camera = viewport->camera();

	float factor;
	factor = (camera->orthoZoom() * 2.0 * camera->pixelAspect()) / viewport->_size[0];

	mouse_delta *= factor;

	Matrix4 view = camera->view().transpose();
	Matrix4 projection = camera->projection().transpose();

	Vector3 s = Project::instance()._selection->scale();
	if (axis() == TransformManipulator::CONSTRAINT_X_AXIS)
	{
		Vector3 d(1.0, 0.0, 0.0);
		// convert translation axis to screen space
		Vector3 screen_axis;
		screen_axis = view * d;
		screen_axis = projection * screen_axis;
		screen_axis.normalize();
		// match translation axis to 2d screen axis
		float transform_delta;
		if (fabs(screen_axis[0]) > EPSILON)
			transform_delta = mouse_delta[0];
		else
			transform_delta = mouse_delta[1];

		// scale
		s += d * transform_delta;
	}
	else if (axis() == TransformManipulator::CONSTRAINT_Y_AXIS)
	{
		Vector3 d(0.0, 1.0, 0.0);
		// convert translation axis to screen space
		Vector3 screen_axis;
		screen_axis = view * d;
		screen_axis = projection * screen_axis;
		screen_axis.normalize();
		// match translation axis to 2d screen axis
		float transform_delta;
		if (fabs(screen_axis[0]) > EPSILON)
			transform_delta = mouse_delta[0];
		else
			transform_delta = mouse_delta[1];

		// scale
		s += d * -transform_delta;
	}
	else if (axis() == TransformManipulator::CONSTRAINT_Z_AXIS)
	{
		Vector3 d(0.0, 0.0, 1.0);
		// convert translation axis to screen space
		Vector3 screen_axis;
		screen_axis = view * d;
		screen_axis = projection * screen_axis;
		screen_axis.normalize();
		// match translation axis to 2d screen axis
		float transform_delta;
		if (fabs(screen_axis[0]) > EPSILON)
			transform_delta = mouse_delta[0];
		else
			transform_delta = mouse_delta[1];

		// scale
		s += d * -transform_delta;
	}
	else
	{
		// use initial delta since last one is already scaled by factor in both directions
		mouse_delta = delta;
		mouse_delta[0] *= factor;
		mouse_delta[1] *= ((camera->orthoZoom() * 2.0) / viewport->_size[1]);
		s += camera->right() * mouse_delta[0];
		s += camera->up() * -mouse_delta[1];
	}
	Project::instance()._selection->scale(s);
}
