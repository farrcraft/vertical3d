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

#include "Camera.h"
#include "Project.h"

using namespace v3D;

Camera::Camera()
{
}

Camera::~Camera()
{
}

/*
based on gluUnProject
takes a screen space coordinate and the viewport dimensions
and returns the world space coordinate
*/
Vector3 Camera::unproject(const Vector3 & point, int viewport[4])
{
	Vector3 p;
	// normalize point to range [-1, 1]
	p[0] = (point[0] - viewport[0]) * 2.0 / viewport[2] - 1.0;
	// flip y so 0 is at the bottom of the viewport and viewport size is the top
	p[1] = ((viewport[3] - point[1]) - viewport[1]) * 2.0 / viewport[3] - 1.0;
	p[2] = 2.0 * point[2] - 1.0;

	// get inverse transformation matrix
	Matrix4 m, inv;
	m = projection() * view();
	inv = m;
	inv.inverse();
	real_t w = inv[12] * p[0] + inv[13] * p[1] + inv[14] * p[2] + inv[15];

	return ((inv * p) / w);
}

/*
based on gluProject
takes a world space coordinate and the viewport dimensions
returns the screen space coordinate
*/
Vector3 Camera::project(const Vector3 & point, int viewport[4])
{
	Vector3 p;
	p = view() * point;
	p = projection() * p;

	p[0] = viewport[0] + (1.0 + p[0]) * viewport[2] / 2.0;
	p[1] = viewport[1] + (1.0 + p[1]) * viewport[3] / 2.0;
	p[2] = (1.0 + p[2]) / 2.0;

	return p;
}

/*
build either an orthographic or perspective projection matrix
the resulting matrix is similar to what glOrtho() and glFrustum() would build
*/
void Camera::createProjection(void)
{
	float aspect = pixelAspect();
	if (!orthographic())
	{
		/*
			glFrustum(left, right, bottom, top, near, far)
			glFrustum(-aspect, aspect, -1., 1., _viewport->_camera->near(), _viewport->_camera->far());

			[(2*near)/(right-left)	0						A	0]
			[0						(2*near)/(top-bottom)	B	0]
			[0						0						C	D]
			[0						0						-1	0]

			A = (right + left) / (right - left)
			B = (top + bottom) / (top - bottom)
			C = (far + near) / (far - near)
			D = (2 * near * far) / (far - near)

			instead of doing what glFrustum does we could do gluPerspective
			which does the same as the frustum method
		*/
		// gluPerspective part
		real_t xmin, xmax, ymin, ymax;
		ymax = near() * tan(fov() * PI / 360.0);
		ymin = -ymax;
		xmin = ymin * aspect;
		xmax = ymax * aspect;

		// glFrustum part
		real_t left = xmin;
		real_t right = xmax;
		real_t bottom = ymin;
		real_t top = ymax;
		real_t x = (2.0 * near()) / (right - left);
		real_t y = (2.0 * near()) / (top - bottom);
		real_t A = (right + left) / (right - left);
		real_t B = (top + bottom) / (top - bottom);
		real_t C = -(far() + near()) / (far() - near());
		real_t D = -(2.0 * far() * near()) / (far() - near());

		_projection[0] = x;
		_projection[1] = 0.0;
		_projection[2] = A;
		_projection[3] = 0.0;
		_projection[4] = 0.0;
		_projection[5] = y;
		_projection[6] = B;
		_projection[7] = 0.0;
		_projection[8] = 0.0;
		_projection[9] = 0.0;
		_projection[10] = C;
		_projection[11] = D;
		_projection[12] = 0.0;
		_projection[13] = 0.0;
		_projection[14] = -1.0;
		_projection[15] = 0.0;
	}
	else // orthographic
	{
		/*
			[2 / (right-left)	0					0				tx	]
			[0					2 / (top-bottom)	0				ty	]
			[0					0					-2/(far-near)	tz	]
			[0					0					0				1	]

			tx = (right + left) / (right - left)
			ty = (top + bottom) / (top - bottom)
			tz = (far + near) / (far - near)

			glOrtho(left, right, bottom, top, near, far)
			glOrtho(-1.33, 1.33, -1.0, 1.0, 	0.1, 1.0)

			near/far are distances from camera to the respective clipping planes
			left/right, top/bottom are points on each of the respective horizontal/vertical clipping planes
			the transpose method should be used when setting this matrix in GL (to account for row/column order)
		*/
		aspect *= orthoZoom();
		float left = -1.0 * aspect;
		float right = 1.0 * aspect;
		float top = 1.0 * orthoZoom();
		float bottom = -1.0 * orthoZoom();
		float far_val = far();
		float near_val = near();
		/*
			there are two adaptive options: adaptive projection and adaptive position.
		*/
		if (adaptivePosition() && Project::instance().activeScene())
		{
			Vector3 e = eye();
			// move the eye point so the world is always in front of it
			AABBox bound = Project::instance().activeScene()->bound();
			Vector3 min = bound.min();
			Vector3 max = bound.max();
			if (min != max)
			{
				Vector3 dir = direction();
				// classify min as in front of or behind eye
				float d;
				Vector3 v = e - min;
				float dist = sqrt(v.dot(v));
				d = dir[0] * min[0] + dir[1] * min[1] + dir[2] * min[2] + dist;
				if (d < 0.0) // world extends behind camera - need to reposition it
				{
					Vector3 delta = dir * d;
					e += delta;
					eye(e);
				}
			}
		}

		/*
			get scene bound
			convert bound min & max to eye space
			use largest z + 1 as far
			if smallest z > 0 use as near
			else default
		*/
		if (adaptiveProjection() && Project::instance().activeScene())
		{
			AABBox bound = Project::instance().activeScene()->bound();
			Vector3 min = bound.min();
			Vector3 max = bound.max();
			if (min != max)
			{
				// assert createView() has already been called!
				//createView();
				// bound should always encompass the origin for purposes of drawing the grid
				bound.extend(Vector3(0.0, 0.0, 0.0));
				// include the camera's eye point in the bound
				bound.extend(eye());
				min = bound.min();
				max = bound.max();
				// transform min & max to camera space
				min = _view * min;
				max = _view * max;

				// view transform might've flipped some components of min & max
				if (min[0] > max[0])
					swap(min[0], max[0]);
				if (min[1] > max[1])
					swap(min[1], max[1]);
				if (min[2] > max[2])
					swap(min[2], max[2]);

				far_val = max[2] - min[2] + 2.0;
				if (min[2] <= 0.0)
					near_val = 0.001;
				else
					near_val = min[2];
				// near & far should always be positive
				far_val = fabs(far_val);
				near_val = fabs(near_val);

				far(far_val);
				near(near_val);
			}
		}

		// mesa's glOrtho negates the top value
		float tx = -(right + left) / (right - left);
		float ty = -(top + bottom) / (top - bottom);
		float tz = -(far_val + near_val) / (far_val - near_val);

		_projection[0] = 2.0 / (right - left);
		_projection[1] = _projection[2] = 0.0;
		_projection[2] = 0.0;
		_projection[3] = tx;
		_projection[4] = 0.0;
		_projection[5] = 2.0 / (top - bottom);
		_projection[6] = 0.0;
		_projection[7] = ty;
		_projection[8] = _projection[9] = 0.0;
		_projection[9] = 0.0;
		_projection[10] = -2.0 / (far_val - near_val);
		_projection[11] = tz;
		_projection[12] = _projection[13] = _projection[14] = 0.0;
		_projection[13] = 0.0;
		_projection[14] = 0.0;
		_projection[15] = 1.0;
	}
}

