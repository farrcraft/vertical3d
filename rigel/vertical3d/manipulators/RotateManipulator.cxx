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

#include "RotateManipulator.h"
#include "../Window.h"

using namespace v3D;

RotateManipulator::RotateManipulator()
{
}

RotateManipulator::~RotateManipulator()
{
}

/*
the rotation manipulator is represented by a colored
circle
*/
void RotateManipulator::draw(void)
{
	int sides = 32;
	float radius = 1.0;

	float delta = 2. * PI / sides;
	// x axis
	glLoadName(SELECT_NAME_X_MANIPULATOR);
	if (axis() == CONSTRAINT_X_AXIS)
		glColor3f(0.8, 0.8, 0.0);
	else
		glColor3f(1.0, 0.0, 0.0);
	std::vector<Vector3> points;
	glBegin(GL_LINE_LOOP);
	for (int k = 0; k < sides; k++)
	{
		Vector3 p;
		p[0] = 0.0;
		p[1] = cos(delta * k) * radius;
		p[2] = sin(delta * k) * radius;
		points.push_back(p);
		glVertex3f(p[0], p[1], p[2]);
	}
	glEnd();

	// y axis
	glLoadName(SELECT_NAME_Y_MANIPULATOR);
	if (axis() == CONSTRAINT_Y_AXIS)
		glColor3f(0.8, 0.8, 0.0);
	else
		glColor3f(0.0, 1.0, 0.0);
	glBegin(GL_LINE_LOOP);
	for (int k = 0; k < sides; k++)
	{
		glVertex3f(points[k][1], points[k][0], points[k][2]);
	}
	glEnd();

	// z axis
	glLoadName(SELECT_NAME_Z_MANIPULATOR);
	if (axis() == CONSTRAINT_Z_AXIS)
		glColor3f(0.8, 0.8, 0.0);
	else
		glColor3f(0.0, 0.0, 1.0);
	glBegin(GL_LINE_LOOP);
	for (int k = 0; k < sides; k++)
	{
		glVertex3f(points[k][1], points[k][2], points[k][0]);
	}
	glEnd();

}

void RotateManipulator::transform(const Vector2 & delta)
{
	Vector2 mouse_delta = delta;
	ViewPort * viewport = Window::instance()->activeView();
	Camera * camera = viewport->camera();

	float factor;
	factor = (camera->orthoZoom() * 2.0 * camera->pixelAspect()) / viewport->_size[0];

	mouse_delta *= factor;

	Matrix4 view = camera->view().transpose();
	Matrix4 projection = camera->projection().transpose();

	Quaternion rt;
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

		// rotate
		rt.rotation(d, transform_delta);
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

		// rotate
		rt.rotation(d, transform_delta);
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

		// rotate
		rt.rotation(d, transform_delta);
	}
	else
	{
		// use initial delta since last one is already scaled by factor in both directions
		mouse_delta = delta;
		mouse_delta[0] *= factor;
		mouse_delta[1] *= ((camera->orthoZoom() * 2.0) / viewport->_size[1]);

		Quaternion rx, ry;
		rx.rotation(camera->up(), mouse_delta[0]);
		ry.rotation(camera->right(), mouse_delta[1]);
		rt = rx * ry;
	}
	Quaternion rot = Project::instance()._selection->rotation();
	rot *= rt;
	Project::instance()._selection->rotation(rot);
}
