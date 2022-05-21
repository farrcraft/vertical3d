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

#include "TranslateManipulator.h"
#include "../Window.h"

using namespace v3D;

TranslateManipulator::TranslateManipulator()
{
}

TranslateManipulator::~TranslateManipulator()
{
}

/*
the translation manipulator is represented by
a colored axis line with a cone on the end (an arrow)

should manipulators be classes?
they may have state: dragging single axis, constrained on axis
state only persists while an object is selected. un/re select resets the state

need to know the difference between clicks and drags
click is click/release without movement
drag is click/drag/release

on manipulator axis click - axis is actively constrained - 
constrained manipulator axis is drawn as "selected" - 
middle button drag anywhere moves only on constrained axis

on manipulator axis drag - drag affect is constrained -
manipulator axis is not "selected" after corresponding release

*/
void TranslateManipulator::draw(void)
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
	// make cone at end of axis to form arrow
	int sides = 48;
	float height = 1.5;
	float radius = 0.125;

	float delta = 2.0 * PI / sides;
	// create points along the bottom of the cone
	// also render the bottom cap of the cone at the same time
	std::vector<Vector3> points;
	glBegin(GL_POLYGON);
	glNormal3f(1.0, 0.0, 0.0);
	for (int k = 1; k <= sides; k++)
	{
		Vector3 p;
		p[0] = 1.0;
		p[1] = cos(delta * k) * radius;
		p[2] = sin(delta * k) * radius;
		points.push_back(p);
		glVertex3f(p[0], p[1], p[2]);
	}
	// draw one more vertex to close the polygon
	glVertex3f(points[0][0], points[0][1], points[0][2]);
	glEnd();
	// make the sides of the cone
	glBegin(GL_TRIANGLES);
	for (int k = 1; k <= sides; k++)
	{
		// calculate surface normal
		Vector3 n1, n2;
		Vector3 apex(height, 0.0, 0.0);
		if (k == points.size())
		{
			n1 = points[0] - points[k - 1];
			n2 = apex - points[0];
		}
		else
		{
			n1 = points[k] - points[k - 1];
			n2 = apex - points[k];
		}
		n1.normalize();
		n2.normalize();
		Vector3 norm = n1.cross(n2);
		norm.normalize();
		glNormal3f(norm[0], norm[1], norm[2]);

		glVertex3f(points[k-1][0], points[k-1][1], points[k-1][2]);
		if (k == points.size())
			glVertex3f(points[0][0], points[0][1], points[0][2]);
		else
			glVertex3f(points[k][0], points[k][1], points[k][2]);
		glVertex3f(height, 0.0, 0.0);
	}
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

	glBegin(GL_POLYGON);
	glNormal3f(0.0, 1.0, 0.0);
	for (int k = 0; k < sides; k++)
	{
		glVertex3f(points[k][1], points[k][0], points[k][2]);
	}
	// draw one more vertex to close the polygon
	glVertex3f(points[0][1], points[0][0], points[0][2]);
	glEnd();
	// make the sides of the cone
	glBegin(GL_TRIANGLES);
	for (int k = 1; k <= sides; k++)
	{
		// calculate surface normal
		Vector3 n1, n2;
		Vector3 apex(0.0, height, 0.0);
		if (k == points.size())
		{
			n1 = points[0] - points[k - 1];
			n2 = apex - points[0];
		}
		else
		{
			n1 = points[k] - points[k - 1];
			n2 = apex - points[k];
		}
		n1.normalize();
		n2.normalize();
		Vector3 norm = n1.cross(n2);
		norm.normalize();
		glNormal3f(norm[0], norm[1], norm[2]);

		glVertex3f(points[k-1][1], points[k-1][0], points[k-1][2]);
		if (k == points.size())
			glVertex3f(points[0][1], points[0][0], points[0][2]);
		else
			glVertex3f(points[k][1], points[k][0], points[k][2]);
		glVertex3f(0.0, height, 0.0);
	}
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

	glBegin(GL_POLYGON);
	glNormal3f(0.0, 0.0, 1.0);
	for (int k = 0; k < sides; k++)
	{
		glVertex3f(points[k][1], points[k][2], points[k][0]);
	}
	// draw one more vertex to close the polygon
	glVertex3f(points[0][1], points[0][2], points[0][0]);
	glEnd();
	// make the sides of the cone
	glBegin(GL_TRIANGLES);
	for (int k = 1; k <= sides; k++)
	{
		// calculate surface normal
		Vector3 n1, n2;
		Vector3 apex(0.0, 0.0, height);
		if (k == points.size())
		{
			n1 = points[0] - points[k - 1];
			n2 = apex - points[0];
		}
		else
		{
			n1 = points[k] - points[k - 1];
			n2 = apex - points[k];
		}
		n1.normalize();
		n2.normalize();
		Vector3 norm = n1.cross(n2);
		norm.normalize();
		glNormal3f(norm[0], norm[1], norm[2]);

		glVertex3f(points[k-1][1], points[k-1][2], points[k-1][0]);
		if (k == points.size())
			glVertex3f(points[0][1], points[0][2], points[0][0]);
		else
			glVertex3f(points[k][1], points[k][2], points[k][0]);
		glVertex3f(0.0, 0.0, height);
	}
	glEnd();
}

void TranslateManipulator::transform(const Vector2 & delta)
{
	Vector2 mouse_delta = delta;
	ViewPort * viewport = Window::instance()->activeView();
	Camera * camera = viewport->camera();

	float factor;
	factor = (camera->orthoZoom() * 2.0 * camera->pixelAspect()) / viewport->_size[0];

	mouse_delta *= factor;

	Matrix4 view = camera->view().transpose();
	Matrix4 projection = camera->projection().transpose();

	Vector3 t;
	if (axis() == TransformManipulator::CONSTRAINT_X_AXIS)
	{
		/* need to get the x axis in object space
		if the object has been rotated then the world and object space x axis
		won't be aligned. how this is done depends on the order transformations
		are applied. if translation is done before rotation then the x axis
		will never deviate from world space.
		it will affect the way that interaction with the manipulator is handled
		though.
		*/
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

		// translate
		t = d * transform_delta;
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

		// translate
		t = d * -transform_delta;
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

		// translate
		t = d * -transform_delta;
	}
	else
	{
		// use initial delta since last one is already scaled by factor in both directions
		mouse_delta = delta;
		mouse_delta[0] *= factor;
		mouse_delta[1] *= ((camera->orthoZoom() * 2.0) / viewport->_size[1]);
		t = camera->right() * mouse_delta[0];
		t = camera->up() * -mouse_delta[1];
	}
	Project::instance()._selection->translation(t);
}