void Camera::createView(void)
{
	// set viewing matrix
	_view.identity();
	Vector3 e = -eye();
	if (orthographic())
	{
		// normally for the camera rotation we'd want the inverse
		// the inverse of a pure rotation matrix should also be the transpose
		_view = rotation().matrix().transpose();
		_view.translate(e[0], e[1], e[2]);
	}
	else
	{
		Matrix4 rot = rotation().matrix().transpose();
		_view.translate(e[0], e[1], e[2]);
		_view *= rot;
	}
}

/*
	pan - move horizontally around a fixed axis (the camera's y axis) - camera rotation & look at position changes 
			but eye position doesn't - look left/right
	pan and tilt are types of rotation with restrictions
*/
void Camera::pan(float angle)
{
	Vector3 axis(0.0, 1.0, 0.0);
	Quaternion local_rotation;
	local_rotation.rotation(axis, angle);
	Quaternion total;
	total = rotation();
	total = total * local_rotation;
	rotation(total);
}

/*
	tilt - move vertically around a fixed axis (camera's x axis) - look up/down
*/
void Camera::tilt(float angle)
{
	Vector3 axis(1.0, 0.0, 0.0);
	Quaternion local_rotation;
	local_rotation.rotation(axis, angle);
	Quaternion total;
	total = rotation();
	total = total * local_rotation;
	rotation(total);
}

/*
	dolly - move eye forward or backward along direction of view
	dolly, truck, pedestal are types of translations with special restrictions
	dolly - same as pedestal but use direction vector instead of up vector
*/
void Camera::dolly(float d)
{
	Vector3 ed = direction() * d;
	eye(eye() + ed);
}

/*
	truck - move eye on axis perpendicular to direction of view and up axis - move left/right
	truck - multiply delta value and right vector to get eye delta - right vector must be 
	normalized - add eye delta to current eye position
*/
void Camera::truck(float delta)
{
	Vector3 ed = right() * delta;
	eye(eye() + ed);
}

/*
	zoom - affects the camera lens to zoom in or out (dolly without moving camera)
*/
void Camera::zoom(float z)
{
	orthoZoom(orthoZoom() + z);
}

/*
	pedestal - move eye on up axis - move up/down
	pedestal - same as truck but use up vector instead of right vector
*/
void Camera::pedestal(float delta)
{
	Vector3 ed = up() * delta;
	eye(eye() + ed);
}

Matrix4 Camera::view(void) const
{
	return _view;
}

Matrix4 Camera::projection(void) const
{
	return _projection;
}
