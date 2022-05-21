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

#include "CameraProfile.h"

using namespace v3D;

CameraProfile::CameraProfile() :  _near(0.001), _far(100.), _fov(60.), _orthoZoom(1.), 
			_pixelAspect(1.33), _eye(0., 0., -1.), _up(0., 1., 0.), _right(1., 0., 0.), 
			_direction(0., 0., 1.), _name("Unamed"),
			_options(OPTION_ORTHOGRAPHIC|OPTION_ADAPTIVE_PROJECTION|OPTION_ADAPTIVE_POSITION)
{
}

CameraProfile::~CameraProfile()
{
}

void CameraProfile::lookat(const Vector3 & center)
{
	Vector3 x, y, z;

	// new direction vector
	z = center - _eye;
	z.normalize();
	// start with original up vector
	y = _up;

	// normal of yz plane is new right vector
	x = y.cross(z);
	x.normalize();
	// normal of the xy plane is the new up vector
	y = z.cross(x);
	y.normalize();

	/*
		[  0,  1,  2,  3 ]
		[  4,  5,  6,  7 ]
		[  8,  9, 10, 11 ]
		[ 12, 13, 14, 15 ]

		x = [ 0, 1, 2  ] = right
		y = [ 4, 5, 6  ] = up
		z = [ 8, 9, 10 ] = direction
	*/
	Matrix4 m;
	m[0] = x[0];
	m[1] = x[1];
	m[2] = x[2];
	m[3] = 0.0;
	m[4] = y[0];
	m[5] = y[1];
	m[6] = y[2];
	m[7] = 0.0;
	m[8] = z[0];
	m[9] = z[1];
	m[10] = z[2];
	m[11] = 0.0;
	m[12] = 0.0;
	m[13] = 0.0;
	m[14] = 0.0;
	m[15] = 1.0;

	_rotation = Quaternion(m);
	_up = y;
	_direction = z;
	_right = x;
}

void CameraProfile::profile(const CameraProfile & profile)
{
	_near = profile._near;
	_far = profile._far;
	_fov = profile._fov;
	_orthoZoom = profile._orthoZoom;
	_pixelAspect = profile._pixelAspect;
	_eye = profile._eye;
	_up = profile._up;
	_right = profile._right;
	_direction = profile._direction;
	_name = profile._name;
	_rotation = profile._rotation;
	_options = profile._options;
}

CameraProfile & CameraProfile::operator = (const CameraProfile & p)
{
	profile(p);
	return *this;
}

void CameraProfile::name(const std::string & name)
{
	_name = name;
}

std::string CameraProfile::name(void) const
{
	return _name;
}

void CameraProfile::orthoZoom(float f)
{
	_orthoZoom = f;
}

void CameraProfile::rotation(const Quaternion & q)
{
	_rotation = q;
}

Quaternion CameraProfile::rotation(void) const
{
	return _rotation;
}

float CameraProfile::pixelAspect(void) const
{
	return _pixelAspect;
}

float CameraProfile::fov(void) const
{
	return _fov;
}

void CameraProfile::fov(float f)
{
	_fov = f;
}

Vector3 CameraProfile::up(void) const
{
	return _up;
}

Vector3	CameraProfile::right(void) const
{
	return _right;
}

Vector3	CameraProfile::direction(void) const
{
	return _direction;
}

void CameraProfile::pixelAspect(float aspect)
{
	_pixelAspect = aspect;
}

void CameraProfile::up(const Vector3 & u)
{
	_up = u;
}

void CameraProfile::right(const Vector3 & r)
{
	_right = r;
}

void CameraProfile::direction(const Vector3 & d)
{
	_direction = d;
}


float CameraProfile::near(void) const
{
	return _near;
}

float CameraProfile::far(void) const
{
	return _far;
}

float CameraProfile::orthoZoom(void) const
{
	return _orthoZoom;
}

void CameraProfile::near(float n)
{
	_near = n;
}

void CameraProfile::far(float f)
{
	_far = f;
}

Vector3 CameraProfile::eye(void) const
{
	return _eye;
}

void CameraProfile::eye(const Vector3 & v)
{
	_eye = v;
}

bool CameraProfile::orthographic(void) const
{
	return (_options & OPTION_ORTHOGRAPHIC);
}

void CameraProfile::orthographic(bool ortho)
{
	if (ortho)
		_options |= OPTION_ORTHOGRAPHIC;
	else
		_options &= ~OPTION_ORTHOGRAPHIC;
}

bool CameraProfile::adaptiveProjection(void) const
{
	return (_options & OPTION_ADAPTIVE_PROJECTION);
}

bool CameraProfile::adaptivePosition(void) const
{
	return (_options & OPTION_ADAPTIVE_POSITION);
}

void CameraProfile::adaptiveProjection(bool adaptive)
{
	if (adaptive)
		_options |= OPTION_ADAPTIVE_PROJECTION;
	else
		_options &= ~OPTION_ADAPTIVE_PROJECTION;
}

void CameraProfile::adaptivePosition(bool adaptive)
{
	if (adaptive)
		_options |= OPTION_ADAPTIVE_POSITION;
	else
		_options &= ~OPTION_ADAPTIVE_POSITION;
}
