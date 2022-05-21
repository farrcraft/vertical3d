/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#include <iostream>
#include <cmath>

#include "CameraProfile.h"

using namespace v3d::type;

CameraProfile::CameraProfile(const std::string & name) :  
	near_(0.001f), 
	far_(100.0f), 
	fov_(60.0f), 
	orthoZoom_(1.0f), 
	pixelAspect_(1.33f), 
	eye_(0.0f, 0.0f, -1.0f), 
	up_(0.0f, 1.0f, 0.0f), 
	right_(1.0f, 0.0f, 0.0f), 
	direction_(0.0f, 0.0f, 1.0f), 
	options_(OPTION_ORTHOGRAPHIC|OPTION_DEFAULT)
{
}

CameraProfile::~CameraProfile()
{
}


void CameraProfile::lookat(const Vector3 & center)
{
	Vector3 x, y, z;

	// new direction vector
	z = center - eye_;
	z.normalize();
	// start with original up vector
	y = up_;

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

	rotation_ = Quaternion(m);
	up_ = y;
	direction_ = z;
	right_ = x;
}

void CameraProfile::clone(const CameraProfile & profile)
{
	near_ = profile.near_;
	far_ = profile.far_;
	fov_ = profile.fov_;
	orthoZoom_ = profile.orthoZoom_;
	pixelAspect_ = profile.pixelAspect_;
	eye_ = profile.eye_;
	up_ = profile.up_;
	right_ = profile.right_;
	direction_ = profile.direction_;
	name_ = profile.name_;
	rotation_ = profile.rotation_;
	options_ = profile.options_;
}

CameraProfile & CameraProfile::operator = (const CameraProfile & p)
{
	clone(p);
	return *this;
}

void CameraProfile::eye(const Vector3 & position)
{
	eye_ = position;
}

void CameraProfile::clipping(float near, float far)
{
	near_ = near;
	far_ = far;
}
